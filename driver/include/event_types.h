#pragma once

#include <ntddk.h>

#include "kdamon_config.h"

typedef enum _KDAMON_EVENT_TYPE
{
    KdaMonEventImageLoad,
    KdaMonEventNetwork,
    KdaMonEventProcess,
    KdaMonEventRegistry,
    KdaMonEventThread       // TODO: implemented in v0.10
} KDAMON_EVENT_TYPE;

typedef enum _KDAMON_NETWORK_DIRECTION {
    KDAMON_NETWORK_DIRECTION_INBOUND,
    KDAMON_NETWORK_DIRECTION_OUTBOUND,
} KDAMON_NETWORK_DIRECTION;

typedef enum _KDAMON_REGISTRY_ACTION {
    KDAMON_REGISTRY_ACTION_SET_VALUE,
    KDAMON_REGISTRY_ACTION_DELETE_VALUE,
    KDAMON_REGISTRY_ACTION_CREATE_KEY,
} KDAMON_REGISTRY_ACTION;

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

	WCHAR ImageFileName[KDAMON_REG_PATH_MAX];
} KDAMON_IMAGE_LOAD_EVENT_DATA;

typedef struct _KDAMON_NETWORK_EVENT_DATA {
    HANDLE  ProcessId;
    WCHAR ProcessPath[KDAMON_REG_PATH_MAX];

    UINT8  Protocol;

    ULONG  LocalIp;
    USHORT LocalPort;

    ULONG  RemoteIp;
    USHORT RemotePort;

    KDAMON_NETWORK_DIRECTION Direction;
} KDAMON_NETWORK_EVENT_DATA;

typedef struct _KDAMON_REGISTRY_EVENT_DATA {
    HANDLE ProcessId;
    WCHAR ProcessPath[KDAMON_REG_PATH_MAX];

    KDAMON_REGISTRY_ACTION Action;

    WCHAR KeyPath[KDAMON_REG_PATH_MAX];
    WCHAR ValueName[KDAMON_REG_VALUENAME_MAX];
    ULONG ValueType;
    UCHAR ValueData[KDAMON_REG_VALUEDATA_MAX];
    ULONG ValueDataSize;
    NTSTATUS Status;
} KDAMON_REGISTRY_EVENT_DATA;

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
        KDAMON_REGISTRY_EVENT_DATA Registry;
    } Data;
} KDAMON_EVENT, * PKDAMON_EVENT;