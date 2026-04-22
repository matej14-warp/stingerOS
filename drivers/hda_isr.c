



extern volatile uint8_t *g_hda_base;

void hda_isr(registers_t *regs) {
    (void)regs;
    if (!g_hda_base) return;


    uint32_t status = *(volatile uint32_t*)(g_hda_base + 0x24);


    uint16_t gcap = *(volatile uint16_t*)(g_hda_base + 0x00);
    uint8_t in_streams = (gcap >> 8) & 0x0F;
    uint32_t osd0_bit = (1u << in_streams);

    if (status & osd0_bit) {

        *(volatile uint32_t*)(g_hda_base + 0x24) = osd0_bit;


        uint32_t osd0_base = 0x80 + (in_streams * 0x20);

        *(volatile uint8_t*)(g_hda_base + osd0_base + 0x03) = 0x1C;
    }

    extern void lapic_eoi(void);
    lapic_eoi();
}


