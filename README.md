# KDAMonitor - Kernel Driver Activity Monitor



A Windows kernel driver (in C) for malware sandbox analysis, logging process, image load, network connection and registry activity in real time.

---

## Status

Early development: currently at **v0.1 (driver skeleton)**. No monitoring feature is implemented yet. See [ROADMAP.md](./ROADMAP.md) for planned versions.

---

## Concept

KDAMonitor is made of two components:

- **`driver/`**: a Windows kernel driver (`.sys`) that runs at the kernel level, registering with native kernel callbacks (process, thread, image load, registry) and WFP (network connections) to observe system activity from a point malware cannot easily see or bypass.
- **`client/`** *(not implemented yet, see v0.11)*: a usermode command-line application that connects to the driver to display events in real time. The driver is designed to log independently to disk, so the client is only a convenience viewer, not a required component for data integrity.

The intended use case is malware sandbox analysis: run a sample in an isolated VM with KDAMonitor loaded, and get a full timeline of its process, network, and persistence activity after execution.

---

## Requirements

- Windows 10/11 (test VM recommended, test signing mode enabled)
- Windows Driver Kit (WDK)
- Visual Studio with WDK extension

---

## Project structure (v0.1)

```bash
KDAMonitor/
├── driver/
│   ├── src/
│   │   └── driver_entry.c
│   ├── include/
│   │   └── driver.h
│   └── KDAMonitor.inf
├── docs/
│   └── crashes.md
├── .gitignore
├── README.md
├── ROADMAP.md
└── LICENSE
```

---

## Building

*(To be documented once the build process is finalized: WDK project setup, signing, deployment to test VM.)*

---

## Disclaimer

KDAMonitor is a learning and research project intended for use in isolated, offline sandbox VMs only. It is not intended for production or endpoint protection use.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.