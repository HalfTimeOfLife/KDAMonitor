# CHANGELOG - KDAMonitor

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [0.4] - 2026-07-30
### Added
- `log_writer.c`/`log_writer.h`: system thread draining the event queue, writes timestamped JSONL log file to disk (`C:\KDAMonitor\logs\`)
- `event_queue.c`: `WakeEvent` (`KEVENT`) added to the queue so the log writer thread blocks until new events arrive instead of polling
- `kdamon_config.h`: centralized `KDAMON_DIR`, `KDAMON_LOG_DIR`, `KDAMON_LOG_FILE_PREFIX`, `KDAMON_LOG_FILE_EXTENSION`
- Log writer creates both `C:\KDAMonitor\` and `C:\KDAMonitor\logs\` automatically if missing (no manual setup required beyond the drive root existing)

### Fixed
- Bugcheck `IRQL_NOT_LESS_OR_EQUAL (0xA)` in `KdaMonEventQueueInitialize`: `RtlZeroMemory` was wiping `WakeEvent` right after `KeInitializeEvent`, leaving the event object invalid; fixed by zeroing the struct before initializing any kernel objects inside it
- `STATUS_*` identifiers (`STATUS_BUFFER_OVERFLOW`, `STATUS_INVALID_PARAMETER`, etc.) undeclared: `<ntstatus.h>` must be included before `<ntddk.h>` with `WIN32_NO_STATUS` defined in between (applied in `event_queue.h`, `ioctl.h`, `kdamon_config.h`)
- Unresolved externals `__stdio_common_vswprintf`/`__stdio_common_vsprintf`: `ntstrsafe.h` was using its inline (CRT-dependent) implementation; fixed with `#define NTSTRSAFE_LIB` before the include, plus `ntstrsafe.lib` added to linker dependencies

### Changed
- `driver_entry.c`: removed temporary in-driver queue push/pop test code (validated in v0.3), now wires `KdaMonLogWriterStart`/`KdaMonLogWriterStop` into `DriverEntry`/`DriverUnload`

### Notes
- Validated end-to-end on Windows test VM: device creation, log file creation, event push/pop, JSONL write all confirmed working

---

## [0.3] - 2026-07-29
### Added
- `event_types.h`: `KDAMON_EVENT` structure (type, timestamp, unique ID), `KDAMON_EVENT_TYPE` enum with TODO placeholders for future event types
- `event_queue.c`/`event_queue.h`: ring buffer + spinlock, push/pop API, dropped events counter

### Notes
- Push/pop validated via a temporary in-driver test in `DriverEntry` (FIFO order, unique incrementing IDs, correct count after push/pop)

---

## [0.2] - 2026-07-27
### Added
- `device.c`: device object creation, symbolic link, `DO_BUFFERED_IO` flag
- `ioctl.c`: `IRP_MJ_CREATE`/`IRP_MJ_CLOSE` dispatch routine, `IRP_MJ_DEVICE_CONTROL` dispatch routine
- `kdamon_shared.h`: `IOCTL_KDAMON_ECHO` code and request/reply structures
- `kdamon_config.h`: centralized device name, symbolic link name, driver tag
- `client/`: minimal usermode test client validating the echo round-trip

### Notes
- Validated on Windows test VM

---

## [0.1] - 2026-07-24
### Added
- `DriverEntry` / `DriverUnload`
- Windows version detection via `RtlGetVersion`

### Notes
- Validated on Windows test VM