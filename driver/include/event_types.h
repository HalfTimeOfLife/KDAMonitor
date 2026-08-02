#pragma once

#include <ntddk.h>

typedef enum _KDAMON_EVENT_TYPE
{
    KdaMonEventImageLoad,   // TODO: implemented in v0.6
    KdaMonEventNetwork,     // TODO: implemented in v0.8
    KdaMonEventProcess,     // TODO: implemented in v0.5
    KdaMonEventRegistry,    // TODO: implemented in v0.9
    KdaMonEventThread       // TODO: implemented in v0.10
} KDAMON_EVENT_TYPE;

typedef struct _KDAMON_PROCESS_EVENT_DATA
{
	HANDLE ProcessId;
	HANDLE ParentProcessId;
	BOOLEAN IsCreate;
	WCHAR ImageFileName[260];
} KDAMON_PROCESS_EVENT_DATA;

// TODO: KDAMON_IMAGE_LOAD_EVENT_DATA (v0.6)
// TODO: KDAMON_NETWORK_EVENT_DATA (v0.8)
// TODO: KDAMON_REGISTRY_EVENT_DATA (v0.9)
// TODO: KDAMON_THREAD_EVENT_DATA (v0.10)

typedef struct _KDAMON_EVENT
{
    KDAMON_EVENT_TYPE Type;
    LARGE_INTEGER Timestamp;
    ULONG Id;
    // future members for event data
    union
    {
        KDAMON_PROCESS_EVENT_DATA Process;
    } Data;
} KDAMON_EVENT, * PKDAMON_EVENT;