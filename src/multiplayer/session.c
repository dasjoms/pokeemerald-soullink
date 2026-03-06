#include "global.h"
#include "main.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/queue.h"
#include "multiplayer/transport.h"
#include "gba/isagbprint.h"

#define MP_CONNECT_TIMEOUT_FRAMES  (60 * 10)
#define MP_RECOVERY_TIMEOUT_FRAMES  (60 * 5)
#define MP_RECOVERY_RETRY_INTERVAL_FRAMES 30
#define MP_RECOVERY_CONFIRM_TIMEOUT_FRAMES (60 * 2)
#define MP_PEER_TIMEOUT_FRAMES     (60 * 5)
#define MP_HEARTBEAT_INTERVAL_FRAMES 30

#define MP_PEER_STATUS_DISCONNECTED (1 << 0)

static struct MultiplayerSession sSession;
static enum MpSessionState sSessionState;
static struct MpPeerState sPeerCache[MP_MAX_PEERS];
static EWRAM_DATA struct MultiplayerQueue sOutgoingQueue;
static EWRAM_DATA struct MpMessage sOutgoingMessages[MULTIPLAYER_QUEUE_PRIORITY_COUNT][MULTIPLAYER_QUEUE_CAPACITY];
struct MpMetrics
{
    u32 stateTransitions;
    u32 recvDatagrams;
    u32 droppedDatagrams;
    u32 peerTimeoutCount;
    u32 rejectInvalidSenderCount;
    u32 rejectStaleSeqCount;
    u32 rejectWrongSessionStateCount;
    u32 recoveryFailureCount;
    bool8 recoveryErrorFlag;
};

static struct MpMetrics sMetrics;
static u32 sStateEnterFrame;
static u32 sRecoveryLastAttemptFrame;
static u32 sRecoveryConfirmStartFrame;

static void MpPeer_ResetAll(void);
static void MpPeer_MarkSeen(u8 peerId, u16 seq, u32 nowFrame);
static void MpPeer_MarkDisconnected(u8 peerId);
static bool8 MpPeer_IsTimedOut(u8 peerId, u32 nowFrame);
static void MpSession_AdvanceState(void);
static void MpSession_SetState(enum MpSessionState newState);
static u8 MpSession_NormalizePriority(u8 priority);
static bool8 MpSession_ShouldScheduleHeartbeat(u32 nowFrame);
static void MpSession_ScheduleHeartbeat(void);
static void MpSession_DrainSendQueue(void);

// Multiplayer session manager policy:
// - Must not depend on union-room state machine symbols.

static void MpPeer_ResetAll(void)
{
    u8 i;

    for (i = 0; i < MP_MAX_PEERS; i++)
    {
        sPeerCache[i].active = FALSE;
        sPeerCache[i].playerId = i;
        sPeerCache[i].lastSeenFrame = 0;
        sPeerCache[i].lastSeqRecv = 0;
        sPeerCache[i].lastSeqSent = 0;
        sPeerCache[i].statusBits = 0;
    }
}

static void MpPeer_MarkSeen(u8 peerId, u16 seq, u32 nowFrame)
{
    if (peerId >= MP_MAX_PEERS)
        return;

    sPeerCache[peerId].active = TRUE;
    sPeerCache[peerId].playerId = peerId;
    sPeerCache[peerId].lastSeenFrame = nowFrame;
    sPeerCache[peerId].lastSeqRecv = seq;
    sPeerCache[peerId].statusBits &= ~MP_PEER_STATUS_DISCONNECTED;
}

static void MpPeer_MarkDisconnected(u8 peerId)
{
    if (peerId >= MP_MAX_PEERS)
        return;

    sPeerCache[peerId].active = FALSE;
    sPeerCache[peerId].statusBits |= MP_PEER_STATUS_DISCONNECTED;
}

static bool8 MpPeer_IsTimedOut(u8 peerId, u32 nowFrame)
{
    if (peerId >= MP_MAX_PEERS || !sPeerCache[peerId].active)
        return FALSE;

    return (nowFrame - sPeerCache[peerId].lastSeenFrame) >= MP_PEER_TIMEOUT_FRAMES;
}

static void MpSession_AdvanceState(void)
{
    u32 elapsedFrames = gMain.vblankCounter2 - sStateEnterFrame;

    switch (sSessionState)
    {
    case MP_STATE_DISCONNECTED:
        break;
    case MP_STATE_CONNECTING:
        if (MultiplayerTransportLink_IsConnected() && MultiplayerTransportLink_IsHandshakeReady())
        {
            MpSession_SetState(MP_STATE_ACTIVE);
            sStateEnterFrame = gMain.vblankCounter2;
        }
        else if (elapsedFrames >= MP_CONNECT_TIMEOUT_FRAMES)
        {
            MpSession_SetState(MP_STATE_DISCONNECTED);
            sStateEnterFrame = gMain.vblankCounter2;
            MultiplayerSession_Stop(&sSession);
        }
        break;
    case MP_STATE_ACTIVE:
        if (MultiplayerTransportLink_IsDegraded())
        {
            MpSession_SetState(MP_STATE_RECOVERING);
            sStateEnterFrame = gMain.vblankCounter2;
            sRecoveryLastAttemptFrame = gMain.vblankCounter2 - MP_RECOVERY_RETRY_INTERVAL_FRAMES;
            sRecoveryConfirmStartFrame = 0;
        }
        break;
    case MP_STATE_RECOVERING:
        if (MultiplayerTransportLink_IsConnected() && MultiplayerTransportLink_IsHandshakeReady())
        {
            if (sRecoveryConfirmStartFrame == 0)
                sRecoveryConfirmStartFrame = gMain.vblankCounter2;

            if ((gMain.vblankCounter2 - sRecoveryConfirmStartFrame) >= MP_RECOVERY_CONFIRM_TIMEOUT_FRAMES)
            {
                sMetrics.recoveryErrorFlag = FALSE;
                MpSession_SetState(MP_STATE_ACTIVE);
                sStateEnterFrame = gMain.vblankCounter2;
            }
        }
        else
        {
            sRecoveryConfirmStartFrame = 0;

            if ((gMain.vblankCounter2 - sRecoveryLastAttemptFrame) >= MP_RECOVERY_RETRY_INTERVAL_FRAMES)
            {
                sRecoveryLastAttemptFrame = gMain.vblankCounter2;
                (void)MultiplayerTransportLink_TryRecover();
            }

            if (elapsedFrames >= MP_RECOVERY_TIMEOUT_FRAMES)
            {
                sMetrics.recoveryFailureCount++;
                sMetrics.recoveryErrorFlag = TRUE;
                MpSession_SetState(MP_STATE_DISCONNECTED);
                sStateEnterFrame = gMain.vblankCounter2;
                MultiplayerSession_Stop(&sSession);
            }
        }
        break;
    }
}

static void MpSession_SetState(enum MpSessionState newState)
{
    if (sSessionState != newState)
    {
        sSessionState = newState;
        sMetrics.stateTransitions++;
    }
}

static u8 MpSession_NormalizePriority(u8 priority)
{
    if (priority >= MULTIPLAYER_QUEUE_PRIORITY_COUNT)
        return MULTIPLAYER_QUEUE_PRIORITY_NORMAL;

    return priority;
}

static bool8 MpSession_ShouldScheduleHeartbeat(u32 nowFrame)
{
    return ((nowFrame % MP_HEARTBEAT_INTERVAL_FRAMES) == 0);
}

static void MpSession_ScheduleHeartbeat(void)
{
    struct MpMessage msg;

    CpuFill32(0, &msg, sizeof(msg));
    msg.header.protocolVersion = MP_PROTOCOL_VERSION_1;
    msg.header.type = MP_MSG_HEARTBEAT;
    msg.header.priority = MULTIPLAYER_QUEUE_PRIORITY_LOW;
    msg.header.senderId = sSession.localPlayerId;
    msg.header.seq = ++sPeerCache[sSession.localPlayerId].lastSeqSent;
    msg.header.payloadLen = 0;

    MpSession_EnqueueMessage(&msg);
}

static void MpSession_DrainSendQueue(void)
{
    struct MultiplayerPacket nextPacket;

    while (MultiplayerQueue_PopNext(&sOutgoingQueue, &nextPacket))
    {
        if (!MultiplayerTransportLink_Send(nextPacket.data, nextPacket.size))
        {
            MultiplayerQueue_Push(&sOutgoingQueue, &nextPacket, ((const struct MpMessage *)nextPacket.data)->header.priority);
            break;
        }
    }
}

void MpSession_Init(void)
{
    CpuFill32(0, &sMetrics, sizeof(sMetrics));
    sSessionState = MP_STATE_DISCONNECTED;
    MultiplayerQueue_Reset(&sOutgoingQueue);
    sStateEnterFrame = gMain.vblankCounter2;
    sRecoveryLastAttemptFrame = 0;
    sRecoveryConfirmStartFrame = 0;
    MpPeer_ResetAll();

    MultiplayerSession_Init(&sSession);
}

void MpSession_Reset(void)
{
    MultiplayerSession_Stop(&sSession);
    CpuFill32(0, &sMetrics, sizeof(sMetrics));
    sSessionState = MP_STATE_DISCONNECTED;
    MultiplayerQueue_Reset(&sOutgoingQueue);
    sStateEnterFrame = gMain.vblankCounter2;
    sRecoveryLastAttemptFrame = 0;
    sRecoveryConfirmStartFrame = 0;
    MpPeer_ResetAll();
}

void MpSession_StartConnecting(u8 startIntentFlags)
{
    AGB_ASSERT(startIntentFlags & MP_SESSION_START_INTENT_EXPLICIT);

    MultiplayerQueue_Reset(&sOutgoingQueue);

    MultiplayerSession_Start(&sSession);
    sMetrics.recoveryErrorFlag = FALSE;
    MpSession_SetState(MP_STATE_CONNECTING);
    sStateEnterFrame = gMain.vblankCounter2;
    sRecoveryLastAttemptFrame = 0;
    sRecoveryConfirmStartFrame = 0;
}

void MpSession_TickOverworldPre(void)
{
    if (sSessionState == MP_STATE_DISCONNECTED)
        return;

    MultiplayerSession_Tick(&sSession);
    MpSession_AdvanceState();

    if (sSessionState == MP_STATE_ACTIVE && MpSession_ShouldScheduleHeartbeat(gMain.vblankCounter2))
        MpSession_ScheduleHeartbeat();

    MpSession_DrainSendQueue();

    if (sSessionState != MP_STATE_DISCONNECTED)
        MpPeer_MarkSeen(sSession.localPlayerId, sPeerCache[sSession.localPlayerId].lastSeqRecv, gMain.vblankCounter2);
}

void MpSession_TickOverworldPost(void)
{
    u8 i;
    u32 nowFrame = gMain.vblankCounter2;

    for (i = 0; i < MP_MAX_PEERS; i++)
    {
        if (MpPeer_IsTimedOut(i, nowFrame))
        {
            sMetrics.peerTimeoutCount++;
            MpPeer_MarkDisconnected(i);
        }
    }
}

enum MpSessionState MpSession_GetState(void)
{
    return sSessionState;
}

bool8 MpSession_IsActive(void)
{
    return sSession.active;
}

bool8 MpSession_EnqueueMessage(const struct MpMessage *msg)
{
    u8 priority;
    u16 writeIndex;
    struct MultiplayerPacket packet;

    if (msg == NULL)
        return FALSE;

    priority = MpSession_NormalizePriority(msg->header.priority);
    writeIndex = sOutgoingQueue.rings[priority].tail;
    sOutgoingMessages[priority][writeIndex] = *msg;

    packet.data = &sOutgoingMessages[priority][writeIndex];
    packet.size = sizeof(*msg);

    return MultiplayerQueue_Push(&sOutgoingQueue, &packet, priority);
}

bool8 MpSession_IsPeerIdValid(u8 peerId)
{
    return (peerId < MP_MAX_PEERS);
}

bool8 MpSession_IsLoopbackEnabled(void)
{
    return FALSE;
}

u8 MpSession_GetLocalPlayerId(void)
{
    return sSession.localPlayerId;
}

u8 MpSession_GetConnectedPlayerCount(void)
{
    return sSession.playerCount;
}

bool8 MpSession_IsSenderInSessionRoster(u8 senderId)
{
    if (!MpSession_IsPeerIdValid(senderId))
        return FALSE;

    return senderId < sSession.playerCount;
}

bool8 MpSession_IsIncomingSeqAcceptable(u8 peerId, u16 seq)
{
    s16 delta;

    if (!MpSession_IsPeerIdValid(peerId))
        return FALSE;

    if (!sPeerCache[peerId].active)
        return TRUE;

    delta = (s16)(seq - sPeerCache[peerId].lastSeqRecv);
    return (delta > 0);
}

void MpSession_OnPeerMessageAccepted(u8 peerId, u16 seq)
{
    sMetrics.recvDatagrams++;
    MpPeer_MarkSeen(peerId, seq, gMain.vblankCounter2);
}

void MpSession_OnPeerMessageRejected(u8 peerId, enum MpRejectReason reason)
{
    sMetrics.recvDatagrams++;
    sMetrics.droppedDatagrams++;

    switch (reason)
    {
    case MP_REJECT_REASON_INVALID_SENDER:
        sMetrics.rejectInvalidSenderCount++;
        break;
    case MP_REJECT_REASON_STALE_SEQ:
        sMetrics.rejectStaleSeqCount++;
        break;
    case MP_REJECT_REASON_WRONG_SESSION_STATE:
        sMetrics.rejectWrongSessionStateCount++;
        break;
    case MP_REJECT_REASON_NONE:
    default:
        break;
    }

    if (!MpSession_IsPeerIdValid(peerId))
        return;

    sPeerCache[peerId].statusBits |= MP_PEER_STATUS_DISCONNECTED;
}

void MpSession_GetMetricsSnapshot(struct MpMetricsSnapshot *snapshot)
{
    u8 priority;

    if (snapshot == NULL)
        return;

    snapshot->stateTransitions = sMetrics.stateTransitions;
    snapshot->recvDatagrams = sMetrics.recvDatagrams;
    snapshot->droppedDatagrams = sMetrics.droppedDatagrams;
    for (priority = 0; priority < MULTIPLAYER_QUEUE_PRIORITY_COUNT; priority++)
        snapshot->queueOverflowsByPriority[priority] = MultiplayerQueue_GetOverflowCountByPriority(&sOutgoingQueue, priority);
    snapshot->handlerRejectCount = MpDispatch_GetRejectCount();
    snapshot->peerTimeoutCount = sMetrics.peerTimeoutCount;
    snapshot->rejectInvalidSenderCount = sMetrics.rejectInvalidSenderCount;
    snapshot->rejectStaleSeqCount = sMetrics.rejectStaleSeqCount;
    snapshot->rejectWrongSessionStateCount = sMetrics.rejectWrongSessionStateCount;
    snapshot->recoveryFailureCount = sMetrics.recoveryFailureCount;
    snapshot->recoveryErrorFlag = sMetrics.recoveryErrorFlag;
}

u8 MpSession_GetPeerCacheKnownCount(void)
{
    return MP_MAX_PEERS;
}

u8 MpSession_GetPeerCacheActiveCount(void)
{
    u8 i;
    u8 count = 0;

    for (i = 0; i < MP_MAX_PEERS; i++)
    {
        if (sPeerCache[i].active)
            count++;
    }

    return count;
}

u8 MpSession_GetPeerHealth(u8 playerId)
{
    if (playerId >= MP_MAX_PEERS)
        return 0;

    return sPeerCache[playerId].statusBits;
}

void MultiplayerSession_Init(struct MultiplayerSession *session)
{
    session->active = FALSE;
    session->localPlayerId = 0;
    session->playerCount = 0;

    MultiplayerTransportLink_Init();
    MultiplayerDispatch_Init();
}

void MultiplayerSession_Start(struct MultiplayerSession *session)
{
    session->active = TRUE;
    session->localPlayerId = MpTransport_GetLocalPlayerId();
    session->playerCount = GetLinkPlayerCount();
}

void MultiplayerSession_Stop(struct MultiplayerSession *session)
{
    session->active = FALSE;
}

void MultiplayerSession_Tick(struct MultiplayerSession *session)
{
    if (!session->active)
        return;

    session->localPlayerId = MpTransport_GetLocalPlayerId();
    session->playerCount = GetLinkPlayerCount();

    MultiplayerDispatch_Tick(session);
}
