











static inline int32_t qmul(int32_t a, int32_t b) {
  return (int32_t)(((int64_t)a * b) >> 15);
}

static inline int16_t clamp16(int32_t v) {
  if (v > 32767)
    return 32767;
  if (v < -32768)
    return -32768;
  return (int16_t)v;
}


static inline void br_refill(mp3_decoder_t *d) {
  while (d->bit_left <= 24 && d->pos < d->size) {
    d->bit_buf = (d->bit_buf << 8) | d->data[d->pos++];
    d->bit_left += 8;
  }
}
static inline uint32_t br_peek(mp3_decoder_t *d, int n) {
  if (n == 0)
    return 0;
  br_refill(d);
  return (d->bit_buf >> (d->bit_left - n)) & ((1u << n) - 1u);
}
static inline uint32_t br_read(mp3_decoder_t *d, int n) {
  if (n == 0)
    return 0;
  uint32_t v = br_peek(d, n);
  d->bit_left -= n;
  return v;
}


typedef struct {
  const uint8_t *buf;
  int len;
  int pos;
  uint32_t bb;
  int bl;
} rbr_t;

static inline void rbr_refill(rbr_t *r) {
  while (r->bl <= 24 && r->pos < r->len) {
    r->bb = (r->bb << 8) | r->buf[r->pos++];
    r->bl += 8;
  }
}
static inline uint32_t rbr_read(rbr_t *r, int n) {
  if (n == 0)
    return 0;
  rbr_refill(r);
  if (r->bl < n) {

    uint32_t v = (r->bb << (n - r->bl)) & ((1u << n) - 1u);
    r->bl = 0;
    return v;
  }
  uint32_t v = (r->bb >> (r->bl - n)) & ((1u << n) - 1u);
  r->bl -= n;
  return v;
}
static inline int rbr_peek1(rbr_t *r) {
  rbr_refill(r);
  if (r->bl < 1)
    return 0;
  return (int)((r->bb >> (r->bl - 1)) & 1u);
}
static inline int rbr_read1(rbr_t *r) { return (int)rbr_read(r, 1); }

static inline int rbr_bits_read(const rbr_t *r) { return r->pos * 8 - r->bl; }




static const uint32_t g_sr[4][3] = {
    {11025, 12000, 8000},
    {0, 0, 0},
    {22050, 24000, 16000},
    {44100, 48000, 32000},
};


static const int g_br_m1[16] = {0,   32,  40,  48,  56,  64,  80,  96,
                                112, 128, 160, 192, 224, 256, 320, 0};
static const int g_br_m2[16] = {0,  8,  16, 24,  32,  40,  48,  56,
                                64, 80, 96, 112, 128, 144, 160, 0};


static const uint8_t g_sfb_long_m1[3][23] = {
     {4,  4,  4,  4,  4,  4,  6,  6,  8,  8,   10, 12,
                 16, 20, 24, 28, 34, 42, 50, 54, 76, 158, 0},
     {4,  4,  4,  4,  4,  4,  6,  6,  6,  10,  10, 14,
                 14, 16, 20, 26, 32, 38, 46, 52, 60, 254, 0},
     {4,  4,  4,  4,  4,  4,  6,  6,  8,   10, 12, 16,
                 20, 24, 30, 38, 46, 56, 68, 84, 102, 26, 0},
};

static const uint8_t g_sfb_long_m2[3][23] = {
     {6,  6,  6,  6,  6,  6,  8,  10, 12, 14, 16, 18,
                 22, 26, 32, 38, 46, 54, 62, 70, 76, 36, 0},
     {6,  6,  6,  6,  6,  6,  8,  10, 12, 14, 16, 18,
                 22, 26, 32, 38, 46, 54, 62, 70, 76, 36, 0},
     {6,  6,  6,  6,  6,  6,  8,  10,  12, 14, 16, 20,
                 24, 28, 34, 42, 50, 54, 76, 158, 0,  0,  0},
};

static const uint8_t g_sfb_short_m1[3][14] = {
    {4, 4, 4, 4, 6, 8, 10, 12, 14, 18, 22, 30, 56, 0},
    {4, 4, 4, 4, 6, 6, 8, 10, 14, 18, 26, 32, 42, 0},
    {4, 4, 4, 4, 6, 8, 12, 16, 20, 26, 34, 42, 12, 0},
};
static const uint8_t g_sfb_short_m2[3][14] = {
    {4, 4, 4, 6, 6, 8, 10, 14, 18, 26, 32, 42, 18, 0},
    {4, 4, 4, 6, 8, 8, 10, 14, 18, 24, 32, 44, 12, 0},
    {4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, 0},
};


static const uint8_t g_slen[16][2] = {
    {0, 0}, {0, 1}, {0, 2}, {0, 3}, {3, 0}, {1, 1}, {1, 2}, {1, 3},
    {2, 1}, {2, 2}, {2, 3}, {3, 1}, {3, 2}, {3, 3}, {4, 2}, {4, 3},
};


static const int g_pretab[22] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0};



static uint32_t g_pow43[REQTAB_SIZE];


static int16_t g_imdct_cos[18][36];


static const int16_t g_synth_win[512] = {
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -2,
    -2,
    -2,
    -2,
    -3,
    -3,
    -4,
    -4,
    -5,
    -5,
    -6,
    -7,
    -7,
    -8,
    -9,
    -10,
    -11,
    -13,
    -14,
    -16,
    -17,
    -19,
    -21,
    -24,
    -26,
    -29,
    -31,
    -35,
    -38,
    -41,
    -45,
    -49,
    -53,
    -58,
    -63,
    -68,
    -73,
    -79,
    -85,
    -91,
    -97,
    -104,
    -111,
    -117,
    -125,
    -132,
    -139,
    -147,
    -154,
    -161,
    -169,
    -176,
    -183,
    -190,
    -196,
    -202,
    -208,
    213,
    218,
    222,
    225,
    227,
    228,
    228,
    227,
    224,
    221,
    215,
    208,
    200,
    189,
    177,
    163,
    146,
    127,
    106,
    83,
    57,
    29,
    -2,
    -36,
    -72,
    -111,
    -153,
    -197,
    -244,
    -294,
    -347,
    -401,
    -459,
    -519,
    -581,
    -645,
    -711,
    -779,
    -848,
    -919,
    -991,
    -1064,
    -1137,
    -1210,
    -1284,
    -1357,
    -1429,
    -1500,
    -1569,
    -1635,
    -1698,
    -1756,
    -1810,
    -1859,
    -1902,
    -1938,
    -1967,
    -1989,
    -2002,
    -2007,
    -2002,
    -1989,
    -1967,
    -1938,
    1902,
    1859,
    1810,
    1756,
    1698,
    1635,
    1569,
    1500,
    1429,
    1357,
    1284,
    1210,
    1137,
    1064,
    991,
    919,
    848,
    779,
    711,
    645,
    581,
    519,
    459,
    401,
    347,
    294,
    244,
    197,
    153,
    111,
    72,
    36,
    2,
    -29,
    -57,
    -83,
    -106,
    -127,
    -146,
    -163,
    -177,
    -189,
    -200,
    -208,
    -215,
    -221,
    -224,
    -227,
    -228,
    -228,
    -227,
    -225,
    -222,
    -218,
    -213,
    -208,
    -202,
    -196,
    -190,
    -183,
    -176,
    -169,
    -161,
    -154,
    147,
    139,
    132,
    125,
    117,
    111,
    104,
    97,
    91,
    85,
    79,
    73,
    68,
    63,
    58,
    53,
    49,
    45,
    41,
    38,
    35,
    31,
    29,
    26,
    24,
    21,
    19,
    17,
    16,
    14,
    13,
    11,
    10,
    9,
    8,
    7,
    7,
    6,
    5,
    5,
    4,
    4,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    4,
    4,
    5,
    5,
    6,
    7,
    7,
    8,
    9,
    10,
    11,
    13,
    14,
    16,
    17,
    19,
    21,
    24,
    26,
    29,
    31,
    35,
    38,
    41,
    45,
    49,
    53,
    58,
    63,
    68,
    73,
    79,
    85,
    91,
    97,
    104,
    111,
    117,
    125,
    132,
    139,
    147,
    154,
    161,
    169,
    176,
    183,
    190,
    196,
    202,
    208,
    213,
    218,
    222,
    225,
    227,
    228,
    228,
    227,
    224,
    221,
    215,
    208,
    200,
    189,
    177,
    163,
    146,
    127,
    106,
    83,
    57,
    29,
    -2,
    -36,
    -72,
    -111,
    -153,
    -197,
    -244,
    -294,
    -347,
    -401,
    -459,
    -519,
    -581,
    -645,
    -711,
    -779,
    -848,
    -919,
    -991,
    -1064,
    -1137,
    -1210,
    -1284,
    -1357,
    -1429,
    -1500,
    -1569,
    -1635,
    -1698,
    -1756,
    -1810,
    -1859,
    -1902,
    -1938,
    -1967,
    -1989,
    -2002,
    -2007,
    -2002,
    -1989,
    -1967,
    -1938,
    -1902,
    -1859,
    -1810,
    -1756,
    -1698,
    -1635,
    -1569,
    -1500,
    -1429,
    -1357,
    -1284,
    -1210,
    -1137,
    -1064,
    -991,
    -919,
    -848,
    -779,
    -711,
    -645,
    -581,
    -519,
    -459,
    -401,
    -347,
    -294,
    -244,
    -197,
    -153,
    -111,
    -72,
    -36,
    -2,
    29,
    57,
    83,
    106,
    127,
    146,
    163,
    177,
    189,
    200,
    208,
    215,
    221,
    224,
    227,
    228,
    228,
    227,
    225,
    222,
    218,
    213,
    208,
    202,
    196,
    190,
    183,
    176,
    169,
    161,
    154,
    147,
    139,
    132,
    125,
    117,
    111,
    104,
    97,
    91,
    85,
    79,
    73,
    68,
    63,
    58,
    53,
    49,
    45,
    41,
    38,
    35,
    31,
    29,
    26,
    24,
    21,
    19,
    17,
    16,
    14,
    13,
    11,
    10,
    9,
    8,
    7,
    7,
    6,
    5,
    5,
    4,
    4,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};



typedef struct {
  uint8_t bits;
  uint16_t code;
  int8_t x, y;
} hc_t;


static const uint8_t g_linbits[32] = {0,  0,  0, 0, 0, 0, 0, 0, 0,  0, 0,
                                      0,  0,  0, 0, 0, 1, 2, 3, 4,  6, 8,
                                      10, 13, 4, 5, 6, 7, 8, 9, 11, 13};


static const hc_t hc1[] = {{1, 0x1, 0, 0},
                           {3, 0x2, 0, 1},
                           {3, 0x3, 1, 0},
                           {5, 0x0, 1, 1},
                           {0, 0, 0, 0}};


static const hc_t hc2[] = {{1, 0x1, 0, 0}, {4, 0x5, 0, 1}, {4, 0x6, 1, 0},
                           {7, 0x7, 1, 1}, {4, 0x4, 0, 2}, {7, 0x5, 2, 0},
                           {6, 0x3, 1, 2}, {6, 0x2, 2, 1}, {8, 0x1, 2, 2},
                           {0, 0, 0, 0}};

static const hc_t hc3[] = {{2, 0x3, 0, 0}, {2, 0x2, 0, 1}, {3, 0x3, 1, 0},
                           {4, 0x3, 1, 1}, {4, 0x2, 0, 2}, {5, 0x3, 2, 0},
                           {5, 0x2, 1, 2}, {5, 0x1, 2, 1}, {6, 0x1, 2, 2},
                           {0, 0, 0, 0}};

static const hc_t hc5[] = {{1, 0x1, 0, 0},  {4, 0x5, 0, 1}, {4, 0x6, 1, 0},
                           {7, 0xE, 1, 1},  {5, 0xD, 0, 2}, {7, 0xC, 2, 0},
                           {7, 0xB, 2, 1},  {7, 0xA, 1, 2}, {8, 0x9, 2, 2},
                           {6, 0x8, 0, 3},  {8, 0x7, 3, 0}, {9, 0x6, 3, 1},
                           {9, 0x5, 1, 3},  {9, 0x4, 3, 2}, {9, 0x3, 2, 3},
                           {10, 0x2, 3, 3}, {0, 0, 0, 0}};

static const hc_t hc6[] = {{3, 0x7, 0, 0}, {3, 0x6, 0, 1}, {3, 0x5, 1, 0},
                           {5, 0x4, 1, 1}, {4, 0x3, 0, 2}, {5, 0x2, 2, 0},
                           {6, 0x1, 2, 1}, {6, 0x7, 1, 2}, {7, 0x6, 2, 2},
                           {6, 0x5, 0, 3}, {7, 0x4, 3, 0}, {7, 0x3, 3, 1},
                           {7, 0x2, 1, 3}, {8, 0x1, 3, 2}, {8, 0x7, 2, 3},
                           {8, 0x6, 3, 3}, {0, 0, 0, 0}};


static int hc_decode(rbr_t *r, const hc_t *tab, int *ox, int *oy) {

  int max_bits = 0;
  for (int i = 0; tab[i].bits != 0; i++)
    if (tab[i].bits > max_bits)
      max_bits = tab[i].bits;


  rbr_refill(r);
  uint32_t acc;
  if (r->bl >= max_bits) {
    acc = (r->bb >> (r->bl - max_bits)) & ((1u << max_bits) - 1u);
  } else {

    acc = r->bb & ((1u << r->bl) - 1u);
    acc <<= (max_bits - r->bl);
  }

  for (int i = 0; tab[i].bits != 0; i++) {
    uint32_t candidate = acc >> (max_bits - tab[i].bits);
    if (candidate == tab[i].code) {

      if (r->bl >= tab[i].bits) {
        r->bl -= tab[i].bits;
      } else {
        r->bl = 0;
      }
      *ox = tab[i].x;
      *oy = tab[i].y;
      return 0;
    }
  }

  if (r->bl > 0)
    r->bl--;
  *ox = 0;
  *oy = 0;
  return -1;
}

static void decode_big_values(rbr_t *r, int *vals, int n, int table_id) {
  const hc_t *tab;
  int linbits = 0;

  if (table_id == 0) {
    for (int i = 0; i < n; i++)
      vals[i] = 0;
    return;
  }
  switch (table_id) {
  case 1:
    tab = hc1;
    break;
  case 2:
    tab = hc2;
    break;
  case 3:
    tab = hc3;
    break;
  case 5:
    tab = hc5;
    break;
  case 6:
    tab = hc6;
    break;
  default:
    linbits = (table_id < 32) ? g_linbits[table_id] : 0;
    tab = hc6;
    break;
  }

  for (int i = 0; i + 1 < n; i += 2) {
    int x = 0, y = 0;
    hc_decode(r, tab, &x, &y);
    if (linbits && x == 15)
      x += (int)rbr_read(r, linbits);
    if (x && rbr_read1(r))
      x = -x;
    if (linbits && y == 15)
      y += (int)rbr_read(r, linbits);
    if (y && rbr_read1(r))
      y = -y;
    vals[i] = x;
    vals[i + 1] = y;
  }

  if (n & 1)
    vals[n - 1] = 0;
}


static void decode_count1(rbr_t *r, int *vals, int max_n, int table) {
  int i = 0;
  (void)table;
  while (i + 3 < max_n) {
    uint32_t c = rbr_read(r, 4);
    for (int k = 0; k < 4; k++) {
      int flag = (c >> (3 - k)) & 1;
      int v = 0;
      if (flag) {
        v = rbr_read1(r) ? -1 : 1;
      }
      vals[i + k] = v;
    }
    i += 4;
  }

  for (; i < max_n; i++)
    vals[i] = 0;
}



static uint32_t icbrt64(uint64_t n) {
  if (n == 0)
    return 0;

  uint64_t r = 1;
  uint64_t tmp = n;
  while (tmp > 7) {
    tmp >>= 3;
    r <<= 1;
  }
  r <<= 1;

  for (int iter = 0; iter < 64; iter++) {
    uint64_t r2 = r * r;
    if (r2 == 0)
      break;
    uint64_t r_new = (2 * r + n / r2) / 3;
    if (r_new >= r)
      break;
    r = r_new;
  }

  while (r > 0 && r * r * r > n)
    r--;
  return (uint32_t)r;
}

static void pow43_init(void) {
  g_pow43[0] = 0;
  for (int x = 1; x < REQTAB_SIZE; x++) {

    uint64_t n = (uint64_t)x * 16777216ULL;
    uint32_t cbrt_x256 = icbrt64(n);

    uint64_t val = (uint64_t)x * cbrt_x256 / 256;
    g_pow43[x] = (uint32_t)val;
  }
}





static const int16_t g_cos72[72] = {32767,
                                    32729,
                                    32610,
                                    32413,
                                    32137,
                                    31786,
                                    31357,
                                    30852,
                                    30273,
                                    29621,
                                    28898,
                                    28106,
                                    27245,
                                    26319,
                                    25329,
                                    24279,
                                    23170,
                                    22005,
                                    20787,
                                    19519,
                                    18204,
                                    16846,
                                    15446,
                                    14010,
                                    12539,
                                    11039,
                                    9512,
                                    7962,
                                    6393,
                                    4808,
                                    3212,
                                    1608,
                                    0,
                                    -1608,
                                    -3212,
                                    -4808,
                                    -6393,
                                    -7962,
                                    -9512,
                                    -11039,
                                    -12539,
                                    -14010,
                                    -15446,
                                    -16846,
                                    -18204,
                                    -19519,
                                    -20787,
                                    -22005,
                                    -23170,
                                    -24279,
                                    -25329,
                                    -26319,
                                    -27245,
                                    -28106,
                                    -28898,
                                    -29621,
                                    -30273,
                                    -30852,
                                    -31357,
                                    -31786,
                                    -32137,
                                    -32413,
                                    -32610,
                                    -32729,
                                    -32767,
                                    -32729,
                                    -32610,
                                    -32413,
                                    -32137,
                                    -31786,
                                    -31357,
                                    -30852,
                                     -30273,
                                    -29621,
                                    -28898,
                                    -28106};


static int16_t cos_pi_over_72(int num) {

  int k = num % 144;
  if (k < 0)
    k += 144;
  if (k < 72) {
    return g_cos72[k];
  } else {

    int kp = 144 - k;
    if (kp == 72)
      return g_cos72[72 % 72];

    if (k == 72)
      return -32767;

    return g_cos72[kp];
  }
}

static void imdct36_init(void) {
  for (int k = 0; k < 18; k++) {
    for (int n = 0; n < 36; n++) {
      int num = (2 * n + 1) * (2 * k + 1);
      g_imdct_cos[k][n] = cos_pi_over_72(num);
    }
  }
}


static void imdct36(const int32_t *in, int32_t *out) {
  for (int n = 0; n < 36; n++) {
    int64_t sum = 0;
    for (int k = 0; k < 18; k++) {
      sum += (int64_t)in[k] * (int32_t)g_imdct_cos[k][n];
    }

    out[n] = (int32_t)(sum >> 15);
  }
}



static int32_t g_overlap[2][576];



static int32_t requan(int val, int global_gain, int sf_shift_q) {
  if (val == 0)
    return 0;
  int sign = (val < 0) ? -1 : 1;
  int av = (val < 0) ? -val : val;
  if (av >= REQTAB_SIZE)
    av = REQTAB_SIZE - 1;

  uint32_t p43 = g_pow43[av];


  int exp_q = global_gain - 210 - sf_shift_q;

  int shift = exp_q >> 2;

  int32_t r;
  if (shift >= 0) {
    if (shift > 16)
      shift = 16;
    r = (int32_t)(p43 << (unsigned)shift);
  } else {
    int rshift = -shift;
    if (rshift > 24)
      return 0;
    r = (int32_t)(p43 >> (unsigned)rshift);
  }
  return sign * r;
}


typedef struct {
  int part2_3_length;
  int big_values;
  int global_gain;
  int scalefac_compress;
  int window_switching;
  int block_type;
  int mixed_block;
  int table_select[3];
  int subblock_gain[3];
  int region_address[2];
  int preflag;
  int scalefac_scale;
  int count1table_select;
} granule_t;



static int decode_scalefactors_mpeg1(rbr_t *r, granule_t *gr, int gran,
                                     const int scfsi[4],
                                     int scalefac_l_prev[22],
                                     int scalefac_l[22]) {
  int slen1 = g_slen[gr->scalefac_compress][0];
  int slen2 = g_slen[gr->scalefac_compress][1];
  int start = rbr_bits_read(r);

  if (gr->window_switching && gr->block_type == 2) {


    int bands1 = gr->mixed_block ? 8 : 6;
    for (int band = 0; band < 12; band++) {
      int slen = (band < bands1) ? slen1 : slen2;
      for (int win = 0; win < 3; win++)
        rbr_read(r, slen);
    }
    for (int b = 0; b < 22; b++)
      scalefac_l[b] = 0;
  } else {

    for (int band = 0; band < 22; band++) {
      int slen = (band < 11) ? slen1 : slen2;

      int band_grp = (band < 6) ? 0 : (band < 11) ? 1 : (band < 16) ? 2 : 3;
      if (gran > 0 && scfsi[band_grp]) {

        scalefac_l[band] = scalefac_l_prev[band];
      } else {
        scalefac_l[band] = (slen > 0) ? (int)rbr_read(r, slen) : 0;
      }
    }
  }
  return rbr_bits_read(r) - start;
}



static void decode_granule_spectral(rbr_t *r, const granule_t *gr, int mpeg1,
                                    int sr_idx, const int scalefac_l[22],
                                    int32_t xr[576]) {
  int is[576];
  for (int i = 0; i < 576; i++)
    is[i] = 0;


  int bv = gr->big_values * 2;
  if (bv > 575)
    bv = 575;
  bv &= ~1;


  const uint8_t *sfb_w = mpeg1 ? g_sfb_long_m1[sr_idx] : g_sfb_long_m2[sr_idx];
  int r0 = gr->region_address[0] + 1;
  int r1 = gr->region_address[1] + 1;
  int reg0_end = 0, reg1_end = 0;
  for (int i = 0; i < r0 && i < 22; i++)
    reg0_end += sfb_w[i];
  for (int i = 0; i < r0 + r1 && i < 22; i++)
    reg1_end += sfb_w[i];
  if (reg0_end > bv)
    reg0_end = bv;
  if (reg1_end > bv)
    reg1_end = bv;

  decode_big_values(r, is, reg0_end, gr->table_select[0]);
  decode_big_values(r, is + reg0_end, reg1_end - reg0_end, gr->table_select[1]);
  decode_big_values(r, is + reg1_end, bv - reg1_end, gr->table_select[2]);


  decode_count1(r, is + bv, 576 - bv, gr->count1table_select);


  int sf_mult =
      gr->scalefac_scale ? 4 : 2;
  int pos = 0;
  for (int band = 0; band < 21 && pos < 576; band++) {
    int width = sfb_w[band];
    int sf = scalefac_l[band] * sf_mult;
    if (gr->preflag)
      sf += g_pretab[band] * sf_mult;
    for (int j = 0; j < width && pos < 576; j++, pos++) {
      xr[pos] = requan(is[pos], gr->global_gain, sf);
    }
  }
  for (; pos < 576; pos++)
    xr[pos] = 0;
}


static void imdct_overlap(const int32_t *xr, int32_t *time_out, int ch) {
  for (int blk = 0; blk < 32; blk++) {
    int32_t tmp[36];
    imdct36(xr + blk * 18, tmp);
    for (int i = 0; i < 18; i++) {
      time_out[blk * 18 + i] = tmp[i] + g_overlap[ch][blk * 18 + i];
      g_overlap[ch][blk * 18 + i] = tmp[i + 18];
    }
  }
}


void mp3_init(mp3_decoder_t *dec, const uint8_t *data, uint32_t size) {
  dec->data = data;
  dec->size = size;
  dec->pos = 0;
  dec->bit_buf = 0;
  dec->bit_left = 0;
  dec->res_len = 0;
  dec->res_pos = 0;

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 576; j++)
      g_overlap[i][j] = 0;
  }

  pow43_init();
  imdct36_init();


  if (size >= 10 && data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
    uint32_t id3_size = ((uint32_t)(data[6] & 0x7F) << 21) |
                        ((uint32_t)(data[7] & 0x7F) << 14) |
                        ((uint32_t)(data[8] & 0x7F) << 7) |
                        ((uint32_t)(data[9] & 0x7F));
    dec->pos = 10 + id3_size;
    terminal_printf("[mp3] skipped ID3v2 tag (%u bytes)\n", id3_size + 10);
  }
}



static int find_sync(mp3_decoder_t *dec) {
  while (dec->pos + 3 < dec->size) {
    uint8_t b0 = dec->data[dec->pos];
    uint8_t b1 = dec->data[dec->pos + 1];
    if (b0 == 0xFF && (b1 & 0xE0) == 0xE0) {

      int layer = (b1 >> 1) & 3;
      if (layer == 1)
        return 1;
    }
    dec->pos++;
  }
  return 0;
}


int mp3_decode_frame(mp3_decoder_t *dec, int16_t *out, uint32_t *sample_rate,
                     int *channels) {
  if (!find_sync(dec))
    return 0;

  uint32_t hdr_pos = dec->pos;
  if (hdr_pos + 4 > dec->size)
    return 0;
  const uint8_t *h = dec->data + hdr_pos;


  int mpeg_ver = (h[1] >> 3) & 3;
  int layer = (h[1] >> 1) & 3;
  int br_idx = (h[2] >> 4) & 0xF;
  int sr_idx = (h[2] >> 2) & 3;
  int padding = (h[2] >> 1) & 1;
  int ch_mode = (h[3] >> 6) & 3;
  int nch = (ch_mode == 3) ? 1 : 2;

  if (layer != 1) {
    dec->pos++;
    return -1;
  }
  if (sr_idx == 3) {
    dec->pos++;
    return -1;
  }

  int mpeg1 = (mpeg_ver == 3);
  uint32_t sr = g_sr[mpeg_ver][sr_idx];
  if (sr == 0) {
    dec->pos++;
    return -1;
  }

  int bitrate = mpeg1 ? g_br_m1[br_idx] : g_br_m2[br_idx];
  if (bitrate == 0) {
    dec->pos++;
    return -1;
  }


  int frame_size = mpeg1 ? (144 * bitrate * 1000 / (int)sr + padding)
                         : (72 * bitrate * 1000 / (int)sr + padding);

  if ((int)dec->pos + frame_size > (int)dec->size)
    return 0;

  *sample_rate = sr;
  *channels = nch;


  int side_info_off = (int)hdr_pos + 4;
  int side_info_size = mpeg1 ? (nch == 1 ? 17 : 32) : (nch == 1 ? 9 : 17);
  int main_data_off = side_info_off + side_info_size;

  if (main_data_off > (int)dec->size) {
    dec->pos += (uint32_t)frame_size;
    return -1;
  }


  const uint8_t *si = dec->data + side_info_off;
  int mdb;
  if (mpeg1)
    mdb = ((si[0] << 1) | (si[1] >> 7)) & 0x1FF;
  else
    mdb = si[0] & 0xFF;

  int ngran = mpeg1 ? 2 : 1;


  int main_data_len = frame_size - (main_data_off - (int)hdr_pos);
  if (main_data_len < 0)
    main_data_len = 0;


  if (mdb > dec->res_len)
    mdb = dec->res_len;

  uint8_t md_buf[4096];
  int md_len = 0;


  int res_start = dec->res_len - mdb;
  for (int i = res_start; i < dec->res_len && md_len < (int)sizeof(md_buf) - 1;
       i++)
    md_buf[md_len++] = dec->reservoir[i];


  for (int i = 0; i < main_data_len && md_len < (int)sizeof(md_buf) - 1; i++)
    md_buf[md_len++] = dec->data[main_data_off + i];


  {
    int new_res_len = dec->res_len + main_data_len;
    if (new_res_len > 4090) {

      int discard = new_res_len - 4090;
      int keep = dec->res_len - discard;
      if (keep < 0) {
        keep = 0;
        discard = dec->res_len;
      }
      for (int i = 0; i < keep; i++)
        dec->reservoir[i] = dec->reservoir[i + discard];
      dec->res_len = keep;
    }
    for (int i = 0; i < main_data_len && dec->res_len < 4090; i++)
      dec->reservoir[dec->res_len++] = dec->data[main_data_off + i];
  }


  rbr_t r;
  r.buf = md_buf;
  r.len = md_len;
  r.pos = 0;
  r.bb = 0;
  r.bl = 0;


  rbr_t sir;
  sir.buf = si;
  sir.len = side_info_size;
  sir.pos = 0;
  sir.bb = 0;
  sir.bl = 0;


  rbr_read(&sir, mpeg1 ? 9 : 8);

  rbr_read(&sir, mpeg1 ? (nch == 1 ? 5 : 3) : (nch == 1 ? 5 : 3));


  int scfsi[2][4];
  for (int ch = 0; ch < nch; ch++)
    for (int b = 0; b < 4; b++)
      scfsi[ch][b] = mpeg1 ? rbr_read1(&sir) : 0;

  granule_t gr[2][2];
  for (int g = 0; g < ngran; g++) {
    for (int ch = 0; ch < nch; ch++) {
      granule_t *gp = &gr[g][ch];
      gp->part2_3_length = (int)rbr_read(&sir, 12);
      gp->big_values = (int)rbr_read(&sir, 9);
      gp->global_gain = (int)rbr_read(&sir, 8);
      gp->scalefac_compress = (int)rbr_read(&sir, mpeg1 ? 4 : 9);
      gp->window_switching = rbr_read1(&sir);
      if (gp->window_switching) {
        gp->block_type = (int)rbr_read(&sir, 2);
        gp->mixed_block = rbr_read1(&sir);
        gp->table_select[0] = (int)rbr_read(&sir, 5);
        gp->table_select[1] = (int)rbr_read(&sir, 5);
        gp->table_select[2] = 0;
        gp->subblock_gain[0] = (int)rbr_read(&sir, 3);
        gp->subblock_gain[1] = (int)rbr_read(&sir, 3);
        gp->subblock_gain[2] = (int)rbr_read(&sir, 3);

        gp->region_address[0] = 7;
        gp->region_address[1] = 36;
      } else {
        gp->block_type = 0;
        gp->mixed_block = 0;
        gp->table_select[0] = (int)rbr_read(&sir, 5);
        gp->table_select[1] = (int)rbr_read(&sir, 5);
        gp->table_select[2] = (int)rbr_read(&sir, 5);
        gp->region_address[0] = (int)rbr_read(&sir, 4);
        gp->region_address[1] = (int)rbr_read(&sir, 3);
      }
      gp->preflag = rbr_read1(&sir);
      gp->scalefac_scale = rbr_read1(&sir);
      gp->count1table_select = rbr_read1(&sir);
    }
  }


  int out_idx = 0;


  int scalefac_l[2][2][22];
  for (int g = 0; g < 2; g++)
    for (int ch = 0; ch < 2; ch++)
      for (int b = 0; b < 22; b++)
        scalefac_l[g][ch][b] = 0;


  int32_t time_ch[2][576];

  for (int g = 0; g < ngran; g++) {
    for (int ch = 0; ch < nch; ch++) {
      granule_t *gp = &gr[g][ch];


      int bits_start = rbr_bits_read(&r);


      int sf_bits = decode_scalefactors_mpeg1(
          &r, gp, g, scfsi[ch],
          (g > 0) ? scalefac_l[0][ch] : scalefac_l[0][ch],
          scalefac_l[g][ch]);


      int32_t xr[576];
      decode_granule_spectral(&r, gp, mpeg1, sr_idx, scalefac_l[g][ch], xr);


      int bits_used = rbr_bits_read(&r) - bits_start;
      int bits_remaining = gp->part2_3_length - bits_used;
      if (bits_remaining > 0 && bits_remaining < 4096)
        rbr_read(&r, bits_remaining > 32 ? 32 : bits_remaining);
      (void)sf_bits;


      imdct_overlap(xr, time_ch[ch], ch);
    }


    if (nch == 1) {
      for (int i = 0; i < 576 && out_idx < MP3_MAX_SAMPLES; i++) {
        out[out_idx++] = clamp16(time_ch[0][i] >> 3);
      }
    } else {
      for (int i = 0; i < 576 && out_idx + 1 < MP3_MAX_SAMPLES; i++) {
        out[out_idx++] = clamp16(time_ch[0][i] >> 3);
        out[out_idx++] = clamp16(time_ch[1][i] >> 3);
      }
    }
  }

  dec->pos = hdr_pos + (uint32_t)frame_size;
  return out_idx;
}

