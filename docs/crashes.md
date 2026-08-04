# Crash Log

Kernel crashes encountered during development, with cause and fix for each. Kept as-is (not cleaned up) as a record of debugging progress.
The directory [./dumps/](./dumps/) contains all the dumps for each crash.

---

## Crash #1 - IRQL_NOT_LESS_OR_EQUAL (0xA) on KeSetEvent

> See [.\dumps\1_IRQL_NOT_LESS_OR_EQUAL.dmp](.\dumps\1_IRQL_NOT_LESS_OR_EQUAL.dmp)

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

## Crash #2 - PAGE_FAULT_IN_NONPAGED_AREA (0x50)

> See [.\dumps\2_PAGE_FAULT_IN_NONPAGED_AREA.dmp](.\dumps\2_PAGE_FAULT_IN_NONPAGED_AREA.dmp)

**Bugcheck:** `PAGE_FAULT_IN_NONPAGED_AREA (0x50)`
**Faulting location:** `nt!ObQueryNameStringMode`, called from `nt!IoDeleteDevice`, called from `KdaMonDeleteDevice` (`device.c`), called from `DriverUnload` (`driver_entry.c`)
**Faulting instruction:** `mov rax, [rcx+0A0h]` with `rcx` pointing to freed pool memory — read access to a dangling `DEVICE_OBJECT` pointer

### Cause

`DriverEntry` was missing a `return status;` after logging successful initialization:

```c
	KdPrint((DRIVER_TAG " [SUCCESS]: Initialized successfully\n"));

cleanup_process:
	KdaMonProcessCallbackUnregister();
cleanup_wfp:
	KdaMonWfpSessionCleanup();
cleanup_logwriter:
	KdaMonLogWriterStop();
cleanup_queue:
	KdaMonEventQueueDestroy();
cleanup_device:
	KdaMonDeleteDevice(g_DeviceObject);
cleanup_none:
	return status;
```

Without a `return` or `goto` after the success log, execution fell straight through into the entire cleanup cascade — including `KdaMonDeleteDevice(g_DeviceObject)`, even on a fully successful init. `DriverEntry` then returned `STATUS_SUCCESS` despite having just torn everything down, including the device object.

`KdaMonDeleteDevice` took the pointer by value, so it had no way to null out the caller's `g_DeviceObject` after freeing it:

```c
void KdaMonDeleteDevice(_In_opt_ PDEVICE_OBJECT DeviceObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(KDAMON_SYMLINK_NAME);
	IoDeleteSymbolicLink(&symLink);

	if (DeviceObject != NULL)
	{
		IoDeleteDevice(DeviceObject);
	}
	...
}
```

`g_DeviceObject` was left dangling (non-`NULL`, but pointing at freed memory). When `DriverUnload` later ran, it called `KdaMonDeleteDevice(g_DeviceObject)` a second time. `IoDeleteDevice` internally calls `ObQueryNameString` to resolve the object's name before removing it, which dereferenced the freed `DEVICE_OBJECT` and crashed.

> Without this change another crash occurs (See [.\dumps\3_SYSTEM_THREAD_EXCEPTION_NOT_HANDLED.dmp](.\dumps\3_SYSTEM_THREAD_EXCEPTION_NOT_HANDLED.dmp)).

### Fix

1. Added the missing `return status;` in `DriverEntry` right after the success log, so the cleanup cascade only runs on actual failure paths.
2. Changed `KdaMonDeleteDevice` to take a `PDEVICE_OBJECT*` and null the caller's pointer after deletion, as defense in depth against any future double-cleanup bug:

```c
void KdaMonDeleteDevice(_Inout_ PDEVICE_OBJECT* DeviceObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(KDAMON_SYMLINK_NAME);
	IoDeleteSymbolicLink(&symLink);

	if (*DeviceObject != NULL)
	{
		IoDeleteDevice(*DeviceObject);
		*DeviceObject = NULL;
	}
	...
}
```

Callers now pass `&g_DeviceObject` instead of `g_DeviceObject`.