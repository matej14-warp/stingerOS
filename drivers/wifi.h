/* scorpion OS - drivers/wifi.h */
#ifndef WIFI_H
#define WIFI_H
#include <stdint.h>

extern int wifi_active;

typedef struct {
    char    ssid[33];
    uint8_t bssid[6];
    int8_t  rssi;
    uint8_t channel;
    int     encrypted;
} wifi_ap_t;

#define WIFI_MAX_APS 16
extern wifi_ap_t wifi_aps[];
extern int       wifi_ap_count;

/* Probe routines — each returns 0 on success */
int wifi_rtl8188_probe(void);
int wifi_rtl8192_probe(void);
int wifi_ath9k_probe(void);
int wifi_mt7601_probe(void);
int wifi_iwlwifi_probe(void);
int wifi_rt2800_probe(void);
int wifi_brcmfmac_probe(void);

int wifi_scan(void);
int wifi_connect(const char *ssid, const char *password);
int wifi_disconnect(void);
int wifi_status(char *buf, int buflen);   /* writes status string into buf */

#endif /* WIFI_H */
