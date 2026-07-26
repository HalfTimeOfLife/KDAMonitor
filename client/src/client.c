#include <windows.h>
#include <stdio.h>
#include "..\include\client.h"
#include "..\..\driver\include\kdamon_shared.h"

int Error(const char* message) {
    printf(CLIENT_TAG " [ERROR]: %s (error=%lu)\n", message, GetLastError());
    return 1;
}

int main(int argc, const char* argv[]) {
    HANDLE hDevice = CreateFileW(
        L"\\\\.\\KDAMonitor",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return Error("Failed to open device");
    }

    printf(CLIENT_TAG " [SUCCESS]: Device opened successfully\n");

    KDAMON_ECHO_REQUEST request;
    request.Value = 42;

    KDAMON_ECHO_REPLY reply;
    DWORD bytesReturned = 0;

    BOOL success = DeviceIoControl(
        hDevice,
        IOCTL_KDAMON_ECHO,
        &request, sizeof(request),
        &reply, sizeof(reply),
        &bytesReturned,
        NULL
    );

    if (!success)
    {
        CloseHandle(hDevice);
        return Error("DeviceIoControl failed");
    }

    printf(CLIENT_TAG " [INFO]: Sent %lu, received %lu (bytes returned: %lu)\n",
        request.Value, reply.Value, bytesReturned);

    if (reply.Value == request.Value)
    {
        printf(CLIENT_TAG " [SUCCESS]: Echo matches\n");
    }
    else
    {
        printf(CLIENT_TAG " [ERROR]: Echo mismatch\n");
    }

    CloseHandle(hDevice);
    return 0;
}