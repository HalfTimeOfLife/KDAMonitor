#pragma once

#include <ntddk.h>

NTSTATUS KdaMonCreateDevice(_In_ PDRIVER_OBJECT DriverObject, _Outptr_ PDEVICE_OBJECT* DeviceObject);
void KdaMonDeleteDevice(_In_opt_ PDEVICE_OBJECT* DeviceObject);