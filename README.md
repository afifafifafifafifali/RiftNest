# RiftNest
> [!WARNING]
> "SOME FEATURES MAY REQUIRE ROOT PRIVILIGES"

A lightweight container runtime for Linux, written in C11. Runs rootfs images using either **proot** (ptrace-based, no root required) or **QEMU** (full VM via TCG emulation with 9p shared filesystems). Backends are auto-detected at runtime with proot preferred and QEMU as fallback. 

## Features

- **Dual backend** -- proot (fast, unprivileged) and QEMU (full isolation, TCG)
- **Auto-detection** -- proot is tried first, QEMU fallback if unavailable
- **Image management** -- pull rootfs tarballs from any URL, list, remove
- **Instance lifecycle** -- create persistent instances, run ephemeral, exec, destroy
- **Networking** -- rootlesskit+slirp4netns for proot, built-in user-mode for QEMU
- **9p filesystem sharing** -- QEMU backend shares rootfs via virtio-9p
- **Zero dependencies beyond curl** -- proot binary is auto-downloaded on first build

## Requirements

- Linux (x86_64)
- `cc` (GCC only)
- `make`
- `curl` (for image pulls)
- `tar` (for image extraction)
- **Root privileges** (effective UID 0) for full isolation and all features

Optional (for QEMU backend):
- `qemu-system-x86_64` >= 10.0.0
- A Linux kernel (`vmlinuz`) and optionally an initramfs in `~/.riftnest/kernel/`

Optional (for networking):
- `rootlesskit` + `slirp4netns` (proot backend, works without root)
- QEMU's built-in user-mode networking (QEMU backend, requires root)

## Installation

```sh
git clone https://github.com/afifafifafifafifali/RiftNest.git
cd RiftNest
make -j2
```

The `target/riftnest` binary is produced. On first build, the Makefile downloads a static proot binary into `target/proot`.

## Quick Start

```sh
# Pull an Alpine Linux rootfs
./target/riftnest pull alpine \
  https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.2-x86_64.tar.gz

# Run a command inside it (ephemeral)
./target/riftnest run alpine cat /etc/alpine-release

# Create a persistent instance
./target/riftnest create mybox alpine

# Execute into the instance
./target/riftnest exec mybox /bin/sh

# List images and instances
./target/riftnest list

# Clean up
./target/riftnest destroy mybox
./target/riftnest rmi alpine
```

## Docs
Visit the ``docs/docs.md`` file.

## License
Licensed under the CC-BY-SA 4.0
Copyright (c) Afif Ali Saadman & Corex Team, 2026. RiftNest containerization platform.
