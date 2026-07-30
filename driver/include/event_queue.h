#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <ntddk.h>
#undef WIN32_NO_STATUS

#include "event_types.h"

BOOLEAN KdaMonEventQueueInitialize(VOID);
VOID KdaMonEventQueueDestroy(VOID);
BOOLEAN KdaMonEventQueuePush(_In_ KDAMON_EVENT* Event);

_Success_(return != FALSE)
BOOLEAN KdaMonEventQueuePop(_Out_ KDAMON_EVENT * Event);

ULONG KdaMonEventQueueCount(VOID);
PRKEVENT KdaMonEventQueueGetWakeEvent(VOID);