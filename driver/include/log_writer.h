#pragma once

#include <ntddk.h>

BOOLEAN KdaMonLogWriterStart(_In_ PDRIVER_OBJECT DriverObject);

VOID KdaMonLogWriterStop(VOID);