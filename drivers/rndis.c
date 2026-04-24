/* scorpion OS - drivers/rndis.c
   RTL8139 PCI Ethernet driver — rewritten from scratch based on OSDev Wiki.
   https://wiki.osdev.org/RTL8139

   Key facts from OSDev + real-hardware testing:
   - RTL8139 uses I/O port space (BAR0 bit0=1), not MMIO
   - TX buffers MUST be 4-byte aligned or 2 garbage bytes prepend each packet
   - After soft reset, MAC must be re-written to IDR0 registers
   - OWN bit (bit13 of TSD) = 0 means "give descriptor to NIC"
   - After NIC DMA-reads the data, it sets OWN=1
   - TOK (bit15 of TSD) is set when wire transmission completes
   - For polling we check OWN=1 (DMA done) which is sufficient
   - QEMU note: RST bit may read as 1 before reset; ignore and continue
*/

#include "rndis.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

/* ---- I/O helpers ---- */
static inline uint8_t  inb_r(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw_r(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t inl_r(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb_r(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw_r(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void outl_r(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

/* ---- PCI config ---- */
static uint32_t pci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    uint32_t addr=0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC);
    outl_r(0xCF8,addr); return inl_r(0xCFC);
}
static void pci_write(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg,uint32_t val){
    outl_r(0xCF8,0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl_r(0xCFC,val);
}

/* ---- RTL8139 register offsets (from IO base) ---- */
#define RTL_IDR0     0x00   /* MAC address (6 bytes) */
#define RTL_MAR0     0x08   /* Multicast filter */
#define RTL_TSD0     0x10   /* TX Status descriptors 0-3 (4 x 32-bit) */
#define RTL_TSAD0    0x20   /* TX Start Address descriptors 0-3 */
#define RTL_RBSTART  0x30   /* RX Buffer Start Address */
#define RTL_CMD      0x37   /* Command register */
#define RTL_CAPR     0x38   /* Current Address of Packet Read */
#define RTL_CBR      0x3A   /* Current Buffer Address (read-only) */
#define RTL_IMR      0x3C   /* Interrupt Mask Register */
#define RTL_ISR      0x3E   /* Interrupt Status Register */
#define RTL_TCR      0x40   /* TX Configuration Register */
#define RTL_RCR      0x44   /* RX Configuration Register */
#define RTL_MPC      0x4C   /* Missed Packet Counter */
#define RTL_9346CR   0x50   /* 93C46 Command Register */
#define RTL_CONFIG1  0x52   /* Configuration Register 1 */
#define RTL_MSR      0x58   /* Media Status Register */
#define RTL_BMCR     0x62   /* Basic Mode Control Register (MII) */

/* CMD bits */
#define CMD_RST     0x10
#define CMD_RE      0x08
#define CMD_TE      0x04
#define CMD_RXEMPTY 0x01

/* ISR bits */
#define ISR_TOK     0x0004
#define ISR_ROK     0x0001
#define ISR_TER     0x0008
#define ISR_RER     0x0002
#define ISR_RXOVW   0x0010

/* TSD bits */
#define TSD_OWN     (1u<<13)  /* OWN=1: NIC finished DMA, slot free for software */
#define TSD_TOK     (1u<<15)  /* wire TX complete */
#define TSD_TUN     (1u<<14)  /* TX underrun */

/* RX buffer */
#define RX_BUF_SIZE  (8192 + 16 + 1500)  /* 8KB ring + header + wrap guard */
#define TX_BUF_SIZE  1536
#define TX_SLOTS     4

/* Buffers — static in BSS, 4-byte aligned.
   TX buffers MUST be 4-byte aligned per OSDev wiki or 2 garbage bytes
   get prepended to every transmitted packet on real hardware. */
static uint8_t rx_ring[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t tx_bufs[TX_SLOTS][TX_BUF_SIZE] __attribute__((aligned(4)));

static uint16_t io_base  = 0;
static uint8_t  rtl_mac[6];
static int      rtl_ready = 0;
static int      tx_cur    = 0;   /* next TX slot to use (0-3) */
static uint16_t rx_ptr    = 0;   /* our read pointer into rx_ring */

static net_receive_cb_t rx_cb = NULL;

/* Expose for ping etc */
int  net_send_raw_frame(const uint8_t *d, size_t l){ return rndis_send(d,l); }
void net_poll_pub(void){ rndis_poll(); }

/* ---- forward declaration ---- */
void rndis_poll(void);

int rndis_init(void) {
    /* Scan PCI for RTL8139 (vendor 0x10EC, device 0x8139) */
    for (uint8_t bus=0; bus<8; bus++) {
        for (uint8_t dev=0; dev<32; dev++) {
            uint32_t id = pci_read(bus,dev,0,0);
            if ((id & 0xFFFF) != 0x10EC || (id>>16) != 0x8139) continue;

            /* Enable bus mastering + I/O space in command register */
            uint32_t cmd = pci_read(bus,dev,0,0x04);
            pci_write(bus,dev,0,0x04, cmd | 0x07);  /* I/O | Memory | BusMaster */

            /* BAR0 — RTL8139 always uses I/O space (bit 0 = 1) */
            uint32_t bar0 = pci_read(bus,dev,0,0x10);
            if (!(bar0 & 1)) {
                /* Some emulators report as MMIO — try BAR1 for I/O */
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
    /* 1. Power on (CONFIG1 = 0) */
    outb_r(io_base + RTL_CONFIG1, 0x00);

    /* 2. Software reset */
    outb_r(io_base + RTL_CMD, CMD_RST);
    /* Wait for reset to complete — QEMU note: bit may read 1 before reset,
       so wait for it to clear rather than checking before sending */
    for (int i = 0; i < 1000000; i++) {
        if (!(inb_r(io_base + RTL_CMD) & CMD_RST)) break;
    }

    /* 3. Read MAC from IDR0 registers (before any config that might clear it) */
    for (int i = 0; i < 6; i++)
        rtl_mac[i] = inb_r(io_base + RTL_IDR0 + i);

    /* 4. Unlock 9346 EEPROM for writing, re-write MAC to IDR registers.
       On some real hardware the reset clears the MAC — always re-write it.
       Sequence: write 0xC0 to 9346CR to unlock, write MAC, restore 0x00. */
    outb_r(io_base + RTL_9346CR, 0xC0);  /* config write enable */
    outl_r(io_base + RTL_IDR0,
           (uint32_t)rtl_mac[0] | ((uint32_t)rtl_mac[1]<<8) |
           ((uint32_t)rtl_mac[2]<<16) | ((uint32_t)rtl_mac[3]<<24));
    outl_r(io_base + RTL_IDR0 + 4,
           (uint32_t)rtl_mac[4] | ((uint32_t)rtl_mac[5]<<8));
    outb_r(io_base + RTL_9346CR, 0x00);  /* lock config */

    /* 5. Set RX buffer start address */
    outl_r(io_base + RTL_RBSTART, (uint32_t)(uintptr_t)rx_ring);

    /* 6. Enable TX and RX in CMD */
    outb_r(io_base + RTL_CMD, CMD_TE | CMD_RE);

    /* 7. TX Configuration:
       bits 10:8 = DMA burst size (110 = 1024 bytes = max safe)
       bits 5:4  = IFG = 11 (standard inter-frame gap)
       bit  3    = TXRR = 0 (no retry limit) */
    outl_r(io_base + RTL_TCR, 0x00000600 | (6u<<8));

    /* 8. RX Configuration:
       bit 0 (AAP)  = accept all packets (promiscuous)
       bit 1 (APM)  = accept physical match
       bit 2 (AM)   = accept multicast
       bit 3 (AB)   = accept broadcast  ← CRITICAL for DHCP!
       bit 7 (WRAP) = wrap around ring buffer
       bits 12:11   = max DMA burst (110 = 1024 bytes)
       bits 14:13   = RBLEN = 00 (8KB ring) */
    outl_r(io_base + RTL_RCR, 0x0F | (1u<<7) | (6u<<8));

    /* 9. Set IMR: enable ROK + TOK + error bits */
    outw_r(io_base + RTL_IMR, ISR_ROK | ISR_TOK | ISR_RER | ISR_TER | ISR_RXOVW);

    /* 10. Clear any stale ISR bits */
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

    /* Wait for OWN bit = 1 meaning this slot's DMA is done and we can reuse it.
       On first use TSD reads 0 (OWN=0) but that's fine — the chip hasn't been
       given this slot yet so we can write to it freely.
       On reuse (after wrapping), we need OWN=1 before overwriting the buffer. */
    for (int w = 0; w < 200000; w++) {
        uint32_t tsd = inl_r(io_base + RTL_TSD0 + slot*4);
        /* OWN=1: NIC done, or TSD=0 (fresh slot never used): both safe to write */
        if ((tsd & TSD_OWN) || tsd == 0) break;
    }

    /* Copy packet into aligned TX buffer */
    for (size_t i = 0; i < len; i++) tx_bufs[slot][i] = data[i];

    /* Write TX address first, then length (writing length triggers DMA).
       OWN bit = 0 in the length word = give descriptor to NIC. */
    outl_r(io_base + RTL_TSAD0 + slot*4, (uint32_t)(uintptr_t)tx_bufs[slot]);
    outl_r(io_base + RTL_TSD0  + slot*4, (uint32_t)(len & 0x1FFF)); /* OWN=0 */

    return 0;
}

void rndis_poll(void) {
    if (!rtl_ready) return;

    /* Read and clear ISR */
    uint16_t isr = inw_r(io_base + RTL_ISR);
    if (!isr) return;
    outw_r(io_base + RTL_ISR, isr);  /* write-to-clear */

    /* Handle RX overflow: reset the receiver */
    if (isr & ISR_RXOVW) {
        outb_r(io_base + RTL_CMD, 0);           /* stop RX/TX */
        outl_r(io_base + RTL_RBSTART, (uint32_t)(uintptr_t)rx_ring);
        outw_r(io_base + RTL_CAPR, 0);
        rx_ptr = 0;
        outb_r(io_base + RTL_CMD, CMD_TE | CMD_RE);
    }

    /* Drain RX ring: process packets while RxBufEmpty bit is clear */
    while (!(inb_r(io_base + RTL_CMD) & CMD_RXEMPTY)) {
        /* Packet header is 4 bytes before the data in the ring.
           Byte 0-1: status word, byte 2-3: packet length (including 4-byte CRC) */
        uint16_t off  = (uint16_t)(rx_ptr % (8192));
        uint8_t *hdr  = rx_ring + off;
        uint16_t status = *(uint16_t*)(hdr);
        uint16_t pkt_len = *(uint16_t*)(hdr + 2);

        /* Sanity check: incomplete frame magic length 0xFFF0 per OSDev wiki */
        if (pkt_len == 0xFFF0) break;

        /* ROK must be set */
        if (!(status & 0x01)) break;

        /* Valid packet: length includes 4-byte CRC, frame starts after 4-byte header */
        if (pkt_len >= 4 && pkt_len <= 1518) {
            uint16_t frame_len = (uint16_t)(pkt_len - 4);
            if (rx_cb) rx_cb(hdr + 4, frame_len);
        }

        /* Advance read pointer: +4 header, +pkt_len data, round up to dword, wrap */
        rx_ptr = (uint16_t)((rx_ptr + pkt_len + 4 + 3) & ~3);

        /* Update CAPR: hardware reads CAPR+16 as the actual read position */
        outw_r(io_base + RTL_CAPR, (uint16_t)((rx_ptr - 16) & 0xFFFF));
    }
}

void rndis_get_mac(uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) mac[i] = rtl_mac[i];
}
void rndis_set_receive_cb(net_receive_cb_t cb) { rx_cb = cb; }
