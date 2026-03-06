#ifndef GUARD_MULTIPLAYER_TYPES_H
#define GUARD_MULTIPLAYER_TYPES_H

#include "global.h"

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

enum MultiplayerLinkState
{
    MULTIPLAYER_LINK_OFFLINE,
    MULTIPLAYER_LINK_CONNECTING,
    MULTIPLAYER_LINK_ONLINE,
    MULTIPLAYER_LINK_ERROR,
};

#endif // GUARD_MULTIPLAYER_TYPES_H
