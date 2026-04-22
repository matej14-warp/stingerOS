@echo off
"C:\Program Files\qemu\qemu-system-i386.exe" ^
    -cdrom stinger.iso^
    -m 300M ^
    -cpu max ^
    -smp 2 ^
    -serial stdio ^
    -device e1000,netdev=net0 ^
    -netdev user,id=net0 ^
    -audiodev dsound,id=snd0 ^
    -device intel-hda ^
    -device hda-duplex,audiodev=snd0

