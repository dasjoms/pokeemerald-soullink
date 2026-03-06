#include "global.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/queue.h"
#include "multiplayer/transport.h"

static struct MultiplayerSession sSession;
static enum MpSessionState sSessionState;
static struct MpPeerState sPeerCache[MP_MAX_PEERS];
static struct MpMessage sMessageQueue[MULTIPLAYER_QUEUE_CAPACITY];
static u8 sMessageQueueTail;
static u8 sMessageQueueCount;

static void MpSession_ClearPeerCache(void)
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

void MpSession_Init(void)
{
    sSessionState = MP_STATE_DISCONNECTED;
    sMessageQueueTail = 0;
    sMessageQueueCount = 0;
    MpSession_ClearPeerCache();

    MultiplayerSession_Init(&sSession);
}

void MpSession_Reset(void)
{
    MultiplayerSession_Stop(&sSession);
    sSessionState = MP_STATE_DISCONNECTED;
    sMessageQueueTail = 0;
    sMessageQueueCount = 0;
    MpSession_ClearPeerCache();
}

void MpSession_StartConnecting(void)
{
    MultiplayerSession_Start(&sSession);
    sSessionState = MP_STATE_CONNECTING;
}

void MpSession_TickOverworldPre(void)
{
    if (sSessionState == MP_STATE_DISCONNECTED)
        return;

    MultiplayerSession_Tick(&sSession);

    if (sSessionState == MP_STATE_CONNECTING)
        sSessionState = MP_STATE_ACTIVE;
}

void MpSession_TickOverworldPost(void)
{
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
