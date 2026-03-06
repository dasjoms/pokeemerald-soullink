#ifndef GUARD_MULTIPLAYER_TRANSPORT_H
#define GUARD_MULTIPLAYER_TRANSPORT_H

#include "global.h"

void MpTransport_Init(void);
void MpTransport_Shutdown(void);
bool8 MpTransport_IsConnected(void);
bool32 MpTransport_SendDatagram(const void *data, u16 size);
u16 MpTransport_PollReceive(void *data, u16 capacity);
u8 MpTransport_GetLocalPlayerId(void);

void MultiplayerTransportLink_Init(void);
bool32 MultiplayerTransportLink_Send(const void *data, u16 size);
u16 MultiplayerTransportLink_Recv(void *data, u16 capacity);
bool8 MultiplayerTransportLink_IsConnected(void);
bool8 MultiplayerTransportLink_IsHandshakeReady(void);
bool8 MultiplayerTransportLink_IsDegraded(void);
bool8 MultiplayerTransportLink_TryRecover(void);

#endif // GUARD_MULTIPLAYER_TRANSPORT_H
