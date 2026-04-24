#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>

#define WIFI_SSID_MAX  32
#define WIFI_MAX_NETS  24

typedef enum { WIFI_OPEN=0, WIFI_WEP, WIFI_WPA, WIFI_WPA2, WIFI_WPA3 } wifi_sec_t;

typedef struct {
    char       ssid[WIFI_SSID_MAX+1];
    uint8_t    bssid[6];
    int8_t     rssi;
    uint8_t    channel;
    wifi_sec_t security;
} wifi_net_t;

typedef struct wifi_driver {
    const char *name;
    const char *chipset;
    int  (*probe)(void);
    int  (*scan)(wifi_net_t *out, int max);
    int  (*connect)(const char *ssid, const char *pass);
    int  (*disconnect)(void);
    int  (*is_connected)(void);
    void (*get_mac)(uint8_t mac[6]);
    int  (*get_ip)(uint8_t ip[4]);
} wifi_driver_t;

extern wifi_driver_t *wifi_active;

void wifi_register(wifi_driver_t *drv);
int  wifi_scan(wifi_net_t *out, int max);
int  wifi_connect(const char *ssid, const char *pass);
int  wifi_disconnect(void);
void wifi_print_nets(const wifi_net_t *nets, int count);

/* Probe functions */
int wifi_rtl8188_probe(void);
int wifi_rtl8192_probe(void);
int wifi_ath9k_probe(void);
int wifi_mt7601_probe(void);

#endif
