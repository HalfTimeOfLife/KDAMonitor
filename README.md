# KDAMonitor - Kernel Driver Activity Monitor

A Windows kernel driver (in C) for logging process, image load, network connection and registry activity in real time.

> ⚠️ Learning/research project: intended for isolated sandbox VMs only, do not use in production.
---

## Status

Early development: currently at **v0.1 (driver skeleton)**. No monitoring feature is implemented yet. See [ROADMAP.md](./ROADMAP.md) for planned versions.

---

## Concept

KDAMonitor is made of two components:

- **`driver/`**: a Windows kernel driver (`.sys`) that runs at the kernel level, registering with native kernel callbacks (process, thread, image load, registry) and WFP (network) to observe system activity from a point malware cannot easily see or bypass.
- **`client/`** *(not implemented yet, see v0.11)*: a usermode command-line application that connects to the driver to display events in real time. The driver is designed to log independently to disk, so the client is only a convenience viewer.

The intended use case is malware sandbox analysis: run a sample in an isolated VM with KDAMonitor loaded, and get a full timeline (through logs) of its process, network, and persistence activity after execution.

---

## Requirements

- Windows 10/11 (test VM recommended, test signing mode enabled)
- Windows Driver Kit (WDK)
- Visual Studio with WDK extension

---

## Project structure (v0.1)

```bash
KDAMonitor/
├── docs
│   └── crashes.md
├── driver
│   ├── include
│   │   └── driver.h
│   ├── src
│   │   └── driver_entry.c
│   ├── KDAMonitor.inf
│   ├── KDAMonitor.vcxproj
│   └── packages.config
├── .gitignore
├── KDAMonitor.sln
├── LICENSE
├── README.md
└── ROADMAP.md
```

---

## Building

**Not documented yet.** The driver currently has no observable behavior beyond load/unload (see [ROADMAP.md](./ROADMAP.md)). A proper build and installation guide will be added once the core mechanisms (device, event queue, disk logging) are in place around v0.4.

---

## Resources

### Official documentation
- [Windows Driver Kit documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/)
- [Getting Started with Windows Drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/)
- [Kernel-Mode Driver APIs reference](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/)
- [Windows Filtering Platform documentation](https://learn.microsoft.com/en-us/windows/win32/fwp/windows-filtering-platform-start-page)
- [Debugging Tools for Windows / WinDbg](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/)

### Code samples
- [microsoft/Windows-driver-samples](https://github.com/microsoft/Windows-driver-samples)

### Books
- *Windows Kernel Programming*, Pavel Yosifovich (2nd edition)
- *Windows Internals*, Yosifovich / Solomon / Ionescu (7th edition) — used as a reference, not read cover to cover

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.