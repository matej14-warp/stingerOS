@echo off
"C:\Program Files\qemu\qemu-system-i386.exe" ^
    -cdrom scorpion.iso ^
    -m 128M ^
    -serial stdio ^
    -device rtl8139,netdev=net0 ^
    -netdev user,id=net0
