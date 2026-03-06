#ifndef GUARD_MULTIPLAYER_SESSION_H
#define GUARD_MULTIPLAYER_SESSION_H

#include "multiplayer/queue.h"
#include "multiplayer/types.h"

// Multiplayer session manager policy:
// - Must not depend on union-room state machine symbols.

enum MpSessionStartIntent
{
    MP_SESSION_START_INTENT_NONE = 0,
    MP_SESSION_START_INTENT_EXPLICIT = (1 << 0),
};

struct MpMetricsSnapshot
{
    u32 stateTransitions;
    u32 recvDatagrams;
    u32 droppedDatagrams;
    u32 queueOverflowsByPriority[MULTIPLAYER_QUEUE_PRIORITY_COUNT];
    u32 handlerRejectCount;
    u32 peerTimeoutCount;
};

void MpSession_Init(void);
void MpSession_Reset(void);
void MpSession_StartConnecting(u8 startIntentFlags);
void MpSession_TickOverworldPre(void);
void MpSession_TickOverworldPost(void);
enum MpSessionState MpSession_GetState(void);
bool8 MpSession_IsActive(void);
bool8 MpSession_EnqueueMessage(const struct MpMessage *msg);
bool8 MpSession_IsPeerIdValid(u8 peerId);
void MpSession_OnPeerMessageAccepted(u8 peerId, u16 seq);
void MpSession_OnPeerMessageRejected(u8 peerId);
void MpSession_GetMetricsSnapshot(struct MpMetricsSnapshot *snapshot);

// Read-only peer cache accessors (for debug UI).
u8 MpSession_GetPeerCacheKnownCount(void);
u8 MpSession_GetPeerCacheActiveCount(void);
u8 MpSession_GetPeerHealth(u8 playerId);

void MultiplayerSession_Init(struct MultiplayerSession *session);
void MultiplayerSession_Start(struct MultiplayerSession *session);
void MultiplayerSession_Stop(struct MultiplayerSession *session);
void MultiplayerSession_Tick(struct MultiplayerSession *session);

#endif // GUARD_MULTIPLAYER_SESSION_H
