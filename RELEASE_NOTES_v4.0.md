# stinger OS v4.0 — indev release
### "the mental damage update"

> **this is an early indev build for testers and nerds only.**
> expect rough edges, missing features, and general chaos.
> that's part of the charm.

---

## what's new since v3.1

### kernel
- **SMP support** — APIC-based multicore, detects and boots APs via MADT, IPI dispatch to secondary CPUs
- **VBE framebuffer** — high-res graphics mode replacing VGA text, ANSI escape sequence support, 8x16 font rendering
- **fixed ANSI color palette** — corrected VGA→RGB mapping (cyan was brown. it's not anymore)
- **framebuffer performance** — replaced naive pixel loop with `rep movsd/stosd` for memcpy/memset, batched string flips instead of full-screen blit per character. dramatically faster boot log
- **signals/IPC subsystem** — basic inter-process signalling
- **dynamic linker** — in-kernel module loading via dynlink
- **ACPI** — RSDP/RSDT/MADT parsing, CPU enumeration
- **kapi** — kernel API surface for userspace calls

### drivers
- **ne2000** — full ISA/PCI NE2000 ethernet driver (PCI 0x10EC:0x8029, ISA fallback at 0x300/0x280/0x320/0x340)
- **virtio-net** — VirtIO legacy 0.9/transitional 1.0 network driver
- **e1000** — Intel e1000 ethernet (82540EM and variants)
- **RTL8139** — RNDIS-style driver, tested and working in QEMU
- **AHCI** — SATA controller support
- **USB** — basic USB controller detection
- **mouse** — PS/2 mouse with IntelliMouse scroll wheel extension, fixed inverted Y axis
- **sound** — PC speaker driver
- **wifi** — WiFi adapter detection (limited)

### networking
- **full TCP/IP stack** — ARP, DHCP, IP, ICMP, TCP, UDP all implemented from scratch
- **DHCP** — automatic IP configuration with 3-attempt retry and static fallback (192.168.1.100)
- **DNS** — hostname resolution
- **HTTP GET** — fetch web content over TCP
- **ping** — ICMP echo with RTT
- **IRC client** — yes really
- **SSH client** — yes really
- **netcat (nc)** — raw TCP connections
- **rtl-cfg** — interactive network configurator TUI
- **fetch** — stinger package manager, pulls packages from cdn.voided.uk

### filesystem
- **stingerFS (SFS)** — in-memory filesystem, full read/write
- **partition detection** — MBR partition table parsing
- **disk I/O** — ATA PIO + AHCI read/write

### shell
- **140+ commands** — full list below
- **YaoiGUI** — VBE compositor/desktop environment (`yaoigui` to launch)
- **pipe support** — `cmd1 | cmd2`
- **output redirection** — `cmd > file`
- **multiline commands** — trailing `\`
- **tab completion**
- **history** — `!!` repeat last, `!N` repeat nth
- **aliases** — `alias`, `unalias`
- **environment variables** — `export`, `unset`, `printenv`
- **directory stack** — `pushd`, `popd`, `dirs`
- **custom prompt** — `prompt \u@\h:\w$`
- **first-run setup wizard** — `setup`
- **MBF scripting** — in-kernel script runtime

### auth
- multi-user support, password hashing, login/logout, `passwd`, `adduser`

### crypto
- AES implementation (`tiny_aes`)
- MD5, SHA256, CRC32, base64, rot13

---

## full command list (140+)

```
acpi adduser alias append ascii base64 based banner beep boykisser brainrot
calc cat cd chad chmod chown clear cls color colour compress cowsay cp crc32
curl cut date decompress df dhcp diff diskinfo diskload dllist dlopen dmesg
dns doge doomscroll draw du echo emacs env exit export false fdisk fetch
figlet find flip fortune free fstab gigachad grep groups halt head help
hexcalc hexdump hi history hostname id ifconfig install irc kill less ligma
lmao ln lolcat logout ls lscpu lspci lsusb make man matrix mbf md5sum
meminfo memtest mkdir mkpart modules more morse mv nano nc netcfg nl
nslookup nyan od ohio passwd paste pipes ping play popd poweroff printenv
ps pushd pwd random ratio reboot repeat reset rev rickroll rm rmpart rot13
route rtl-cfg run scrap seq sha256sum shrug shutdown skibidi sl sleep
smpinfo sort sound speed spin ssh stat strings tac tableflip tail tee time
top touch toupper tolower tr tree true type unalias uniq unset uname updog
uptime watch wc wget which whoami wifi xxd yaoigui yeet yes 42
```

---

## known issues / missing stuff

- **this is a indev build** — some features may be partially implemented or behave unexpectedly
- pipe support is best-effort (no real stdout capture yet)
- WiFi is detection-only, no actual connection
- disk install (`install` command) may be unstable
- ASCII banner in shell is still a work in progress typographically

---

## tested on

- QEMU `-device rtl8139` with user networking
- x86 i686, 128MB RAM, VBE 800x600+

---

## how to run

```
qemu-system-i386 -cdrom stinger.iso -m 128M -device rtl8139,netdev=net0 -netdev user,id=net0
```

or just double-click `run.bat` on Windows.

default login: `root` / `root`

---

*stinger OS by goober/modrejzralok — built on a potato, shipped anyway*


