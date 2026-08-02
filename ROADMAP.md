# ROADMAP - KDAMonitor

Format inspired by [Keep a Changelog](https://keepachangelog.com/).

---

## [0.6] - Image Load Callback
Second sensor: logs every DLL/driver loaded into a process.

### Added
- `image_callback.c`: `PsSetLoadImageNotifyRoutine`, logs DLL/driver loads
- `log_writer.c`: dedicated write function, `KdaMonLogWriterWriteImageEvent`, for image load events, per established pattern

---

## [0.7] - WFP Session
Sets up WFP needed for network monitoring.

### Added
- `wfp_session.c`: WFP provider/sublayer setup and teardown
- Clean load/unload validated (no orphaned filters)

---

## [0.8] - Network Callout
Third sensor: logs connections (PID, IP/port, protocol).

### Added
- `wfp_callout.c`: notification-only outbound connection logging
- Captures PID, IP/port, protocol
- `log_writer.c`: dedicated write function, `KdaMonLogWriterWriteNetworkEvent`, for network events, per established pattern

---

## [0.9] - Registry Callback
Fourth sensor: logs registry activity.

### Added
- `registry_callback.c`: logs create/set/delete value
- `log_writer.c`: dedicated write function, `KdaMonLogWriterWriteRegistryEvent`, for registry events, per established pattern

---

## [0.10] - Thread Callback
Last sensor: capture thread create/exit events.

### Added
- `thread_callback.c`: `PsSetCreateThreadNotifyRoutine`, logs thread create/exit
- `log_writer.c`: dedicated write function, `KdaMonLogWriterWriteThreadEvent`, for thread events, per established pattern

---

## [0.11] - Usermode Client
A way to see events live in a terminal instead of only reading the log file afterward.

### Added
- `client/src/main.c`, `event_reader.c`: connects/disconnects to device at will, no data loss
- Real-time console display of incoming events

---

## [0.12] - Structural Refactor
Refactoring.
### Added
- Moves `*_callback.c` files into `driver/src/callbacks/`
- Moves `wfp_*.c` files into `driver/src/network/`

---

## [1.0] - Stabilization
Validated against real malware samples in isolated VM and write full docs + README.

### Added
- Full README + usage docs

---

## Status summary

| Version | File(s) | Feature | Status |
|---|---|---|---|
| v0.1 | `driver_entry.c` | Driver skeleton (load/unload) | Shipped |
| v0.2 | `device.c`, `ioctl.c` | Device object + IOCTL | Shipped |
| v0.3 | `event_queue.c` | Kernel event queue | Shipped |
| v0.4 | `log_writer.c` | Logging | Shipped |
| v0.5 | `process_callback.c` | Process create/exit monitoring | Shipped |
| v0.6 | `image_callback.c` | Image/DLL load monitoring | Planned |
| v0.7 | `wfp_session.c` | WFP session setup | Planned |
| v0.8 | `wfp_callout.c` | Network connection monitoring | Planned |
| v0.9 | `registry_callback.c` | Registry activity monitoring | Planned |
| v0.10 | `thread_callback.c` | Thread create/exit monitoring | Planned |
| v0.11 | `client/` | Usermode client | Planned |
| v0.12 | Refactor | Structural cleanup | Planned |
| v1.0 | Consolidation | Stabilization + release | Planned |