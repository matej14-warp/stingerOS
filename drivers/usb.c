/* scorpion OS - drivers/usb.c
   USB host controller detection (UHCI/OHCI/EHCI/xHCI) via PCI.
   We detect and enumerate controllers but don't implement a full
   HCD stack — full USB is handled in usbnet.c for USB networking.
   This file satisfies the kernel init sequence and lsusb.           */

#include "usb.h"
#include "pci.h"
#include "../kernel/terminal.h"
#include <stdint.h>

#define USB_CLASS 0x0C
#define USB_SUBCLASS_USB 0x03

typedef struct {
    uint8_t  bus, dev, fn;
    uint16_t vendor, device;
    uint8_t  prog_if;   /* 0x00=UHCI 0x10=OHCI 0x20=EHCI 0x30=xHCI */
    uint32_t bar0;
} usb_ctrl_t;

#define MAX_USB_CTRLS 8
static usb_ctrl_t g_usb_ctrls[MAX_USB_CTRLS];
static int        g_usb_count = 0;

static const char *usb_type_name(uint8_t prog_if) {
    switch (prog_if) {
    case 0x00: return "UHCI";
    case 0x10: return "OHCI";
    case 0x20: return "EHCI";
    case 0x30: return "xHCI";
    default:   return "USB";
    }
}

int usb_init(void) {
    g_usb_count = 0;
    for (int i = 0; i < g_pci_count && g_usb_count < MAX_USB_CTRLS; i++) {
        pci_device_t *d = &g_pci_devices[i];
        if (d->class_code != USB_CLASS || d->subclass != USB_SUBCLASS_USB) continue;
        usb_ctrl_t *c = &g_usb_ctrls[g_usb_count++];
        c->bus    = d->bus;
        c->dev    = d->dev;
        c->fn     = d->fn;
        c->vendor = d->vendor;
        c->device = d->device;
        c->prog_if= d->prog_if;
        c->bar0   = d->bar[0] & ~0xF;
        pci_enable_busmaster(d->bus, d->dev, d->fn);
        terminal_printf("[usb] %s controller at %d:%d vendor=%04x dev=%04x\n",
            usb_type_name(c->prog_if), c->bus, c->dev, c->vendor, c->device);
    }
    if (!g_usb_count) terminal_printf("[usb] no USB controllers found\n");
    return g_usb_count ? 0 : -1;
}

void usb_scan_devices(void) {
    /* Without a full HCD stack we just report the number of controllers */
    terminal_printf("[usb] %d controller(s) detected\n", g_usb_count);
}

int usb_get_device_count(void) { return g_usb_count; }

void usb_list_devices(void) {
    if (!g_usb_count) { terminal_printf("(no USB controllers)\n"); return; }
    for (int i=0;i<g_usb_count;i++) {
        terminal_printf("  %s  %04x:%04x  bus %d dev %d\n",
            usb_type_name(g_usb_ctrls[i].prog_if),
            g_usb_ctrls[i].vendor, g_usb_ctrls[i].device,
            g_usb_ctrls[i].bus, g_usb_ctrls[i].dev);
    }
}
