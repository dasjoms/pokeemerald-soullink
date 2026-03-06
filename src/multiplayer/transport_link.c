#include "global.h"
#include "link.h"
#include "multiplayer/transport.h"

void MpTransport_Init(void)
{
    OpenLink();
}

void MpTransport_Shutdown(void)
{
    ClearLinkCallback();
    CloseLink();
}

bool8 MpTransport_IsConnected(void)
{
    if (HasLinkErrorOccurred())
        return FALSE;

    if (gWirelessCommType)
        return GetLinkPlayerCount() > 1;

    return IsLinkConnectionEstablished() && GetLinkPlayerCount() > 1;
}

bool32 MpTransport_SendDatagram(const void *data, u16 size)
{
    if (data == NULL || size == 0)
        return FALSE;

    if (!IsLinkTaskFinished())
        return FALSE;

    return SendBlock(0, data, size);
}

u16 MpTransport_PollReceive(void *data, u16 capacity)
{
    u16 copySize;
    u16 recvSize;
    u8 status;
    u8 i;

    if (data == NULL || capacity == 0)
        return 0;

    status = GetBlockReceivedStatus();
    if (status == 0)
        return 0;

    for (i = 0; i < MAX_RFU_PLAYERS; i++)
    {
        if ((status & (1 << i)) != 0)
        {
            recvSize = GetBlockReceivedSize(i);
            copySize = min(recvSize, capacity);
            memcpy(data, gBlockRecvBuffer[i], copySize);
            ResetBlockReceivedFlag(i);
            return copySize;
        }
    }

    return 0;
}

u8 MpTransport_GetLocalPlayerId(void)
{
    return GetMultiplayerId();
}

void MultiplayerTransportLink_Init(void)
{
    MpTransport_Init();
}

bool32 MultiplayerTransportLink_Send(const void *data, u16 size)
{
    return MpTransport_SendDatagram(data, size);
}

u16 MultiplayerTransportLink_Recv(void *data, u16 capacity)
{
    return MpTransport_PollReceive(data, capacity);
}

bool8 MultiplayerTransportLink_IsConnected(void)
{
    return MpTransport_IsConnected();
}

bool8 MultiplayerTransportLink_IsHandshakeReady(void)
{
    return MpTransport_IsConnected();
}

bool8 MultiplayerTransportLink_IsDegraded(void)
{
    return HasLinkErrorOccurred();
}

bool8 MultiplayerTransportLink_TryRecover(void)
{
    if (MpTransport_IsConnected())
        return TRUE;

    MpTransport_Shutdown();
    MpTransport_Init();

    return MpTransport_IsConnected();
}
