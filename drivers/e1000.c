/* scorpion OS - drivers/e1000.c
   Intel e1000 Ethernet driver.
   PCI vendor 0x8086, devices: 0x100E (82540EM, QEMU default), 0x100F, 0x10D3.
   MMIO-based. Uses a 16-slot RX descriptor ring and 8-slot TX ring.           */

#include "e1000.h"
#include "pci.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

static inline void outl_e(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline uint32_t inl_e(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}

/* e1000 MMIO register offsets */
#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008
#define E1000_EECD    0x0010
#define E1000_EERD    0x0014
#define E1000_ICR     0x00C0
#define E1000_IMS     0x00D0
#define E1000_IMC     0x00D8
#define E1000_RCTL    0x0100
#define E1000_TCTL    0x0400
#define E1000_TIPG    0x0410
#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818
#define E1000_TDBAL   0x3800
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_RAL     0x5400
#define E1000_RAH     0x5404
#define E1000_MTA     0x5200

#define E1000_CTRL_RST  (1u<<26)
#define E1000_CTRL_SLU  (1u<<6)
#define E1000_CTRL_ASDE (1u<<5)
#define E1000_RCTL_EN   (1u<<1)
#define E1000_RCTL_SBP  (1u<<2)
#define E1000_RCTL_UPE  (1u<<3)
#define E1000_RCTL_MPE  (1u<<4)
#define E1000_RCTL_BAM  (1u<<15)
#define E1000_RCTL_BSIZE_2048 0
#define E1000_RCTL_SECRC (1u<<26)
#define E1000_TCTL_EN   (1u<<1)
#define E1000_TCTL_PSP  (1u<<3)
#define E1000_TCTL_CT   (0x10u<<4)
#define E1000_TCTL_COLD (0x40u<<12)

/* Legacy RX descriptor */
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

/* Legacy TX descriptor */
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

#define RX_DESC_COUNT 16
#define TX_DESC_COUNT 8
#define RX_BUF_SIZE   2048

static volatile uint32_t *e1000_mmio = NULL;
static uint8_t e1000_mac[6];
static int     e1000_ready = 0;
static net_receive_cb_t e1000_rx_cb = NULL;

static e1000_rx_desc_t rx_descs[RX_DESC_COUNT] __attribute__((aligned(16)));
static e1000_tx_desc_t tx_descs[TX_DESC_COUNT] __attribute__((aligned(16)));
static uint8_t rx_bufs[RX_DESC_COUNT][RX_BUF_SIZE];
static uint8_t tx_buf[TX_DESC_COUNT][RX_BUF_SIZE];

static uint32_t rx_tail = 0, tx_tail = 0;

static inline uint32_t e_read(uint32_t reg) {
    return e1000_mmio[reg/4];
}
static inline void e_write(uint32_t reg, uint32_t val) {
    e1000_mmio[reg/4] = val;
}

static void e1000_read_mac_eeprom(void) {
    /* Try EEPROM read via EERD.
       Bit 4 = done on 82540EM, bit 1 = done on some other variants.
       Try both. */
    int eeprom_ok = 0;
    for (int i = 0; i < 3; i++) {
        e_write(E1000_EERD, (uint32_t)(1 | (i << 8)));
        uint32_t v = 0;
        for (int t = 0; t < 100000; t++) {
            v = e_read(E1000_EERD);
            if (v & (1<<4)) { eeprom_ok = 1; break; } /* 82540EM done bit */
            if (v & (1<<1)) { eeprom_ok = 1; break; } /* other variant done bit */
        }
        uint16_t w = (uint16_t)(v >> 16);
        e1000_mac[i*2]   = (uint8_t)(w & 0xFF);
        e1000_mac[i*2+1] = (uint8_t)(w >> 8);
    }
    /* Fallback: read MAC from RAL/RAH if EEPROM failed or gave zeros */
    if (!eeprom_ok || (e1000_mac[0]==0 && e1000_mac[1]==0 && e1000_mac[2]==0)) {
        uint32_t ral = e_read(E1000_RAL);
        uint32_t rah = e_read(E1000_RAH);
        e1000_mac[0] = (uint8_t)(ral & 0xFF);
        e1000_mac[1] = (uint8_t)(ral >> 8);
        e1000_mac[2] = (uint8_t)(ral >> 16);
        e1000_mac[3] = (uint8_t)(ral >> 24);
        e1000_mac[4] = (uint8_t)(rah & 0xFF);
        e1000_mac[5] = (uint8_t)((rah >> 8) & 0xFF);
    }
}

int e1000_init(void) {
    /* Find e1000 on PCI */
    static const uint16_t devices[] = {0x100E,0x100F,0x10D3,0x1533,0x153A,0x10EA,0x1502,0x107C,0};
    pci_device_t *dev = NULL;
    for (int di=0; devices[di] && !dev; di++)
        dev = pci_find(0x8086, devices[di]);
    if (!dev) return -1;

    pci_enable_busmaster(dev->bus, dev->dev, dev->fn);
    /* BAR0: bit0=0 means MMIO, bit0=1 means I/O port.
       e1000 always uses MMIO (BAR0 is a memory BAR). Mask off lower 4 bits. */
    uint32_t bar0 = dev->bar[0];
    if (bar0 & 1) {
        terminal_printf("[e1000] BAR0 is I/O space, not supported\n");
        return -1;
    }
    bar0 &= ~0xF;
    if (!bar0) return -1;
    e1000_mmio = (volatile uint32_t *)(uintptr_t)bar0;

    terminal_printf("[e1000] found at %d:%d mmio=%p\n", dev->bus, dev->dev, e1000_mmio);

    /* Reset */
    e_write(E1000_CTRL, e_read(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i=0;i<100000;i++);
    e_write(E1000_CTRL, e_read(E1000_CTRL) | E1000_CTRL_SLU | E1000_CTRL_ASDE);

    /* Read MAC */
    e1000_read_mac_eeprom();
    /* Also write to RAL/RAH */
    uint32_t ral = (uint32_t)e1000_mac[0] | ((uint32_t)e1000_mac[1]<<8) |
                   ((uint32_t)e1000_mac[2]<<16) | ((uint32_t)e1000_mac[3]<<24);
    uint32_t rah = (uint32_t)e1000_mac[4] | ((uint32_t)e1000_mac[5]<<8) | (1u<<31);
    e_write(E1000_RAL, ral);
    e_write(E1000_RAH, rah);

    /* Clear MTA */
    for (int i=0;i<128;i++) e_write(E1000_MTA + i*4, 0);

    /* RX setup */
    for (int i=0;i<RX_DESC_COUNT;i++) {
        rx_descs[i].addr   = (uint64_t)(uintptr_t)rx_bufs[i];
        rx_descs[i].status = 0;
    }
    e_write(E1000_RDBAL, (uint32_t)(uintptr_t)rx_descs);
    e_write(E1000_RDBAH, 0);
    e_write(E1000_RDLEN, RX_DESC_COUNT * 16);
    e_write(E1000_RDH, 0);
    e_write(E1000_RDT, RX_DESC_COUNT - 1);
    e_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);
    rx_tail = 0;

    /* TX setup */
    for (int i=0;i<TX_DESC_COUNT;i++) {
        tx_descs[i].addr   = (uint64_t)(uintptr_t)tx_buf[i];
        tx_descs[i].status = 1;  /* DD - done */
    }
    e_write(E1000_TDBAL, (uint32_t)(uintptr_t)tx_descs);
    e_write(E1000_TDBAH, 0);
    e_write(E1000_TDLEN, TX_DESC_COUNT * 16);
    e_write(E1000_TDH, 0);
    e_write(E1000_TDT, 0);
    e_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
    e_write(E1000_TIPG, 0x00702008);
    tx_tail = 0;

    /* Disable interrupts */
    e_write(E1000_IMC, 0xFFFFFFFF);

    e1000_ready = 1;
    terminal_printf("[e1000] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        e1000_mac[0],e1000_mac[1],e1000_mac[2],
        e1000_mac[3],e1000_mac[4],e1000_mac[5]);
    return 0;
}

int e1000_send(const uint8_t *data, size_t len) {
    if (!e1000_ready || len == 0 || len > RX_BUF_SIZE) return -1;
    uint32_t slot = tx_tail;
    /* Wait for DD bit (bit 0 of status) = descriptor done = slot free.
       Timeout after ~100k reads to avoid infinite hang. */
    for (int i = 0; i < 100000; i++) {
        if (tx_descs[slot].status & 0x01) break;
    }
    /* Copy data and fill descriptor */
    for (size_t i = 0; i < len; i++) tx_buf[slot][i] = data[i];
    tx_descs[slot].addr   = (uint64_t)(uintptr_t)tx_buf[slot];
    tx_descs[slot].length = (uint16_t)len;
    tx_descs[slot].cmd    = 0x0B;  /* EOP | IFCS | RS */
    tx_descs[slot].status = 0;     /* clear DD — chip will set it when done */
    tx_descs[slot].cso    = 0;
    tx_descs[slot].css    = 0;
    tx_descs[slot].special = 0;
    tx_tail = (tx_tail + 1) % TX_DESC_COUNT;
    e_write(E1000_TDT, tx_tail);   /* kick the tail pointer to trigger TX */
    return 0;
}

void e1000_poll(void) {
    if (!e1000_ready) return;
    while (rx_descs[rx_tail].status & 1) {
        uint16_t len = rx_descs[rx_tail].length;
        if (e1000_rx_cb && len > 0)
            e1000_rx_cb(rx_bufs[rx_tail], len);
        rx_descs[rx_tail].status = 0;
        e_write(E1000_RDT, rx_tail);
        rx_tail = (rx_tail + 1) % RX_DESC_COUNT;
    }
}

void e1000_get_mac(uint8_t mac[6]) { for(int i=0;i<6;i++) mac[i]=e1000_mac[i]; }
void e1000_set_rx_cb(net_receive_cb_t cb) { e1000_rx_cb = cb; }
int  e1000_is_ready(void) { return e1000_ready; }
