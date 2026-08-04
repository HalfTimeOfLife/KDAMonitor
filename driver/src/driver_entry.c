#include "driver.h"
#include "device.h"
#include "ioctl.h"
#include "kdamon_config.h"
#include "event_queue.h"
#include "log_writer.h"
#include "process_callback.h"
#include "image_callback.h"
#include "wfp_session.h"


PDEVICE_OBJECT g_DeviceObject = NULL;

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint((DRIVER_TAG " [INFO]: Driver Unload begin\n"));
	KdaMonImageCallbackUnregister();
	KdaMonProcessCallbackUnregister();

	KdaMonLogWriterStop();

	KdaMonEventQueueDestroy();
	KdaMonWfpSessionCleanup();

	KdaMonDeleteDevice(&g_DeviceObject);
	KdPrint((DRIVER_TAG " [INFO]: Driver Unload complete\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint((DRIVER_TAG " [INFO]: DriverEntry begin\n"));
	NTSTATUS status;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = KdaMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = KdaMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KdaMonDeviceControl;

	status = KdaMonCreateDevice(DriverObject, &g_DeviceObject);
	if (!NT_SUCCESS(status))
	{
		goto cleanup_none;
	}
	
	// --- Initialize the event queue ---
	if (!KdaMonEventQueueInitialize())
	{
		KdPrint((DRIVER_TAG " [ERROR]: EventQueueInitialize failed\n"));
		status = STATUS_UNSUCCESSFUL;
		goto cleanup_device;
	}

	// --- Start the log writer thread ---
	if (!KdaMonLogWriterStart(DriverObject))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonLogWriterStart failed\n"));
		status = STATUS_UNSUCCESSFUL;
		goto cleanup_queue;
	}

	// --- Initialize WFP session ---
	status = KdaMonWfpSessionInit();
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonWfpSessionInit failed\n"));
		goto cleanup_logwriter;
	}

	// --- Register process creation callback ---
	status = KdaMonProcessCallbackRegister();
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonProcessCallbackRegister failed\n"));
		goto cleanup_wfp;
	}

	// --- Register image load callback ---
	status = KdaMonImageCallbackRegister();
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [ERROR]: KdaMonImageCallbackRegister failed\n"));
		goto cleanup_process;
	}

	KdPrint((DRIVER_TAG " [SUCCESS]: Initialized successfully\n"));
	return STATUS_SUCCESS;

cleanup_process:
	KdaMonProcessCallbackUnregister();
cleanup_wfp:
	KdaMonWfpSessionCleanup();
cleanup_logwriter:
	KdaMonLogWriterStop();
cleanup_queue:
	KdaMonEventQueueDestroy();
cleanup_device:
	KdaMonDeleteDevice(&g_DeviceObject);
cleanup_none:
	return status;
}
