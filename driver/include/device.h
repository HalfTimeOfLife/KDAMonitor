#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <ntddk.h>
#undef WIN32_NO_STATUS

NTSTATUS KdaMonCreateDevice(_In_ PDRIVER_OBJECT DriverObject, _Outptr_ PDEVICE_OBJECT* DeviceObject);
void KdaMonDeleteDevice(_Inout_ PDEVICE_OBJECT* DeviceObject);