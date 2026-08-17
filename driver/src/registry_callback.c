#include "registry_callback.h"
#include "kdamon_config.h"
#include "event_types.h"
#include "event_queue.h"

NTKERNELAPI
NTSTATUS SeLocateProcessImageName(_In_ PEPROCESS Process, _Out_ PUNICODE_STRING* pImageFileName);


LARGE_INTEGER g_RegistryCookie = { 0 };

// --- Resolution helpers ---

static NTSTATUS KdaMonRegistryResolveKeyPath(_In_ PVOID Object, _Out_writes_bytes_(KeyPathBufferSize) PWCHAR KeyPathBuffer, _In_ ULONG KeyPathBufferSize)
{
    NTSTATUS status;
    PCUNICODE_STRING ObjectName = NULL;

    KeyPathBuffer[0] = L'\0';

    if (Object == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = CmCallbackGetKeyObjectIDEx(
        &g_RegistryCookie,
        Object,
        NULL,
        &ObjectName,
        0
    );
    if (!NT_SUCCESS(status) || ObjectName == NULL)
    {
        return status;
    }

    ULONG charsToCopy = min(ObjectName->Length / sizeof(WCHAR), KeyPathBufferSize - 1);
    RtlCopyMemory(KeyPathBuffer, ObjectName->Buffer, charsToCopy * sizeof(WCHAR));
    KeyPathBuffer[charsToCopy] = L'\0';

    CmCallbackReleaseKeyObjectIDEx(ObjectName);

    return STATUS_SUCCESS;
}

static NTSTATUS KdaMonRegistryResolveProcessPath(_Out_writes_z_(Length) PWCHAR Buffer, _In_ ULONG Length)
{
    NTSTATUS status;
    PUNICODE_STRING imageName = NULL;

    Buffer[0] = L'\0';

    status = SeLocateProcessImageName(PsGetCurrentProcess(), &imageName);
    if (!NT_SUCCESS(status) || imageName == NULL)
    {
        return status;
    }

    ULONG charsToCopy = min(imageName->Length / sizeof(WCHAR), Length - 1);
    RtlCopyMemory(Buffer, imageName->Buffer, charsToCopy * sizeof(WCHAR));
    Buffer[charsToCopy] = L'\0';

    ExFreePool(imageName);

    return STATUS_SUCCESS;
}

// --- Handlers ---

static VOID KdaMonRegistryHandleSetValueKey(_In_opt_ PREG_SET_VALUE_KEY_INFORMATION Info)
{
    if (Info == NULL || Info->ValueName == NULL)
    {
        return;
    }

    KDAMON_EVENT Event = { 0 };
    Event.Type = KdaMonEventRegistry;
    KeQuerySystemTimePrecise(&Event.Timestamp);

    Event.Data.Registry.ProcessId = PsGetCurrentProcessId();
    KdaMonRegistryResolveProcessPath(
        Event.Data.Registry.ProcessPath,
        RTL_NUMBER_OF(Event.Data.Registry.ProcessPath)
    );

    Event.Data.Registry.Action = KDAMON_REGISTRY_ACTION_SET_VALUE;

    KdaMonRegistryResolveKeyPath(
        Info->Object,
        Event.Data.Registry.KeyPath,
        RTL_NUMBER_OF(Event.Data.Registry.KeyPath)
    );

    ULONG nameChars = min(
        Info->ValueName->Length / sizeof(WCHAR),
        RTL_NUMBER_OF(Event.Data.Registry.ValueName) - 1
    );
    RtlCopyMemory(Event.Data.Registry.ValueName, Info->ValueName->Buffer, nameChars * sizeof(WCHAR));
    Event.Data.Registry.ValueName[nameChars] = L'\0';

    Event.Data.Registry.ValueType = Info->Type;

    ULONG dataSize = min(Info->DataSize, KDAMON_REG_VALUEDATA_MAX);
    if (Info->Data != NULL && dataSize > 0)
    {
        RtlCopyMemory(Event.Data.Registry.ValueData, Info->Data, dataSize);
    }
    Event.Data.Registry.ValueDataSize = dataSize;

    KdaMonEventQueuePush(&Event);
}

static VOID KdaMonRegistryHandleDeleteValueKey(_In_opt_ PREG_DELETE_VALUE_KEY_INFORMATION Info)
{
    if (Info == NULL || Info->ValueName == NULL)
    {
        return;
    }

    KDAMON_EVENT Event = { 0 };
    Event.Type = KdaMonEventRegistry;
    KeQuerySystemTimePrecise(&Event.Timestamp);

    Event.Data.Registry.ProcessId = PsGetCurrentProcessId();
    KdaMonRegistryResolveProcessPath(
        Event.Data.Registry.ProcessPath,
        RTL_NUMBER_OF(Event.Data.Registry.ProcessPath)
    );

    Event.Data.Registry.Action = KDAMON_REGISTRY_ACTION_DELETE_VALUE;

    KdaMonRegistryResolveKeyPath(
        Info->Object,
        Event.Data.Registry.KeyPath,
        RTL_NUMBER_OF(Event.Data.Registry.KeyPath)
    );

    ULONG nameChars = min(
        Info->ValueName->Length / sizeof(WCHAR),
        RTL_NUMBER_OF(Event.Data.Registry.ValueName) - 1
    );
    RtlCopyMemory(Event.Data.Registry.ValueName, Info->ValueName->Buffer, nameChars * sizeof(WCHAR));
    Event.Data.Registry.ValueName[nameChars] = L'\0';

    Event.Data.Registry.ValueType = 0;
    Event.Data.Registry.ValueDataSize = 0;

    KdaMonEventQueuePush(&Event);
}

static VOID KdaMonRegistryHandlePostCreateKeyEx(_In_opt_ PREG_POST_OPERATION_INFORMATION Info)
{
    if (Info == NULL)
    {
        return;
    }

    KDAMON_EVENT Event = { 0 };
    Event.Type = KdaMonEventRegistry;
    KeQuerySystemTimePrecise(&Event.Timestamp);

    Event.Data.Registry.ProcessId = PsGetCurrentProcessId();
    KdaMonRegistryResolveProcessPath(
        Event.Data.Registry.ProcessPath,
        RTL_NUMBER_OF(Event.Data.Registry.ProcessPath)
    );

    Event.Data.Registry.Action = KDAMON_REGISTRY_ACTION_CREATE_KEY;

    KdaMonRegistryResolveKeyPath(
        Info->Object,
        Event.Data.Registry.KeyPath,
        RTL_NUMBER_OF(Event.Data.Registry.KeyPath)
    );


    Event.Data.Registry.ValueName[0] = L'\0';
    Event.Data.Registry.ValueType = 0;
    Event.Data.Registry.ValueDataSize = 0;
    Event.Data.Registry.Status = Info->Status;

    KdaMonEventQueuePush(&Event);
}

// --- Dispatcher ---

static NTSTATUS KdaMonRegistryCallback(_In_ PVOID CallbackContext, _In_opt_ PVOID Argument1, _In_opt_ PVOID Argument2)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    switch (notifyClass)
    {
    case RegNtPreSetValueKey:
        KdaMonRegistryHandleSetValueKey((PREG_SET_VALUE_KEY_INFORMATION)Argument2);
        break;

    case RegNtPreDeleteValueKey:
        KdaMonRegistryHandleDeleteValueKey((PREG_DELETE_VALUE_KEY_INFORMATION)Argument2);
        break;

    case RegNtPostCreateKeyEx:
        KdaMonRegistryHandlePostCreateKeyEx((PREG_POST_OPERATION_INFORMATION)Argument2);
        break;

    default:
        break;
    }

    return STATUS_SUCCESS;
}

// --- Register / Unregister ---

NTSTATUS KdaMonRegistryCallbackRegister(_In_ PDRIVER_OBJECT DriverObject) 
{
    NTSTATUS status;
    UNICODE_STRING altitude;

    RtlInitUnicodeString(&altitude, KDAMON_REG_ALTITUDE);

    status = CmRegisterCallbackEx(
        KdaMonRegistryCallback,
        &altitude,
        DriverObject,
        NULL,
        &g_RegistryCookie,
        NULL
    );
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: CmRegisterCallbackEx failed: 0x%X\n", status));
        return status;
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: Registry callback registered\n"));
    return STATUS_SUCCESS;
}

VOID KdaMonRegistryCallbackUnregister(VOID)
{
    if (g_RegistryCookie.QuadPart != 0)
    {
        CmUnRegisterCallback(g_RegistryCookie);
        g_RegistryCookie.QuadPart = 0;
        KdPrint((DRIVER_TAG " [SUCCESS]: Registry callback unregistered\n"));
    }
}