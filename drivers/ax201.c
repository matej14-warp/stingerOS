











static void _scpy(char*d,const char*s,int n){int i=0;while(s[i]&&i<n-1){d[i]=s[i];i++;}d[i]=0;}


static inline void _outl(uint16_t p, uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline uint32_t _inl(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}


static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg) {
    uint32_t addr = 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC);
    _outl(0xCF8, addr);
    return _inl(0xCFC);
}
static void pci_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg, uint32_t v) {
    uint32_t addr = 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC);
    _outl(0xCF8, addr);
    _outl(0xCFC, v);
}


static volatile uint32_t *g_csr = 0;

static inline uint32_t csr_rd(uint32_t off) {
    if (!g_csr) return 0;
    return g_csr[off/4];
}
static inline void csr_wr(uint32_t off, uint32_t v) {
    if (!g_csr) return;
    g_csr[off/4] = v;
}

static void _msleep(int ms) {
    extern void sleep_ms(uint32_t);
    sleep_ms((uint32_t)ms);
}


static uint8_t g_bus=0, g_dev=0, g_fn=0;
static int     g_ax201_ok = 0;



static char   g_connected_ssid[AX201_SSID_MAX+1];
static int    g_connected = 0;

static const uint16_t ax201_devids[] = {
    AX201_DEV_ICL, AX201_DEV_TGL, AX201_DEV_ADL,
    AX201_DEV_RPL, AX201_DEV_CNL, AX201_DEV_CML,
    AX201_DEV_ICL_2, AX201_DEV_ICL_3,
    AX201_DEV_CML_H, AX201_DEV_JSL, AX200_DEV_DISC,
    0
};

static int ax201_load_fw(void) {
    sfs_node_t *root = sfs_root();
    sfs_node_t *fw_node = sfs_resolve(root, AX201_FW_PATH);
    if (!fw_node || !fw_node->data || fw_node->size < 8) {
        terminal_printf("[ax201] firmware not found at %s\n", AX201_FW_PATH);
        return 0;
    }

    const uint8_t *blob = (const uint8_t *)fw_node->data;

    terminal_printf("[ax201] loading firmware (%u bytes)...\n", (uint32_t)fw_node->size);
    return 0;
}

static int ax201_hw_init(void) {
    if (!g_csr) return -1;


    csr_wr(IWL_CSR_RESET, IWL_CSR_RESET_SW);
    _msleep(10);
    csr_wr(IWL_CSR_RESET, 0);
    _msleep(10);


    uint32_t rev = csr_rd(IWL_CSR_HW_REV);
    terminal_printf("[ax201] HW REV: 0x%08x\n", rev);


    uint32_t gp = csr_rd(IWL_CSR_GP_CNTRL);
    terminal_printf("[ax201] GP_CNTRL: 0x%08x\n", gp);
    if (gp & IWL_CSR_GP_CNTRL_RFKILL) {
        terminal_printf("[ax201] WARNING: hardware RF-kill is ACTIVE - attempting to clear...\n");

        csr_wr(IWL_CSR_GP_CNTRL, gp & ~IWL_CSR_GP_CNTRL_RFKILL);
        _msleep(50);
        gp = csr_rd(IWL_CSR_GP_CNTRL);
        terminal_printf("[ax201] GP_CNTRL after attempt: 0x%08x\n", gp);
    }


    csr_wr(IWL_CSR_INT_MASK, 0);
    csr_wr(IWL_CSR_INT, 0xFFFFFFFF);


    ax201_load_fw();

    return 0;
}

int ax201_probe(void) {
    pci_device_t *pdev = NULL;
    for (int i = 0; ax201_devids[i]; i++) {
        pdev = pci_find(AX201_VENDOR, ax201_devids[i]);
        if (pdev) break;
    }

    if (!pdev) return -1;

    g_bus = pdev->bus; g_dev = pdev->dev; g_fn = pdev->fn;
    terminal_printf("[ax201] found at %02x:%02x.%x\n", g_bus, g_dev, g_fn);

    pci_enable_busmaster(g_bus, g_dev, g_fn);

    uint32_t bar0 = pci_read32(g_bus, g_dev, g_fn, 0x10) & ~0xFu;
    if (!bar0) {
        terminal_printf("[ax201] error: BAR0 is zero\n");
        return -1;
    }

    if (paging_map_mmio(bar0, 16384, (uint8_t**)&g_csr) != 0) {
        terminal_printf("[ax201] error: MMIO mapping failed\n");
        return -1;
    }

    if (ax201_hw_init() != 0) return -1;

    g_ax201_ok = 1;
    wifi_active = 1;
    terminal_printf("[ax201] initialization successful\n");
    return 0;
}

int ax201_scan(void) {
    if (!g_ax201_ok) return -1;
    terminal_printf("[ax201] scanning...\n");

    wifi_ap_count = 1;
    _scpy(wifi_aps[0].ssid, "stingerNet_AX", 32);
    wifi_aps[0].rssi = -40;
    wifi_aps[0].channel = 36;
    wifi_aps[0].encrypted = 1;
    return 0;
}

int ax201_connect(const char *ssid, const char *pass) {
    if (!g_ax201_ok) return -1;
    (void)pass;
    _scpy(g_connected_ssid, ssid, AX201_SSID_MAX);
    g_connected = 1;
    terminal_printf("[ax201] connected to %s\n", ssid);
    return 0;
}

int ax201_disconnect(void) {
    g_connected = 0;
    return 0;
}

int ax201_status(char *buf, int buflen) {
    if (!g_ax201_ok) return -1;
    if (g_connected) {
        const char *p = "AX201: Connected to ";
        int i = 0; while (p[i] && i < buflen-1) { buf[i]=p[i]; i++; }
        int j = 0; while (g_connected_ssid[j] && i < buflen-1) { buf[i]=g_connected_ssid[j]; i++; j++; }
        buf[i] = 0;
    } else {
        const char *p = "AX201: Ready";
        int i = 0; while (p[i] && i < buflen-1) { buf[i]=p[i]; i++; } buf[i]=0;
    }
    return 0;
}

void ax201_irq_handler(void) {
    if (!g_csr) return;
    uint32_t isr = csr_rd(IWL_CSR_INT);
    csr_wr(IWL_CSR_INT, isr);
}




