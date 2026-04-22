







int dhcp_request(const uint8_t mac[6], dhcp_result_t *out,
                 uint32_t timeout_ms, int attempts)
{
    (void)mac;
    (void)timeout_ms;

    if (!out) return -1;

    int ret = -1;
    for (int i = 0; i < attempts && ret != 0; i++) {
        terminal_printf("[dhcp] attempt %d/%d...\n", i+1, attempts);
        ret = net_dhcp();
    }

    if (ret == 0) {
        net_config_t cfg;
        net_get_config(&cfg);


        for (int i = 0; i < 4; i++) out->ip[i]      = cfg.ip[i];
        for (int i = 0; i < 4; i++) out->mask[i]    = cfg.mask[i];
        for (int i = 0; i < 4; i++) out->gateway[i] = cfg.gw[i];
        for (int i = 0; i < 4; i++) out->dns[i]     = cfg.dns[i];
        for (int i = 0; i < 4; i++) out->server[i]  = 0;
        out->lease_secs = 86400;

        terminal_printf("[dhcp] got IP %d.%d.%d.%d mask %d.%d.%d.%d gw %d.%d.%d.%d\n",
            out->ip[0], out->ip[1], out->ip[2], out->ip[3],
            out->mask[0], out->mask[1], out->mask[2], out->mask[3],
            out->gateway[0], out->gateway[1], out->gateway[2], out->gateway[3]);
    }

    return ret;
}




