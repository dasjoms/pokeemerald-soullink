#include "global.h"
#include "multiplayer/dispatch.h"
#include "multiplayer/session.h"
#include "multiplayer/transport.h"

static u32 sDispatchRejectCount;

static bool8 MpDispatch_HandleHeartbeat(const struct MpMessage *msg)
{
    return (msg->header.payloadLen == 0);
}

static bool8 MpDispatch_HandleUnhandled(const struct MpMessage *msg)
{
    (void)msg;
    return FALSE;
}

const MpDispatchHandler gMpDispatchHandlers[MP_DISPATCH_MESSAGE_TYPE_COUNT] = {
    [MP_MSG_HEARTBEAT] = MpDispatch_HandleHeartbeat,
};

void MultiplayerDispatch_Init(void)
{
    sDispatchRejectCount = 0;
}

void MultiplayerDispatch_Tick(struct MultiplayerSession *session)
{
    struct MpMessage msg;
    u16 payloadSize;
    u16 recvSize;

    (void)session;

    while ((recvSize = MultiplayerTransportLink_Recv(&msg, sizeof(msg))) != 0)
    {
        if (recvSize < sizeof(struct MpMessageHeader))
        {
            sDispatchRejectCount++;
            continue;
        }

        payloadSize = recvSize - sizeof(msg.header);
        if (msg.header.payloadLen > payloadSize)
        {
            sDispatchRejectCount++;
            MpSession_OnPeerMessageRejected(msg.header.senderId);
            continue;
        }

        MpDispatch_HandleInbound(&msg);
    }
}

bool8 MpDispatch_HandleInbound(const struct MpMessage *msg)
{
    MpDispatchHandler handler;

    if (msg == NULL)
    {
        sDispatchRejectCount++;
        return FALSE;
    }

    if (msg->header.protocolVersion != MP_PROTOCOL_VERSION_1)
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    if (msg->header.type >= MP_DISPATCH_MESSAGE_TYPE_COUNT)
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    if (msg->header.payloadLen > MP_MESSAGE_PAYLOAD_MAX)
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    if (!MpSession_IsPeerIdValid(msg->header.senderId))
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    if (!MpSession_IsIncomingSeqAcceptable(msg->header.senderId, msg->header.seq))
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    handler = gMpDispatchHandlers[msg->header.type];
    if (handler == NULL)
        handler = MpDispatch_HandleUnhandled;

    if (!handler(msg))
    {
        sDispatchRejectCount++;
        MpSession_OnPeerMessageRejected(msg->header.senderId);
        return FALSE;
    }

    MpSession_OnPeerMessageAccepted(msg->header.senderId, msg->header.seq);
    return TRUE;
}

u32 MpDispatch_GetRejectCount(void)
{
    return sDispatchRejectCount;
}
