
















static int g_intel_gpu_active = 0;

int intel_gpu_init(void) {

    if (vbe_active()) {
        terminal_printf("[intel_gpu] VBE already active, skipping BAR init\n");
        return -1;
    }


    pci_device_t *gpu = NULL;
    for (int i = 0; i < g_pci_count; i++) {
        pci_device_t *d = &g_pci_devices[i];
        if (d->vendor == INTEL_VENDOR &&
            d->class_code == PCI_CLASS_DISPLAY &&
            d->subclass   == PCI_SUBCLASS_VGA) {
            gpu = d;
            break;
        }
    }

    if (!gpu) {
        terminal_printf("[intel_gpu] no Intel display controller found\n");
        return -1;
    }

    terminal_printf("[intel_gpu] found Intel GPU %04x:%04x at %02x:%02x.%x\n",
        gpu->vendor, gpu->device, gpu->bus, gpu->dev, gpu->fn);


    uint32_t bar2 = gpu->bar[2];


    if (bar2 & 1) {
        terminal_printf("[intel_gpu] BAR2 is I/O space — unexpected, aborting\n");
        return -1;
    }

    uint32_t fb_addr = bar2 & ~0xFu;

    if (fb_addr == 0 || fb_addr == 0xFFFFFFF0u) {
        terminal_printf("[intel_gpu] BAR2 not mapped (0x%x) — is PCI mem enabled?\n", bar2);
        return -1;
    }


    pci_enable_busmaster(gpu->bus, gpu->dev, gpu->fn);

    uint32_t w     = IGPU_DEFAULT_W;
    uint32_t h     = IGPU_DEFAULT_H;
    uint32_t pitch = w * (IGPU_DEFAULT_BPP / 8);

    terminal_printf("[intel_gpu] GMADR fb @ 0x%x  %dx%d pitch=%d\n",
        fb_addr, w, h, pitch);

    vbe_init_direct(fb_addr, pitch, w, h);

    if (!vbe_active()) {
        terminal_printf("[intel_gpu] vbe_init_direct failed\n");
        return -1;
    }

    g_intel_gpu_active = 1;
    terminal_printf("[intel_gpu] Intel UHD framebuffer active\n");
    return 0;
}

int intel_gpu_active(void) { return g_intel_gpu_active; }




