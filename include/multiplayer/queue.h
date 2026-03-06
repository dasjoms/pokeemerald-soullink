#ifndef GUARD_MULTIPLAYER_QUEUE_H
#define GUARD_MULTIPLAYER_QUEUE_H

#include "multiplayer/types.h"

#define MULTIPLAYER_QUEUE_CAPACITY 16

struct MultiplayerQueue
{
    u16 head;
    u16 tail;
    u16 count;
    struct MultiplayerPacket entries[MULTIPLAYER_QUEUE_CAPACITY];
};

void MultiplayerQueue_Init(struct MultiplayerQueue *queue);
bool32 MultiplayerQueue_Enqueue(struct MultiplayerQueue *queue, const struct MultiplayerPacket *packet);
bool32 MultiplayerQueue_Dequeue(struct MultiplayerQueue *queue, struct MultiplayerPacket *packet);

#endif // GUARD_MULTIPLAYER_QUEUE_H
