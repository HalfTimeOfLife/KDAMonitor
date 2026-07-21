# ROADMAP - KDAMonitor

Format inspired by [Keep a Changelog](https://keepachangelog.com/) — one section per version, only committed work listed, no wishlist/future speculation.

---

## [0.1] - Driver Skeleton
### Kernel concepts
- Driver Object lifecycle
### Added
- `driver_entry.c`: `DriverEntry` / `DriverUnload`, minimal load/unload cycle
- Validated on Windows test VM (no BSOD, no leak)

---

## [0.2] - Device + IOCTL Echo
### Kernel concepts
- IRP handling, Device I/O
### Added
- `device.c`: device object creation/deletion
- `ioctl.c`: `IRP_MJ_DEVICE_CONTROL` handler, echo round-trip
- `kdamon_shared.h`: IOCTL codes shared between driver and client

---

## [0.3] - Event Queue
### Kernel concepts
- Kernel synchronization (spinlocks), pool memory
### Added
- `event_queue.c`: ring buffer + spinlock, push/pop API
- Designed for reuse by every callback introduced from v0.5 onward

---

## [0.4] - Log Writer
### Kernel concepts
- System threads, `ZwWriteFile`, PASSIVE_LEVEL execution
### Added
- `log_writer.c`: system thread draining the event queue
- Writes JSONL log to disk (one file per session), independent of client presence
- Reuses `event_queue.c` introduced in v0.3

---

## [0.5] - Process Callback
### Kernel concepts
- Notify routines, callback IRQL
### Added
- `process_callback.c`: `PsSetCreateProcessNotifyRoutineEx`, logs process create/exit
- Pushes into `event_queue.c` (v0.3)

---

## [0.6] - Image Load Callback
### Kernel concepts
- `UNICODE_STRING` parsing, path filtering
### Added
- `image_callback.c`: `PsSetLoadImageNotifyRoutine`, logs DLL/driver loads
- Key signal for process injection detection

---

## [0.7] - WFP Session
### Kernel concepts
- WFP provider/sublayer lifecycle
### Added
- `wfp_session.c`: WFP provider/sublayer setup and teardown
- Clean load/unload validated (no orphaned filters)

---

## [0.8] - Network Callout
### Kernel concepts
- WFP callouts, notification-only filters
### Added
- `wfp_callout.c`: notification-only outbound connection logging
- Captures PID, remote IP/port, protocol — primary C2 detection signal
- Reuses `wfp_session.c` introduced in v0.7

---

## [0.9] - Registry Callback
### Kernel concepts
- `CmRegisterCallbackEx`, altitude
### Added
- `registry_callback.c`: logs create/set/delete value
- Targets persistence indicators (Run keys, services)

---

## [0.10] - Thread Callback
### Added
- `thread_callback.c`: `PsSetCreateThreadNotifyRoutine`, logs thread create/exit

---

## [0.11] - Usermode Client
### Kernel concepts
- Usermode Device I/O
### Added
- `client/src/main.c`, `event_reader.c`: connects/disconnects to device at will, no data loss
- Real-time console display of incoming events

---

## [0.12] - Structural Refactor
### Added
- Moves `*_callback.c` files into `driver/src/callbacks/`
- Moves `wfp_*.c` files into `driver/src/network/`
- No new feature, no behavior change

---

## [1.0] - Stabilization
### Added
- Driver Verifier clean run across all callbacks
- Full README + usage docs
- Validated against real malware samples in isolated VM
- Tagged GitHub release

---

## Status summary

| Version | File(s) | Feature | Status |
|---|---|---|---|
| v0.1 | `driver_entry.c` | Driver skeleton (load/unload) | Planned |
| v0.2 | `device.c`, `ioctl.c` | Device object + IOCTL echo | Planned |
| v0.3 | `event_queue.c` | Kernel event queue | Planned |
| v0.4 | `log_writer.c` | Disk logging (JSONL, autonomous) | Planned |
| v0.5 | `process_callback.c` | Process create/exit monitoring | Planned |
| v0.6 | `image_callback.c` | Image/DLL load monitoring | Planned |
| v0.7 | `wfp_session.c` | WFP session setup | Planned |
| v0.8 | `wfp_callout.c` | Network connection monitoring | Planned |
| v0.9 | `registry_callback.c` | Registry activity monitoring | Planned |
| v0.10 | `thread_callback.c` | Thread create/exit monitoring | Planned |
| v0.11 | `client/` | Usermode real-time client | Planned |
| v0.12 | Refactor | Structural cleanup (`callbacks/`, `network/`) | Planned |
| v1.0 | Consolidation | Stabilization + release | Planned |