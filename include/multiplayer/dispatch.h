#ifndef GUARD_MULTIPLAYER_DISPATCH_H
#define GUARD_MULTIPLAYER_DISPATCH_H

#include <limits.h>

#include "multiplayer/types.h"

typedef bool8 (*MpDispatchHandler)(const struct MpMessage *msg);

#define MP_DISPATCH_MESSAGE_TYPE_COUNT UCHAR_MAX + 1

extern const MpDispatchHandler gMpDispatchHandlers[MP_DISPATCH_MESSAGE_TYPE_COUNT];

void MultiplayerDispatch_Init(void);
void MultiplayerDispatch_Tick(struct MultiplayerSession *session);
bool8 MpDispatch_HandleInbound(const struct MpMessage *msg);
u32 MpDispatch_GetRejectCount(void);

#endif // GUARD_MULTIPLAYER_DISPATCH_H
