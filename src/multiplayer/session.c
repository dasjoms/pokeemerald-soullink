#include "global.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/queue.h"
#include "multiplayer/transport.h"

#define MP_CONNECT_TIMEOUT_FRAMES  (60 * 10)
#define MP_RECOVERY_TIMEOUT_FRAMES (60 * 5)
#define MP_PEER_TIMEOUT_FRAMES     (60 * 5)

#define MP_PEER_STATUS_DISCONNECTED (1 << 0)

static struct MultiplayerSession sSession;
static enum MpSessionState sSessionState;
static struct MpPeerState sPeerCache[MP_MAX_PEERS];
static struct MpMessage sMessageQueue[MULTIPLAYER_QUEUE_CAPACITY];
static u8 sMessageQueueTail;
static u8 sMessageQueueCount;
static u32 sStateEnterFrame;

static void MpPeer_ResetAll(void);
static void MpPeer_MarkSeen(u8 peerId, u16 seq, u32 nowFrame);
static void MpPeer_MarkDisconnected(u8 peerId);
static bool8 MpPeer_IsTimedOut(u8 peerId, u32 nowFrame);
static void MpSession_AdvanceState(void);

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
            sSessionState = MP_STATE_ACTIVE;
            sStateEnterFrame = gMain.vblankCounter2;
        }
        else if (elapsedFrames >= MP_CONNECT_TIMEOUT_FRAMES)
        {
            sSessionState = MP_STATE_DISCONNECTED;
            sStateEnterFrame = gMain.vblankCounter2;
            MultiplayerSession_Stop(&sSession);
        }
        break;
    case MP_STATE_ACTIVE:
        if (MultiplayerTransportLink_IsDegraded())
        {
            sSessionState = MP_STATE_RECOVERING;
            sStateEnterFrame = gMain.vblankCounter2;
        }
        break;
    case MP_STATE_RECOVERING:
        if (MultiplayerTransportLink_TryRecover())
        {
            sSessionState = MP_STATE_ACTIVE;
            sStateEnterFrame = gMain.vblankCounter2;
        }
        else if (elapsedFrames >= MP_RECOVERY_TIMEOUT_FRAMES)
        {
            sSessionState = MP_STATE_DISCONNECTED;
            sStateEnterFrame = gMain.vblankCounter2;
            MultiplayerSession_Stop(&sSession);
        }
        break;
    }
}

void MpSession_Init(void)
{
    sSessionState = MP_STATE_DISCONNECTED;
    sMessageQueueTail = 0;
    sMessageQueueCount = 0;
    sStateEnterFrame = gMain.vblankCounter2;
    MpPeer_ResetAll();

    MultiplayerSession_Init(&sSession);
}

void MpSession_Reset(void)
{
    MultiplayerSession_Stop(&sSession);
    sSessionState = MP_STATE_DISCONNECTED;
    sMessageQueueTail = 0;
    sMessageQueueCount = 0;
    sStateEnterFrame = gMain.vblankCounter2;
    MpPeer_ResetAll();
}

void MpSession_StartConnecting(void)
{
    MultiplayerSession_Start(&sSession);
    sSessionState = MP_STATE_CONNECTING;
    sStateEnterFrame = gMain.vblankCounter2;
}

void MpSession_TickOverworldPre(void)
{
    if (sSessionState == MP_STATE_DISCONNECTED)
        return;

    MultiplayerSession_Tick(&sSession);
    MpSession_AdvanceState();

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
            MpPeer_MarkDisconnected(i);
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
    if (msg == NULL)
        return FALSE;

    if (sMessageQueueCount >= MULTIPLAYER_QUEUE_CAPACITY)
        return FALSE;

    sMessageQueue[sMessageQueueTail] = *msg;
    sMessageQueueTail = (sMessageQueueTail + 1) % MULTIPLAYER_QUEUE_CAPACITY;
    sMessageQueueCount++;

    return TRUE;
}

bool8 MpSession_IsPeerIdValid(u8 peerId)
{
    return (peerId < MP_MAX_PEERS);
}

void MpSession_OnPeerMessageAccepted(u8 peerId, u16 seq)
{
    MpPeer_MarkSeen(peerId, seq, gMain.vblankCounter2);
}

void MpSession_OnPeerMessageRejected(u8 peerId)
{
    if (!MpSession_IsPeerIdValid(peerId))
        return;

    sPeerCache[peerId].statusBits |= MP_PEER_STATUS_DISCONNECTED;
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
}

void MultiplayerSession_Stop(struct MultiplayerSession *session)
{
    session->active = FALSE;
}

void MultiplayerSession_Tick(struct MultiplayerSession *session)
{
    if (!session->active)
        return;

    MultiplayerDispatch_Tick(session);
}
