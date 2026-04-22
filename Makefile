


TARGET_PREFIX ?= i686-linux-gnu
CC     = $(TARGET_PREFIX)-gcc
LD     = $(TARGET_PREFIX)-ld
AS     = nasm
GRUB   = grub-mkrescue

GCC_INC := $(shell $(CC) -print-file-name=include)

CFLAGS  = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
           -fno-stack-protector -fno-pic \
           -nostdlib -nostdinc \
           -I$(GCC_INC) \
           -I. \
           -I./kernel \
           -I./drivers \
           -I./fs \
           -I./fetch \
           -I./mbf \
           -I./shell \
           -I./auth \
           -I./crypto \
           -I./net

ASFLAGS = -f elf32
LIBGCC := $(shell $(CC) -m32 -print-libgcc-file-name)
LDFLAGS = -m elf_i386 -T boot/linker.ld --oformat elf32-i386

KERNEL_ELF = build/kernel.elf
ISO        = stinger.iso

C_SOURCES = \
    kernel/terminal.c \
    kernel/descriptor_tables.c \
    kernel/kmalloc.c \
    kernel/kernel.c \
    kernel/paging.c \
    kernel/acpi.c \
    kernel/smp.c \
    kernel/signal.c \
    kernel/dynlink.c \
    kernel/kapi.c \
    kernel/vbe.c \
    kernel/sched.c \
    fs/sfs.c \
    fs/partition.c \
    fs/sfs_disk.c \
    drivers/keyboard.c \
    drivers/ata.c \
    drivers/ahci.c \
    drivers/ax201.c \
    drivers/intel_gpu.c \
    drivers/i915_gen11.c \
    drivers/wifi.c \
    drivers/wifi_extra.c \
    net/dhcp_client.c \
    drivers/rndis.c \
    drivers/e1000.c \
    drivers/virtio_net.c \
    drivers/usbnet.c \
    drivers/ne2000.c \
    drivers/net.c \
    drivers/pci.c \
    drivers/usb.c \
    drivers/xhci.c \
    drivers/sound.c \
    drivers/hda.c \
    drivers/hda_isr.c \
    drivers/mp3.c \
    drivers/media.c \
    drivers/mouse.c \
    fetch/zip.c \
    fetch/fetch.c \
    mbf/mbf.c \
    auth/auth.c \
    crypto/tiny_aes.c \
    net/ssh.c \
    net/irc.c \
    net/dns.c \
    shell/async_exec.c \
    shell/file_dialog.c \
    shell/shell.c \
    shell/nano.c \
    shell/draw.c \
    shell/make.c \
    shell/games.c \
    shell/yaoigui.c \
    shell/femboygl.c \
    gui/gui.c

ASM_SOURCES = \
    boot/boot.asm

C_OBJS   = $(patsubst %.c, build/%.o, $(C_SOURCES))
ASM_OBJS = $(patsubst %.asm, build/%.o, $(ASM_SOURCES))
ALL_OBJS = $(ASM_OBJS) $(C_OBJS)

.PHONY: all clean iso run debug check-toolchain

all: $(KERNEL_ELF)

build/drivers/media.o: drivers/media.c drivers/media.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC: $<"

$(KERNEL_ELF): $(ALL_OBJS) boot/linker.ld
	@mkdir -p build
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS) $(LIBGCC)
	@echo "  Linked: $@"

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC: $<"

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
	@echo "  AS: $<"


MODULE_CFLAGS = $(CFLAGS) -fPIC
MODULE_LDFLAGS = -m elf_i386 -shared -nostdlib

MODS = modules/calc.bin modules/term.bin modules/fm.bin modules/ide.bin \
       modules/morse.bin modules/ascii.bin modules/browser.bin modules/settings.bin \
       modules/draw.bin modules/sysmon.bin

modules/%.bin: gui/app_%.c
	@mkdir -p modules
	$(CC) $(MODULE_CFLAGS) -c $< -o build/module_$*.o
	$(LD) $(MODULE_LDFLAGS) build/module_$*.o -o $@
	@echo "  MOD: $@"

iso: $(KERNEL_ELF)
	python3 update_modules.py
	@mkdir -p isodir/boot/grub
	cp $(KERNEL_ELF)  isodir/boot/kernel.elf
	cp boot/grub.cfg  isodir/boot/grub/grub.cfg

	@if [ -d modules ]; then \
		cp -r modules/* isodir/; \
		echo "  Added modules to ISO"; \
	fi
	@rm -f $(ISO)
	$(GRUB) -o $(ISO) isodir
	@echo "  ISO: $(ISO)"

run: iso
	qemu-system-i386 \
		-cdrom $(ISO) \
		-m 128M \
		-smp 2 \
		-serial stdio \
		-vga std \
		-device e1000,netdev=net0 \
		-netdev user,id=net0,dns=8.8.8.8 \
		-audiodev none,id=snd0 -machine pcspk-audiodev=snd0

run-hd: iso hd.img
	qemu-system-i386 \
		-cdrom $(ISO) \
		-hda hd.img \
		-m 128M \
		-smp 2 \
		-serial stdio \
		-device e1000,netdev=net0 \
		-netdev user,id=net0,dns=8.8.8.8 \
		-audiodev none,id=snd0 -machine pcspk-audiodev=snd0 \
		-usb -device usb-kbd

hd.img:
	dd if=/dev/zero of=hd.img bs=1M count=256

debug: iso
	qemu-system-i386 \
		-cdrom $(ISO) \
		-m 128M \
		-serial stdio \
		-device e1000,netdev=net0 \
		-netdev user,id=net0 \
		-s -S &
	$(TARGET_PREFIX)-gdb $(KERNEL_ELF) \
		-ex "target remote :1234" \
		-ex "break kernel_main"

clean:
	rm -rf build isodir $(ISO)

check-toolchain:
	@which $(CC)   || (echo "ERROR: $(CC) not found." && exit 1)
	@which $(AS)   || (echo "ERROR: nasm not found." && exit 1)
	@which $(GRUB) || (echo "ERROR: grub-mkrescue not found." && exit 1)
	@echo "Toolchain OK"




