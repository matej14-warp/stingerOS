





static void print_ip4(const uint8_t *ip)
{
    terminal_printf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static void parse_ip4(const char *s, uint8_t out[4])
{
    int a=0,b=0,c=0,d=0;

    const char *p = s;
    while (*p && *p != '.') a = a*10 + (*p++ - '0');
    if (*p) p++;
    while (*p && *p != '.') b = b*10 + (*p++ - '0');
    if (*p) p++;
    while (*p && *p != '.') c = c*10 + (*p++ - '0');
    if (*p) p++;
    while (*p)               d = d*10 + (*p++ - '0');
    out[0]=(uint8_t)a; out[1]=(uint8_t)b;
    out[2]=(uint8_t)c; out[3]=(uint8_t)d;
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}



static void cmd_addr(void)
{
    net_config_t cfg; net_get_config(&cfg);
    terminal_printf("1: lo: <LOOPBACK,UP>\n");
    terminal_printf("    link/loopback 00:00:00:00:00:00\n");
    terminal_printf("    inet 127.0.0.1/8\n\n");
    terminal_printf("2: eth0: <%s>\n",
                    cfg.configured ? "BROADCAST,MULTICAST,UP,LOWER_UP" :
                                     "BROADCAST,MULTICAST");
    terminal_printf("    link/ether %02x:%02x:%02x:%02x:%02x:%02x\n",
        cfg.mac[0],cfg.mac[1],cfg.mac[2],cfg.mac[3],cfg.mac[4],cfg.mac[5]);
    if (cfg.configured) {

        int prefix = 0;
        for (int i = 0; i < 4; i++) {
            uint8_t m = cfg.mask[i];
            while (m & 0x80) { prefix++; m = (uint8_t)(m << 1); }
        }
        terminal_printf("    inet ");
        print_ip4(cfg.ip);
        terminal_printf("/%d brd ", prefix);

        for (int i = 0; i < 4; i++) {
            if (i) terminal_printf(".");
            terminal_printf("%d", cfg.ip[i] | (uint8_t)(~cfg.mask[i]));
        }
        terminal_printf(" scope global eth0\n");
    } else {
        terminal_printf("    (no address — run: ip dhcp)\n");
    }
}

static void cmd_link(void)
{
    net_config_t cfg; net_get_config(&cfg);
    terminal_printf("1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536\n");
    terminal_printf("    link/loopback 00:00:00:00:00:00\n\n");
    terminal_printf("2: eth0: <BROADCAST,MULTICAST,%s> mtu 1500\n",
                    cfg.configured ? "UP,LOWER_UP" : "DOWN");
    terminal_printf("    link/ether %02x:%02x:%02x:%02x:%02x:%02x brd ff:ff:ff:ff:ff:ff\n",
        cfg.mac[0],cfg.mac[1],cfg.mac[2],cfg.mac[3],cfg.mac[4],cfg.mac[5]);
}

static void cmd_route(void)
{
    net_config_t cfg; net_get_config(&cfg);
    if (!cfg.configured) {
        terminal_printf("(no routes — run: ip dhcp)\n");
        return;
    }

    terminal_printf("default via ");
    print_ip4(cfg.gw);
    terminal_printf(" dev eth0\n");


    for (int i = 0; i < 4; i++) {
        if (i) terminal_printf(".");
        terminal_printf("%d", cfg.ip[i] & cfg.mask[i]);
    }
    int prefix = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t m = cfg.mask[i];
        while (m & 0x80) { prefix++; m = (uint8_t)(m << 1); }
    }
    terminal_printf("/%d dev eth0 scope link\n", prefix);
}

static void cmd_dhcp(void)
{
    terminal_printf("Running DHCP...\n");
    int ok = -1;

    for (int i = 0; i < 5 && ok != 0; i++) {
        terminal_printf("  attempt %d/5\n", i+1);
        ok = net_dhcp();
    }
    if (ok == 0) {
        terminal_printf("DHCP OK:\n");
        cmd_addr();
    } else {
        terminal_printf("DHCP failed — check cable / adapter\n");
    }
}

static void cmd_set(char **argv, int argc)
{
    if (argc < 6) {
        terminal_printf("usage: ip set <addr> <mask> <gateway> <dns>\n");
        terminal_printf("  e.g: ip set 192.168.1.10 255.255.255.0 192.168.1.1 8.8.8.8\n");
        return;
    }
    net_config_t cfg; net_get_config(&cfg);
    parse_ip4(argv[2], cfg.ip);
    parse_ip4(argv[3], cfg.mask);
    parse_ip4(argv[4], cfg.gw);
    parse_ip4(argv[5], cfg.dns);
    cfg.configured = 1;
    net_set_config(&cfg);
    terminal_printf("eth0: static config applied\n");
    cmd_addr();
}

static void print_usage(void)
{
    terminal_printf("Usage: ip <subcommand>\n");
    terminal_printf("  ip addr   / ip a       — show addresses\n");
    terminal_printf("  ip link   / ip l       — show link state\n");
    terminal_printf("  ip route  / ip r       — show routing table\n");
    terminal_printf("  ip dhcp               — acquire address via DHCP\n");
    terminal_printf("  ip set <addr> <mask> <gw> <dns>  — static config\n");
}


void bin_ip(char **argv, int argc, sfs_node_t *cwd)
{
    (void)cwd;
    if (argc < 2) { print_usage(); return; }

    const char *sub = argv[1];
    if (str_eq(sub,"addr")  || str_eq(sub,"a"))  { cmd_addr();  return; }
    if (str_eq(sub,"link")  || str_eq(sub,"l"))  { cmd_link();  return; }
    if (str_eq(sub,"route") || str_eq(sub,"r"))  { cmd_route(); return; }
    if (str_eq(sub,"dhcp"))                       { cmd_dhcp();  return; }
    if (str_eq(sub,"set"))                        { cmd_set(argv, argc); return; }

    terminal_printf("ip: unknown subcommand '%s'\n", sub);
    print_usage();
}




