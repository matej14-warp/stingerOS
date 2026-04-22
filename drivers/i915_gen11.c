













typedef struct {
  uint32_t mmio_base;
  uint8_t *mmio_virt;
  uint32_t aper_base;
  int initialized;
} i915_dev_t;

typedef struct {
  uint32_t *ptr;
  uint32_t phys;
  uint32_t size;
  uint32_t tail;
  int pending_kick;
  int batching;
} i915_ring_t;

static i915_dev_t g_dev = {0};
static i915_ring_t g_ring = {0};









static inline uint32_t mmio_read(uint32_t reg) {
  return *(volatile uint32_t *)(g_dev.mmio_virt + reg);
}
static inline void mmio_write(uint32_t reg, uint32_t val) {
  *(volatile uint32_t *)(g_dev.mmio_virt + reg) = val;
}






static inline void ring_emit(uint32_t dw) {
  g_ring.ptr[g_ring.tail / 4] = dw;
  g_ring.tail += 4;
  if (g_ring.tail >= g_ring.size)
    g_ring.tail = 0;
}


static inline void ring_align(void) {
  if (g_ring.tail & 4)
    ring_emit(0);
}


static void ring_kick(int force) {
  if (g_ring.batching && !force) return;
  if (!force && !g_ring.pending_kick) return;

  __asm__ volatile("sfence" ::: "memory");
  mmio_write(I915_REG_BCS_TAIL, g_ring.tail);
  g_ring.pending_kick = 0;
}

void i915_batch_begin(void) {
  g_ring.batching = 1;
}

void i915_batch_end(void) {
  g_ring.batching = 0;
  ring_kick(1);
}

void i915_flush_batch(void) {
  ring_kick(1);
}


static void ring_sync(void) {
  int timeout = 1000000;
  while (timeout--) {
    if (mmio_read(I915_REG_BCS_HEAD) == g_ring.tail)
      break;
    __asm__ volatile("pause");
  }
  if (timeout <= 0)
    terminal_printf("[i915] WARNING: BCS ring sync timeout\n");
}


static void ring_flush_and_wait(void) {
  ring_kick(1);
  ring_sync();
}






int gpu_init_i915(void) {
  terminal_printf("[i915] Initialising Intel GPU (i915 BLT engine)...\n");


  pci_device_t *gpu = NULL;
  for (int i = 0; i < g_pci_count; i++) {
    if (g_pci_devices[i].vendor == INTEL_VENDOR_ID &&
        (g_pci_devices[i].class_code == 0x03)) {
      gpu = &g_pci_devices[i];
      break;
    }
  }

  if (!gpu) {
    for (int i = 0; i < g_pci_count; i++) {
      if (g_pci_devices[i].vendor == INTEL_VENDOR_ID &&
          (g_pci_devices[i].bar[0] & 0xFFFFFFF0) > (1 << 20)) {
        gpu = &g_pci_devices[i];
        break;
      }
    }
  }
  if (!gpu) {
    terminal_printf("[i915] No Intel GPU found in PCI scan\n");
    return -1;
  }

  g_dev.mmio_base = gpu->bar[0] & 0xFFFFFFF0;
  g_dev.aper_base = gpu->bar[2] & 0xFFFFFFF0;

  terminal_printf(
      "[i915] GPU BAR0 (MMIO) phys=0x%08x  BAR2 (aper) phys=0x%08x\n",
      g_dev.mmio_base, g_dev.aper_base);


  if (paging_map_mmio(g_dev.mmio_base, 16 * 1024 * 1024, &g_dev.mmio_virt) !=
      0) {
    terminal_printf("[i915] ERROR: failed to map MMIO BAR0\n");
    return -2;
  }





  uint8_t *raw = (uint8_t *)kzalloc(RING_SIZE + RING_ALIGN);
  if (!raw) {
    terminal_printf("[i915] ERROR: ring allocation failed\n");
    return -3;
  }

  uintptr_t raw_u = (uintptr_t)raw;
  uintptr_t aligned = (raw_u + RING_ALIGN - 1) & ~(uintptr_t)(RING_ALIGN - 1);


  extern int paging_map_wc(uint32_t phys_addr, size_t size, uint8_t **virt_addr);
  uint8_t *wc_virt = NULL;
  uint32_t ring_phys = (uint32_t)aligned;

  if (paging_map_wc(ring_phys, RING_SIZE, &wc_virt) != 0) {
      terminal_printf("[i915] WARNING: failed to map ring WC, using identity\n");
      g_ring.ptr = (uint32_t *)aligned;
  } else {
      g_ring.ptr = (uint32_t *)wc_virt;
  }

  g_ring.phys = ring_phys;
  g_ring.size = RING_SIZE;
  g_ring.tail = 0;
  g_ring.pending_kick = 0;

  terminal_printf("[i915] BCS ring at phys=0x%08x size=%u KB\n", g_ring.phys,
                  RING_SIZE / 1024);


  mmio_write(I915_REG_BCS_START, g_ring.phys);
  mmio_write(I915_REG_BCS_CTL, (RING_SIZE - PAGE_SIZE) | 1u);
  mmio_write(I915_REG_BCS_HEAD, 0);
  mmio_write(I915_REG_BCS_TAIL, 0);


  uint32_t head = mmio_read(I915_REG_BCS_HEAD);
  terminal_printf("[i915] BCS HEAD after init = 0x%08x (expect 0)\n", head);

  g_dev.initialized = 1;
  terminal_printf("[i915] i915 BLT engine ready\n");
  return 0;
}







void i915_blit_fb(void *dst_ignored, void *src, int pitch_bytes, int h) {
  (void)dst_ignored;

  if (!g_dev.initialized || !src || pitch_bytes <= 0 || h <= 0)
    return;

  extern uint32_t vbe_fb_phys_addr;
  if (!vbe_fb_phys_addr) return;

  uint32_t src_phys = (uint32_t)(uintptr_t)src;
  uint32_t dst_phys = vbe_fb_phys_addr;
  uint32_t pitch = (uint32_t)pitch_bytes;
  uint32_t width_px = pitch / 4;

  ring_align();

  ring_emit(0x26 << 23 | 2);
  ring_emit(0);
  ring_emit(0);
  ring_emit(0);
  ring_align();


  ring_emit(GFX_OP_XY_FAST_COPY_BLT | (10 - 2));
  ring_emit(0x88000000u | MOCS_CACHED | pitch);
  ring_emit(0);
  ring_emit(((uint32_t)h << 16) | width_px);
  ring_emit(dst_phys);
  ring_emit(0);
  ring_emit(MOCS_CACHED | pitch);
  ring_emit(0);
  ring_emit(src_phys);
  ring_emit(0);
  ring_align();

  g_ring.pending_kick = 1;
}






void i915_blit_fb_rect(void *dst_ignored, void *src, int pitch_bytes, int dx,
                        int dy, int w, int h) {
  (void)dst_ignored;

  if (!g_dev.initialized || !src || pitch_bytes <= 0 || w <= 0 || h <= 0)
    return;

  extern uint32_t vbe_fb_phys_addr;
  if (!vbe_fb_phys_addr) return;

  uint32_t pitch = (uint32_t)pitch_bytes;
  uint32_t src_phys = (uint32_t)(uintptr_t)src + (uint32_t)dy * pitch + (uint32_t)dx * 4;
  uint32_t dst_phys = vbe_fb_phys_addr + (uint32_t)dy * pitch + (uint32_t)dx * 4;

  ring_align();



  ring_emit(GFX_OP_XY_FAST_COPY_BLT | (10 - 2));
  ring_emit(0x88000000u | MOCS_CACHED | pitch);
  ring_emit(0);
  ring_emit(((uint32_t)h << 16) | (uint32_t)w);
  ring_emit(dst_phys);
  ring_emit(0);
  ring_emit(MOCS_CACHED | pitch);
  ring_emit(0);
  ring_emit(src_phys);
  ring_emit(0);
  ring_align();

  g_ring.pending_kick = 1;
}

void i915_blit_rect_phys(uint32_t dst_phys, uint32_t src_phys,
                         uint32_t dst_pitch, uint32_t src_pitch,
                         int w, int h) {
  if (!g_dev.initialized || w <= 0 || h <= 0) return;

  ring_align();

  ring_emit(GFX_OP_XY_FAST_COPY_BLT | (10 - 2));
  ring_emit(0x88000000u | MOCS_CACHED | dst_pitch);
  ring_emit(0);
  ring_emit(((uint32_t)h << 16) | (uint32_t)w);
  ring_emit(dst_phys);
  ring_emit(0);
  ring_emit(MOCS_CACHED | src_pitch);
  ring_emit(0);
  ring_emit(src_phys);
  ring_emit(0);
  ring_align();

  g_ring.pending_kick = 1;
}




void i915_fill_rect(void *dst_phys_ptr, uint32_t pitch, int x1, int y1, int x2,
                    int y2, uint32_t color) {
  if (!g_dev.initialized)
    return;
  if (x2 <= x1 || y2 <= y1)
    return;

  uint32_t dst_phys = (uint32_t)(uintptr_t)dst_phys_ptr;

  ring_align();

  ring_emit(GFX_OP_XY_COLOR_BLT | (3u << 20) | (6 - 2));
  ring_emit((0xCCu << 16) | MOCS_CACHED | pitch);
  ring_emit(((uint32_t)y1 << 16) | (uint32_t)x1);
  ring_emit(((uint32_t)y2 << 16) | (uint32_t)x2);
  ring_emit(dst_phys);
  ring_emit(color);
  ring_align();

  g_ring.pending_kick = 1;
}






int i915_active(void) { return g_dev.initialized; }
void gpu_hw_flush(void) { ring_flush_and_wait(); }
void gpu_sync(void) { ring_sync(); }





static uint32_t *g_cursor_ptr = NULL;
static uint32_t  g_cursor_phys = 0;

void i915_hw_cursor_init(void *argb_pixels) {
  if (!g_dev.initialized) return;


  if (!g_cursor_ptr) {
    g_cursor_ptr = (uint32_t *)kzalloc(64 * 64 * 4);
    if (!g_cursor_ptr) return;
    g_cursor_phys = (uint32_t)(uintptr_t)g_cursor_ptr;
  }

  if (argb_pixels) {
    for (int i = 0; i < 64 * 64; i++) g_cursor_ptr[i] = ((uint32_t*)argb_pixels)[i];
  } else {

    for (int i = 0; i < 64 * 64; i++) g_cursor_ptr[i] = 0;
  }


  mmio_write(CUR_BASE_A, g_cursor_phys);
  mmio_write(CUR_CTL_A, (1u << 31) | 0x27);
  mmio_write(CUR_POS_A, 0);
}

void i915_hw_cursor_move(int x, int y) {
  if (!g_dev.initialized) return;

  uint32_t pos = ((uint32_t)y << 16) | (uint32_t)x;
  mmio_write(CUR_POS_A, pos);
}





void i915_fbc_init(uint32_t fb_phys, uint32_t fb_size) {
    if (!g_dev.initialized) return;


    uint32_t cfb_size = 8 * 1024 * 1024;
    void *cfb = kzalloc(cfb_size);
    if (!cfb) return;

    uint32_t cfb_phys = (uint32_t)(uintptr_t)cfb;


    mmio_write(FBC_CFB_BASE, cfb_phys);
    mmio_write(FBC_CTL, (1u << 31) | (1u << 30) | (3u << 19));

    terminal_printf("[i915] FBC enabled with %u KB CFB at 0x%08x\n", (unsigned)(cfb_size / 1024), cfb_phys);
}





void i915_gpu_wake(void) {
    if (!g_dev.initialized) return;

    mmio_write(I915_REG_FORCEWAKE_GT, 0x00010001);
}

void i915_gpu_turbo(int on) {
    if (!g_dev.initialized) return;

    if (on) mmio_write(I915_REG_RPNSWREQ, 0x1F000000);
    else    mmio_write(I915_REG_RPNSWREQ, 0x00000000);
}

void i915_page_flip(uint32_t phys_addr) {
    if (!g_dev.initialized) return;

    mmio_write(PLANE_SURF_1_A, phys_addr);

    __asm__ volatile("sfence" ::: "memory");
}





