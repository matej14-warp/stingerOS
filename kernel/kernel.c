/* scorpion OS - kernel/kernel.c  v4.0
   New in v4.0: ACPI power-off, PCI scan, SMP, USB, sound, signals,
   dynamic linker, DNS, module system                                 */

#include "terminal.h"
#include "descriptor_tables.h"
#include "kmalloc.h"
#include "acpi.h"
#include "smp.h"
#include "signal.h"
#include "dynlink.h"
#include "kapi.h"
#include "sched.h"
#include "../fs/sfs.h"
#include "../drivers/keyboard.h"
#include "../drivers/rndis.h"
#include "../drivers/net.h"
#include "../drivers/ata.h"
#include "../drivers/ahci.h"
#include "../drivers/wifi.h"
#include "../drivers/pci.h"
#include "../drivers/usb.h"
#include "../drivers/sound.h"
#include "../drivers/mouse.h"
#include "../drivers/e1000.h"
#include "../drivers/virtio_net.h"
#include "../drivers/ne2000.h"
#include "../drivers/usbnet.h"
#include "../drivers/wifi_extra.h"
#include "../fs/partition.h"
#include "../fs/sfs_disk.h"
#include "../auth/auth.h"
#include "../shell/shell.h"
#include "../shell/make.h"
#include "../shell/yaoigui.h"
extern void shell_init(void);
extern void __attribute__((noreturn)) yaoigui_run(void);
#include "../net/dns.h"
#include "vbe.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;   /* physical address of vbe_mode_info_t */
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

static volatile uint32_t ticks = 0;
static void timer_handler(registers_t *regs) { 
    ticks++; 
    schedule(regs);
}
uint32_t get_ticks(void) { return ticks; }
void sleep_ms(uint32_t ms) {
    uint32_t start = ticks;
    /* Use elapsed time comparison to handle ticks counter wraparound
       at ~49 days of uptime (1000 Hz, 32-bit counter).               */
    while ((uint32_t)(ticks - start) < ms)
        __asm__ volatile("sti; hlt");
}
static void timer_init(uint32_t freq) {
    register_interrupt_handler(32, timer_handler);
    uint32_t divisor = 1193180 / freq;
    uint8_t lo = (uint8_t)(divisor & 0xFF);
    uint8_t hi = (uint8_t)((divisor >> 8) & 0xFF);
    __asm__ volatile("outb %0, $0x43" : : "a"((uint8_t)0x36));
    __asm__ volatile("outb %0, $0x40" : : "a"(lo));
    __asm__ volatile("outb %0, $0x40" : : "a"(hi));
}

/* ---- Boot mode selection ----------------------------------------
   Shown after login. Returns 1 for GUI, 0 for CLI.
   Times out to GUI after 5 seconds if framebuffer is available,
   or to CLI if VBE is not active.                                 */
static int boot_mode_prompt(void) {
    if (!vbe_active()) {
        /* No framebuffer — CLI only */
        terminal_writestring("[scorpion] No VBE framebuffer — starting CLI\n");
        return 0;
    }

    terminal_writestring("\n");
    terminal_writestring("  +------------------------------------------+\n");
    terminal_writestring("  |   ScorpionOS v4.0  --  Boot Mode          |\n");
    terminal_writestring("  |                                            |\n");
    terminal_writestring("  |   Start GUI compositor?  [Y/n]            |\n");
    terminal_writestring("  |   (defaults to GUI after 5s timeout)      |\n");
    terminal_writestring("  +------------------------------------------+\n");
    terminal_writestring("\n  > ");

    /* keyboard_getchar_timeout blocks up to 5000 ms then returns 0 */
    char k = keyboard_getchar_timeout(5000);

    if (k == 'n' || k == 'N') {
        terminal_writestring("n — starting CLI\n");
        return 0;
    }
    /* 'y', 'Y', Enter, or timeout (k==0) all start GUI */
    if (k == 0)
        terminal_writestring("(timeout) — starting GUI\n");
    else
        terminal_writestring("y — starting GUI\n");
    return 1;
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    (void)magic;
    terminal_init();

    /* Try VBE framebuffer first (multiboot flags bit 11 = vbe_mode_info present) */
    if (mbi && (mbi->flags & (1 << 11)) && mbi->vbe_mode_info) {
        if (vbe_init(mbi->vbe_mode_info) == 0) {
            vbe_terminal_init();
        }
    }

    if (vbe_active()) {
        vbe_terminal_setcolor(0x0A); /* light green on black */
        vbe_terminal_writestring("[scorpion] VBE framebuffer active\n");
        vbe_terminal_setcolor(0x07); /* light grey on black */
    } else {
        terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("[scorpion] kernel v4.0 starting...\n");
        terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }

    terminal_printf("[scorpion] GDT/IDT...\n");
    gdt_init(); idt_init();
    __asm__ volatile("sti");
    terminal_printf("[scorpion] interrupts enabled\n");

    terminal_printf("[scorpion] PIT @ 1000Hz...\n");
    timer_init(1000);

    if (mbi && (mbi->flags & 1))
        terminal_printf("[scorpion] RAM: %dKB lower, %dKB upper (%dMB)\n",
                        mbi->mem_lower, mbi->mem_upper, mbi->mem_upper/1024);

    terminal_printf("[scorpion] ScorpionFS...\n");
    sfs_init();

    /* Embed /etc/installer.mbf into in-memory SFS */
    {
        #include "../etc/installer_mbf.h"
        sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
        if(!etc) etc = sfs_mkdir(sfs_root(), "etc");
        if(etc) sfs_write_file(etc, "installer.mbf",
                    installer_mbf, installer_mbf_len);
    }

    /* v4.0 new subsystems */
    terminal_printf("[scorpion] ACPI...\n");
    if (acpi_init() == 0) { acpi_enable(); terminal_printf("[scorpion] ACPI ready\n"); }
    else terminal_printf("[scorpion] ACPI unavailable (QEMU fallback)\n");

    terminal_printf("[scorpion] PCI scan...\n");
    pci_scan();

    terminal_printf("[scorpion] SMP/APIC...\n");
    lapic_init();
    int ncpus = smp_detect();
    terminal_printf("[scorpion] %d CPU(s)\n", ncpus);
    for (int i = 1; i < ncpus && i < SMP_MAX_CPUS; i++) {
        terminal_printf("[smp] waking AP %d...\n", i);
        if (smp_boot_ap(g_cpus[i].apic_id, 8) == 0)
            terminal_printf("[smp] AP %d online\n", i);
        else
            terminal_printf("[smp] AP %d failed\n", i);
    }

    terminal_printf("[scorpion] PC speaker ready\n");
    sound_play_startup();

    for (int i = 0; i < NSIG; i++) g_sigstate.handlers[i] = SIG_DFL;
    terminal_printf("[scorpion] signals/IPC ready\n");

    dynlink_init();
    kapi_init();
    sched_init();

    /* Original subsystems */
    terminal_printf("[scorpion] keyboard...\n");
    keyboard_init();

    terminal_printf("[scorpion] ATA scan...\n");
    ata_init();
    ahci_init();

    /* Auto-mount SFS partition if one exists */
    {
        static part_entry_t _parts[16];
        int _nparts = partition_detect_all(_parts, 16);
        int _drive; uint32_t _lba;
        int _pi = sfs_find_sync_target(_parts, _nparts, &_drive, &_lba);
        if (_pi >= 0) {
            terminal_printf("[scorpion] SFS partition found, mounting...\n");
            if (sfs_load_from_disk(_drive, _lba) == 0) {
                sfs_set_persist_target(_drive, _lba);
                terminal_printf("[scorpion] SFS mounted and auto-persist enabled\n");
            } else {
                terminal_printf("[scorpion] SFS mount failed, starting fresh\n");
            }
        } else {
            terminal_printf("[scorpion] no SFS partition found, filesystem is RAM-only\n");
        }
    }

    terminal_printf("[scorpion] USB...\n");
    if (usb_init() == 0) usb_scan_devices();

    terminal_printf("[scorpion] mouse...\n");
    mouse_init();

    terminal_printf("[scorpion] WiFi...\n");
    wifi_rtl8188_probe(); wifi_rtl8192_probe();
    wifi_ath9k_probe();   wifi_mt7601_probe();
    wifi_iwlwifi_probe(); wifi_rt2800_probe(); wifi_brcmfmac_probe();
    if (!wifi_active)
        terminal_printf("[scorpion] no WiFi adapter\n");

    /* Ethernet: try each driver in priority order, first win */
    terminal_printf("[scorpion] Ethernet...\n");
    int net_up = 0;
    { uint8_t _mac[6];
    if (!net_up && rndis_init() == 0) {
        rndis_get_mac(_mac);
        rndis_set_receive_cb(net_receive_frame);
        net_set_poll_fn(rndis_poll);
        net_set_send_fn(rndis_send);
        net_init(); net_set_mac(_mac);
        terminal_printf("[scorpion] RTL8139 ready\n"); net_up = 1;
    }
    if (!net_up && e1000_init() == 0) {
        e1000_get_mac(_mac);
        e1000_set_rx_cb(net_receive_frame);
        net_set_poll_fn(e1000_poll);
        net_set_send_fn(e1000_send);
        net_init(); net_set_mac(_mac);
        terminal_printf("[scorpion] e1000 ready\n"); net_up = 1;
    }
    if (!net_up && virtio_net_init() == 0) {
        virtio_net_get_mac(_mac);
        virtio_net_set_rx_cb(net_receive_frame);
        net_set_poll_fn(virtio_net_poll);
        net_set_send_fn(virtio_net_send);
        net_init(); net_set_mac(_mac);
        terminal_printf("[scorpion] virtio-net ready\n"); net_up = 1;
    }
    if (!net_up && ne2000_init() == 0) {
        ne2000_get_mac(_mac);
        ne2000_set_rx_cb(net_receive_frame);
        net_set_poll_fn(ne2000_poll);
        net_set_send_fn(ne2000_send);
        net_init(); net_set_mac(_mac);
        terminal_printf("[scorpion] ne2000 ready\n"); net_up = 1;
    }
    if (!net_up && usbnet_probe() == 0) {
        usbnet_get_mac(_mac);
        usbnet_set_rx_cb(net_receive_frame);
        net_set_poll_fn(usbnet_poll);
        net_set_send_fn(usbnet_send);
        net_init(); net_set_mac(_mac);
        terminal_printf("[scorpion] USB-NET ready\n"); net_up = 1;
    }
    if (!net_up) {
        net_init();   /* still init stack even with no adapter */
        terminal_printf("[scorpion] WARNING: no network adapter found\n");
    }
    } /* _mac scope */
    /* DHCP skipped at boot to avoid hang — run 'dhcp' after login */
    terminal_printf("[net] skipping DHCP at boot. run 'dhcp' after login to configure.\n");
    dns_init();

    make_init();

    terminal_printf("[scorpion] v4.0 init complete\n\n");
    auth_init_etc();
    login_prompt();

    shell_init();

    /* Check if GUI autostart is enabled (default: yes) */
    {
        int start_gui = 1; /* default on */
        sfs_node_t *etc2 = sfs_resolve(sfs_root(), "/etc");
        if (etc2) {
            sfs_node_t *f = sfs_find_child(etc2, "gui_autostart");
            if (f && f->data && f->size > 0)
                start_gui = ((char*)f->data)[0] != '0';
        }
        if (start_gui) yaoigui_run(); /* noreturn */
    }
    shell_run();

    kernel_panic("shell returned unexpectedly", NULL);
    __asm__ volatile("cli");
    for (;;) __asm__ volatile("hlt");
}