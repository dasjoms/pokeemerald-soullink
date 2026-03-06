#ifndef GUARD_MULTIPLAYER_SESSION_H
#define GUARD_MULTIPLAYER_SESSION_H

#include "multiplayer/types.h"

void MultiplayerSession_Init(struct MultiplayerSession *session);
void MultiplayerSession_Start(struct MultiplayerSession *session);
void MultiplayerSession_Stop(struct MultiplayerSession *session);
void MultiplayerSession_Tick(struct MultiplayerSession *session);

#endif // GUARD_MULTIPLAYER_SESSION_H
