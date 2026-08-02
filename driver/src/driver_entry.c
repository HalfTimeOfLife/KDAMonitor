#include "driver.h"
#include "device.h"
#include "ioctl.h"
#include "kdamon_config.h"
#include "event_queue.h"
#include "log_writer.h"
#include "process_callback.h"
#include "image_callback.h"


PDEVICE_OBJECT g_DeviceObject = NULL;

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	KdaMonImageCallbackUnregister();
	KdaMonProcessCallbackUnregister();
	KdaMonLogWriterStop();
	KdaMonEventQueueDestroy();
	KdaMonDeleteDevice(g_DeviceObject);
	KdPrint((DRIVER_TAG " [SUCCESS]: Driver Unload called\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = KdaMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = KdaMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KdaMonDeviceControl;

	NTSTATUS status = KdaMonCreateDevice(DriverObject, &g_DeviceObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	
	// --- Initialize the event queue ---
	if (!KdaMonEventQueueInitialize())
	{
		KdPrint((DRIVER_TAG " [ERROR]: EventQueueInitialize failed\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// --- Start the log writer thread ---
	if (!KdaMonLogWriterStart(DriverObject))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonLogWriterStart failed\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// --- Register process creation callback ---
	if (!NT_SUCCESS(KdaMonProcessCallbackRegister()))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonProcessCallbackRegister failed\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// --- Register image load callback ---
	if (!NT_SUCCESS(KdaMonImageCallbackRegister()))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonImageCallbackRegister failed\n"));
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint((DRIVER_TAG " [SUCCESS]: Initialized successfully\n"));
	return STATUS_SUCCESS;
}
