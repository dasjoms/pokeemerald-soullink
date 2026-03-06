#ifndef GUARD_MULTIPLAYER_QUEUE_H
#define GUARD_MULTIPLAYER_QUEUE_H

#include "multiplayer/types.h"

#define MULTIPLAYER_QUEUE_CAPACITY 16

enum MultiplayerQueuePriority
{
    MULTIPLAYER_QUEUE_PRIORITY_LOW,
    MULTIPLAYER_QUEUE_PRIORITY_NORMAL,
    MULTIPLAYER_QUEUE_PRIORITY_HIGH,
    MULTIPLAYER_QUEUE_PRIORITY_COUNT,
};

struct MultiplayerQueueRing
{
    u16 head;
    u16 tail;
    u16 count;
    struct MultiplayerPacket entries[MULTIPLAYER_QUEUE_CAPACITY];
};

struct MultiplayerQueue
{
    struct MultiplayerQueueRing rings[MULTIPLAYER_QUEUE_PRIORITY_COUNT];
    u16 count;
    u32 overflowCount;
    u32 overflowByPriority[MULTIPLAYER_QUEUE_PRIORITY_COUNT];
};

void MultiplayerQueue_Init(struct MultiplayerQueue *queue);
void MultiplayerQueue_Reset(struct MultiplayerQueue *queue);
bool32 MultiplayerQueue_Push(struct MultiplayerQueue *queue, const struct MultiplayerPacket *packet, u8 priority);
bool32 MultiplayerQueue_PopNext(struct MultiplayerQueue *queue, struct MultiplayerPacket *packet);
u16 MultiplayerQueue_GetCount(const struct MultiplayerQueue *queue);
u32 MultiplayerQueue_GetOverflowCount(const struct MultiplayerQueue *queue);
u32 MultiplayerQueue_GetOverflowCountByPriority(const struct MultiplayerQueue *queue, u8 priority);

#endif // GUARD_MULTIPLAYER_QUEUE_H
