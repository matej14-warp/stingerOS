/* scorpion OS - drivers/rndis.c  (RTL8139 PCI NIC backend)
   Replaces the USB/RNDIS stub. Targets QEMU -device rtl8139.

   RTL8139 memory map (I/O BAR0):
     0x00  MAC[0..5]
     0x08  MAR (multicast)
     0x10  TxStatus0-3   (one per TX descriptor)
     0x20  TxAddr0-3     (physical address of TX buffer)
     0x30  RxBufAddr
     0x34  RxBufHead     (early rx byte count / head)
     0x38  RxBufTail     (tail = CAPR)
     0x3C  IntrStatus
     0x3E  IntrMask
     0x40  TxConfig
     0x44  RxConfig
     0x50  TimerCount
     0x52  MissedPktCtr
     0x60  AutoNegAd
     0x62  AutoNegLp
     0x64  AutoNegExp
     0x37  Cmd                                                   */

#include "rndis.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* ---- Port I/O ---- */
static inline uint8_t  inb(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t inl(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void outl(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

/* ---- Helpers ---- */
static void r_memset(void *d,uint8_t v,size_t n){uint8_t*p=d;while(n--)*p++=v;}
static void r_memcpy(void *d,const void*s,size_t n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}
static void r_spin(uint32_t n){for(volatile uint32_t i=0;i<n;i++);}

/* ---- PCI ---- */
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

static uint32_t pci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    outl(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    return inl(PCI_DATA);
}
static void pci_write(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg,uint32_t val){
    outl(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl(PCI_DATA,val);
}

/* ---- RTL8139 register offsets ---- */
#define RTL_MAC       0x00
#define RTL_MAR       0x08
#define RTL_TXSTATUS0 0x10   /* +4 per descriptor (0-3) */
#define RTL_TXADDR0   0x20   /* +4 per descriptor (0-3) */
#define RTL_RXBUF     0x30
#define RTL_RXEARLYCNT 0x34
#define RTL_RXEARLYSTS 0x36
#define RTL_CMD       0x37
#define RTL_CAPR      0x38   /* Current Address of Packet Read */
#define RTL_CBR       0x3A   /* Current Buffer Address (RX head, read-only) */
#define RTL_INTRMASK  0x3C
#define RTL_INTRSTATUS 0x3E
#define RTL_TXCONFIG  0x40
#define RTL_RXCONFIG  0x44
#define RTL_TIMER     0x48
#define RTL_RXMISSED  0x4C
#define RTL_CFG9346   0x50
#define RTL_CONFIG0   0x51
#define RTL_CONFIG1   0x52
#define RTL_MSR       0x58   /* Media Status */
#define RTL_MII       0x5A

/* CMD bits */
#define CMD_RESET     0x10
#define CMD_RXENABLE  0x08
#define CMD_TXENABLE  0x04
#define CMD_BUFRESET  0x01   /* not used */

/* TxStatus bits */
#define TX_OWN        0x00002000  /* DMA in progress (0=owned by NIC) */
#define TX_OK         0x00008000
#define TX_UNDERRUN   0x00004000

/* RxStatus (in ring) */
#define RX_OK         0x0001
#define RX_FRAME_ALIGN 0x0002
#define RX_RUNT       0x0010
#define RX_LONG       0x0020
#define RX_CRCERR     0x0004

/* Interrupt bits */
#define INT_RXOK      0x0001
#define INT_RXERR     0x0002
#define INT_TXOK      0x0004
#define INT_TXERR     0x0008
#define INT_RXOVF     0x0010

/* RxConfig */
#define RX_ACCEPT_PHYS  0x01   /* Accept physical match */
#define RX_ACCEPT_MULTI 0x04   /* Accept multicast */
#define RX_ACCEPT_BCAST 0x08   /* Accept broadcast */
#define RX_WRAP         0x80   /* Ring wrap */
#define RX_BUF_64K      0x03   /* 64K+16 ring */
#define RX_BUF_32K      0x02
#define RX_BUF_16K      0x01
#define RX_BUF_8K       0x00

/* ---- RX ring: 32K + 16 bytes overflow ---- */
#define RX_BUF_SIZE  (32768 + 16)
#define TX_BUF_SIZE  1536
#define TX_DESC_COUNT 4

/* Statically allocated buffers (must be in low physical memory, virt==phys) */
static uint8_t  rx_ring[RX_BUF_SIZE]  __attribute__((aligned(4)));
static uint8_t  tx_buf[TX_DESC_COUNT][TX_BUF_SIZE] __attribute__((aligned(4)));

/* ---- Driver state ---- */
static uint16_t rtl_base   = 0;
static uint8_t  rtl_mac[6] = {0};
static int      rtl_ready  = 0;
static int      tx_desc    = 0;      /* next TX descriptor to use (0-3) */
static uint16_t rx_pos     = 0;      /* byte offset into rx_ring */
static net_receive_cb_t recv_cb = NULL;

static inline uint32_t phys(const void *p){return (uint32_t)(uintptr_t)p;}

/* ---- RTL8139 init ---- */
static int rtl8139_init(void){
    /* Power on */
    outb(rtl_base + RTL_CONFIG1, 0x00);
    r_spin(100000);

    /* Software reset */
    outb(rtl_base + RTL_CMD, CMD_RESET);
    uint32_t t = 200000;
    while((inb(rtl_base + RTL_CMD) & CMD_RESET) && t--) r_spin(10);
    if(inb(rtl_base + RTL_CMD) & CMD_RESET){
        terminal_printf("[rtl8139] reset timeout\n");
        return -1;
    }

    /* Read MAC from NIC registers */
    for(int i=0;i<6;i++) rtl_mac[i] = inb(rtl_base + RTL_MAC + i);
    terminal_printf("[rtl8139] MAC: %x:%x:%x:%x:%x:%x\n",
        rtl_mac[0],rtl_mac[1],rtl_mac[2],
        rtl_mac[3],rtl_mac[4],rtl_mac[5]);

    /* Unlock config registers */
    outb(rtl_base + RTL_CFG9346, 0xC0);

    /* Set RX buffer physical address */
    r_memset(rx_ring, 0, RX_BUF_SIZE);
    outl(rtl_base + RTL_RXBUF, phys(rx_ring));

    /* Set TX buffer physical addresses */
    for(int i=0;i<TX_DESC_COUNT;i++)
        outl(rtl_base + RTL_TXADDR0 + i*4, phys(tx_buf[i]));

    /* RxConfig: accept physical + broadcast + multicast, 32K ring, no wrap */
    outl(rtl_base + RTL_RXCONFIG,
         RX_ACCEPT_PHYS | RX_ACCEPT_BCAST | RX_ACCEPT_MULTI |
         (RX_BUF_32K << 11) | RX_WRAP |
         (7 << 8));   /* max DMA burst = unlimited */

    /* TxConfig: default interframe gap, max DMA burst */
    outl(rtl_base + RTL_TXCONFIG, 0x03000700);

    /* Disable all interrupts (we poll) */
    outw(rtl_base + RTL_INTRMASK, 0x0000);
    outw(rtl_base + RTL_INTRSTATUS, 0xFFFF);

    /* Lock config registers */
    outb(rtl_base + RTL_CFG9346, 0x00);

    /* Enable RX + TX */
    outb(rtl_base + RTL_CMD, CMD_RXENABLE | CMD_TXENABLE);

    rx_pos  = 0;
    tx_desc = 0;
    return 0;
}

/* ---- Public API ---- */

int rndis_init(void){
    terminal_printf("[rtl8139] scanning PCI for RTL8139 (10EC:8139)...\n");

    int found = 0;
    uint8_t found_bus = 0, found_dev = 0;
    for(uint8_t bus=0; bus<8 && !found; bus++){
        for(uint8_t dev=0; dev<32 && !found; dev++){
            uint32_t id = pci_read(bus,dev,0,0x00);
            if(id == 0xFFFFFFFF) continue;
            uint16_t vid = id & 0xFFFF;
            uint16_t did = (id >> 16) & 0xFFFF;
            if(vid == 0x10EC && did == 0x8139){
                terminal_printf("[rtl8139] found at PCI %d:%d\n", bus, dev);
                found_bus = bus; found_dev = dev; found = 1;
            }
        }
    }
    if(!found){
        terminal_printf("[rtl8139] not found\n");
        return -1;
    }

    /* Enable bus master + I/O space */
    uint32_t pcicmd = pci_read(found_bus, found_dev, 0, 0x04);
    pci_write(found_bus, found_dev, 0, 0x04, pcicmd | 0x07);

    /* Read I/O BAR (BAR0) */
    uint32_t bar0 = pci_read(found_bus, found_dev, 0, 0x10);
    rtl_base = (uint16_t)(bar0 & 0xFFFC);
    terminal_printf("[rtl8139] I/O base = 0x%x\n", rtl_base);

    if(rtl8139_init() != 0) return -1;

    rtl_ready = 1;
    terminal_printf("[rtl8139] ready\n");
    return 0;
}

void rndis_get_mac(mac_addr_t mac){
    for(int i=0;i<6;i++) mac[i] = rtl_mac[i];
}

void rndis_set_receive_cb(net_receive_cb_t cb){
    recv_cb = cb;
}

int rndis_send(const uint8_t *data, size_t len){
    if(!rtl_ready) return -1;
    if(len > TX_BUF_SIZE) return -2;

    /* Wait for this descriptor to be free (OWN bit clear = NIC done) */
    uint32_t t = 200000;
    while((inl(rtl_base + RTL_TXSTATUS0 + tx_desc*4) & TX_OWN) && t--)
        r_spin(10);

    r_memcpy(tx_buf[tx_desc], data, len);
    if(len < 60){ r_memset(tx_buf[tx_desc]+len, 0, 60-len); len=60; }

    /* Write length + clear OWN to hand to NIC */
    outl(rtl_base + RTL_TXSTATUS0 + tx_desc*4, (uint32_t)len & 0x1FFF);

    tx_desc = (tx_desc + 1) & 3;
    return 0;
}

void rndis_poll(void){
    if(!rtl_ready || !recv_cb) return;

    /* Poll until RX buffer is empty (CBR == CAPR+16 wrapped) */
    while(1){
        /* CMD bit 0 = RxBufEmpty */
        if(inb(rtl_base + RTL_CMD) & 0x01) break;

        /* rx_pos points into rx_ring. Each packet is:
             uint16_t status
             uint16_t length  (includes 4-byte CRC)
             uint8_t  data[length-4]
           Followed by 4-byte CRC, then padded to DWORD alignment. */
        uint8_t  *hdr  = rx_ring + rx_pos;
        uint16_t status = (uint16_t)(hdr[0] | (hdr[1]<<8));
        uint16_t plen   = (uint16_t)(hdr[2] | (hdr[3]<<8));

        if(!(status & RX_OK) || plen < 8 || plen > 1522){
            /* Bad packet or ring corruption — reset position */
            rx_pos = (uint16_t)(inw(rtl_base + RTL_CBR) % RX_BUF_SIZE);
            outw(rtl_base + RTL_CAPR, (uint16_t)(rx_pos - 16));
            break;
        }

        /* Ethernet frame sits right after the 4-byte header */
        uint16_t frame_len = plen - 4; /* strip CRC */
        recv_cb(hdr + 4, frame_len);

        /* Advance rx_pos: 4 (header) + plen, DWORD-aligned, wrapped */
        rx_pos = (uint16_t)(((rx_pos + 4 + plen + 3) & ~3) % RX_BUF_SIZE);
        /* Tell NIC where we've read up to (CAPR = rx_pos - 16) */
        outw(rtl_base + RTL_CAPR, (uint16_t)((rx_pos - 16) & 0xFFFF));
    }
}
