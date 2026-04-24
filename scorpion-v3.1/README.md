# scorpion OS

A 32-bit operating system with a module-based package manager (`fetch`).

---

## v2.0 New Features

### Disk & Partition Management
New `drivers/ata.c` — ATA/IDE PIO driver (28-bit LBA, primary + secondary channels, drive identification).

New `fs/partition.c` — MBR partition table read/write with overlap checking and 1 MiB alignment.

Shell commands:
| Command | Description |
|---|---|
| `diskinfo` | Show detected ATA drives and sizes |
| `lspart` | Print all partition entries |
| `fdisk` | Interactive partition editor (n/d/p/w/q) |
| `mkpart <d> <i> <type> <lba> <mb>` | Create partition (types: af=SFS 83=Linux 0b=FAT32) |
| `rmpart <d> <i>` | Delete partition |
| `format <d> <i>` | Write ScorpionFS superblock to partition |
| `install` | Guided installer — format, mkdir, write grub.cfg, patch MBR |

### WiFi Drivers
New `drivers/wifi.c` — driver vtable framework with four real chipset probes:

| Driver | Chipset | Bus |
|---|---|---|
| `rtl8188` | Realtek RTL8188CUS/EUS | USB |
| `rtl8192cu` | Realtek RTL8192CU | USB |
| `ath9k` | Atheros AR9271 / AR9285 | USB + PCI |
| `mt7601u` | MediaTek MT7601U | USB |

Shell: `wifi scan | connect <ssid> [pass] | disconnect | status`

### Extra CLI Commands
`write <file> <text>`, `wget <url>`, `ping <ip>`, `ifconfig`, `free`, `uname`

---

## Architecture

```
boot/
  boot.asm          - Multiboot2 assembly stub, GDT/IDT flush, ISR/IRQ stubs
  grub.cfg          - GRUB2 config
  linker.ld         - Kernel linker script (loads at 1 MiB)

kernel/
  kernel.c          - kernel_main(): init sequence + handoff to shell
  terminal.c/h      - VGA text-mode driver (80x25, colour, scrolling, printf)
  descriptor_tables.c/h  - GDT (5 entries) + IDT (256 entries) + PIC remap
  kmalloc.c/h       - Kernel heap allocator (free-list, 4 MiB @ 0x400000)

fs/
  sfs.c/h           - ScorpionFS: simple in-memory tree filesystem
                      Supports: mkdir, create, write, delete, resolve, list

drivers/
  keyboard.c/h      - PS/2 keyboard driver (IRQ1, scancode map, key buffer)
  rndis.c/h         - RNDIS USB networking driver (PCI USB scan + protocol)
  net.c/h           - TCP/IP stack: ARP, DHCP, IP, TCP, HTTP GET, DNS

fetch/
  fetch.c/h         - Package manager (downloads + extracts + runs setup)
  zip.c/h           - Minimal ZIP reader (STORE + DEFLATE)

mbf/
  mbf.c/h           - Manly Batch File interpreter (if/else, vars, commands)

shell/
  shell.c/h         - Interactive shell with readline and built-in commands

cdn-backend/
  server.py         - Python CDN server (port 5000)
  fetch3/           - Package directories
    hello-world/
      main.c
      setup-runtime.mbf
```

---

## Building

### Prerequisites

You need an **i686-elf cross-compiler**. On most Linux distros:

```bash
# Option 1: Build from source (recommended)
# Follow https://wiki.osdev.org/GCC_Cross-Compiler
# Target: i686-elf

# Option 2: Use a pre-built cross-compiler
# Arch Linux:
yay -S i686-elf-gcc i686-elf-binutils

# Ubuntu/Debian (from OSDev toolchain PPA or build manually)
# See: https://github.com/lordmilko/i686-elf-tools

# Also needed:
sudo apt install nasm grub-pc-bin grub-common xorriso  # Ubuntu/Debian
sudo pacman -S nasm grub xorriso                        # Arch
```

### Build the kernel

```bash
cd scorpion/
make            # builds build/kernel.elf
make iso        # builds scorpion.iso (needs grub-mkrescue + xorriso)
```

### Run in QEMU

```bash
make run
# or manually:
qemu-system-i386 \
  -cdrom scorpion.iso \
  -m 128M \
  -device usb-net,netdev=net0 \
  -netdev user,id=net0 \
  -usb
```

The `-device usb-net` option provides a RNDIS USB network device.
QEMU's `user` networking will handle DHCP and forward traffic to the host.

### Debug with GDB

```bash
make debug
# In another terminal:
i686-elf-gdb build/kernel.elf -ex "target remote :1234"
```

---

## Shell Commands

| Command           | Description                              |
|-------------------|------------------------------------------|
| `ls [dir]`        | List directory contents                  |
| `cd [dir]`        | Change directory                         |
| `pwd`             | Print working directory                  |
| `cat <file>`      | Print file contents                      |
| `mkdir <name>`    | Create a directory                       |
| `rm <path>`       | Delete a file or empty directory         |
| `echo <text>`     | Print text                               |
| `fetch <package>` | Download and install a module            |
| `mbf <script>`    | Run a .mbf script file                   |
| `clear`           | Clear the screen                         |
| `reboot`          | Reboot the system                        |
| `halt`            | Halt the system                          |
| `help`            | Show help                                |

---

## Package Manager (`fetch`)

Packages are served from `cdn.voided.uk/fetch3/<package>` as `.zip` files.

### Install a package

```
scorpion# fetch hello-world
```

Flow:
1. HTTP GET `http://cdn.voided.uk/fetch3/hello-world`
2. Download `.zip` → `/tmp/hello-world/`
3. Extract all files, show contents
4. Prompt: `Proceed with installation? [Y/n]`
5. If yes: run `setup-runtime.mbf` from the extracted dir

### Package structure

```
my-package/
  setup-runtime.mbf   ← required: runs on install confirm
  main.c              ← your module files
  README              ← optional
  ...
```

---

## MBF Script Format (`.mbf`)

Manly Batch File - simple install/setup scripting language.

```mbf
# This is a comment

echo Installing my module...

set version 2.0
set name    mymodule

mkdir /modules/mymodule
copy  main.c /modules/mymodule
install main.c

if version == 2.0 {
    echo Version 2.0 installed!
} else {
    echo Unknown version installed.
}

echo Done!
```

### MBF Commands

| Command              | Description                                      |
|----------------------|--------------------------------------------------|
| `echo <text>`        | Print text                                       |
| `set <var> <value>`  | Set a variable                                   |
| `$VAR`               | Expand a variable                                |
| `mkdir <path>`       | Create directory on ScorpionFS                   |
| `copy <src> <dst>`   | Copy file to directory                           |
| `rm <path>`          | Delete file/dir                                  |
| `install <file>`     | Copy file to `/bin/`                             |
| `reboot`             | Reboot the system                                |
| `halt`               | Halt the system                                  |
| `if VAR == val { }` | Conditional (== or !=), optional `else { }`      |

---

## CDN Backend

```bash
cd scorpion/cdn-backend/
python3 server.py
```

Runs on port `5000`. API:

| Endpoint               | Returns                          |
|------------------------|----------------------------------|
| `GET /`                | Server info + package list JSON  |
| `GET /fetch3`          | Package list JSON                |
| `GET /fetch3/<name>`   | Package as `.zip` file           |

### Adding a package

```bash
mkdir cdn-backend/fetch3/my-package
echo 'echo hello' > cdn-backend/fetch3/my-package/setup-runtime.mbf
# Add any .c files or other content
```

The server auto-zips the directory on every request (no restart needed).

---

## Filesystem (ScorpionFS)

Simple in-memory tree filesystem. Directories and files are nodes in a
linked list tree. All data lives in the kernel heap (4 MiB).

Default tree at boot:
```
/
├── bin/
├── etc/
│   └── version
├── modules/
├── tmp/
└── dev/
```

---

## Networking

The network stack supports:
- **ARP**: send requests, cache responses, reply to requests
- **DHCP**: broadcast DISCOVER, parse OFFER/ACK, fallback to 192.168.1.100
- **IP**: send/receive IPv4 packets
- **TCP**: connect, send, recv, close (simple blocking sockets)
- **HTTP**: GET requests (used by `fetch`)
- **DNS**: basic A-record query (routes via gateway)

The RNDIS driver scans PCI for a USB controller and initializes the
RNDIS protocol. In QEMU use `-device usb-net,netdev=net0 -netdev user,id=net0`.

---

## ScorpionFS Node Limits

- Max nodes: 512
- Max filename length: 64 chars
- Heap: 4 MiB (shared between all allocations)

---

## License

scorpion OS is released under the MIT License.
