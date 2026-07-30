# Crash Log

Kernel crashes encountered during development, with cause and fix for each. Kept as-is (not cleaned up) as a record of debugging progress.
The directory [./dumps/](./dumps/) contains all the dumps for each crash.

---

## Crash #1 - IRQL_NOT_LESS_OR_EQUAL (0xA) on KeSetEvent

> See [.\dumps\IRQL_NOT_LESS_OR_EQUAL.dmp](.\dumps\IRQL_NOT_LESS_OR_EQUAL.dmp)

**Bugcheck:** `IRQL_NOT_LESS_OR_EQUAL (0xA)`
**Faulting location:** `nt!KeSetEvent`, called from `KdaMonEventQueuePush` (`event_queue.c`)
**Faulting instruction:** `mov r12, [r12]` with `r12 = 0` — NULL pointer dereference while at `DISPATCH_LEVEL` (IRQL 2)

### Cause

In `KdaMonEventQueueInitialize`, the event was initialized *before* the containing structure was zeroed:

```c
BOOLEAN KdaMonEventQueueInitialize(VOID)
{
    KeInitializeEvent(&g_EventQueue.WakeEvent, SynchronizationEvent, FALSE);

    RtlZeroMemory(&g_EventQueue, sizeof(g_EventQueue));   // wipes WakeEvent right after init

    KeInitializeSpinLock(&g_EventQueue.Lock);

    return TRUE;
}
```

`KeInitializeEvent` sets up the dispatcher object's internal self-referencing wait list. The subsequent `RtlZeroMemory` over the whole struct zeroed out `WakeEvent`, turning it into an invalid kernel object (its internal list pointer became `NULL` instead of self-referencing).

Later, `KdaMonEventQueuePush` calls `KeSetEvent(&g_EventQueue.WakeEvent, ...)` under a spinlock (IRQL = DISPATCH_LEVEL). `KeSetEvent` tries to walk the event's internal wait list, dereferences the `NULL` pointer left by the zeroing, and crashes.

### Fix

Zero the structure first, then initialize the event and spinlock on top of the zeroed memory:

```c
BOOLEAN KdaMonEventQueueInitialize(VOID)
{
    RtlZeroMemory(&g_EventQueue, sizeof(g_EventQueue));

    KeInitializeEvent(&g_EventQueue.WakeEvent, SynchronizationEvent, FALSE);
    KeInitializeSpinLock(&g_EventQueue.Lock);

    return TRUE;
}
```

---