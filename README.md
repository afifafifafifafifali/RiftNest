# RiftNest

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
- `cc` (GCC or Clang)
- `make`
- `curl` (for image pulls)
- `tar` (for image extraction)

Optional (for QEMU backend):
- `qemu-system-x86_64` >= 10.0.0
- A Linux kernel (`vmlinuz`) and optionally an initramfs in `~/.riftnest/kernel/`

Optional (for networking):
- `rootlesskit` + `slirp4netns` (proot backend)
- QEMU's built-in user-mode networking (QEMU backend, no extra deps)

## Installation

```sh
git clone https://github.com/afifafifafifafifali/RiftNest.git
cd riftnest
make
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

## Commands

### `riftnest pull <name> <url>`

Download a rootfs tarball from `<url>` and extract it into the image store. The tarball must be a flat rootfs (not nested under a directory).

```sh
riftnest pull alpine https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.2-x86_64.tar.gz
riftnest pull ubuntu https://cdimage.ubuntu.com/ubuntu-base/releases/24.04/release/ubuntu-base-24.04.2-base-amd64.tar.gz
```

### `riftnest list`

Show all stored images and persistent instances.

```
Images:
  alpine
  ubuntu
Instances:
  mybox
  devserver
```

### `riftnest run <image> [cmd...]`

Run a command in a fresh ephemeral instance of the given image. If no command is specified, `/bin/sh` is started. Changes are discarded when the process exits.

```sh
riftnest run alpine echo "hello from container"
riftnest run alpine /bin/sh
```

### `riftnest create <name> <image>`

Clone an image into a named persistent instance. The instance retains filesystem changes across `exec` calls.

```sh
riftnest create mybox alpine
riftnest create devserver ubuntu
```

### `riftnest exec <instance> [cmd...]`

Execute a command inside a persistent instance. If no command is specified, `/bin/sh` is started.

```sh
riftnest exec mybox /bin/sh
riftnest exec mybox cat /etc/os-release
riftnest exec mybox apk add git    # alpine instance
```

### `riftnest destroy <instance>`

Delete a persistent instance and all its data.

```sh
riftnest destroy mybox
```

### `riftnest rmi <image>`

Remove a stored image.

```sh
riftnest rmi alpine
```

### `riftnest net up <instance> [port-forward]`

Set up networking for an instance. On the proot backend, this creates a network namespace via `unshare --net` and runs `slirp4netns` inside it. On QEMU, networking is handled inline by the VM launch command.

```sh
riftnest net up mybox
riftnest net up mybox 127.0.0.1:8080:80/tcp
```

### `riftnest net down <instance>`

Tear down networking for an instance.

```sh
riftnest net down mybox
```

## Backends

### proot (default)

Uses [proot](https://proot-me.github.io/) to simulate a container via ptrace-based path substitution. No root privileges required.

- Intercepts syscalls to remap filesystem paths
- Fakes root (UID 0) inside the container
- Bind-mounts `/proc`, `/sys`, `/dev`, `/etc/resolv.conf`
- Fast startup, near-native I/O performance
- No real isolation (processes share the host kernel)

### QEMU (fallback)

Uses QEMU system emulation with TCG (no KVM required). Provides full VM isolation.

- Boots a real Linux kernel with `-kernel` and `-initrd`
- Shares rootfs via virtio-9p (`-fsdev` + `-device virtio-9p-pci`)
- Built-in user-mode networking (`-netdev user`)
- Requires a kernel image in `~/.riftnest/kernel/`
- Slower startup, true process/filesystem/network isolation

#### Setting up the QEMU backend

1. Download a kernel and initramfs:

```sh
mkdir -p ~/.riftnest/kernel
# Example: Alpine virt kernel
wget -P ~/.riftnest/kernel \
  https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-virt-3.20.2-x86_64.iso
# Extract vmlinuz-virt and initramfs-virt from the ISO
```

2. Verify QEMU is available:

```sh
qemu-system-x86_64 --version
# Must be >= 10.0.0
```

RiftNest will automatically use QEMU when proot is unavailable.

### Backend selection

RiftNest probes backends in this order:

1. **proot** -- checked first. Probes for the proot binary (in `target/proot`, `~/.riftnest/proot`, or `$PATH`).
2. **QEMU** -- fallback. Requires `qemu-system-x86_64` on `$PATH` with version >= 10.0.0.

If neither is available, RiftNest exits with error code `0xA0020`.

## Directory Structure

```
~/.riftnest/                  # RIFTNEST_HOME (default)
  images/                     # Stored rootfs images
    alpine/                   # Each image is a directory
    ubuntu/
  instances/                  # Persistent instances (cloned from images)
    mybox/
    devserver/
  kernel/                     # QEMU backend: kernel + initramfs
    vmlinuz-virt
    initramfs-virt
  net/                        # Network PID files
    mybox.pid
```

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `RIFTNEST_HOME` | `~/.riftnest` | Base directory for images, instances, and runtime data |

## Error Codes

| Code | Hex | Description |
|---|---|---|
| 1 | `0xA0001` | QEMU not found on PATH |
| 2 | `0xA0002` | QEMU version below minimum 10.0.0 |
| 3 | `0xA0010` | proot not available |
| 4 | `0xA0020` | No backend available |
| 5 | `0xA0030` | Image pull failed (download error) |
| 6 | `0xA0031` | Image extract failed (tar error) |
| 7 | `0xA0040` | Instance creation failed |
| 8 | `0xA0041` | Instance run failed |
| 9 | `0xA0042` | Instance exec failed |
| 10 | `0xA0050` | Network setup failed |
| 11 | `0xA0051` | Network teardown failed |
| 12 | `0xA0060` | 9p mount failed |
| 13 | `0xA0061` | QEMU boot failed |

## Architecture

```
riftnest CLI
    |
    v
backend_probe_select() ----> proot_backend  (preferred)
    |                             |
    | (if proot unavailable)      | fork/exec proot
    v                             |   -r <rootfs>
qemu_backend  (fallback)         |   -b /proc -b /sys -b /dev
    |                             |   -0 (fake root)
    | fork/exec qemu-system-x86_64
    |   -nographic -accel tcg
    |   -kernel <vmlinuz>
    |   -fsdev local,path=<rootfs>,security_model=mapped-xattr
    |   -device virtio-9p-pci
    |   -netdev user
    v
```

### Backend interface

All backends implement the `rn_backend_t` vtable:

```c
typedef struct rn_backend {
    const char *name;
    rn_err_t (*probe)(void);
    rn_err_t (*pull)(const char *name, const char *url);
    rn_err_t (*list_images)(void);
    rn_err_t (*list_instances)(void);
    rn_err_t (*run)(const char *image, int argc, char **argv);
    rn_err_t (*create)(const char *instance, const char *image);
    rn_err_t (*exec)(const char *instance, int argc, char **argv);
    rn_err_t (*destroy)(const char *instance);
    rn_err_t (*net_setup)(const char *instance, const char *host_fwd, const char *guest_port);
    rn_err_t (*net_teardown)(const char *instance);
} rn_backend_t;
```

To add a new backend, implement this interface and register it in `backend_probe.c`.

## License

Copyright (c) Afif Ali Saadman 2026. RiftNest containerization platform.
