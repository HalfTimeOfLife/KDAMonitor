#pragma once

#include <ntddk.h>

#define DRIVER_TAG "[KDAMonitor]"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);