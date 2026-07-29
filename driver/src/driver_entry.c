#include "driver.h"
#include "device.h"
#include "ioctl.h"
#include "kdamon_config.h"
#include "event_queue.h"


PDEVICE_OBJECT g_DeviceObject = NULL;

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

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
	
	if (!KdaMonEventQueueInitialize())
	{
		KdPrint((DRIVER_TAG " [ERROR]: EventQueueInitialize failed\n"));
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint((DRIVER_TAG " [SUCCESS]: Initialized successfully\n"));

	// --- BEGIN TEST QUEUE ---
	KDAMON_EVENT testEvent1 = { 0 };
	testEvent1.Type = KdaMonEventProcess;
	KeQuerySystemTimePrecise(&testEvent1.Timestamp);
	KdaMonEventQueuePush(&testEvent1);

	KDAMON_EVENT testEvent2 = { 0 };
	testEvent2.Type = KdaMonEventNetwork;
	KeQuerySystemTimePrecise(&testEvent2.Timestamp);
	KdaMonEventQueuePush(&testEvent2);

	KdPrint((DRIVER_TAG " [TEST]: Queue count after 2 pushes = %lu\n", KdaMonEventQueueCount()));

	KDAMON_EVENT popped;
	while (KdaMonEventQueuePop(&popped)) {
		KdPrint((DRIVER_TAG " [TEST]: Popped event Id=%lu Type=%d\n", popped.Id, popped.Type));
	}

	KdPrint((DRIVER_TAG " [TEST]: Queue count after pops = %lu\n", KdaMonEventQueueCount()));
	// --- END TEST QUEUE ---

	return STATUS_SUCCESS;
}
