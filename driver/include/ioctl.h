#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <ntddk.h>
#undef WIN32_NO_STATUS

NTSTATUS KdaMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS KdaMonDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);