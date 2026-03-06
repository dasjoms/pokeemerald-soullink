#include "global.h"
#include "multiplayer/queue.h"

void MultiplayerQueue_Init(struct MultiplayerQueue *queue)
{
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

bool32 MultiplayerQueue_Enqueue(struct MultiplayerQueue *queue, const struct MultiplayerPacket *packet)
{
    if (queue->count >= MULTIPLAYER_QUEUE_CAPACITY)
        return FALSE;

    queue->entries[queue->tail] = *packet;
    queue->tail = (queue->tail + 1) % MULTIPLAYER_QUEUE_CAPACITY;
    queue->count++;
    return TRUE;
}

bool32 MultiplayerQueue_Dequeue(struct MultiplayerQueue *queue, struct MultiplayerPacket *packet)
{
    if (queue->count == 0)
        return FALSE;

    *packet = queue->entries[queue->head];
    queue->head = (queue->head + 1) % MULTIPLAYER_QUEUE_CAPACITY;
    queue->count--;
    return TRUE;
}
