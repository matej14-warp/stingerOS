











static spinlock_t vbe_lock = SPINLOCK_INIT;

static void vbe_scroll(int lines);

int vbe_screen_width = 0;
int vbe_screen_height = 0;

static uint32_t *vbe_fb = 0;
uint32_t *g_bb = 0;
int g_pitch = 0;
static int g_active = 0;
static int g_autoflip = 0;

static int g_col = 0, g_row = 0;
static int g_tcols = 0, g_trows = 0;
static uint32_t g_fg = 0x00AAAAAA;
static uint32_t g_bg = 0x00000000;

static int g_esc_state = 0;
static int g_esc_params[8];
static int g_esc_param_count = 0;
static int g_esc_cur_valid = 0;
static int g_esc_cur_param = 0;



static uint8_t g_font[128][16];

static void init_font(void) {
  for (int i = 0; i < 128; i++) {
    for (int j = 0; j < 16; j++) {
      g_font[i][j] = font8x16[i][j];
    }
  }
}

static inline void memcpy32(uint32_t *dst, const uint32_t *src, int count) {
  __asm__ volatile("rep movsl"
                   : "+D"(dst), "+S"(src), "+c"(count)
                   :
                   : "memory");
}

static inline void memset32(uint32_t *dst, uint32_t val, int count) {
  __asm__ volatile("rep stosl" : "+D"(dst), "+c"(count) : "a"(val) : "memory");
}

int vbe_init(uint32_t mode_info_phys) {
  init_font();
  if (!mode_info_phys)
    return -1;
  vbe_mode_info_t *mi = (vbe_mode_info_t *)mode_info_phys;
  if (!mi->framebuffer || !mi->width || !mi->height)
    return -1;

  vbe_screen_width = mi->width;
  vbe_screen_height = mi->height;
  g_pitch = mi->pitch / 4;
  vbe_fb = (uint32_t *)(uintptr_t)mi->framebuffer;

  g_bb = (uint32_t *)kmalloc((size_t)(g_pitch * vbe_screen_height * 4));
  if (!g_bb)
    return -1;


  memset32(g_bb, 0, g_pitch * vbe_screen_height);
  memset32(vbe_fb, 0, g_pitch * vbe_screen_height);

  g_active = 1;
  g_autoflip = 0;
  g_tcols = vbe_screen_width / 8;
  g_trows = (vbe_screen_height - 0) / 16;

  if (g_trows * 16 > vbe_screen_height)
    g_trows--;

  return 0;
}

void vbe_init_direct(uint32_t fb_addr, uint32_t pitch, uint32_t width,
                     uint32_t height) {
  vbe_screen_width = (int)width;
  vbe_screen_height = (int)height;
  g_pitch = (int)(pitch / 4);
  vbe_fb = (uint32_t *)(uintptr_t)fb_addr;


  g_bb = (uint32_t *)kmalloc(g_pitch * height * 4);
  if (!g_bb)
    return;
  memset32(g_bb, 0, (int)(g_pitch * height));
  memset32(vbe_fb, 0, (int)(g_pitch * height));
  g_active = 1;
  g_autoflip = 0;
  g_tcols = (int)width / 8;
  g_trows = (int)height / 16;
  if (g_trows * 16 > (int)height)
    g_trows--;
}

int vbe_active(void) { return g_active; }
void vbe_set_autoflip(int on) { g_autoflip = on; }

uint32_t vbe_fb_phys_addr = 0;

void vbe_rebase_framebuffer(void) {
  if (!g_active || !vbe_fb)
    return;

  g_active = 0;

  uint32_t phys_fb0 = (uint32_t)(uintptr_t)vbe_fb;
  uint32_t fb_size = g_pitch * vbe_screen_height * 4;
  uint32_t phys_fb1 = phys_fb0 + fb_size;

  vbe_fb_phys_addr = phys_fb0;

  uint8_t *virt_fb0 = NULL;
  uint8_t *virt_fb1 = NULL;

  extern int paging_map_wc(uint32_t phys_addr, size_t size,
                           uint8_t **virt_addr);


  if (paging_map_wc(phys_fb0, fb_size, &virt_fb0) == 0 &&
      paging_map_wc(phys_fb1, fb_size, &virt_fb1) == 0) {


    if (g_bb)
      kfree(g_bb);

    vbe_fb = (uint32_t *)virt_fb0;
    g_bb = (uint32_t *)virt_fb1;

    g_active = 1;
    terminal_printf("[vbe] Page Flipping active: FB0=0x%08x FB1=0x%08x\n",
                    phys_fb0, phys_fb1);
  } else {
    terminal_printf(
        "[vbe] ERROR: Failed to map double buffers! VBE output disabled.\n");
  }
}

void vbe_put_pixel(int x, int y, uint32_t colour) {
  if (!g_active || !g_bb)
    return;
  if (x < 0 || y < 0 || x >= vbe_screen_width || y >= vbe_screen_height)
    return;
  g_bb[y * g_pitch + x] = colour;
}

uint32_t vbe_get_pixel(int x, int y) {
  if (!g_active || !g_bb)
    return 0;
  if (x < 0 || y < 0 || x >= vbe_screen_width || y >= vbe_screen_height)
    return 0;
  return g_bb[y * g_pitch + x];
}

typedef struct {
  uint32_t *dst;
  const uint32_t *src;
  uint32_t val;
  int pitch;
  int width;
  int height;
  int x, y;
} vbe_task_t;

static void parallel_vbe_clear(void *arg, int start, int end) {
  vbe_task_t *t = (vbe_task_t *)arg;
  for (int i = start; i < end; i++) {
    memset32(t->dst + i * t->pitch, t->val, t->width);
  }
}

void vbe_clear(uint32_t colour) {
  if (!g_active || !g_bb)
    return;
  vbe_task_t t = {.dst = g_bb, .val = colour, .pitch = g_pitch, .width = vbe_screen_width};
  smp_parallel_for(parallel_vbe_clear, &t, vbe_screen_height);
}

static void parallel_vbe_flip(void *arg, int start, int end) {
  vbe_task_t *t = (vbe_task_t *)arg;
  for (int i = start; i < end; i++) {
    memcpy32(vbe_fb + i * t->pitch, g_bb + i * t->pitch, t->pitch);
  }
}

void vbe_flip(void) {
  if (!g_active || !g_bb || !vbe_fb)
    return;
  vbe_task_t t = {.pitch = g_pitch};
  smp_parallel_for(parallel_vbe_flip, &t, vbe_screen_height);
}

static void parallel_vbe_fill_rect(void *arg, int start, int end) {
  vbe_task_t *t = (vbe_task_t *)arg;
  for (int i = start; i < end; i++) {
    memset32(g_bb + (t->y + i) * t->pitch + t->x, t->val, t->width);
  }
}

void vbe_fill_rect(int x, int y, int w, int h, uint32_t colour) {
  if (!g_active || !g_bb)
    return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > vbe_screen_width) w = vbe_screen_width - x;
  if (y + h > vbe_screen_height) h = vbe_screen_height - y;
  if (w <= 0 || h <= 0) return;

  vbe_task_t t = {.x = x, .y = y, .width = w, .val = colour, .pitch = g_pitch};
  smp_parallel_for(parallel_vbe_fill_rect, &t, h);
}

static void parallel_vbe_blit(void *arg, int start, int end) {
  vbe_task_t *t = (vbe_task_t *)arg;
  for (int i = start; i < end; i++) {
    memcpy32(g_bb + (t->y + i) * t->pitch + t->x, t->src + i * t->width, t->width);
  }
}

void vbe_bb_blit_rect(int x, int y, int w, int h, const uint32_t *pixels) {
  if (!g_active || !g_bb)
    return;
  if (x < 0 || y < 0 || x + w > vbe_screen_width || y + h > vbe_screen_height)
    return;

  vbe_task_t t = {.x = x, .y = y, .width = w, .src = pixels, .pitch = g_pitch};
  smp_parallel_for(parallel_vbe_blit, &t, h);
}

void vbe_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg) {
  if (!g_active || !g_bb)
    return;
  if (x < 0 || y < 0 || x + 8 > vbe_screen_width)
    goto slow;
  {
    uint8_t idx = (uint8_t)ch;
    if (idx >= 128)
      idx = '?';
    const uint8_t *glyph = g_font[idx];
    for (int row = 0; row < 16; row++) {
      int py = y + row;
      if (py >= vbe_screen_height)
        break;
      if (py < 0)
        continue;
      uint8_t bits = glyph[row];
      uint32_t *dst = &g_bb[py * g_pitch + x];
      dst[0] = (bits & 0x80) ? fg : bg;
      dst[1] = (bits & 0x40) ? fg : bg;
      dst[2] = (bits & 0x20) ? fg : bg;
      dst[3] = (bits & 0x10) ? fg : bg;
      dst[4] = (bits & 0x08) ? fg : bg;
      dst[5] = (bits & 0x04) ? fg : bg;
      dst[6] = (bits & 0x02) ? fg : bg;
      dst[7] = (bits & 0x01) ? fg : bg;
    }
    return;
  }
slow:;
  uint8_t idx = (uint8_t)ch;
  if (idx >= 128)
    idx = '?';
  const uint8_t *glyph = g_font[idx];
  for (int row = 0; row < 16; row++) {
    int py = y + row;
    if (py < 0 || py >= vbe_screen_height)
      continue;
    uint8_t bits = glyph[row];
    for (int col = 0; col < 8; col++) {
      int px = x + col;
      if (px < 0 || px >= vbe_screen_width)
        continue;
      g_bb[py * g_pitch + px] = (bits & (0x80 >> col)) ? fg : bg;
    }
  }
}

void vbe_draw_char_mask(int x, int y, char ch, uint32_t fg) {
  if (!g_active || !g_bb)
    return;
  if (x < 0 || y < 0 || x + 8 > vbe_screen_width || y + 16 > vbe_screen_height)
    return;
  uint8_t idx = (uint8_t)ch;
  if (idx >= 128)
    idx = '?';
  const uint8_t *glyph = g_font[idx];
  for (int row = 0; row < 16; row++) {
    uint8_t bits = glyph[row];
    if (!bits)
      continue;
    uint32_t *dst = &g_bb[(y + row) * g_pitch + x];
    if (bits & 0x80)
      dst[0] = fg;
    if (bits & 0x40)
      dst[1] = fg;
    if (bits & 0x20)
      dst[2] = fg;
    if (bits & 0x10)
      dst[3] = fg;
    if (bits & 0x08)
      dst[4] = fg;
    if (bits & 0x04)
      dst[5] = fg;
    if (bits & 0x02)
      dst[6] = fg;
    if (bits & 0x01)
      dst[7] = fg;
  }
}

void vbe_draw_string(const char *s, uint32_t fg, uint32_t bg) {
  int x = g_col * 8, y = g_row * 16;
  while (*s) {
    if (*s == '\n') {
      x = 0;
      g_col = 0;
      y += 16;
      g_row++;
    } else {
      vbe_draw_char(x, y, *s, fg, bg);
      x += 8;
      g_col++;
      if (g_col >= g_tcols) {
        g_col = 0;
        x = 0;
        y += 16;
        g_row++;
      }
    }
    if (g_row >= g_trows) {
      vbe_scroll(1);
      y -= 16;
      g_row--;
    }
    s++;
  }
}

void vbe_flip_rect(int x, int y, int w, int h) {
  if (!g_active || !g_bb || !vbe_fb)
    return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > vbe_screen_width)
    w = vbe_screen_width - x;
  if (y + h > vbe_screen_height)
    h = vbe_screen_height - y;
  if (w <= 0 || h <= 0)
    return;

  for (int i = 0; i < h; i++) {
    memcpy32(vbe_fb + (y + i) * g_pitch + x, g_bb + (y + i) * g_pitch + x, w);
  }
}

void vbe_enable_hw_accel(void) {  }

static uint32_t vga_to_rgb(uint8_t c) {
  static const uint32_t pal[16] = {
      0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA,
      0x00AAAA, 0xAAAAAA, 0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
      0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
  };
  return pal[c & 0xF];
}

static void vbe_scroll(int lines) {
  if (!g_active || !g_bb)
    return;
  int line_pixels = g_pitch * 16;
  int move_lines = g_trows - lines;

  if (move_lines > 0)
    memcpy32(g_bb, g_bb + lines * line_pixels, move_lines * line_pixels);


  int cleared_start = move_lines * line_pixels;
  int clear_count = lines * line_pixels;
  if (cleared_start + clear_count <= g_pitch * vbe_screen_height)
    memset32(g_bb + cleared_start, g_bg, clear_count);

  vbe_flip();
}

void vbe_terminal_init(void) {
  g_col = 0;
  g_row = 0;
  g_fg = 0x00AAAAAA;
  g_bg = 0x00000000;
  g_autoflip = 1;
  g_esc_state = 0;
  if (g_active)
    vbe_clear(g_bg);
}

void vbe_terminal_clear(void) {
  if (!g_active)
    return;
  vbe_clear(g_bg);
  g_col = 0;
  g_row = 0;
}

void vbe_terminal_setcolor(uint8_t vga_attr) {
  g_fg = vga_to_rgb(vga_attr & 0xF);
  g_bg = vga_to_rgb((vga_attr >> 4) & 0xF);
}

void vbe_terminal_update_cursor(void) {

  static int blink = 0;
  blink ^= 1;
  int x = g_col * 8, y = g_row * 16 + 14;
  uint32_t c = blink ? g_fg : g_bg;
  for (int i = 0; i < 8; i++)
    vbe_put_pixel(x + i, y, c);
  if (g_autoflip)
    vbe_flip_rect(x, g_row * 16, 8, 16);
}

static void ansi_sgr(void) {
  for (int i = 0; i < g_esc_param_count; i++) {
    int p = g_esc_params[i];
    if (p == 0) {
      g_fg = 0xAAAAAA;
      g_bg = 0x000000;
    } else if (p == 1) {
      g_fg |= 0x555555;
    } else if (p >= 30 && p <= 37)
      g_fg = vga_to_rgb((uint8_t)(p - 30));
    else if (p == 39)
      g_fg = 0xAAAAAA;
    else if (p >= 40 && p <= 47)
      g_bg = vga_to_rgb((uint8_t)(p - 40));
    else if (p == 49)
      g_bg = 0x000000;
    else if (p >= 90 && p <= 97)
      g_fg = vga_to_rgb((uint8_t)(p - 90 + 8));
    else if (p >= 100 && p <= 107)
      g_bg = vga_to_rgb((uint8_t)(p - 100 + 8));
  }
}

static void ansi_dispatch(char cmd) {
  switch (cmd) {
  case 'm':
    ansi_sgr();
    break;
  case 'A':
    g_row -= (g_esc_params[0] ? g_esc_params[0] : 1);
    if (g_row < 0)
      g_row = 0;
    break;
  case 'B':
    g_row += (g_esc_params[0] ? g_esc_params[0] : 1);
    if (g_row >= g_trows)
      g_row = g_trows - 1;
    break;
  case 'C':
    g_col += (g_esc_params[0] ? g_esc_params[0] : 1);
    if (g_col >= g_tcols)
      g_col = g_tcols - 1;
    break;
  case 'D':
    g_col -= (g_esc_params[0] ? g_esc_params[0] : 1);
    if (g_col < 0)
      g_col = 0;
    break;
  case 'H':
  case 'f':
    g_row = (g_esc_params[0] ? g_esc_params[0] - 1 : 0);
    g_col =
        (g_esc_param_count > 1 && g_esc_params[1] ? g_esc_params[1] - 1 : 0);
    break;
  case 'J':
    vbe_terminal_clear();
    break;
  case 'K':
    vbe_fill_rect(g_col * 8, g_row * 16, (g_tcols - g_col) * 8, 16, g_bg);
    if (g_autoflip)
      vbe_flip_rect(g_col * 8, g_row * 16, (g_tcols - g_col) * 8, 16);
    break;
  case 's':
    g_esc_cur_valid = 1;
    g_esc_cur_param = g_col | (g_row << 16);
    break;
  case 'u':
    if (g_esc_cur_valid) {
      g_col = g_esc_cur_param & 0xFFFF;
      g_row = (g_esc_cur_param >> 16) & 0xFFFF;
    }
    break;
  default:
    break;
  }
}

void vbe_terminal_putchar(char c) {
  spin_lock(&vbe_lock);
  if (!g_active) {
    terminal_putchar(c);
    spin_unlock(&vbe_lock);
    return;
  }


  if (g_esc_state == 1) {
    if (c == '[') {
      g_esc_state = 2;
      g_esc_param_count = 0;
      g_esc_params[0] = 0;
      spin_unlock(&vbe_lock);
      return;
    }
    g_esc_state = 0;
  }
  if (g_esc_state == 2) {
    if (c >= '0' && c <= '9') {
      g_esc_params[g_esc_param_count] =
          g_esc_params[g_esc_param_count] * 10 + (c - '0');
      spin_unlock(&vbe_lock);
      return;
    } else if (c == ';') {
      if (g_esc_param_count < 7) {
        g_esc_param_count++;
        g_esc_params[g_esc_param_count] = 0;
      }
      spin_unlock(&vbe_lock);
      return;
    } else {
      g_esc_param_count++;
      ansi_dispatch(c);
      g_esc_state = 0;
      spin_unlock(&vbe_lock);
      return;
    }
  }
  if (c == '\x1b') {
    g_esc_state = 1;
    spin_unlock(&vbe_lock);
    return;
  }

  if (c == '\n') {
    g_col = 0;
    g_row++;
  } else if (c == '\r') {
    g_col = 0;
  } else if (c == '\b') {
    if (g_col > 0) {
      g_col--;
      vbe_draw_char(g_col * 8, g_row * 16, ' ', g_fg, g_bg);
    }
  } else if (c == '\t') {
    g_col = (g_col + 8) & ~7;
    if (g_col >= g_tcols) {
      g_col = 0;
      g_row++;
    }
  } else {
    if (g_row >= g_trows) {
      vbe_scroll(1);
      g_row = g_trows - 1;
    }
    vbe_draw_char(g_col * 8, g_row * 16, c, g_fg, g_bg);
    if (g_autoflip)
      vbe_flip_rect(g_col * 8, g_row * 16, 8, 16);
    g_col++;
    if (g_col >= g_tcols) {
      g_col = 0;
      g_row++;
    }
    spin_unlock(&vbe_lock);
    return;
  }

  if (g_row >= g_trows) {
    vbe_scroll(1);
    g_row = g_trows - 1;
  }

  if (g_autoflip)
    vbe_flip();
  spin_unlock(&vbe_lock);
}

void vbe_terminal_writestring(const char *s) {
  spin_lock(&vbe_lock);
  if (!g_active) {
    while (*s)
      vbe_terminal_putchar(*s++);
    spin_unlock(&vbe_lock);
    return;
  }
  int dirty_x1 = g_col * 8, dirty_y1 = g_row * 16;
  int dirty_x2 = dirty_x1, dirty_y2 = dirty_y1 + 16;
  int had_dirty = 0;
  int old_autoflip = g_autoflip;
  g_autoflip = 0;
  while (*s) {
    int px = g_col * 8, py = g_row * 16;
    vbe_terminal_putchar(*s++);
    if (px < dirty_x1)
      dirty_x1 = px;
    if (py < dirty_y1)
      dirty_y1 = py;
    if (px + 8 > dirty_x2)
      dirty_x2 = px + 8;
    if (py + 16 > dirty_y2)
      dirty_y2 = py + 16;
    had_dirty = 1;
  }
  g_autoflip = old_autoflip;
  if (old_autoflip && had_dirty) {
    vbe_flip_rect(dirty_x1, dirty_y1, dirty_x2 - dirty_x1,
                  dirty_y2 - dirty_y1 + 16);
  }
  spin_unlock(&vbe_lock);
}

void vbe_terminal_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[512];
  int i = 0;
  while (*fmt && i < 510) {
    if (*fmt != '%') {
      buf[i++] = *fmt++;
      continue;
    }
    fmt++;
    int width = 0;
    while (*fmt >= '0' && *fmt <= '9')
      width = width * 10 + (*fmt++ - '0');
    switch (*fmt++) {
    case 'd': {
      int v = va_arg(args, int);
      char tmp[12];
      int ti = 0, neg = 0;
      if (v < 0) {
        neg = 1;
        v = -v;
      }
      if (v == 0)
        tmp[ti++] = '0';
      while (v) {
        tmp[ti++] = '0' + v % 10;
        v /= 10;
      }
      if (neg)
        tmp[ti++] = '-';
      while (ti > 0)
        buf[i++] = tmp[--ti];
      break;
    }
    case 'u': {
      unsigned v = va_arg(args, unsigned);
      char tmp[12];
      int ti = 0;
      if (v == 0)
        tmp[ti++] = '0';
      while (v) {
        tmp[ti++] = '0' + v % 10;
        v /= 10;
      }
      while (ti > 0)
        buf[i++] = tmp[--ti];
      break;
    }
    case 'x':
    case 'X': {
      unsigned v = va_arg(args, unsigned);
      char tmp[12];
      int ti = 0;
      const char *hex =
          (*(fmt - 1) == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
      if (v == 0)
        tmp[ti++] = '0';
      while (v) {
        tmp[ti++] = hex[v & 0xF];
        v >>= 4;
      }
      while (ti > 0)
        buf[i++] = tmp[--ti];
      break;
    }
    case 's': {
      const char *s = va_arg(args, const char *);
      if (!s)
        s = "(null)";
      while (*s && i < 510)
        buf[i++] = *s++;
      break;
    }
    case 'c':
      buf[i++] = (char)va_arg(args, int);
      break;
    case '%':
      buf[i++] = '%';
      break;
    default:
      break;
    }
  }
  buf[i] = 0;
  va_end(args);
  vbe_terminal_writestring(buf);
}



