





pci_device_t g_pci_devices[PCI_MAX_DEVICES];
int          g_pci_count = 0;

static inline void outl_p(uint16_t p, uint32_t v) {
    __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));
}
static inline uint32_t inl_p(uint16_t p) {
    uint32_t v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v;
}

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg) {
    outl_p(0xCF8, 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    return inl_p(0xCFC);
}

void pci_write(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg, uint32_t val) {
    outl_p(0xCF8, 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl_p(0xCFC, val);
}

void pci_write16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg, uint16_t val) {
    outl_p(0xCF8, 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    __asm__ volatile("outw %0,%1"::"a"(val),"Nd"((uint16_t)(0xCFC + (reg & 2))));
}

void pci_enable_busmaster(uint8_t bus, uint8_t dev, uint8_t fn) {
    uint32_t cmd = pci_read(bus, dev, fn, 0x04);
    pci_write16(bus, dev, fn, 0x04, (uint16_t)((cmd | 0x07) & 0xFFFF));
}

static void probe_device(uint8_t bus, uint8_t dev, uint8_t fn) {
    uint32_t id = pci_read(bus, dev, fn, 0x00);
    if ((id & 0xFFFF) == 0xFFFF || id == 0) return;
    if (g_pci_count >= PCI_MAX_DEVICES) return;

    pci_device_t *d = &g_pci_devices[g_pci_count++];
    d->bus    = bus;
    d->dev    = dev;
    d->fn     = fn;
    d->vendor = (uint16_t)(id & 0xFFFF);
    d->device = (uint16_t)(id >> 16);

    uint32_t cls = pci_read(bus, dev, fn, 0x08);
    d->class_code = (uint8_t)(cls >> 24);
    d->subclass   = (uint8_t)(cls >> 16);
    d->prog_if    = (uint8_t)(cls >> 8);

    uint32_t hdr = pci_read(bus, dev, fn, 0x0C);
    d->header_type = (uint8_t)((hdr >> 16) & 0x7F);

    if (d->header_type == 0) {
        for (int i = 0; i < 6; i++)
            d->bar[i] = pci_read(bus, dev, fn, (uint8_t)(0x10 + i*4));
    } else {
        for (int i = 0; i < 6; i++) d->bar[i] = 0;
    }

    uint32_t irq_r = pci_read(bus, dev, fn, 0x3C);
    d->irq = (uint8_t)(irq_r & 0xFF);
}

void pci_scan(void) {
    g_pci_count = 0;
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read((uint8_t)bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0xFFFF) continue;
            uint32_t hdr = pci_read((uint8_t)bus, dev, 0, 0x0C);
            int multifunction = (hdr >> 23) & 1;
            probe_device((uint8_t)bus, dev, 0);
            if (multifunction) {
                for (uint8_t fn = 1; fn < 8; fn++)
                    probe_device((uint8_t)bus, dev, fn);
            }
        }
    }
    terminal_printf("[pci] %d device(s) found\n", g_pci_count);
}

pci_device_t *pci_find(uint16_t vendor, uint16_t device) {
    for (int i = 0; i < g_pci_count; i++)
        if (g_pci_devices[i].vendor == vendor && g_pci_devices[i].device == device)
            return &g_pci_devices[i];
    return NULL;
}

pci_device_t *pci_find_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < g_pci_count; i++)
        if (g_pci_devices[i].class_code == class_code && g_pci_devices[i].subclass == subclass)
            return &g_pci_devices[i];
    return NULL;
}

void pci_list(void) {
    terminal_printf("Bus  Dev  Fn  Vendor  Device  Class  Subclass  IRQ\n");
    for (int i = 0; i < g_pci_count; i++) {
        pci_device_t *d = &g_pci_devices[i];
        terminal_printf("%3d  %3d  %2d  %04x    %04x    %02x     %02x        %d\n",
            d->bus, d->dev, d->fn,
            d->vendor, d->device,
            d->class_code, d->subclass, d->irq);
    }
}




