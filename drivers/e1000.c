












static volatile uint32_t *g_mmio = NULL;

static inline uint32_t e1000_read(uint32_t reg)
{
    uint32_t v = g_mmio[reg >> 2];
    __asm__ volatile("" ::: "memory");
    return v;
}
static inline void e1000_write(uint32_t reg, uint32_t val)
{
    __asm__ volatile("" ::: "memory");
    g_mmio[reg >> 2] = val;
}




































































typedef struct {
    uint64_t addr;
    uint16_t length, checksum;
    uint8_t  status, errors;
    uint16_t special;
} __attribute__((packed)) rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso, cmd, status, css;
    uint16_t special;
} __attribute__((packed)) tx_desc_t;

static rx_desc_t rx_ring[NUM_RX] __attribute__((aligned(16)));
static tx_desc_t tx_ring[NUM_TX] __attribute__((aligned(16)));
static uint8_t   rx_bufs[NUM_RX][RX_BUF];
static uint8_t   tx_scratch[RX_BUF];

static uint32_t rx_head = 0;
static uint32_t tx_tail = 0;

static uint8_t g_mac[6];
static void (*g_rx_cb)(const uint8_t *, size_t) = NULL;
static int   g_ready = 0;


static void e1000_udelay(uint32_t us)
{

    for (volatile uint32_t i = 0; i < us * 500; i++);
}

static void e1000_msdelay(uint32_t ms)
{
    e1000_udelay(ms * 1000);
}

static void e1000_read_mac(void)
{
    uint32_t ral = e1000_read(E1000_RAL0);
    uint32_t rah = e1000_read(E1000_RAH0);

    if (rah & (1u << 31)) {
        g_mac[0] = (ral >>  0) & 0xFF;
        g_mac[1] = (ral >>  8) & 0xFF;
        g_mac[2] = (ral >> 16) & 0xFF;
        g_mac[3] = (ral >> 24) & 0xFF;
        g_mac[4] = (rah >>  0) & 0xFF;
        g_mac[5] = (rah >>  8) & 0xFF;
    } else {
        terminal_printf("[e1000] WARNING: RAH.AV=0, MAC fallback\n");

        g_mac[0]=0x52; g_mac[1]=0x54; g_mac[2]=0x00;
        g_mac[3]=0x12; g_mac[4]=0x34; g_mac[5]=0x56;
    }
}


int e1000_init(void)
{

    static const uint16_t devids[] = {

        0x100E, 0x100F, 0x107C,

        0x10D3, 0x10EA, 0x1502, 0x1503,

        0x1533, 0x1539,

        0x153A, 0x153B,

        0x155A, 0x1559,

        0x15B7, 0x15B8,
        0x15D7, 0x15D8,

        0x15E3, 0x15D6,

        0x15BB, 0x15BC,
        0x15BD, 0x15BE,

        0x0D55, 0x15DF,

        0x15DC, 0x15FB, 0x15FC,
        0x1DC8,

        0x1A1E, 0x1A1F,
        0
    };

    pci_device_t *dev = NULL;
    uint16_t matched_id = 0;
    for (int i = 0; devids[i] && !dev; i++) {
        dev = pci_find(0x8086, devids[i]);
        if (dev) matched_id = devids[i];
    }
    if (!dev) return -1;

    terminal_printf("[e1000] found PCI %04x:%04x bus=%d dev=%d fn=%d\n",
                    dev->vendor, matched_id, dev->bus, dev->dev, dev->fn);


    pci_enable_busmaster(dev->bus, dev->dev, dev->fn);

    uint32_t bar0 = dev->bar[0] & ~0xFu;
    if (!bar0) {
        terminal_printf("[e1000] BAR0 is zero — PCI not configured\n");
        return -1;
    }
    g_mmio = (volatile uint32_t *)bar0;
    terminal_printf("[e1000] MMIO @ 0x%08x\n", bar0);



    e1000_write(E1000_IMC, 0xFFFFFFFF);
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | CTRL_RST);
    e1000_msdelay(10);

    for (int t = 0; t < 1000; t++) {
        if (!(e1000_read(E1000_CTRL) & CTRL_RST)) break;
        e1000_msdelay(1);
    }
    e1000_write(E1000_IMC, 0xFFFFFFFF);
    e1000_write(E1000_ITR, 0);



    {
        uint32_t ext = e1000_read(E1000_CTRL_EXT);
        ext |= CTRL_EXT_DRV_LOAD;
        e1000_write(E1000_CTRL_EXT, ext);
    }


    {
        uint32_t ctrl = e1000_read(E1000_CTRL);
        ctrl |= CTRL_SLU | CTRL_FD;
        ctrl &= ~(CTRL_FRCSPD | CTRL_FRCDPLX);
        e1000_write(E1000_CTRL, ctrl);
    }


    e1000_read_mac();
    terminal_printf("[e1000] MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
        g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);


    for (int i = 0; i < 128; i++) e1000_write(E1000_MTA + i*4, 0);


    for (int i = 0; i < NUM_RX; i++) {
        rx_ring[i].addr   = (uint64_t)(uint32_t)rx_bufs[i];
        rx_ring[i].status = 0;
    }
    e1000_write(E1000_RDBAL, (uint32_t)rx_ring);
    e1000_write(E1000_RDBAH, 0);
    e1000_write(E1000_RDLEN, (uint32_t)(NUM_RX * sizeof(rx_desc_t)));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, NUM_RX - 1);
    e1000_write(E1000_RDTR, 0);
    e1000_write(E1000_RADV, 0);
    e1000_write(E1000_RCTL,
        RCTL_EN | RCTL_BAM | RCTL_UPE | RCTL_MPE | RCTL_SECRC);


    for (int i = 0; i < NUM_TX; i++) {
        tx_ring[i].status = TXD_STAT_DD;
        tx_ring[i].cmd    = 0;
    }
    e1000_write(E1000_TDBAL, (uint32_t)tx_ring);
    e1000_write(E1000_TDBAH, 0);
    e1000_write(E1000_TDLEN, (uint32_t)(NUM_TX * sizeof(tx_desc_t)));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    tx_tail = 0;


    e1000_write(E1000_TIDV, 0);
    e1000_write(E1000_TADV, 0);


    e1000_write(E1000_TIPG, TIPG_802_3);


    e1000_write(E1000_TCTL,
        TCTL_EN  | TCTL_PSP | TCTL_RTLC | TCTL_MULR |
        (0x0F << TCTL_CT_SHIFT)   |
        (0x3F << TCTL_COLD_SHIFT));


    terminal_printf("[e1000] waiting for link...\n");
    for (int t = 0; t < 3000; t++) {
        if (e1000_read(E1000_STATUS) & 0x02) break;
        e1000_msdelay(1);
    }
    uint32_t status = e1000_read(E1000_STATUS);
    terminal_printf("[e1000] STATUS=0x%08x link=%s\n",
                    status, (status & 0x02) ? "UP" : "DOWN");

    g_ready = 1;
    return 0;
}


int e1000_is_ready(void) { return g_ready; }

void e1000_get_mac(uint8_t out[6])
{
    for (int i = 0; i < 6; i++) out[i] = g_mac[i];
}

void e1000_set_rx_cb(void (*cb)(const uint8_t *, size_t))
{
    g_rx_cb = cb;
}

void e1000_poll(void)
{
    while (rx_ring[rx_head].status & RXD_STAT_DD) {
        uint16_t len = rx_ring[rx_head].length;
        if (g_rx_cb && len > 0)
            g_rx_cb(rx_bufs[rx_head], len);
        rx_ring[rx_head].status = 0;
        e1000_write(E1000_RDT, rx_head);
        rx_head = (rx_head + 1) % NUM_RX;
    }
}

int e1000_send(const uint8_t *data, size_t len)
{
    if (!g_ready) return -1;
    if (len > RX_BUF) len = RX_BUF;


    for (size_t i = 0; i < len; i++) tx_scratch[i] = data[i];

    uint32_t slot = tx_tail;


    for (int t = 0; t < 100000; t++) {
        if (tx_ring[slot].status & TXD_STAT_DD) goto slot_free;

        e1000_poll();
    }
    terminal_printf("[e1000] TX slot %u busy (timeout)\n", slot);
    return -1;

slot_free:
    tx_ring[slot].addr   = (uint64_t)(uint32_t)tx_scratch;
    tx_ring[slot].length = (uint16_t)len;
    tx_ring[slot].cso    = 0;
    tx_ring[slot].css    = 0;
    tx_ring[slot].special= 0;
    tx_ring[slot].status = 0;
    tx_ring[slot].cmd    = TXD_CMD_EOP | TXD_CMD_IFCS | TXD_CMD_RS;

    tx_tail = (slot + 1) % NUM_TX;
    e1000_write(E1000_TDT, tx_tail);


    for (int t = 0; t < 500000; t++) {
        if (tx_ring[slot].status & TXD_STAT_DD) return 0;
    }

    terminal_printf("[e1000] TX timeout slot %u (TIPG=0x%08x TCTL=0x%08x STATUS=0x%08x)\n",
                    slot,
                    e1000_read(E1000_TIPG),
                    e1000_read(E1000_TCTL),
                    e1000_read(E1000_STATUS));
    return -1;
}




