#pragma once

#include <ntddk.h>

NTSTATUS KdaMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS KdaMonDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);