#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <ntddk.h>
#undef WIN32_NO_STATUS

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);