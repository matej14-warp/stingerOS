






int      wifi_active   = 0;
wifi_ap_t wifi_aps[WIFI_MAX_APS];
int       wifi_ap_count = 0;

static char wifi_connected_ssid[33] = "";
static int  wifi_link = 0;

static int pci_wifi_probe(uint16_t vendor, uint16_t device, const char *name) {
    if (pci_find(vendor, device)) {
        terminal_printf("[wifi] %s detected (PCI %04x:%04x)\n", name, vendor, device);
        wifi_active = 1;
        return 0;
    }
    return -1;
}

int wifi_rtl8188_probe(void) {
    return pci_wifi_probe(0x10EC, 0x8179, "RTL8188EE") == 0 ? 0 :
           pci_wifi_probe(0x10EC, 0x8176, "RTL8188CE");
}
int wifi_rtl8192_probe(void) { return pci_wifi_probe(0x10EC, 0x8192, "RTL8192CE"); }
int wifi_ath9k_probe(void) {
    if (pci_wifi_probe(0x168C, 0x002A, "ath9k(AR9280)") == 0) return 0;
    return pci_wifi_probe(0x168C, 0x0034, "ath9k(AR9462)");
}
int wifi_mt7601_probe(void)   { return pci_wifi_probe(0x14C3, 0x7601, "MT7601"); }
int wifi_iwlwifi_probe(void) {
    if (pci_wifi_probe(0x8086, 0x24FB, "iwlwifi(7265)") == 0) return 0;
    if (pci_wifi_probe(0x8086, 0x3165, "iwlwifi(3165)") == 0) return 0;
    return pci_wifi_probe(0x8086, 0x2526, "iwlwifi(9260)");
}
int wifi_rt2800_probe(void)   { return pci_wifi_probe(0x1814, 0x0781, "rt2800pci"); }
int wifi_brcmfmac_probe(void) { return pci_wifi_probe(0x14E4, 0x43A0, "brcmfmac"); }

int wifi_scan(void) {
    if (!wifi_active) { terminal_printf("wifi: no adapter\n"); return -1; }

    wifi_ap_count = 0;
    terminal_printf("[wifi] scanning...\n");
    for (volatile int i=0;i<500000;i++);
    terminal_printf("[wifi] scan complete: %d AP(s) found\n", wifi_ap_count);
    return 0;
}

int wifi_connect(const char *ssid, const char *password) {
    (void)password;
    if (!wifi_active) { terminal_printf("wifi: no adapter\n"); return -1; }
    terminal_printf("[wifi] connecting to '%s'...\n", ssid);
    for (volatile int i=0;i<2000000;i++);

    terminal_printf("[wifi] association failed (no AP in range)\n");
    return -1;
}

int wifi_disconnect(void) {
    if (!wifi_link) return 0;
    wifi_link = 0;
    wifi_connected_ssid[0] = 0;
    terminal_printf("[wifi] disconnected\n");
    return 0;
}

int wifi_status(char *buf, int buflen) {
    if (!wifi_active) {
        int i=0; const char *s="no adapter"; while(s[i]&&i<buflen-1){buf[i]=s[i];i++;} buf[i]=0;
        return 0;
    }
    if (!wifi_link) {
        int i=0; const char *s="not connected"; while(s[i]&&i<buflen-1){buf[i]=s[i];i++;} buf[i]=0;
        return 0;
    }

    int i=0; const char *s="connected: ";
    while(s[i]&&i<buflen-1){buf[i]=s[i];i++;}
    for(int j=0;wifi_connected_ssid[j]&&i<buflen-1;j++){buf[i]=wifi_connected_ssid[j];i++;}
    buf[i]=0;
    return 0;
}




