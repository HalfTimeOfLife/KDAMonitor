# CHANGELOG - KDAMonitor

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [0.9] - 2026-08-17

Fourth sensor: registry activity (create/set/delete value, key creation) captured via `CmRegisterCallbackEx`, following the same push-to-queue/dedicated-writer pipeline as the other sensors.

### Added
- `registry_callback.c`/`.h`: `KdaMonRegistryCallbackRegister`/`KdaMonRegistryCallbackUnregister`, registers a single registry callback at altitude `KDAMON_REG_ALTITUDE` (`360000`), dispatching on `REG_NOTIFY_CLASS` to three handlers: `RegNtPreSetValueKey`, `RegNtPreDeleteValueKey`, `RegNtPostCreateKeyEx`
- `event_types.h`: `KDAMON_REGISTRY_EVENT_DATA` (PID as `HANDLE`, process path, action, key path, value name, value type, raw value data with size, `NTSTATUS` status), `KDAMON_REGISTRY_ACTION` enum (`SET_VALUE`/`DELETE_VALUE`/`CREATE_KEY`)
- Key path resolution via `CmCallbackGetKeyObjectIDEx`, process path resolution via `SeLocateProcessImageName`
- `log_writer.c`: `KdaMonLogWriterWriteRegistryEvent`, dispatched via switch in `KdaMonLogWriterWriteEvent`, with type-aware `value_data` formatting: escaped string for `REG_SZ`/`REG_EXPAND_SZ`, decimal for `REG_DWORD`/`REG_QWORD`, hex for `REG_BINARY` and unhandled types
- `KDAMON_REG_ALTITUDE`, `KDAMON_REG_PATH_MAX`, `KDAMON_REG_VALUENAME_MAX`, `KDAMON_REG_VALUEDATA_MAX` in `kdamon_config.h`
- Registered/unregistered in `DriverEntry`/`DriverUnload`, after the image load callback, consistent with all event producers being torn down before the queue and log writer

### Changed
- `KDAMON_NETWORK_EVENT_DATA.ProcessId` and `KDAMON_REGISTRY_EVENT_DATA.ProcessId` changed from `ULONG` to `HANDLE` for type correctness with Windows PID APIs, matching `KDAMON_PROCESS_EVENT_DATA.ProcessId`; corresponding cast updates in `wfp_callout.c` (`(HANDLE)inMetaValues->processId`) and `log_writer.c` (`(ULONG)(ULONG_PTR)...` for display)

### Notes
- Validated on Windows test VM via `reg add`/`reg delete` covering all three actions and all major value types (`REG_SZ`, `REG_EXPAND_SZ`, `REG_DWORD`, `REG_QWORD`, `REG_BINARY`); dbgview confirms clean registration/unregistration with no BSOD or orphaned callback on unload

### Known limitations
- `SET_VALUE`/`DELETE_VALUE` have no captured `NTSTATUS`
- `REG_MULTI_SZ` is formatted as hex rather than parsed into its component strings

---

## [0.8] - 2026-08-09

Third sensor: outbound and inbound network connections captured via WFP callouts on the ALE layers, using the provider/sublayer infrastructure from v0.7.

### Added
- `guids.c`: centralized `DEFINE_GUID` declarations (session + callout GUIDs), replacing the inline `DEFINE_GUID`/`INITGUID` previously in `wfp_session.c`
- `wfp_callout.c`/`.h`: `KdaMonWfpCalloutRegister`/`KdaMonWfpCalloutUnregister`, registers two callouts (`KDAMonitor Callout Outbound`/`Inbound`) on `FWPM_LAYER_ALE_AUTH_CONNECT_V4` and `FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4`, each with an associated `FwpmFilterAdd`
- `event_types.h`: `KDAMON_NETWORK_EVENT_DATA` (PID, process path, protocol, local/remote IP and port as `ULONG`/`USHORT`, direction), `KDAMON_NETWORK_DIRECTION` enum
- `log_writer.c`: `KdaMonLogWriterWriteNetworkEvent`, dispatched via switch in `KdaMonLogWriterWriteEvent`, per established pattern
- Registered/unregistered in `DriverEntry`/`DriverUnload`, right after the event queue init/before teardown, consistent with all event producers being torn down before the queue and log writer

### Changed
- `wfp_session.c`: `g_EngineHandle` no longer `static`, exposed via `wfp_session.h` so `wfp_callout.c` can reuse the same WFP session handle
- `wfp_session.c`: `subLayer.weight` changed from `0` to `0xFFFF` (highest priority) so the KDAMonitor sublayer's filters are evaluated first among sublayers


### Notes
- Validated on Windows test VM: `netsh wfp show state`/`show filters` confirm provider, sublayer, both callouts, and both filters registered on load, absent on unload

### Known limitations
- IPv6 is not covered: only `FWPM_LAYER_ALE_AUTH_CONNECT_V4`/`RECV_ACCEPT_V4` are registered, and `KDAMON_NETWORK_EVENT_DATA.LocalIp`/`RemoteIp` are `ULONG` (32-bit), which cannot represent an IPv6 address.

---

## [0.7] - 2026-08-04

WFP session infrastructure: opens the WFP engine and registers the provider/sublayer that future network callouts (v0.8) will attach to.

### Added
- `wfp_session.c`/`.h`: `KdaMonWfpSessionInit`/`KdaMonWfpSessionCleanup`, opens a WFP engine session, registers `KDAMonitor Provider` and `KDAMonitor Sublayer` with dedicated GUIDs
- Registered early in `DriverEntry` (right after device creation, before the event queue), consistent with treating the WFP session as infrastructure rather than an event producer

### Changed
- `driver_entry.c`: reordered `DriverEntry`/`DriverUnload` so `KdaMonWfpSessionInit`/`Cleanup` bracket the rest of the pipeline (Device -> WFP -> Queue -> LogWriter -> ProcessCallback -> ImageCallback, mirrored exactly on teardown), keeping the invariant that all event producers (callbacks) are unregistered before the log writer and event queue are torn down
- `driver_entry.c`: corrected two `goto` cleanup targets in the `DriverEntry` failure cascade that pointed to the wrong label (one leaked the WFP session on event queue init failure, one skipped log writer thread shutdown on process callback registration failure)

### Fixed
- `driver/KDAMonitor.vcxproj`: `Release|x64` was missing `AdditionalDependencies` (including `fwpkclnt.lib`, needed by `wfp_session.c`), which would have failed to link on Release builds
- `device.h`/`driver.h`: harmonized the `ntstatus.h`/`ntddk.h`/`WIN32_NO_STATUS` include pattern (already applied elsewhere since v0.4) to remove an include-order dependency
- `KDAMonitor.sln`/`.vcxproj`: removed unused ARM64 configurations (x64-only target)

### Notes
- Validated on Windows test VM: load/unload cycle confirmed via `netsh wfp show state`: `KDAMonitor Provider`/`KDAMonitor Sublayer` absent before load, present after load, absent again after unload

---

## [0.6] - 2026-08-03

Second sensor: every image (DLL/EXE) loaded into any process is captured via `PsSetLoadImageNotifyRoutine`, pushed into the existing queue, and written to the log through the standard pipeline.

### Added
- `image_callback.c`/`.h`: `PsSetLoadImageNotifyRoutine` callback, captures image base, size, properties, system/mapped/partial-map flags, signature level/type, and full image path
- Registered/unregistered in `DriverEntry`/`DriverUnload`, after the process callback (unregistered first, symmetric teardown order)
- `event_types.h`: `KDAMON_IMAGE_LOAD_EVENT_DATA` (PID, image base/size, properties, three BOOLEAN-as-ULONG flags, signature level/type, fixed-size image name)
- `log_writer.c`: `KdaMonLogWriterWriteImageEvent`, dispatched via switch in `KdaMonLogWriterWriteEvent`

----

## [0.5] - 2026-08-02

First sensor: every time a process starts or exits, an event is pushed into the queue built in v0.3, and written to the log through the standard pipeline.

### Added
- `process_callback.c`/`.h`: `PsSetCreateProcessNotifyRoutineEx` callback, logs process create/exit
- Registered/unregistered in `DriverEntry`/`DriverUnload`
- Pushes into `event_queue.c` (v0.3)
- `event_types.h`: `KDAMON_PROCESS_EVENT_DATA` (PID, PPID, create/exit flag, fixed-size image name)
- `log_writer.c`: `KdaMonLogWriterWriteProcessEvent`, dispatched via switch in `KdaMonLogWriterWriteEvent`
- `log_writer.c`: `KdaMonJsonEscapeW` helper for safe JSON string escaping (backslashes, quotes) of NT-style paths

### Changed
- `ppid` is serialized as JSON `null` instead of `0` for exit events, where no parent PID is provided by the kernel

### Fixed
- `EventBuffer` in `log_writer.c` undersized for worst-case process event payload (256 -> 1000 bytes), causing silent event drops on long paths

---

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