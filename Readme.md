# uds-daemon-example

`uds-daemon-example` is a minimal demonstration project that shows how to implement a systemd socket-activated daemon using Unix Domain Sockets (UDS).
It includes structured logging using spdlog and integrates seamlessly with systemd service and socket units.

---

## Features

### Socket Activation
- Daemon only starts when a client connects to the Unix domain socket
- Fully managed and monitored by systemd

### Fast & Structured Logging
- High-performance logging with spdlog
- Uses external fmt formatting backend

### Cross-Architecture Support
- Builds for amd64 and arm64
- Debian packaging ready for cross-build workflows

### Example Client Included
- Used to trigger activation and test the daemon behavior

---
## Build Instructions

This project provides a Makefile to build for multiple architectures and to create Debian packages.  
All builds are designed to run inside a container environment defined via `podman-compose`.

Available build types are:
You can build both natively and as Debian packages:

| Target | Architecture | Build type | Command |
|--------|---------------|------------|---------|
| `amd64-Release` | amd64 | Release binary | `make amd64-Release` |
| `amd64-Debug` | amd64 | Debug binary | `make amd64-Debug` |
| `arm64-Release` | arm64 | Release (cross) | `make arm64-Release` |
| `arm64-Debug` | arm64 | Debug (cross) | `make arm64-Debug` |
| `.deb package` | amd64 | Debian package | `make amd64-debbuild` |
| `.deb package` | arm64 | Debian package (cross) | `make arm64-debbuild` |

### Prerequisites
Before building `uds-daemon-example`, make sure the following system packages are installed:
- `podman`
- `podman-compose` (install globally via pip):

### How to build

By default, running any target will build **inside the configured builder container**:
To create the configured builder container run:

```sh
make builder
```
To build for arm64 (cross-compiled):

```sh
make amd64-Release
make arm64-Release
```

Cleaning Builds

To force a clean re-build, add CLEAN=1:

```sh
make amd64-Release CLEAN=1
```

Debian Package Builds

```sh
make amd64-debbuild
make arm64-debbuild
```
