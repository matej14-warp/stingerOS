/* stinger OS - kernel linker script */
/* Produces a flat 32-bit ELF suitable for GRUB multiboot */

ENTRY(_start)

SECTIONS {
    /* Load at 1 MiB - standard for x86 kernels */
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}


