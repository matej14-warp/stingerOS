# scorpion OS - Makefile
# Requires: i686-linux-gnu-gcc, nasm, grub-mkrescue, xorriso

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
LDFLAGS = -m elf_i386 -T boot/linker.ld --oformat elf32-i386

KERNEL_ELF = build/kernel.elf
ISO        = scorpion.iso

C_SOURCES = \
    kernel/terminal.c \
    kernel/descriptor_tables.c \
    kernel/kmalloc.c \
    kernel/kernel.c \
    fs/sfs.c \
    fs/partition.c \
    fs/sfs_disk.c \
    drivers/keyboard.c \
    drivers/ata.c \
    drivers/wifi.c \
    drivers/rndis.c \
    drivers/net.c \
    fetch/zip.c \
    fetch/fetch.c \
    mbf/mbf.c \
    auth/auth.c \
    crypto/tiny_aes.c \
    net/ssh.c \
    net/irc.c \
    shell/shell.c

ASM_SOURCES = \
    boot/boot.asm

C_OBJS   = $(patsubst %.c, build/%.o, $(C_SOURCES))
ASM_OBJS = $(patsubst %.asm, build/%.o, $(ASM_SOURCES))
ALL_OBJS = $(ASM_OBJS) $(C_OBJS)

.PHONY: all clean iso run debug check-toolchain

all: $(KERNEL_ELF)

$(KERNEL_ELF): $(ALL_OBJS) boot/linker.ld
	@mkdir -p build
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo "  Linked: $@"

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC: $<"

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
	@echo "  AS: $<"

iso: $(KERNEL_ELF)
	@mkdir -p isodir/boot/grub
	cp $(KERNEL_ELF)  isodir/boot/kernel.elf
	cp boot/grub.cfg  isodir/boot/grub/grub.cfg
	$(GRUB) -o $(ISO) isodir
	@echo "  ISO: $(ISO)"

run: iso
	qemu-system-i386 \
		-cdrom $(ISO) \
		-m 128M \
		-serial stdio \
		-device rtl8139,netdev=net0 \
		-netdev user,id=net0

# Run with a 256 MB raw disk attached (for sync/diskload testing)
run-hd: iso hd.img
	qemu-system-i386 \
		-cdrom $(ISO) \
		-hda hd.img \
		-m 128M \
		-serial stdio \
		-device rtl8139,netdev=net0 \
		-netdev user,id=net0

hd.img:
	dd if=/dev/zero of=hd.img bs=1M count=256

debug: iso
	qemu-system-i386 \
		-cdrom $(ISO) \
		-m 128M \
		-serial stdio \
		-device rtl8139,netdev=net0 \
		-netdev user,id=net0 \
		-s -S &
	$(TARGET_PREFIX)-gdb $(KERNEL_ELF) \
		-ex "target remote :1234" \
		-ex "break kernel_main"

clean:
	rm -rf build isodir $(ISO)
# to je prdeli
check-toolchain:
	@which $(CC)   || (echo "ERROR: $(CC) not found." && exit 1)
	@which $(AS)   || (echo "ERROR: nasm not found." && exit 1)
	@which $(GRUB) || (echo "ERROR: grub-mkrescue not found." && exit 1)
	@echo "Toolchain OK"