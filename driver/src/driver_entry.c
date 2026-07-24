#include "driver.h"

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint((DRIVER_TAG " [SUCCESS]: Driver Unload called\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	KdPrint((DRIVER_TAG " [SUCCESS]: Initialized successfully\n"));

	RTL_OSVERSIONINFOW lpVersionInformation = { 0 };
	lpVersionInformation.dwOSVersionInfoSize = sizeof(lpVersionInformation);

	NTSTATUS status = RtlGetVersion(&lpVersionInformation);

	if (NT_SUCCESS(status))
	{
		KdPrint((DRIVER_TAG " [SUCCESS]: Windows %lu.%lu Build %lu\n",
			lpVersionInformation.dwMajorVersion,
			lpVersionInformation.dwMinorVersion,
			lpVersionInformation.dwBuildNumber
			));
	}
	else
	{
		KdPrint((DRIVER_TAG " [ERROR]: Windows version not found\n"));
	}

	return STATUS_SUCCESS;
}
