#pragma once

#include <ntddk.h>

NTSTATUS KdaMonRegistryCallbackRegister(_In_ PDRIVER_OBJECT DriverObject);
VOID KdaMonRegistryCallbackUnregister(VOID);