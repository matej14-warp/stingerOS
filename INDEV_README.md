# stinger OS v4.0 — indev release

> ⚠️ **this is an unfinished indev build. it has not been fully tested. expect crashes, missing features, and general instability. run at your own risk.**

---

## what is this

stinger is a hobby x86 OS built from scratch. no linux, no unix, no borrowed kernel. everything — bootloader, memory, filesystem, networking, graphics, shell — written by hand.

this build was developed partly on a **vidaa smart TV via OnWorks** after the dev's PC was confiscated. a RAID array suffered water damage mid-development and ~50% of source code was lost permanently. it shipped anyway.

---

## what works (mostly)

- VBE framebuffer with ANSI color support
- SMP (multicore, boots secondary CPUs via APIC)
- RTL8139 ethernet + DHCP + TCP/IP stack
- **YaoiGUI** — a Win10-style desktop compositor with draggable windows
  - Terminal app with live command output
  - File Manager with directory browsing
  - Browser (fetches pages over HTTP, renders as plaintext)
  - System Info, Settings
- 140+ shell commands
- In-kernel filesystem (stingerFS)
- SSH client, IRC client, package manager (`fetch`)
- nano text editor (fullscreen)
- Games: snake, tetris, pong

---

## how to run

```
qemu-system-i386 -cdrom stinger.iso -m 128M -device rtl8139,netdev=net0 -netdev user,id=net0
```

or double-click `run.bat` on Windows.

default login: `root` / `root`

---

## known issues

- networking may fail on first boot — run `dhcp` in terminal to retry
- GUI terminal interactive commands (rtl-cfg etc.) partially broken
- ASCII banner letters are questionable
- pipe support is best-effort only
- WiFi detection only, no actual connection

---

*stinger OS by Matyas Vavra*


