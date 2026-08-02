#pragma once
#include <ntddk.h>

NTSTATUS KdaMonImageCallbackRegister(VOID);
VOID KdaMonImageCallbackUnregister(VOID);