#ifndef GUARD_MULTIPLAYER_TRANSPORT_H
#define GUARD_MULTIPLAYER_TRANSPORT_H

#include "global.h"

void MultiplayerTransportLink_Init(void);
bool32 MultiplayerTransportLink_Send(const void *data, u16 size);
u16 MultiplayerTransportLink_Recv(void *data, u16 capacity);

#endif // GUARD_MULTIPLAYER_TRANSPORT_H
