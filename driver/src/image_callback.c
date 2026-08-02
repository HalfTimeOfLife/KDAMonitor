#include "image_callback.h"
#include "event_queue.h"
#include "event_types.h"
#include "kdamon_config.h"

static VOID KdaMonImageNotifyRoutine(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo)
{
	KDAMON_EVENT Event = { 0 };
	Event.Type = KdaMonEventImageLoad;
	KeQuerySystemTimePrecise(&Event.Timestamp);

	Event.Data.ImageLoad.ProcessId = ProcessId;

	if (ImageInfo) {
		Event.Data.ImageLoad.ImageBase = ImageInfo->ImageBase;
		Event.Data.ImageLoad.ImageSize = ImageInfo->ImageSize;
		Event.Data.ImageLoad.Properties = ImageInfo->Properties;
		Event.Data.ImageLoad.SystemModeImage = ImageInfo->SystemModeImage;
		Event.Data.ImageLoad.ImageMappedToAllPids = ImageInfo->ImageMappedToAllPids;
		Event.Data.ImageLoad.ImagePartialMap = ImageInfo->ImagePartialMap;
		Event.Data.ImageLoad.SignatureLevel = ImageInfo->ImageSignatureLevel;
		Event.Data.ImageLoad.SignatureType = ImageInfo->ImageSignatureType;
	}

	if (FullImageName && FullImageName->Buffer != NULL) {
		SIZE_T MaxCopyLength = sizeof(Event.Data.ImageLoad.ImageFileName) - sizeof(WCHAR);
		SIZE_T ImageFileNameLength = (FullImageName->Length < MaxCopyLength) ? FullImageName->Length : MaxCopyLength;
		RtlCopyMemory(Event.Data.ImageLoad.ImageFileName, FullImageName->Buffer, ImageFileNameLength);
		Event.Data.ImageLoad.ImageFileName[ImageFileNameLength / sizeof(WCHAR)] = L'\0';
	}
	else {
		Event.Data.ImageLoad.ImageFileName[0] = L'\0';
	}

	KdaMonEventQueuePush(&Event);
}

NTSTATUS KdaMonImageCallbackRegister(VOID)
{
	NTSTATUS status = PsSetLoadImageNotifyRoutine(KdaMonImageNotifyRoutine);
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG "[ERROR] PsSetLoadImageNotifyRoutine failed: 0x%08X\n", status));
	}
	return status;
}

VOID KdaMonImageCallbackUnregister(VOID)
{
	NTSTATUS status = PsRemoveLoadImageNotifyRoutine(KdaMonImageNotifyRoutine);
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG "[ERROR] PsRemoveLoadImageNotifyRoutine failed: 0x%08X\n", status));
	}
}