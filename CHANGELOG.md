# CHANGELOG - KDAMonitor

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

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