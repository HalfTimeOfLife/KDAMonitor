#pragma once

#include <ntddk.h>

typedef enum _KDAMON_EVENT_TYPE
{
    KdaMonEventImageLoad,
    KdaMonEventNetwork,
    KdaMonEventProcess,
    KdaMonEventRegistry,    // TODO: implemented in v0.9
    KdaMonEventThread       // TODO: implemented in v0.10
} KDAMON_EVENT_TYPE;

typedef enum _KDAMON_NETWORK_DIRECTION {
    KDAMON_NETWORK_DIRECTION_INBOUND,
    KDAMON_NETWORK_DIRECTION_OUTBOUND,
} KDAMON_NETWORK_DIRECTION;

typedef struct _KDAMON_PROCESS_EVENT_DATA
{
	HANDLE ProcessId;
	HANDLE ParentProcessId;
	BOOLEAN IsCreate;
	WCHAR ImageFileName[260];
} KDAMON_PROCESS_EVENT_DATA;

typedef struct _KDAMON_IMAGE_LOAD_EVENT_DATA
{
    HANDLE ProcessId;

    PVOID ImageBase;
    SIZE_T ImageSize;

    ULONG Properties;

    ULONG SystemModeImage;
    ULONG ImageMappedToAllPids;
    ULONG ImagePartialMap;

    ULONG SignatureLevel;
    ULONG SignatureType;

	WCHAR ImageFileName[260];
} KDAMON_IMAGE_LOAD_EVENT_DATA;

typedef struct _KDAMON_NETWORK_EVENT_DATA {
    ULONG  ProcessId;
    WCHAR ProcessPath[260];

    UINT8  Protocol;

    ULONG  LocalIp;
    USHORT LocalPort;

    ULONG  RemoteIp;
    USHORT RemotePort;

    KDAMON_NETWORK_DIRECTION Direction;
} KDAMON_NETWORK_EVENT_DATA;

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
		KDAMON_IMAGE_LOAD_EVENT_DATA ImageLoad;
        KDAMON_NETWORK_EVENT_DATA Network;
    } Data;
} KDAMON_EVENT, * PKDAMON_EVENT;