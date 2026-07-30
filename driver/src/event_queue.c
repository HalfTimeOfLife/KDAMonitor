#include "event_queue.h"

#define KDAMON_EVENT_QUEUE_SIZE 1024

typedef struct _KDAMON_EVENT_QUEUE
{
    KDAMON_EVENT Buffer[KDAMON_EVENT_QUEUE_SIZE];

    ULONG Head;
    ULONG Tail;
    ULONG Count;

    ULONG DroppedEvents;
    ULONG NextId;

    KSPIN_LOCK Lock;
    KEVENT WakeEvent;
} KDAMON_EVENT_QUEUE;

static KDAMON_EVENT_QUEUE g_EventQueue;

static ULONG EventQueueNextIndex(_In_ ULONG Index)
{
    Index++;

    if (Index == KDAMON_EVENT_QUEUE_SIZE)
    {
        Index = 0;
    }

    return Index;
}

BOOLEAN KdaMonEventQueueInitialize(VOID)
{
    RtlZeroMemory(&g_EventQueue, sizeof(g_EventQueue));

    KeInitializeEvent(&g_EventQueue.WakeEvent, SynchronizationEvent, FALSE);
    KeInitializeSpinLock(&g_EventQueue.Lock);

    return TRUE;
}

VOID KdaMonEventQueueDestroy(VOID)
{
}

BOOLEAN KdaMonEventQueuePush(_In_ KDAMON_EVENT* Event)
{
    KIRQL OldIrql;

    if (Event == NULL)
    {
        return FALSE;
    }

    KeAcquireSpinLock(&g_EventQueue.Lock, &OldIrql);

    if (g_EventQueue.Count == KDAMON_EVENT_QUEUE_SIZE)
    {
        g_EventQueue.DroppedEvents++;
        KeReleaseSpinLock(&g_EventQueue.Lock, OldIrql);
        return FALSE;
    }

    Event->Id = g_EventQueue.NextId++;

    g_EventQueue.Buffer[g_EventQueue.Tail] = *Event;
    g_EventQueue.Tail = EventQueueNextIndex(g_EventQueue.Tail);

    g_EventQueue.Count++;

    KeSetEvent(&g_EventQueue.WakeEvent, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(&g_EventQueue.Lock, OldIrql);

    return TRUE;
}

BOOLEAN KdaMonEventQueuePop(_Out_ KDAMON_EVENT* Event)
{
    KIRQL OldIrql;

    if (Event == NULL)
    {
        return FALSE;
    }

    KeAcquireSpinLock(&g_EventQueue.Lock, &OldIrql);

    if (g_EventQueue.Count == 0)
    {
        KeReleaseSpinLock(&g_EventQueue.Lock, OldIrql);
        return FALSE;
    }

    *Event = g_EventQueue.Buffer[g_EventQueue.Head];
    g_EventQueue.Head = EventQueueNextIndex(g_EventQueue.Head);

    g_EventQueue.Count--;

    KeReleaseSpinLock(&g_EventQueue.Lock, OldIrql);

    return TRUE;
}

ULONG KdaMonEventQueueCount(VOID)
{
    KIRQL OldIrql;
    ULONG EventCount;

    KeAcquireSpinLock(&g_EventQueue.Lock, &OldIrql);

    EventCount = g_EventQueue.Count;

    KeReleaseSpinLock(&g_EventQueue.Lock, OldIrql);

    return EventCount;
}

PRKEVENT KdaMonEventQueueGetWakeEvent(VOID)
{
    return &g_EventQueue.WakeEvent;
}