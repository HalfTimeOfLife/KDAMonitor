#include "device.h"
#include "kdamon_config.h"

NTSTATUS KdaMonCreateDevice(_In_ PDRIVER_OBJECT DriverObject, _Outptr_ PDEVICE_OBJECT* DeviceObject)
{
	UNICODE_STRING devName = RTL_CONSTANT_STRING(KDAMON_DEVICE_NAME);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(KDAMON_SYMLINK_NAME);

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		DeviceObject
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [ERROR]: IoCreateDevice failed (0x%08X)\n", status));
		return status;
	}

	(*DeviceObject)->Flags |= DO_BUFFERED_IO;

	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [ERROR]: IoCreateSymbolicLink failed (0x%08X)\n", status));
		IoDeleteDevice(*DeviceObject);
		*DeviceObject = NULL;
		return status;
	}

	(*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

	KdPrint((DRIVER_TAG " [SUCCESS]: Device object and symbolic link created\n"));
	return STATUS_SUCCESS;
}

void KdaMonDeleteDevice(_Inout_ PDEVICE_OBJECT* DeviceObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(KDAMON_SYMLINK_NAME);

	IoDeleteSymbolicLink(&symLink);

	if (*DeviceObject != NULL)
	{
		IoDeleteDevice(*DeviceObject);
		*DeviceObject = NULL;
	}

	KdPrint((DRIVER_TAG " [SUCCESS]: Device object and symbolic link deleted\n"));
}