#include "ioctl.h"
#include "kdamon_shared.h"
#include "kdamon_config.h"


NTSTATUS KdaMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS KdaMonDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR information = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_KDAMON_ECHO:
	{
		ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
		ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

		if (inLen < sizeof(KDAMON_ECHO_REQUEST) || outLen < sizeof(KDAMON_ECHO_REPLY))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PKDAMON_ECHO_REQUEST request = (PKDAMON_ECHO_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		KDAMON_ECHO_REPLY reply;
		reply.Value = request->Value;

		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &reply, sizeof(reply));
		information = sizeof(reply);
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		KdPrint((DRIVER_TAG " [ERROR]: Unknown IOCTL 0x%08X\n",
			stack->Parameters.DeviceIoControl.IoControlCode));
		break;
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}