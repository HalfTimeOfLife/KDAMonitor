#pragma once

#include <ntddk.h>

#include "event_types.h"

BOOLEAN KdaMonEventQueueInitialize(VOID);

VOID KdaMonEventQueueDestroy(VOID);

BOOLEAN KdaMonEventQueuePush(_In_ KDAMON_EVENT* Event);

_Success_(return != FALSE)
BOOLEAN KdaMonEventQueuePop(_Out_ KDAMON_EVENT* Event);

ULONG KdaMonEventQueueCount(VOID);