#ifndef GUARD_MULTIPLAYER_TYPES_H
#define GUARD_MULTIPLAYER_TYPES_H

#include "global.h"
#include "link.h"

#define MP_MAX_PEERS ((MAX_RFU_PLAYERS > MAX_LINK_PLAYERS) ? MAX_RFU_PLAYERS : MAX_LINK_PLAYERS)
#define MP_MESSAGE_PAYLOAD_MAX 96

enum MpProtocolVersion
{
    MP_PROTOCOL_VERSION_1 = 1,
};

enum MpMessageType
{
    MP_MSG_HEARTBEAT,
};

enum MpSessionState
{
    MP_STATE_DISCONNECTED,
    MP_STATE_CONNECTING,
    MP_STATE_ACTIVE,
    MP_STATE_RECOVERING,
};

struct MultiplayerPacket
{
    const void *data;
    u16 size;
};

struct MultiplayerSession
{
    bool8 active;
    u8 localPlayerId;
    u8 playerCount;
};

struct MpPeerState
{
    bool8 active;
    u8 playerId;
    u32 lastSeenFrame;
    u16 lastSeqRecv;
    u16 lastSeqSent;
    u8 statusBits;
};

struct MpMessageHeader
{
    u8 protocolVersion;
    u8 type;
    u8 priority;
    u8 senderId;
    u16 seq;
    u16 payloadLen;
};

struct MpMessage
{
    struct MpMessageHeader header;
    u8 payload[MP_MESSAGE_PAYLOAD_MAX];
};

#endif // GUARD_MULTIPLAYER_TYPES_H
