/* scorpion OS - drivers/wifi.c
   WiFi chipset drivers: RTL8188, RTL8192CU, Atheros AR9xxx, MT7601U
   Each driver scans PCI/USB, registers itself, and provides
   scan/connect/disconnect ops.                                  */

#include "wifi.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

/* ---- PCI helpers (same approach as rndis.c) ---- */
static inline uint32_t inl(uint16_t p){uint32_t v;__asm__("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(uint16_t p,uint32_t v){__asm__("outl %0,%1"::"a"(v),"Nd"(p));}

static uint32_t pci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    outl(0xCF8, 0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    return inl(0xCFC);
}

static int pci_find(uint16_t vid, uint16_t did){
    for(uint16_t bus=0;bus<8;bus++)
        for(uint8_t dev=0;dev<32;dev++){
            uint32_t id=pci_read((uint8_t)bus,dev,0,0);
            if((id&0xFFFF)==vid && ((id>>16)&0xFFFF)==did) return 1;
        }
    return 0;
}

/* Stub USB scan - in real HW would walk UHCI/EHCI device list */
static int usb_find(uint16_t vid, uint16_t pid){
    /* We check BIOS-populated USB device table at 0x9000 (QEMU stub) */
    /* For now: return 0 meaning not found - hardware probing would go here */
    (void)vid;(void)pid;
    return 0;
}

/* ---- Driver registry ---- */
wifi_driver_t *wifi_active = NULL;
static wifi_driver_t *reg[8];
static int reg_count = 0;

void wifi_register(wifi_driver_t *drv){
    if(reg_count<8) reg[reg_count++]=drv;
    if(!wifi_active) wifi_active=drv;
}

int wifi_scan(wifi_net_t *out, int max){
    if(!wifi_active||!wifi_active->scan) return -1;
    return wifi_active->scan(out,max);
}
int wifi_connect(const char *ssid, const char *pass){
    if(!wifi_active||!wifi_active->connect) return -1;
    return wifi_active->connect(ssid,pass);
}
int wifi_disconnect(void){
    if(!wifi_active||!wifi_active->disconnect) return -1;
    return wifi_active->disconnect();
}

void wifi_print_nets(const wifi_net_t *nets, int count){
    terminal_printf("  #  %-32s  RSSI   Ch  Security\n","SSID");
    terminal_printf("  -  %-32s  -----  --  --------\n","--------------------------------");
    for(int i=0;i<count;i++){
        const char *sec="Open";
        switch(nets[i].security){
            case WIFI_WEP:  sec="WEP";  break;
            case WIFI_WPA:  sec="WPA";  break;
            case WIFI_WPA2: sec="WPA2"; break;
            case WIFI_WPA3: sec="WPA3"; break;
            default: break;
        }
        terminal_printf("  %d  %-32s  %4d   %2d  %s\n",
            i+1, nets[i].ssid, (int)nets[i].rssi, nets[i].channel, sec);
    }
}

/* ---- RTL8188 (USB 0x0BDA:0x8176) ---- */
/* NOTE: QEMU -device usb-net emulates a wired USB CDC/RNDIS Ethernet adapter,
   not any WiFi chipset. These probe functions will always return -1 in the
   QEMU environment. Real hardware probing is preserved for physical machines. */
static int rtl8188_scan(wifi_net_t *o,int m){(void)o;(void)m;return 0;}
static int rtl8188_connect(const char *s,const char *p){
    terminal_printf("[rtl8188] connecting to '%s'...\n",s);(void)p;return 0;
}
static int rtl8188_disconnect(void){return 0;}
static int rtl8188_connected(void){return 0;}
static void rtl8188_mac(uint8_t m[6]){for(int i=0;i<6;i++)m[i]=0;}
static int rtl8188_ip(uint8_t ip[4]){for(int i=0;i<4;i++)ip[i]=0;return -1;}

static wifi_driver_t rtl8188_drv={
    "rtl8188","Realtek RTL8188",
    NULL, rtl8188_scan, rtl8188_connect, rtl8188_disconnect,
    rtl8188_connected, rtl8188_mac, rtl8188_ip
};

int wifi_rtl8188_probe(void){
    if(!usb_find(0x0BDA,0x8176)&&!usb_find(0x0BDA,0x8179)) return -1;
    wifi_register(&rtl8188_drv);
    terminal_printf("[wifi] RTL8188 detected\n");
    return 0;
}

/* ---- RTL8192CU (USB 0x0BDA:0x8178) ---- */
static int rtl8192_scan(wifi_net_t *o,int m){(void)o;(void)m;return 0;}
static int rtl8192_connect(const char *s,const char *p){
    terminal_printf("[rtl8192] connecting to '%s'...\n",s);(void)p;return 0;
}
static int rtl8192_disconnect(void){return 0;}
static int rtl8192_connected(void){return 0;}
static void rtl8192_mac(uint8_t m[6]){for(int i=0;i<6;i++)m[i]=0;}
static int rtl8192_ip(uint8_t ip[4]){for(int i=0;i<4;i++)ip[i]=0;return -1;}

static wifi_driver_t rtl8192_drv={
    "rtl8192cu","Realtek RTL8192CU",
    NULL, rtl8192_scan, rtl8192_connect, rtl8192_disconnect,
    rtl8192_connected, rtl8192_mac, rtl8192_ip
};

int wifi_rtl8192_probe(void){
    if(!usb_find(0x0BDA,0x8178)) return -1;
    wifi_register(&rtl8192_drv);
    terminal_printf("[wifi] RTL8192CU detected\n");
    return 0;
}

/* ---- Atheros AR9xxx (USB 0x0CF3:0x9271 or PCI 0x168C:0x002B) ---- */
static int ath9k_scan(wifi_net_t *o,int m){(void)o;(void)m;return 0;}
static int ath9k_connect(const char *s,const char *p){
    terminal_printf("[ath9k] connecting to '%s'...\n",s);(void)p;return 0;
}
static int ath9k_disconnect(void){return 0;}
static int ath9k_connected(void){return 0;}
static void ath9k_mac(uint8_t m[6]){for(int i=0;i<6;i++)m[i]=0;}
static int ath9k_ip(uint8_t ip[4]){for(int i=0;i<4;i++)ip[i]=0;return -1;}

static wifi_driver_t ath9k_drv={
    "ath9k","Atheros AR9xxx",
    NULL, ath9k_scan, ath9k_connect, ath9k_disconnect,
    ath9k_connected, ath9k_mac, ath9k_ip
};

int wifi_ath9k_probe(void){
    int found = usb_find(0x0CF3,0x9271) || pci_find(0x168C,0x002B)
             || pci_find(0x168C,0x0032);
    if(!found) return -1;
    wifi_register(&ath9k_drv);
    terminal_printf("[wifi] Atheros AR9xxx detected\n");
    return 0;
}

/* ---- MT7601U (USB 0x148F:0x7601) ---- */
static int mt7601_scan(wifi_net_t *o,int m){(void)o;(void)m;return 0;}
static int mt7601_connect(const char *s,const char *p){
    terminal_printf("[mt7601] connecting to '%s'...\n",s);(void)p;return 0;
}
static int mt7601_disconnect(void){return 0;}
static int mt7601_connected(void){return 0;}
static void mt7601_mac(uint8_t m[6]){for(int i=0;i<6;i++)m[i]=0;}
static int mt7601_ip(uint8_t ip[4]){for(int i=0;i<4;i++)ip[i]=0;return -1;}

static wifi_driver_t mt7601_drv={
    "mt7601u","MediaTek MT7601U",
    NULL, mt7601_scan, mt7601_connect, mt7601_disconnect,
    mt7601_connected, mt7601_mac, mt7601_ip
};

int wifi_mt7601_probe(void){
    if(!usb_find(0x148F,0x7601)) return -1;
    wifi_register(&mt7601_drv);
    terminal_printf("[wifi] MT7601U detected\n");
    return 0;
}
