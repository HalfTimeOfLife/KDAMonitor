#pragma once
#include <ntddk.h>

NTSTATUS KdaMonProcessCallbackRegister(VOID);
VOID KdaMonProcessCallbackUnregister(VOID);