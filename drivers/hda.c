









static inline void _outb(uint16_t p, uint8_t v) {
  __asm__ volatile("outb %0,%1" ::"a"(v), "Nd"(p));
}
static inline uint8_t _inb(uint16_t p) {
  uint8_t v;
  __asm__ volatile("inb  %1,%0" : "=a"(v) : "Nd"(p));
  return v;
  (void)_outb;
}
static inline void _outl(uint16_t p, uint32_t v) {
  __asm__ volatile("outl %0,%1" ::"a"(v), "Nd"(p));
}
static inline uint32_t _inl(uint16_t p) {
  uint32_t v;
  __asm__ volatile("inl  %1,%0" : "=a"(v) : "Nd"(p));
  return v;
  (void)_outl;
}


volatile uint8_t *g_hda_base = NULL;
static int g_hda_ok = 0;

static inline uint8_t hda_rb(uint32_t off) { return g_hda_base[off]; }
static inline uint16_t hda_rw(uint32_t off) {
  return *(volatile uint16_t *)(g_hda_base + off);
}
static inline uint32_t hda_rl(uint32_t off) {
  return *(volatile uint32_t *)(g_hda_base + off);
}
static inline void hda_wb(uint32_t off, uint8_t v) { g_hda_base[off] = v; }
static inline void hda_ww(uint32_t off, uint16_t v) {
  *(volatile uint16_t *)(g_hda_base + off) = v;
}
static inline void hda_wl(uint32_t off, uint32_t v) {
  *(volatile uint32_t *)(g_hda_base + off) = v;
}




































static uint32_t g_out_stream_base = 0;























  (((uint32_t)(codec) << 28) | ((uint32_t)(nid) << 20) |                       \
   ((uint32_t)(verb) << 8) | (uint32_t)(payload))









  0x3B0


























static uint32_t g_corb[CORB_ENTRIES] __attribute__((aligned(128)));
static uint64_t g_rirb[RIRB_ENTRIES] __attribute__((aligned(128)));


typedef struct {
  uint32_t addr_lo;
  uint32_t addr_hi;
  uint32_t length;
  uint32_t flags;
} bdl_entry_t;


static bdl_entry_t g_bdl_raw[BDL_ENTRIES + 1] __attribute__((aligned(128)));
static bdl_entry_t *g_bdl = NULL;


static uint8_t g_codec_addr = 0xFF;
static uint8_t g_afg_nid = 0;
static uint8_t g_dac_nid = 0;
static uint8_t g_pin_nid = 0;
static uint8_t g_vol = 200;


static void hda_udelay(int us) {
  for (int i = 0; i < us; i++)
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}


static uint32_t g_rirb_rp = 0;

static void corb_send(uint32_t verb) {
  uint16_t wp = hda_rw(HDA_CORBWP) & 0xFF;
  wp = (wp + 1) % CORB_ENTRIES;
  g_corb[wp] = verb;
  __asm__ volatile("" ::: "memory");
  hda_ww(HDA_CORBWP, wp);
}

static uint64_t rirb_read(void) {
  int timeout = 2000;
  while (timeout-- > 0) {
    uint16_t wp = hda_rw(HDA_RIRBWP) & 0xFF;
    if (wp != (uint16_t)(g_rirb_rp & 0xFF)) {
      g_rirb_rp = (g_rirb_rp + 1) % RIRB_ENTRIES;
      __asm__ volatile("" ::: "memory");
      return g_rirb[g_rirb_rp];
    }
    hda_udelay(1);
  }
  return 0xFFFFFFFFFFFFFFFFull;
}

static uint32_t codec_cmd(uint8_t codec, uint8_t nid, uint32_t verb,
                          uint32_t payload) {
  uint32_t v = HDA_VERB(codec, nid, verb, payload);
  corb_send(v);
  uint64_t resp = rirb_read();
  return (uint32_t)(resp & 0xFFFFFFFF);
}


static int hda_reset(void) {
  uint32_t gctl = hda_rl(HDA_GCTL);
  hda_wl(HDA_GCTL, gctl & ~1u);
  int t = 0;
  while ((hda_rl(HDA_GCTL) & 1) && t++ < 50000)
    hda_udelay(1);

  hda_udelay(100);

  hda_wl(HDA_GCTL, hda_rl(HDA_GCTL) | 1u);
  t = 0;
  while (!(hda_rl(HDA_GCTL) & 1) && t++ < 50000)
    hda_udelay(1);

  if (!(hda_rl(HDA_GCTL) & 1)) {
    terminal_printf("[hda] reset failed\n");
    return -1;
  }

  hda_wl(HDA_INTCTL, 0);
  hda_udelay(1000);
  return 0;
}


static int hda_corb_rirb_init(void) {
  int t;

  hda_wb(HDA_CORBCTL, 0);
  hda_wb(HDA_RIRBCTL, 0);
  hda_udelay(100);


  hda_wb(HDA_CORBSIZE, 0x02);
  hda_wb(HDA_RIRBSIZE, 0x02);


  uint32_t corb_phys = paging_get_phys((uint32_t)(uintptr_t)g_corb);
  uint32_t rirb_phys = paging_get_phys((uint32_t)(uintptr_t)g_rirb);
  hda_wl(HDA_CORBLBASE, corb_phys);
  hda_wl(HDA_CORBUBASE, 0);
  hda_wl(HDA_RIRBLBASE, rirb_phys);
  hda_wl(HDA_RIRBUBASE, 0);


  hda_ww(HDA_CORBRP, 0x8000);
  t = 0;
  while (!(hda_rw(HDA_CORBRP) & 0x8000) && t++ < 10000)
    hda_udelay(1);
  hda_ww(HDA_CORBRP, 0x0000);
  t = 0;
  while ((hda_rw(HDA_CORBRP) & 0x8000) && t++ < 10000)
    hda_udelay(1);


  hda_ww(HDA_RIRBWP, 0x8000);
  hda_udelay(50);
  g_rirb_rp = 0;

  hda_ww(HDA_RINTCNT, 1);


  hda_wb(HDA_CORBCTL, 0x02);
  hda_wb(HDA_RIRBCTL, 0x02);
  hda_udelay(100);

  terminal_printf("[hda] CORB/RIRB running\n");
  return 0;
}


static int hda_find_codec(void) {
  uint16_t statests = hda_rw(HDA_STATESTS);
  if (statests == 0) {
    terminal_printf("[hda] no codecs found (STATESTS=0)\n");
    return -1;
  }
  for (int i = 0; i < 15; i++) {
    if (statests & (1u << i)) {
      g_codec_addr = (uint8_t)i;
      uint32_t vid = codec_cmd(g_codec_addr, 0, V_GET_PARAM, P_VENDOR_ID);
      terminal_printf("[hda] codec %d: vendor=%04x device=%04x\n", i,
                      (vid >> 16) & 0xFFFF, vid & 0xFFFF);
      return 0;
    }
  }
  return -1;
}


static void hda_find_afg(void) {
  uint32_t sub = codec_cmd(g_codec_addr, 0, V_GET_PARAM, P_SUBNODE_COUNT);
  uint8_t start = (sub >> 16) & 0xFF;
  uint8_t count = sub & 0xFF;
  terminal_printf("[hda] root subnodes: start=%d count=%d\n", start, count);

  for (int nid = (int)start; nid < (int)start + (int)count; nid++) {
    uint32_t fgtype = codec_cmd(g_codec_addr, nid, V_GET_PARAM, P_FG_TYPE);
    if ((fgtype & 0xFF) == 0x01) {
      g_afg_nid = nid;
      terminal_printf("[hda] AFG at NID %d\n", nid);
      return;
    }
  }
  g_afg_nid = 1;
  terminal_printf("[hda] AFG not found, defaulting to NID 1\n");
}

static void hda_find_dac_pin(void) {
  uint32_t sub =
      codec_cmd(g_codec_addr, g_afg_nid, V_GET_PARAM, P_SUBNODE_COUNT);
  uint8_t start = (sub >> 16) & 0xFF;
  uint8_t count = sub & 0xFF;
  terminal_printf("[hda] AFG subnodes: start=%d count=%d\n", start, count);

  uint8_t best_pin = 0;
  int best_pin_score = -1;

  for (int nid = (int)start; nid < (int)start + (int)count; nid++) {
    uint32_t wcap = codec_cmd(g_codec_addr, nid, V_GET_PARAM, P_WIDGET_CAP);
    uint8_t wtype = (wcap >> 20) & 0xF;

    if (wtype == WT_OUTPUT && g_dac_nid == 0) {
      g_dac_nid = nid;
      terminal_printf("[hda] DAC at NID %d\n", nid);
    }

    if (wtype == WT_PIN) {
      uint32_t defcfg = codec_cmd(g_codec_addr, nid, 0xF1C, 0);
      uint8_t port_conn = (defcfg >> 30) & 0x3;
      uint8_t def_dev = (defcfg >> 20) & 0xF;
      int score = 0;
      if (port_conn == 0)
        score += 10;
      if (def_dev == 2)
        score += 5;
      if (def_dev == 0)
        score += 4;
      if (score > best_pin_score) {
        best_pin_score = score;
        best_pin = nid;
      }
    }
  }

  if (g_dac_nid == 0) {
    g_dac_nid = 2;
    terminal_printf("[hda] DAC defaulting to NID 2\n");
  }
  if (best_pin == 0) {
    best_pin = 0x14;
  }

  g_pin_nid = best_pin;
  terminal_printf("[hda] output pin at NID %d\n", g_pin_nid);
}


static void hda_program_codec(void) {


  if (g_afg_nid == 0) {
    terminal_printf("[hda] ERROR: Invalid AFG NID\n");
    return;
  }
  if (g_dac_nid == 0) {
    terminal_printf("[hda] ERROR: Invalid DAC NID\n");
    return;
  }
  if (g_pin_nid == 0) {
    terminal_printf("[hda] ERROR: Invalid PIN NID\n");
    return;
  }


  codec_cmd(g_codec_addr, g_afg_nid, V_SET_POWER, 0x00);
  hda_udelay(2000);


  codec_cmd(g_codec_addr, g_dac_nid, V_SET_POWER, 0x00);


  codec_cmd(g_codec_addr, g_dac_nid, V_SET_STREAM_CH, (2 << 4) | 0);


  codec_cmd(g_codec_addr, g_dac_nid, 0x002, 0x0011);


  uint8_t gain = (uint8_t)(g_vol / 2);


  codec_cmd(g_codec_addr, g_dac_nid, V_AMP_GAIN_OUT_LR, gain & 0x7F);


  codec_cmd(g_codec_addr, g_pin_nid, V_SET_PIN_WIDGET, 0xC0);


  codec_cmd(g_codec_addr, g_pin_nid, V_AMP_GAIN_OUT_LR, gain & 0x7F);
  codec_cmd(g_codec_addr, 0x0B, V_AMP_GAIN_OUT_LR, gain & 0x7F);
  codec_cmd(g_codec_addr, 0x0C, V_AMP_GAIN_OUT_LR, gain & 0x7F);


  uint32_t connlist = codec_cmd(g_codec_addr, g_pin_nid, V_GET_CONN_LIST, 0);
  uint8_t nconn = connlist & 0x7F;
  uint8_t dac_idx = 0;
  for (uint8_t ci = 0; ci < nconn && ci < 4; ci++) {
    uint32_t entry = codec_cmd(g_codec_addr, g_pin_nid, V_GET_CONN_LIST, ci);
    if ((entry & 0xFF) == g_dac_nid) {
      dac_idx = ci;
      break;
    }
  }
  codec_cmd(g_codec_addr, g_pin_nid, V_SET_CONN_SEL, dac_idx);


  codec_cmd(g_codec_addr, g_pin_nid, V_SET_POWER, 0x00);

  terminal_printf("[hda] codec programmed (DAC %d -> Pin %d, vol %d/255)\n",
                  g_dac_nid, g_pin_nid, g_vol);
}


static void hda_stream_stop(void) {
  uint32_t ctl = hda_rl(HDA_SD0_CTL);
  hda_wl(HDA_SD0_CTL, ctl & ~0x00000002u);
  hda_udelay(100);
  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0xFC000000u);
}

static void hda_stream_setup(const int16_t *data, uint32_t n_frames) {
  uint32_t byte_count = n_frames * 2 * sizeof(int16_t);

  hda_stream_stop();


  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0x00000001u);
  hda_udelay(100);
  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) & ~0x00000001u);
  hda_udelay(100);


  uintptr_t raw_bdl = (uintptr_t)g_bdl_raw;
  g_bdl = (bdl_entry_t *)((raw_bdl + 127) & ~127);

  uint32_t phys_data = paging_get_phys((uint32_t)(uintptr_t)data);

  g_bdl[0].addr_lo = phys_data;
  g_bdl[0].addr_hi = 0;
  g_bdl[0].length = byte_count;
  g_bdl[0].flags = 0x01;

  __asm__ volatile("" ::: "memory");

  uint32_t bdl_phys = paging_get_phys((uint32_t)(uintptr_t)g_bdl);
  hda_wl(HDA_SD0_BDPL, bdl_phys);
  hda_wl(HDA_SD0_BDPU, 0);
  hda_wl(HDA_SD0_CBL, byte_count);
  hda_ww(HDA_SD0_LVI, 0);
  hda_ww(HDA_SD0_FMT, 0x0011);


  uint16_t gcap = hda_rw(HDA_GCAP);
  uint8_t in_streams = (gcap >> 8) & 0x0F;
  uint32_t ssync = hda_rl(HDA_SSYNC);
  hda_wl(HDA_SSYNC, ssync & ~(1u << in_streams));


  uint32_t ctl_val = (2u << 20) | 0x000004u;
  hda_wl(HDA_SD0_CTL, ctl_val);


  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0xFC000000u);

  terminal_printf("[hda] BDL@0x%08x Data@0x%08x CTL=0x%08x\n", bdl_phys,
                  phys_data, hda_rl(HDA_SD0_CTL));


  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0x02u);
  hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0xFC000000u);
}



int hda_available(void) { return g_hda_ok; }


void hda_set_volume(uint8_t vol) {
  g_vol = vol;
  if (!g_hda_ok)
    return;
  uint8_t gain = (uint8_t)(vol / 2);
  codec_cmd(g_codec_addr, g_dac_nid, V_AMP_GAIN_OUT_LR, gain & 0x7F);
  codec_cmd(g_codec_addr, g_pin_nid, V_AMP_GAIN_OUT_LR, gain & 0x7F);
}

void hda_stop(void) {
  if (!g_hda_ok)
    return;
  hda_stream_stop();
}


static int16_t g_audio_buf[MAX_AUDIO_BUF / 2] __attribute__((aligned(128)));



void hda_play_pcm(const int16_t *data, uint32_t n_frames) {
  if (!g_hda_ok || !data || n_frames == 0)
    return;

  uint32_t offset = 0;
  while (offset < n_frames) {
    uint32_t chunk = n_frames - offset;
    if (chunk > HDA_MAX_CHUNK_FRAMES)
      chunk = HDA_MAX_CHUNK_FRAMES;

    const int16_t *src = data + offset * 2;
    for (uint32_t i = 0; i < chunk * 2; i++)
      g_audio_buf[i] = src[i];

    hda_stream_setup(g_audio_buf, chunk);


    for (int t = 0; t < 5000; t++) {
      if (hda_rl(HDA_SD0_LPIB) > 0)
        break;
      hda_udelay(100);
    }

    uint32_t ctl = hda_rl(HDA_SD0_CTL);
    uint32_t lpib = hda_rl(HDA_SD0_LPIB);
    terminal_printf(
        "[hda] stream running (%u frames, CTL=0x%08x, LPIB=0x%08x)\n", chunk,
        ctl, lpib);

    uint32_t ms = (chunk * 1000u) / 48000u;
    if (ms == 0)
      ms = 1;
    for (uint32_t w = 0; w < ms; w += 10) {
      hda_udelay(10000);
      hda_wl(HDA_SD0_CTL, hda_rl(HDA_SD0_CTL) | 0xFC000000u);
    }

    offset += chunk;
  }
}

void hda_init(void) {
  g_hda_ok = 0;
  g_codec_addr = 0xFF;
  g_afg_nid = 0;
  g_dac_nid = 0;
  g_pin_nid = 0;
  terminal_printf("[hda] init started\n");

  if (g_hda_base == NULL) {
    terminal_printf("[hda] ERROR: HDA controller base not mapped\n");
    return;
  }

  if (hda_reset() != 0) {
    terminal_printf("[hda] ERROR: Controller reset failed\n");
    return;
  }
  hda_corb_rirb_init();


  uint16_t gcap = hda_rw(HDA_GCAP);
  uint8_t out_streams = (gcap >> 12) & 0x0F;
  uint8_t in_streams = (gcap >> 8) & 0x0F;
  g_out_stream_base = 0x80 + (in_streams * 0x20);
  terminal_printf("[hda] GCAP=%04x: in=%d, out=%d, stream_base=0x%02x\n", gcap,
                  in_streams, out_streams, g_out_stream_base);


  hda_ww(HDA_STATESTS, 0xFFFF);


  hda_wl(HDA_INTCTL, 0x80000000u | (1u << in_streams));


  if (hda_find_codec() != 0) {
    terminal_printf("[hda] ERROR: No codec found\n");
    return;
  }

  hda_find_afg();
  hda_find_dac_pin();

  terminal_printf("[hda] programming codec...\n");
  hda_program_codec();

  g_hda_ok = 1;
  terminal_printf("[hda] ready — Realtek/HDA audio active\n");
}

