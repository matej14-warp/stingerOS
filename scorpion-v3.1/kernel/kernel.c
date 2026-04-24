/* scorpion OS - kernel/kernel.c
   Main kernel entry point - called from boot.asm      */

#include "terminal.h"
#include "descriptor_tables.h"
#include "kmalloc.h"
#include "../fs/sfs.h"
#include "../drivers/keyboard.h"
#include "../drivers/rndis.h"
#include "../drivers/net.h"
#include "../drivers/ata.h"
#include "../drivers/wifi.h"
#include "../fs/partition.h"
#include "../auth/auth.h"
#include "../shell/shell.h"
#include <stdint.h>
#include <stddef.h>

/* Multiboot info structure (partial) */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    /* ... more fields we don't need right now */
} multiboot_info_t;

/* ---- Timer (PIT) ---- */
static volatile uint32_t ticks = 0;

static void timer_handler(registers_t *regs) {
    (void)regs;
    ticks++;
}

static void timer_init(uint32_t freq) {
    register_interrupt_handler(32, timer_handler); /* IRQ0 = INT32 */
    uint32_t divisor = 1193180 / freq;
    /* Command: channel 0, lobyte/hibyte, square wave mode */
    uint8_t lo = (uint8_t)(divisor & 0xFF);
    uint8_t hi = (uint8_t)((divisor >> 8) & 0xFF);
    __asm__ volatile("outb %0, $0x43" : : "a"((uint8_t)0x36));
    __asm__ volatile("outb %0, $0x40" : : "a"(lo));
    __asm__ volatile("outb %0, $0x40" : : "a"(hi));
}

/* ---- Kernel main ---- */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    (void)magic;
    /* 1. Set up VGA terminal first so we can print */
    terminal_init();
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("[scorpion] kernel starting...\n");
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    /* 2. Descriptor tables (GDT + IDT) */
    terminal_printf("[scorpion] initializing GDT...\n");
    gdt_init();
    terminal_printf("[scorpion] initializing IDT...\n");
    idt_init();

    /* 3. Enable interrupts */
    __asm__ volatile("sti");
    terminal_printf("[scorpion] interrupts enabled\n");

    /* 4. PIT timer at 1000 Hz */
    terminal_printf("[scorpion] initializing PIT timer @ 1000Hz...\n");
    timer_init(1000);

    /* 5. Report memory */
    if(mbi && (mbi->flags & 1)) {
        uint32_t mem_mb = (mbi->mem_upper) / 1024;
        terminal_printf("[scorpion] memory: lower=%dKB upper=%dKB (%dMB)\n",
                        mbi->mem_lower, mbi->mem_upper, mem_mb);
    }

    /* 6. Heap (static bump allocator, no init needed) */
    terminal_printf("[scorpion] heap ready (4MB at 0x400000)\n");

    /* 7. ScorpionFS */
    terminal_printf("[scorpion] mounting ScorpionFS...\n");
    sfs_init();
    terminal_printf("[scorpion] filesystem ready\n");

    /* 8. Keyboard */
    terminal_printf("[scorpion] initializing keyboard...\n");
    keyboard_init();

    /* 8b. ATA disk + partition detection */
    terminal_printf("[scorpion] scanning ATA drives...\n");
    ata_init();

    /* 8c. WiFi drivers (physical hardware only — not present in QEMU rtl8139 config) */
    terminal_printf("[scorpion] probing WiFi adapters...\n");
    wifi_rtl8188_probe();
    wifi_rtl8192_probe();
    wifi_ath9k_probe();
    wifi_mt7601_probe();
    if(!wifi_active)
        terminal_printf("[scorpion] no WiFi adapter (expected: QEMU uses rtl8139 wired NIC)\n");

    /* 9. Network — RTL8139 PCI NIC */
    terminal_printf("[scorpion] initializing RTL8139 network...\n");
    if(rndis_init() != 0)
        terminal_printf("[scorpion] WARNING: RTL8139 init failed, network unavailable\n");
    net_init();
    terminal_printf("[scorpion] requesting DHCP lease...\n");
    net_dhcp();

    /* 10. Hand off to login prompt, then shell */
    terminal_printf("[scorpion] kernel init complete\n\n");
    auth_init_etc();
    login_prompt();
    shell_run();

    /* Should never reach here */
    terminal_printf("[scorpion] PANIC: shell returned!\n");
    __asm__ volatile("cli");
    for(;;) __asm__ volatile("hlt");
}
