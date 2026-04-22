







static inline uint8_t  inb_r(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw_r(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t inl_r(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb_r(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw_r(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void outl_r(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}


static uint32_t pci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    uint32_t addr=0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC);
    outl_r(0xCF8,addr); return inl_r(0xCFC);
}
static void pci_write(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg,uint32_t val){
    outl_r(0xCF8,0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl_r(0xCFC,val);
}












































static uint8_t rx_ring[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t tx_bufs[TX_SLOTS][TX_BUF_SIZE] __attribute__((aligned(4)));

static uint16_t io_base  = 0;
static uint8_t  rtl_mac[6];
static int      rtl_ready = 0;
static int      tx_cur    = 0;
static uint16_t rx_ptr    = 0;

static net_receive_cb_t rx_cb = NULL;


int  net_send_raw_frame(const uint8_t *d, size_t l){ return rndis_send(d,l); }
void net_poll_pub(void){ rndis_poll(); }


void rndis_poll(void);

int rndis_init(void) {

    for (uint8_t bus=0; bus<8; bus++) {
        for (uint8_t dev=0; dev<32; dev++) {
            uint32_t id = pci_read(bus,dev,0,0);
            if ((id & 0xFFFF) != 0x10EC || (id>>16) != 0x8139) continue;


            uint32_t cmd = pci_read(bus,dev,0,0x04);
            pci_write(bus,dev,0,0x04, cmd | 0x07);


            uint32_t bar0 = pci_read(bus,dev,0,0x10);
            if (!(bar0 & 1)) {

                bar0 = pci_read(bus,dev,0,0x14);
                if (!(bar0 & 1)) {
                    terminal_printf("[rtl8139] no I/O BAR found\n");
                    return -1;
                }
            }
            io_base = (uint16_t)(bar0 & ~3);
            terminal_printf("[rtl8139] found bus=%d dev=%d io=0x%x\n", bus, dev, io_base);
            goto found;
        }
    }
    return -1;

found:

    outb_r(io_base + RTL_CONFIG1, 0x00);


    outb_r(io_base + RTL_CMD, CMD_RST);

    for (int i = 0; i < 1000000; i++) {
        if (!(inb_r(io_base + RTL_CMD) & CMD_RST)) break;
    }


    for (int i = 0; i < 6; i++)
        rtl_mac[i] = inb_r(io_base + RTL_IDR0 + i);


    outb_r(io_base + RTL_9346CR, 0xC0);
    outl_r(io_base + RTL_IDR0,
           (uint32_t)rtl_mac[0] | ((uint32_t)rtl_mac[1]<<8) |
           ((uint32_t)rtl_mac[2]<<16) | ((uint32_t)rtl_mac[3]<<24));
    outl_r(io_base + RTL_IDR0 + 4,
           (uint32_t)rtl_mac[4] | ((uint32_t)rtl_mac[5]<<8));
    outb_r(io_base + RTL_9346CR, 0x00);


    outl_r(io_base + RTL_RBSTART, (uint32_t)(uintptr_t)rx_ring);


    outb_r(io_base + RTL_CMD, CMD_TE | CMD_RE);


    outl_r(io_base + RTL_TCR, 0x00000600 | (6u<<8));


    outl_r(io_base + RTL_RCR, 0x0F | (1u<<7) | (6u<<8));


    outw_r(io_base + RTL_IMR, ISR_ROK | ISR_TOK | ISR_RER | ISR_TER | ISR_RXOVW);


    outw_r(io_base + RTL_ISR, 0xFFFF);

    rx_ptr = 0;
    tx_cur = 0;
    rtl_ready = 1;

    terminal_printf("[rtl8139] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        rtl_mac[0],rtl_mac[1],rtl_mac[2],
        rtl_mac[3],rtl_mac[4],rtl_mac[5]);
    return 0;
}

int rndis_send(const uint8_t *data, size_t len) {
    if (!rtl_ready || len == 0 || len > 1514) return -1;

    int slot = tx_cur;
    tx_cur = (tx_cur + 1) % TX_SLOTS;


    for (int w = 0; w < 200000; w++) {
        uint32_t tsd = inl_r(io_base + RTL_TSD0 + slot*4);

        if ((tsd & TSD_OWN) || tsd == 0) break;
    }


    for (size_t i = 0; i < len; i++) tx_bufs[slot][i] = data[i];


    outl_r(io_base + RTL_TSAD0 + slot*4, (uint32_t)(uintptr_t)tx_bufs[slot]);
    outl_r(io_base + RTL_TSD0  + slot*4, (uint32_t)(len & 0x1FFF));

    return 0;
}

void rndis_poll(void) {
    if (!rtl_ready) return;


    uint16_t isr = inw_r(io_base + RTL_ISR);
    if (!isr) return;
    outw_r(io_base + RTL_ISR, isr);


    if (isr & ISR_RXOVW) {
        outb_r(io_base + RTL_CMD, 0);
        outl_r(io_base + RTL_RBSTART, (uint32_t)(uintptr_t)rx_ring);
        outw_r(io_base + RTL_CAPR, 0);
        rx_ptr = 0;
        outb_r(io_base + RTL_CMD, CMD_TE | CMD_RE);
    }


    while (!(inb_r(io_base + RTL_CMD) & CMD_RXEMPTY)) {

        uint16_t off  = (uint16_t)(rx_ptr % (8192));
        uint8_t *hdr  = rx_ring + off;
        uint16_t status = *(uint16_t*)(hdr);
        uint16_t pkt_len = *(uint16_t*)(hdr + 2);


        if (pkt_len == 0xFFF0) break;


        if (!(status & 0x01)) break;


        if (pkt_len >= 4 && pkt_len <= 1518) {
            uint16_t frame_len = (uint16_t)(pkt_len - 4);
            if (rx_cb) rx_cb(hdr + 4, frame_len);
        }


        rx_ptr = (uint16_t)((rx_ptr + pkt_len + 4 + 3) & ~3);


        outw_r(io_base + RTL_CAPR, (uint16_t)((rx_ptr - 16) & 0xFFFF));
    }
}

void rndis_get_mac(uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) mac[i] = rtl_mac[i];
}
void rndis_set_receive_cb(net_receive_cb_t cb) { rx_cb = cb; }




