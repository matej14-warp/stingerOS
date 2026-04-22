









static uint8_t  *g_cap_regs = NULL;
static uint8_t  *g_op_regs  = NULL;
static int       g_xhci_ready = 0;

static inline uint32_t xhci_read32(uint8_t *base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static inline void xhci_write32(uint8_t *base, uint32_t offset, uint32_t val) {
    *(volatile uint32_t *)(base + offset) = val;
}

static void xhci_delay(int ms) {
    extern void sleep_ms(uint32_t);
    sleep_ms((uint32_t)ms);
}

int xhci_init(void) {
    pci_device_t *dev = pci_find_class(PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB);

    if (!dev || dev->prog_if != PCI_PROGIF_XHCI) {

        dev = NULL;
        for (int i = 0; i < g_pci_count; i++) {
            if (g_pci_devices[i].class_code == PCI_CLASS_SERIAL_BUS &&
                g_pci_devices[i].subclass == PCI_SUBCLASS_USB &&
                g_pci_devices[i].prog_if == PCI_PROGIF_XHCI) {
                dev = &g_pci_devices[i];
                break;
            }
        }
    }

    if (!dev) {
        terminal_printf("[xhci] no xHCI controller found\n");
        return -1;
    }

    terminal_printf("[xhci] initializing %04x:%04x at %d:%d\n", dev->vendor, dev->device, dev->bus, dev->dev);


    uint32_t bar0 = dev->bar[0] & ~0xF;
    if (paging_map_mmio(bar0, 65536, (uint8_t**)&g_cap_regs) != 0) {
        terminal_printf("[xhci] MMIO mapping failed\n");
        return -1;
    }

    uint8_t cap_len = (uint8_t)xhci_read32(g_cap_regs, XHCI_CAP_CAPLENGTH);
    g_op_regs = g_cap_regs + cap_len;


    xhci_write32(g_op_regs, XHCI_OP_USBCMD, xhci_read32(g_op_regs, XHCI_OP_USBCMD) & ~CMD_RS);
    while (!(xhci_read32(g_op_regs, XHCI_OP_USBSTS) & STS_HCH));

    xhci_write32(g_op_regs, XHCI_OP_USBCMD, CMD_HCRST);
    int timeout = 1000;
    while ((xhci_read32(g_op_regs, XHCI_OP_USBCMD) & CMD_HCRST) && timeout--) xhci_delay(1);

    if (timeout <= 0) {
        terminal_printf("[xhci] controller reset timeout\n");
        return -1;
    }

    terminal_printf("[xhci] controller ready\n");
    g_xhci_ready = 1;
    return 0;
}

void xhci_poll(void) {
    if (!g_xhci_ready) return;

}




