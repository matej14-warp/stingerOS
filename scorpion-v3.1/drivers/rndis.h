#ifndef RNDIS_H
#define RNDIS_H

#include <stdint.h>
#include <stddef.h>

/* RNDIS message types */
#define RNDIS_INITIALIZE_MSG        0x00000002
#define RNDIS_INITIALIZE_CMPLT     0x80000002
#define RNDIS_HALT_MSG              0x00000003
#define RNDIS_QUERY_MSG             0x00000004
#define RNDIS_QUERY_CMPLT          0x80000004
#define RNDIS_SET_MSG               0x00000005
#define RNDIS_SET_CMPLT            0x80000005
#define RNDIS_RESET_MSG             0x00000006
#define RNDIS_RESET_CMPLT          0x80000006
#define RNDIS_INDICATE_STATUS_MSG  0x00000007
#define RNDIS_KEEPALIVE_MSG        0x00000008
#define RNDIS_KEEPALIVE_CMPLT      0x80000008
#define RNDIS_PACKET_MSG           0x00000001

/* RNDIS OIDs */
#define OID_GEN_CURRENT_PACKET_FILTER  0x0001010E
#define OID_802_3_CURRENT_ADDRESS      0x01010102
#define NDIS_PACKET_TYPE_DIRECTED      0x0001
#define NDIS_PACKET_TYPE_BROADCAST     0x0004
#define NDIS_PACKET_TYPE_ALL_MULTICAST 0x0008
#define NDIS_PACKET_TYPE_PROMISCUOUS   0x0020

/* Ethernet frame size */
#define ETH_MAX_FRAME 1514
#define ETH_HEADER    14

typedef struct {
    uint32_t msg_type;
    uint32_t msg_len;
    uint32_t req_id;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t max_xfer_size;
} __attribute__((packed)) rndis_init_msg_t;

typedef struct {
    uint32_t msg_type;
    uint32_t msg_len;
    uint32_t req_id;
    uint32_t status;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t device_flags;
    uint32_t medium;
    uint32_t max_packets_per_xfer;
    uint32_t max_xfer_size;
    uint32_t packet_align;
    uint32_t af_list_offset;
    uint32_t af_list_size;
} __attribute__((packed)) rndis_init_cmplt_t;

typedef struct {
    uint32_t msg_type;
    uint32_t msg_len;
    uint32_t data_offset;
    uint32_t data_length;
    uint32_t oob_data_offset;
    uint32_t oob_data_length;
    uint32_t num_oob_elements;
    uint32_t per_packet_info_offset;
    uint32_t per_packet_info_length;
    uint32_t reserved[2];
} __attribute__((packed)) rndis_packet_msg_t;

/* MAC address */
typedef uint8_t mac_addr_t[6];

/* Network receive callback */
typedef void (*net_receive_cb_t)(const uint8_t *data, size_t len);

/* Public API */
int  rndis_init(void);
int  rndis_send(const uint8_t *data, size_t len);
void rndis_poll(void);
void rndis_get_mac(mac_addr_t mac);
void rndis_set_receive_cb(net_receive_cb_t cb);

#endif /* RNDIS_H */
