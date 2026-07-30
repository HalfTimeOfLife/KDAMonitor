#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <ntddk.h>
#undef WIN32_NO_STATUS

#define DRIVER_TAG "[KDAMonitor]"

#define KDAMON_DEVICE_NAME L"\\Device\\KDAMonitor"
#define KDAMON_SYMLINK_NAME L"\\DosDevices\\KDAMonitor"

#define KDAMON_DIR L"\\??\\C:\\KDAMonitor\\"
#define KDAMON_LOG_DIR L"\\??\\C:\\KDAMonitor\\logs\\"
#define KDAMON_LOG_FILE_PREFIX L"kdamon_"
#define KDAMON_LOG_FILE_EXTENSION L".jsonl"