/* scorpion OS - drivers/ne2000.c
   NE2000 (ISA/PCI) Ethernet driver.
   PCI: 0x10EC:0x8029  (QEMU: -device ne2k_pci)
   ISA fallback: probes 0x300, 0x280, 0x320, 0x340                    */

#include "ne2000.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

static inline uint8_t  inb_n(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw_n(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t inl_n(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb_n(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw_n(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void outl_n(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

/* PCI config space helpers */
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC
static uint32_t npci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    outl_n(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    return inl_n(PCI_DATA);
}
static void npci_write(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg,uint32_t val){
    outl_n(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl_n(PCI_DATA,val);
}

/* NE2000 register offsets from I/O base */
#define NE_CMD    0x00
#define NE_PSTART 0x01
#define NE_PSTOP  0x02
#define NE_BNRY   0x03
#define NE_TPSR   0x04
#define NE_TBCR0  0x05
#define NE_TBCR1  0x06
#define NE_ISR    0x07
#define NE_RSAR0  0x08
#define NE_RSAR1  0x09
#define NE_RBCR0  0x0A
#define NE_RBCR1  0x0B
#define NE_RCR    0x0C
#define NE_TCR    0x0D
#define NE_DCR    0x0E
#define NE_IMR    0x0F
#define NE_DATA   0x10
#define NE_RESET  0x1F

/* Page 1 registers */
#define NE_PAR0  0x01
#define NE_CURR  0x07
#define NE_MAR0  0x08

#define NE_CMD_PAGE0 0x00
#define NE_CMD_PAGE1 0x40
#define NE_CMD_STOP  0x01
#define NE_CMD_START 0x02
#define NE_CMD_TXP   0x04
#define NE_CMD_RD2   0x20

#define NE_RX_START 0x46
#define NE_RX_STOP  0x80
#define NE_TX_PAGE  0x40

static uint16_t ne_base = 0;
static uint8_t  ne_mac[6];
static int      ne_ready = 0;
static uint8_t  ne_curr = NE_RX_START;
static net_receive_cb_t ne_rx_cb = NULL;

static inline uint8_t ne_in(uint8_t reg)            { return inb_n(ne_base+reg); }
static inline void    ne_out(uint8_t reg, uint8_t v) { outb_n(ne_base+reg, v); }

static void ne_remote_read(uint16_t src, uint8_t *dst, uint16_t len) {
    ne_out(NE_RBCR0, len & 0xFF);
    ne_out(NE_RBCR1, (len >> 8) & 0xFF);
    ne_out(NE_RSAR0, src & 0xFF);
    ne_out(NE_RSAR1, (src >> 8) & 0xFF);
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | 0x08); /* remote read */
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t w = inw_n(ne_base + NE_DATA);
        if (i     < len) dst[i]   = (uint8_t)(w & 0xFF);
        if (i + 1 < len) dst[i+1] = (uint8_t)(w >> 8);
    }
    ne_out(NE_ISR, 0x40); /* clear RDC */
}

int ne2000_init(void) {
    uint16_t io = 0;

    /* PCI scan: 0x10EC:0x8029 */
    for (uint8_t bus = 0; bus < 8 && !io; bus++)
        for (uint8_t dev = 0; dev < 32 && !io; dev++) {
            uint32_t id = npci_read(bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0x10EC && ((id >> 16) & 0xFFFF) == 0x8029) {
                npci_write(bus, dev, 0, 0x04, npci_read(bus, dev, 0, 0x04) | 0x05);
                uint32_t bar = npci_read(bus, dev, 0, 0x10);
                if (bar & 1) io = (uint16_t)(bar & ~3u);
                terminal_printf("[ne2000] PCI at bus%d dev%d io=0x%x\n", bus, dev, io);
            }
        }

    /* ISA fallback */
    if (!io) {
        uint16_t probes[] = {0x300, 0x280, 0x320, 0x340, 0};
        for (int i = 0; probes[i]; i++) {
            uint16_t base = probes[i];
            outb_n(base + NE_RESET, inb_n(base + NE_RESET));
            for (volatile int j = 0; j < 50000; j++);
            if (!(inb_n(base + NE_ISR) & 0x80)) continue;
            outb_n(base + NE_CMD, NE_CMD_PAGE0 | NE_CMD_STOP | NE_CMD_RD2);
            for (volatile int j = 0; j < 5000; j++);
            outb_n(base + 0x0E, 0x49);
            uint8_t dcr_back = inb_n(base + 0x0E);
            if (dcr_back == 0xFF || dcr_back == 0x00) continue;
            io = base;
            terminal_printf("[ne2000] ISA probe at 0x%x\n", io);
            break;
        }
    }

    if (!io) return -1;
    ne_base = io;

    /* Reset and configure */
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_STOP | NE_CMD_RD2);
    for (volatile int i = 0; i < 5000; i++);
    ne_out(NE_ISR, 0xFF);
    ne_out(NE_DCR, 0x49);
    ne_out(NE_RBCR0, 0); ne_out(NE_RBCR1, 0);
    ne_out(NE_RCR, 0x20);
    ne_out(NE_TCR, 0x02);
    ne_out(NE_PSTART, NE_RX_START); ne_out(NE_PSTOP, NE_RX_STOP);
    ne_out(NE_BNRY, NE_RX_START);
    ne_out(NE_TPSR, NE_TX_PAGE);
    ne_out(NE_IMR, 0x00);

    /* Read MAC from PROM via remote DMA */
    uint8_t prom[32];
    ne_out(NE_RBCR0, 32); ne_out(NE_RBCR1, 0);
    ne_out(NE_RSAR0, 0);  ne_out(NE_RSAR1, 0);
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | 0x08);
    for (int i = 0; i < 32; i++) prom[i] = inb_n(ne_base + NE_DATA);
    for (int i = 0; i < 6; i++) ne_mac[i] = prom[i * 2];

    /* Page 1: set PAR + CURR */
    ne_out(NE_CMD, NE_CMD_PAGE1 | NE_CMD_STOP | NE_CMD_RD2);
    for (int i = 0; i < 6; i++) ne_out(NE_PAR0 + i, ne_mac[i]);
    for (int i = 0; i < 8; i++) ne_out(NE_MAR0 + i, 0xFF);
    ne_curr = NE_RX_START + 1;
    ne_out(NE_CURR, ne_curr);

    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | NE_CMD_RD2);
    ne_out(NE_TCR, 0x00);
    ne_out(NE_RCR, 0x04 | 0x08);
    ne_out(NE_IMR, 0x01);

    ne_ready = 1;
    terminal_printf("[ne2000] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        ne_mac[0], ne_mac[1], ne_mac[2], ne_mac[3], ne_mac[4], ne_mac[5]);
    terminal_printf("[ne2000] ready\n");
    return 0;
}

int ne2000_send(const uint8_t *data, size_t len) {
    if (!ne_ready || len > 1514) return -1;
    uint16_t dma_addr = (uint16_t)(NE_TX_PAGE * 256);
    ne_out(NE_RBCR0, len & 0xFF); ne_out(NE_RBCR1, (len >> 8) & 0xFF);
    ne_out(NE_RSAR0, dma_addr & 0xFF); ne_out(NE_RSAR1, (dma_addr >> 8) & 0xFF);
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | 0x10); /* remote write */
    for (size_t i = 0; i < len; i += 2) {
        uint16_t w = (uint16_t)data[i] | (i + 1 < len ? (uint16_t)data[i+1] << 8 : 0);
        outw_n(ne_base + NE_DATA, w);
    }
    ne_out(NE_ISR, 0x40);
    ne_out(NE_TPSR, NE_TX_PAGE);
    ne_out(NE_TBCR0, len & 0xFF); ne_out(NE_TBCR1, (len >> 8) & 0xFF);
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | NE_CMD_TXP | NE_CMD_RD2);
    for (int i = 0; i < 10000; i++) { if (ne_in(NE_ISR) & 0x02) break; }
    ne_out(NE_ISR, 0x0A);
    return 0;
}

void ne2000_poll(void) {
    if (!ne_ready) return;
    ne_out(NE_CMD, NE_CMD_PAGE1 | NE_CMD_START | NE_CMD_RD2);
    uint8_t curr = ne_in(NE_CURR);
    ne_out(NE_CMD, NE_CMD_PAGE0 | NE_CMD_START | NE_CMD_RD2);
    uint8_t bnry = ne_in(NE_BNRY) + 1;
    if (bnry >= NE_RX_STOP) bnry = NE_RX_START;
    while (bnry != curr) {
        uint16_t addr = (uint16_t)(bnry * 256);
        uint8_t hdr[4];
        ne_remote_read(addr, hdr, 4);
        uint16_t pktlen = (uint16_t)(hdr[2] | (hdr[3] << 8));
        if (pktlen > 4 && pktlen <= 1520 && ne_rx_cb) {
            uint8_t buf[1520];
            ne_remote_read(addr + 4, buf, pktlen - 4);
            ne_rx_cb(buf, pktlen - 4);
        }
        bnry++;
        if (bnry >= NE_RX_STOP) bnry = NE_RX_START;
        ne_out(NE_BNRY, bnry > NE_RX_START ? bnry - 1 : NE_RX_STOP - 1);
    }
    (void)ne_curr;
}

void ne2000_get_mac(uint8_t mac[6]) { for (int i = 0; i < 6; i++) mac[i] = ne_mac[i]; }
void ne2000_set_rx_cb(net_receive_cb_t cb) { ne_rx_cb = cb; }
int  ne2000_is_ready(void) { return ne_ready; }
