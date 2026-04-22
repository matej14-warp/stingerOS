









typedef struct {
    uint8_t  bus, dev, fn;
    uint16_t vendor, device;
    uint8_t  prog_if;
    uint32_t bar0;
} usb_ctrl_t;


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




