#include "kdamon_config.h"

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "log_writer.h"
#include "event_queue.h"
#include "event_types.h"

#define WAIT_OBJECT_COUNT 2

// --- Module-level state ---

static PVOID g_ThreadObject = NULL;
static KEVENT g_StopEvent;
static HANDLE g_LogFileHandle = NULL;

// --- Forward declarations ---

static VOID KdaMonLogWriterThread(_In_ PVOID StartContext);

// --- Format helper ---

static const char* KdaMonEventTypeToString(_In_ KDAMON_EVENT_TYPE Type)
{
    switch (Type)
    {
    case KdaMonEventProcess:   return "Process";
    case KdaMonEventThread:    return "Thread";
    case KdaMonEventImageLoad: return "ImageLoad";
    case KdaMonEventNetwork:   return "Network";
    case KdaMonEventRegistry:  return "Registry";
    default:                   return "Unknown";
    }
}

static BOOLEAN KdaMonJsonEscapeW(_In_ PCWSTR Source, _Out_writes_z_(DestSize) PSTR Dest, _In_ SIZE_T DestSize)
{
    SIZE_T Out = 0;

    if (DestSize == 0)
    {
        return FALSE;
    }

    for (SIZE_T i = 0; Source[i] != L'\0'; i++)
    {
        WCHAR Ch = Source[i];

        if (Ch == L'\\' || Ch == L'"')
        {
            if (Out + 2 >= DestSize)
            {
                Dest[Out] = '\0';
                return FALSE;
            }
            Dest[Out++] = '\\';
            Dest[Out++] = (CHAR)Ch;
        }
        else if (Ch >= 0x20 && Ch < 0x7F)
        {
            if (Out + 1 >= DestSize)
            {
                Dest[Out] = '\0';
                return FALSE;
            }
            Dest[Out++] = (CHAR)Ch;
        }
    }

    Dest[Out] = '\0';
    return TRUE;
}

// --- Write event helpers ---

static NTSTATUS KdaMonLogWriterWriteProcessEvent(_In_ const KDAMON_EVENT* Event, _Out_writes_z_(BufferSize) PSTR EventBuffer, _In_ SIZE_T BufferSize)
{
    CHAR EscapedImage[520];
    CHAR PpidField[16];

    if (!KdaMonJsonEscapeW(Event->Data.Process.ImageFileName, EscapedImage, sizeof(EscapedImage)))
    {
        KdPrint((DRIVER_TAG " [WARNING]: Image path truncated during JSON escape (event %lu)\n", Event->Id));
    }

    if (Event->Data.Process.IsCreate && Event->Data.Process.ParentProcessId != NULL)
    {
        RtlStringCbPrintfA(PpidField, sizeof(PpidField), "%lu",
            (ULONG)(ULONG_PTR)Event->Data.Process.ParentProcessId);
    }
    else
    {
        RtlStringCbCopyA(PpidField, sizeof(PpidField), "null");
    }

    return RtlStringCbPrintfA(
        EventBuffer,
        BufferSize,
        "{\"id\":%lu,\"type\":\"%s\",\"timestamp\":%lld,"
        "\"pid\":%lu,\"ppid\":%s,\"is_create\":%s,\"image\":\"%s\"}\n",
        Event->Id,
        KdaMonEventTypeToString(Event->Type),
        Event->Timestamp.QuadPart,
        (ULONG)(ULONG_PTR)Event->Data.Process.ProcessId,
        PpidField,
        Event->Data.Process.IsCreate ? "true" : "false",
        EscapedImage
    );
}

// TODO: KdaMonLogWriterWriteImageEvent (v0.6)
// TODO: KdaMonLogWriterWriteNetworkEvent (v0.8)
// TODO: KdaMonLogWriterWriteRegistryEvent (v0.9)
// TODO: KdaMonLogWriterWriteThreadEvent (v0.10)

// --- File I/O helpers ---

static NTSTATUS KdaMonLogWriterOpenFile(VOID)
{
    LARGE_INTEGER CurrentTime;
    TIME_FIELDS TimeFields;
    WCHAR FileBuffer[512];
    UNICODE_STRING LogFilePath;
    UNICODE_STRING LogDirPath;
    UNICODE_STRING LogParentPath;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributesFile;
    OBJECT_ATTRIBUTES ObjectAttributesDir;
    OBJECT_ATTRIBUTES ObjectAttributesParent;
    HANDLE dirHandle = NULL;
    HANDLE parentHandle = NULL;
    NTSTATUS status;

    // --- 0. Create/verify the parent directory (C:\KDAMonitor) ---
    RtlInitUnicodeString(&LogParentPath, KDAMON_DIR);

    InitializeObjectAttributes(
        &ObjectAttributesParent,
        &LogParentPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &parentHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &ObjectAttributesParent,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: ZwCreateFile for parent directory failed (0x%08X)\n", status));
        return status;
    }

    ZwClose(parentHandle);

    // --- 1. Create/verify the log directory ---
    RtlInitUnicodeString(&LogDirPath, KDAMON_LOG_DIR);

    InitializeObjectAttributes(
        &ObjectAttributesDir,
        &LogDirPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &dirHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &ObjectAttributesDir,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: ZwCreateFile for log directory failed (0x%08X)\n", status));
        return status;
    }

    ZwClose(dirHandle);

    // --- 2. Build the timestamped log file path ---
    KeQuerySystemTime(&CurrentTime);
    RtlTimeToTimeFields(&CurrentTime, &TimeFields);

    LogFilePath.Buffer = FileBuffer;
    LogFilePath.Length = 0;
    LogFilePath.MaximumLength = sizeof(FileBuffer);

    RtlUnicodeStringPrintf(
        &LogFilePath,
        KDAMON_LOG_DIR
        KDAMON_LOG_FILE_PREFIX
        L"%04hu%02hu%02hu_%02hu%02hu%02hu"
        KDAMON_LOG_FILE_EXTENSION,
        TimeFields.Year,
        TimeFields.Month,
        TimeFields.Day,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second
    );

    // --- 3. Create the log file itself ---
    InitializeObjectAttributes(
        &ObjectAttributesFile,
        &LogFilePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &g_LogFileHandle,
        FILE_APPEND_DATA | SYNCHRONIZE,
        &ObjectAttributesFile,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: ZwCreateFile for log file failed (0x%08X)\n", status));
        return status;
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: Log file opened/created\n"));

    return STATUS_SUCCESS;
}

static VOID KdaMonLogWriterCloseFile(VOID)
{
    if (g_LogFileHandle != NULL) {
        ZwClose(g_LogFileHandle);
        g_LogFileHandle = NULL;
    }
    
}

static NTSTATUS KdaMonLogWriterWriteEvent(_In_ const KDAMON_EVENT* Event)
{
    CHAR EventBuffer[1000];
    size_t Length;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS status;

    switch (Event->Type)
    {
    case KdaMonEventProcess:
        status = KdaMonLogWriterWriteProcessEvent(Event, EventBuffer, sizeof(EventBuffer));
        break;
		// TODO: case KdaMonEventImageLoad: (v0.6)
		// TODO: case KdaMonEventNetwork: (v0.8)
		// TODO: case KdaMonEventRegistry: (v0.9)
		// TODO: case KdaMonEventThread: (v0.10)

    default:
        status = RtlStringCbPrintfA(
            EventBuffer,
            sizeof(EventBuffer),
            "{\"id\":%lu,\"type\":\"%s\",\"timestamp\":%lld}\n",
            Event->Id,
            KdaMonEventTypeToString(Event->Type),
            Event->Timestamp.QuadPart
        );
        break;
    }
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: Event formatting failed (0x%08X)\n", status));
        return status;
    }

    status = RtlStringCbLengthA(EventBuffer, sizeof(EventBuffer), &Length);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: RtlStringCbLengthA for log entry size failed (0x%08X)\n", status));
        return status;
    }

    status = ZwWriteFile(
        g_LogFileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        EventBuffer,
        (ULONG)Length,
        NULL,
        NULL
    );
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: ZwWriteFile failed (0x%08X)\n", status));
        return status;
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: Event %lu written to log file\n", Event->Id));
    return STATUS_SUCCESS;
}

// --- Log Writer Controllers ---

BOOLEAN KdaMonLogWriterStart(_In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS status;
    HANDLE threadHandle;

    KeInitializeEvent(&g_StopEvent, NotificationEvent, FALSE);

    status = KdaMonLogWriterOpenFile();
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: KdaMonLogWriterStart: failed to open log file (0x%08X)\n", status));
        return FALSE;
    }

    status = IoCreateSystemThread(
        DriverObject,
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        KdaMonLogWriterThread,
        NULL
    );
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: IoCreateSystemThread failed (0x%08X)\n", status));
        KdaMonLogWriterCloseFile();
        return FALSE;
    }

    status = ObReferenceObjectByHandle(
        threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &g_ThreadObject,
        NULL
    );
    ZwClose(threadHandle);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: ObReferenceObjectByHandle failed (0x%08X)\n", status));
        return FALSE;
    }


    KdPrint((DRIVER_TAG " [SUCCESS]: Log writer started\n"));
    return TRUE;
}

VOID KdaMonLogWriterStop(VOID)
{
    if (g_ThreadObject == NULL) {
        return;
    }

    KeSetEvent(&g_StopEvent, IO_NO_INCREMENT, FALSE);

    KeWaitForSingleObject(g_ThreadObject, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(g_ThreadObject);
    g_ThreadObject = NULL;

    KdaMonLogWriterCloseFile();

    KdPrint((DRIVER_TAG " [SUCCESS]: Log writer stopped\n"));
}

// --- Thread routine ---

static VOID KdaMonLogWriterThread(_In_ PVOID StartContext)
{
    UNREFERENCED_PARAMETER(StartContext);

    PRKEVENT WakeEvent = KdaMonEventQueueGetWakeEvent();
    PVOID WaitObjects[WAIT_OBJECT_COUNT];
    NTSTATUS WaitStatus;
    KDAMON_EVENT Event;


    WaitObjects[0] = &g_StopEvent;
    WaitObjects[1] = WakeEvent;

    KdPrint((DRIVER_TAG " [SUCCESS]: Log writer thread started\n"));

    for (;;) {
        WaitStatus = KeWaitForMultipleObjects(
            WAIT_OBJECT_COUNT,
            WaitObjects,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            NULL,
            NULL
        );

        if (WaitStatus == STATUS_WAIT_0)
        {
            break;
        }

        while (KdaMonEventQueuePop(&Event))
        {
            KdaMonLogWriterWriteEvent(&Event);
        }
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: Log writer thread exiting\n"));

    PsTerminateSystemThread(STATUS_SUCCESS);
}

