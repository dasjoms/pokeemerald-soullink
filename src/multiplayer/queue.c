#include "global.h"
#include "multiplayer/queue.h"

static u8 NormalizePriority(u8 priority)
{
    if (priority >= MULTIPLAYER_QUEUE_PRIORITY_COUNT)
        return MULTIPLAYER_QUEUE_PRIORITY_NORMAL;

    return priority;
}

static void ResetRing(struct MultiplayerQueueRing *ring)
{
    ring->head = 0;
    ring->tail = 0;
    ring->count = 0;
}

static bool32 PushToRing(struct MultiplayerQueueRing *ring, const struct MultiplayerPacket *packet)
{
    if (ring->count >= MULTIPLAYER_QUEUE_CAPACITY)
        return FALSE;

    ring->entries[ring->tail] = *packet;
    ring->tail = (ring->tail + 1) % MULTIPLAYER_QUEUE_CAPACITY;
    ring->count++;
    return TRUE;
}

static bool32 PopFromRing(struct MultiplayerQueueRing *ring, struct MultiplayerPacket *packet)
{
    if (ring->count == 0)
        return FALSE;

    *packet = ring->entries[ring->head];
    ring->head = (ring->head + 1) % MULTIPLAYER_QUEUE_CAPACITY;
    ring->count--;
    return TRUE;
}

void MultiplayerQueue_Init(struct MultiplayerQueue *queue)
{
    MultiplayerQueue_Reset(queue);
}

void MultiplayerQueue_Reset(struct MultiplayerQueue *queue)
{
    u8 i;

    for (i = 0; i < MULTIPLAYER_QUEUE_PRIORITY_COUNT; i++)
    {
        ResetRing(&queue->rings[i]);
        queue->overflowByPriority[i] = 0;
    }

    queue->count = 0;
    queue->overflowCount = 0;
}

bool32 MultiplayerQueue_Push(struct MultiplayerQueue *queue, const struct MultiplayerPacket *packet, u8 priority)
{
    struct MultiplayerQueueRing *ring;

    priority = NormalizePriority(priority);
    ring = &queue->rings[priority];

    // Overflow policy:
    // - LOW: drop newest (incoming packet).
    // - NORMAL: drop oldest queued packet, then enqueue incoming packet.
    // - HIGH: reject enqueue.
    if (ring->count >= MULTIPLAYER_QUEUE_CAPACITY)
    {
        queue->overflowCount++;
        queue->overflowByPriority[priority]++;

        if (priority == MULTIPLAYER_QUEUE_PRIORITY_LOW || priority == MULTIPLAYER_QUEUE_PRIORITY_HIGH)
            return FALSE;

        ring->head = (ring->head + 1) % MULTIPLAYER_QUEUE_CAPACITY;
        ring->count--;
        queue->count--;
    }

    if (!PushToRing(ring, packet))
        return FALSE;

    queue->count++;
    return TRUE;
}

bool32 MultiplayerQueue_PopNext(struct MultiplayerQueue *queue, struct MultiplayerPacket *packet)
{
    u8 priority;

    for (priority = MULTIPLAYER_QUEUE_PRIORITY_HIGH + 1; priority > 0; priority--)
    {
        u8 idx = priority - 1;

        if (PopFromRing(&queue->rings[idx], packet))
        {
            queue->count--;
            return TRUE;
        }
    }

    return FALSE;
}

u16 MultiplayerQueue_GetCount(const struct MultiplayerQueue *queue)
{
    return queue->count;
}

u32 MultiplayerQueue_GetOverflowCount(const struct MultiplayerQueue *queue)
{
    return queue->overflowCount;
}

u32 MultiplayerQueue_GetOverflowCountByPriority(const struct MultiplayerQueue *queue, u8 priority)
{
    priority = NormalizePriority(priority);
    return queue->overflowByPriority[priority];
}
