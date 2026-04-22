








typedef struct {
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
    uint8_t server[4];
    uint32_t lease_secs;
} dhcp_result_t;


int dhcp_request(const uint8_t mac[6], dhcp_result_t *out,
                 uint32_t timeout_ms, int attempts);






