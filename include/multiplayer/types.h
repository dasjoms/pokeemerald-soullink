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

#endif // GUARD_MULTIPLAYER_TYPES_H
