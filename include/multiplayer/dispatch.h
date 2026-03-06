#ifndef GUARD_MULTIPLAYER_DISPATCH_H
#define GUARD_MULTIPLAYER_DISPATCH_H

#include "multiplayer/types.h"

void MultiplayerDispatch_Init(void);
void MultiplayerDispatch_Tick(struct MultiplayerSession *session);

#endif // GUARD_MULTIPLAYER_DISPATCH_H
