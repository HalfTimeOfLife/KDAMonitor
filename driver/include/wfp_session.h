#pragma once

#include <ntddk.h>

#define NDIS630
#include <ndis.h>

#include <fwpmk.h>

extern const GUID KDAMON_WFP_PROVIDER_GUID;
extern const GUID KDAMON_WFP_SUBLAYER_GUID;

extern HANDLE g_EngineHandle;

NTSTATUS KdaMonWfpSessionInit(void);

VOID KdaMonWfpSessionCleanup(void);