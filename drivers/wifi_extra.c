



void wifi_rtlcfg_print_status(void) {
    char buf[128];
    wifi_status(buf, 128);
    terminal_printf("WiFi: %s\n", buf);
}




