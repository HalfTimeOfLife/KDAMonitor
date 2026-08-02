#include "process_callback.h"
#include "event_queue.h"
#include "event_types.h"
#include "kdamon_config.h"

static VOID KdaMonProcessNotifyRoutine(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);

	KDAMON_EVENT Event = { 0 };
	Event.Type = KdaMonEventProcess;
	KeQuerySystemTimePrecise(&Event.Timestamp);

	Event.Data.Process.ProcessId = ProcessId;

    if (CreateInfo) {
		// --- Process creation case ---
		Event.Data.Process.ParentProcessId = CreateInfo->ParentProcessId;
		Event.Data.Process.IsCreate = TRUE;

		PCUNICODE_STRING ImageFileName = CreateInfo->ImageFileName;

		if (ImageFileName && ImageFileName->Buffer != NULL) {
			SIZE_T MaxCopyLength = sizeof(Event.Data.Process.ImageFileName) - sizeof(WCHAR);
			SIZE_T ImageFileNameLength = (ImageFileName->Length < MaxCopyLength) ? ImageFileName->Length : MaxCopyLength;
			RtlCopyMemory(Event.Data.Process.ImageFileName, ImageFileName->Buffer, ImageFileNameLength);
			Event.Data.Process.ImageFileName[ImageFileNameLength / sizeof(WCHAR)] = L'\0';
		}
    }
    else {
		// --- Process termination case ---
		Event.Data.Process.ParentProcessId = NULL;
		Event.Data.Process.IsCreate = FALSE;
		Event.Data.Process.ImageFileName[0] = L'\0';
    }
	KdaMonEventQueuePush(&Event);
}

NTSTATUS KdaMonProcessCallbackRegister(VOID)
{
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(KdaMonProcessNotifyRoutine, FALSE);
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG "[ERROR] PsSetCreateProcessNotifyRoutineEx failed: 0x%08X\n", status));
	}
    return status;
}

VOID KdaMonProcessCallbackUnregister(VOID)
{
    NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(KdaMonProcessNotifyRoutine, TRUE);
    if (!NT_SUCCESS(status))
    {
		KdPrint((DRIVER_TAG "[ERROR] PsSetCreateProcessNotifyRoutineEx failed: 0x%08X\n", status));
    }
}

