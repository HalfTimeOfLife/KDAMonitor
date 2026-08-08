#pragma once

#include <ntddk.h>

#define NDIS630
#include <ndis.h>

#include <fwpsk.h>
#include <fwpmk.h>

extern const GUID KDAMON_WFP_CALLOUT_OUTBOUND_GUID;
extern const GUID KDAMON_WFP_CALLOUT_INBOUND_GUID;


NTSTATUS KdaMonWfpCalloutRegister(_In_ PDEVICE_OBJECT DeviceObject);

VOID KdaMonWfpCalloutUnregister();