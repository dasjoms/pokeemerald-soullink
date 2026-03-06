#include "global.h"
#include "multiplayer/transport.h"

void MultiplayerTransportLink_Init(void)
{
}

bool32 MultiplayerTransportLink_Send(const void *data, u16 size)
{
    (void)data;
    (void)size;
    return FALSE;
}

u16 MultiplayerTransportLink_Recv(void *data, u16 capacity)
{
    (void)data;
    (void)capacity;
    return 0;
}
