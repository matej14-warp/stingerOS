














static uint32_t g_seed = 0xDEADBEEF;
static uint32_t rng(void) {
    g_seed = g_seed * 1664525u + 1013904223u;
    return g_seed;
}
static int rng_range(int lo, int hi) {
    return lo + (int)((rng() >> 8) % (uint32_t)(hi - lo + 1));
}

static void gc(int col, int row, uint8_t color) {
    terminal_setcolor(color);
    terminal_printf("\x1b[%d;%dH", row + 1, col + 1);
}
static void gputch(int col, int row, uint8_t color, char c) {
    gc(col, row, color);
    terminal_putchar(c);
}
static void gstr(int col, int row, uint8_t color, const char *s) {
    gc(col, row, color);
    terminal_writestring(s);
}

static void delay(int ms) {
    for (int m = 0; m < ms; m++)
        for (volatile int i = 0; i < 8000; i++);
}


static int g_itoa(int v, char *buf) {
    if (v == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
    int neg = 0, i = 0;
    char tmp[12];
    if (v < 0) { neg = 1; v = -v; }
    while (v) { tmp[i++] = '0' + v % 10; v /= 10; }
    if (neg) tmp[i++] = '-';
    int len = i;
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = 0;
    return len;
}

static void gnum(int col, int row, uint8_t color, int v) {
    char buf[12]; g_itoa(v, buf);
    gstr(col, row, color, buf);
}

















void cmd_snake(void) {
    terminal_clear();
    g_seed ^= 0xCAFEBABE;

    int sx[SN_MAX], sy[SN_MAX];
    int slen = 4;
    int dx = 1, dy = 0;
    int ndx = 1, ndy = 0;
    int fx, fy;
    int score = 0;
    int alive = 1;

    for (int i = 0; i < slen; i++) { sx[i] = 20 - i; sy[i] = SN_H / 2 + 1; }
    fx = rng_range(1, SN_W); fy = rng_range(1, SN_H);

    uint8_t bord = terminal_make_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    for (int x = 0; x < W; x++) { gputch(x, 0, bord, '-'); gputch(x, SN_H + 1, bord, '-'); }
    for (int y = 1; y <= SN_H; y++) { gputch(0, y, bord, '|'); gputch(W - 1, y, bord, '|'); }
    gstr(2, 0, COL_CYAN, "SNAKE");
    gstr(2, SN_H + 2, COL_DARK, "arrows=move  q=quit");

    while (alive) {
        char c = keyboard_poll();
        if (c == 'q' || c == 'Q') break;
        if (c == '\x1b') {
            char c2 = keyboard_poll();
            if (c2 == '[') {
                char c3 = keyboard_poll();
                if      (c3 == 'A' && dy != 1)  { ndx = 0; ndy = -1; }
                else if (c3 == 'B' && dy != -1) { ndx = 0; ndy = 1;  }
                else if (c3 == 'C' && dx != -1) { ndx = 1; ndy = 0;  }
                else if (c3 == 'D' && dx != 1)  { ndx = -1;ndy = 0;  }
            }
        }
        dx = ndx; dy = ndy;
        gputch(sx[slen - 1], sy[slen - 1], COL_NORM, ' ');
        for (int i = slen - 1; i > 0; i--) { sx[i] = sx[i-1]; sy[i] = sy[i-1]; }
        sx[0] += dx; sy[0] += dy;
        if (sx[0] < 1 || sx[0] > SN_W || sy[0] < 1 || sy[0] > SN_H) { alive = 0; break; }
        for (int i = 1; i < slen; i++)
            if (sx[0] == sx[i] && sy[0] == sy[i]) { alive = 0; break; }
        if (!alive) break;
        if (sx[0] == fx && sy[0] == fy) {
            score += 10;
            if (slen < SN_MAX) { sx[slen] = sx[slen-1]; sy[slen] = sy[slen-1]; slen++; }
            do {
                fx = rng_range(1, SN_W); fy = rng_range(1, SN_H);
                int ok = 1;
                for (int i = 0; i < slen; i++) if (sx[i]==fx && sy[i]==fy) { ok=0; break; }
                if (ok) break;
            } while (1);
            gputch(fx, fy, COL_RED, '@');
        }
        gputch(sx[0], sy[0], COL_GREEN, 'O');
        for (int i = 1; i < slen; i++) gputch(sx[i], sy[i], COL_CYAN, 'o');
        gputch(fx, fy, COL_RED, '@');
        gstr(60, 0, COL_YELLOW, "Score: ");
        gnum(67, 0, COL_WHITE, score);
        delay(120 - (score / 20 > 80 ? 80 : score / 20));
    }

    gstr(30, 12, COL_RED, "GAME OVER");
    gstr(27, 13, COL_NORM, "Score: ");
    gnum(34, 13, COL_WHITE, score);
    gstr(25, 14, COL_DARK, "press any key...");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}








static const int8_t T_PIECES[7][4][4][2] = {
     {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
     {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
     {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
     {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
     {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
     {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
     {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}},
};
static const uint8_t T_COLORS[7] = {
    VGA_COLOR_LIGHT_CYAN, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_LIGHT_MAGENTA,
    VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_RED, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_WHITE,
};

static uint8_t t_board[TH][TW];

static int t_fits(int piece, int rot, int px, int py) {
    for (int i = 0; i < 4; i++) {
        int x = px + T_PIECES[piece][rot][i][0];
        int y = py + T_PIECES[piece][rot][i][1];
        if (x < 0 || x >= TW || y >= TH) return 0;
        if (y >= 0 && t_board[y][x]) return 0;
    }
    return 1;
}

static void t_draw_cell(int bx, int by, uint8_t color, char c) {
    gputch(TBX + bx * 2,     TBY + by, color, c);
    gputch(TBX + bx * 2 + 1, TBY + by, color, c);
}

static void t_draw_board(void) {
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            if (t_board[y][x]) t_draw_cell(x, y, (uint8_t)(t_board[y][x] - 1), '
            else                t_draw_cell(x, y, COL_DARK, '.');
        }
}

static void t_draw_piece(int piece, int rot, int px, int py, char c, uint8_t color) {
    for (int i = 0; i < 4; i++) {
        int x = px + T_PIECES[piece][rot][i][0];
        int y = py + T_PIECES[piece][rot][i][1];
        if (y >= 0) t_draw_cell(x, y, color, c);
    }
}

static int t_clear_lines(void) {
    int cleared = 0;
    for (int y = TH - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < TW; x++) if (!t_board[y][x]) { full = 0; break; }
        if (full) {
            cleared++;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < TW; x++) t_board[yy][x] = t_board[yy-1][x];
            for (int x = 0; x < TW; x++) t_board[0][x] = 0;
            y++;
        }
    }
    return cleared;
}

void cmd_tetris(void) {
    terminal_clear();
    g_seed ^= 0x13371337;
    for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) t_board[y][x] = 0;

    int score = 0, level = 1, lines_total = 0;
    int piece = rng_range(0, 6), rot = 0, px = 3, py = -1;
    int next_piece = rng_range(0, 6);
    int alive = 1, tick = 0, tick_max = 30;

    uint8_t bord = COL_DARK;
    for (int y = 0; y < TH + 2; y++) {
        gputch(TBX - 1, TBY - 1 + y, bord, '|');
        gputch(TBX + TW*2, TBY - 1 + y, bord, '|');
    }
    for (int x = 0; x < TW * 2 + 2; x++) {
        gputch(TBX - 1 + x, TBY - 1, bord, '-');
        gputch(TBX - 1 + x, TBY + TH, bord, '-');
    }
    gstr(TBX + TW*2 + 3, TBY,      COL_CYAN, "TETRIS");
    gstr(TBX + TW*2 + 3, TBY + 2,  COL_NORM, "NEXT:");
    gstr(TBX + TW*2 + 3, TBY + 8,  COL_NORM, "SCORE:");
    gstr(TBX + TW*2 + 3, TBY + 10, COL_NORM, "LEVEL:");
    gstr(TBX + TW*2 + 3, TBY + 12, COL_NORM, "LINES:");
    gstr(1, H - 2, COL_DARK, "arrows=move  z/x=rotate  space=drop  q=quit");

    while (alive) {
        char c = keyboard_poll();
        if (c == 'q' || c == 'Q') break;
        if (c == '\x1b') {
            char c2 = keyboard_poll();
            if (c2 == '[') {
                char c3 = keyboard_poll();
                if (c3 == 'C' && t_fits(piece, rot, px + 1, py)) px++;
                if (c3 == 'D' && t_fits(piece, rot, px - 1, py)) px--;
                if (c3 == 'B' && t_fits(piece, rot, px, py + 1)) py++;
                if (c3 == 'A') { while (t_fits(piece, rot, px, py + 1)) py++; tick = tick_max; }
            }
        }
        if (c == 'z' || c == 'Z') { int nr=(rot+3)%4; if(t_fits(piece,nr,px,py)) rot=nr; }
        if (c == 'x' || c == 'X') { int nr=(rot+1)%4; if(t_fits(piece,nr,px,py)) rot=nr; }
        if (c == ' ') { while (t_fits(piece, rot, px, py + 1)) py++; tick = tick_max; }

        tick++;
        if (tick >= tick_max) {
            tick = 0;
            if (t_fits(piece, rot, px, py + 1)) {
                py++;
            } else {
                for (int i = 0; i < 4; i++) {
                    int bx2 = px + T_PIECES[piece][rot][i][0];
                    int by2 = py + T_PIECES[piece][rot][i][1];
                    if (by2 >= 0) t_board[by2][bx2] = (uint8_t)(T_COLORS[piece] + 1);
                }
                int cl = t_clear_lines();
                lines_total += cl;
                static const int pts[] = {0, 100, 300, 500, 800};
                score += pts[cl > 4 ? 4 : cl] * level;
                level = 1 + lines_total / 10;
                tick_max = 32 - level * 2;
                if (tick_max < 4) tick_max = 4;
                piece = next_piece; next_piece = rng_range(0, 6);
                px = 3; py = -1; rot = 0;
                if (!t_fits(piece, rot, px, py + 1)) alive = 0;
            }
        }

        t_draw_board();
        t_draw_piece(piece, rot, px, py, '
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 8; x++)
                gputch(TBX + TW*2 + 3 + x, TBY + 3 + y, COL_DARK, ' ');
        for (int i = 0; i < 4; i++) {
            int nx = T_PIECES[next_piece][0][i][0];
            int ny = T_PIECES[next_piece][0][i][1];
            gputch(TBX + TW*2 + 3 + nx*2,     TBY + 3 + ny, T_COLORS[next_piece], '
            gputch(TBX + TW*2 + 3 + nx*2 + 1, TBY + 3 + ny, T_COLORS[next_piece], '
        }
        gnum(TBX + TW*2 + 3, TBY + 9,  COL_WHITE, score);
        gnum(TBX + TW*2 + 3, TBY + 11, COL_WHITE, level);
        gnum(TBX + TW*2 + 3, TBY + 13, COL_WHITE, lines_total);
        delay(16);
    }

    gstr(TBX, TBY + TH / 2, COL_RED, "  GAME OVER  ");
    gstr(TBX, TBY + TH/2+1, COL_NORM, "press any key");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}







void cmd_pong(void) {
    terminal_clear();
    g_seed ^= 0xB00B5;

    int player_y = PG_H / 2 - PAD_H / 2;
    int cpu_y    = PG_H / 2 - PAD_H / 2;
    int p_score  = 0, c_score = 0;

    int bx = (PG_W / 2) << 8, by = (PG_H / 2) << 8;
    int bdx = (rng() & 1) ? 180 : -180;
    int bdy = (rng() & 1) ? 120 : -120;
    int alive = 1;

    uint8_t bord = COL_DARK;
    for (int x = 0; x < W; x++) { gputch(x, 0, bord, '='); gputch(x, PG_H + 1, bord, '='); }
    gstr(2,  0, COL_CYAN, "PONG");
    gstr(30, 0, COL_DARK, "w/s = move  q = quit");
    for (int y = 1; y <= PG_H; y++) gputch(PG_W/2, y, COL_DARK, ':');

    while (alive) {
        char c = keyboard_poll();
        if (c == 'q' || c == 'Q') break;
        if ((c == 'w' || c == 'W') && player_y > 1)             player_y--;
        if ((c == 's' || c == 'S') && player_y + PAD_H < PG_H) player_y++;
        if (c == '\x1b') {
            char c2 = keyboard_poll();
            if (c2 == '[') {
                char c3 = keyboard_poll();
                if (c3 == 'A' && player_y > 1)             player_y--;
                if (c3 == 'B' && player_y + PAD_H < PG_H) player_y++;
            }
        }

        int ball_row = by >> 8;
        int cpu_mid  = cpu_y + PAD_H / 2;
        if (ball_row < cpu_mid && cpu_y > 1)             cpu_y--;
        else if (ball_row > cpu_mid && cpu_y + PAD_H < PG_H) cpu_y++;

        int old_bx = (bx >> 8), old_by = (by >> 8);
        gputch(old_bx + 1, old_by + 1, COL_NORM, ' ');
        bx += bdx; by += bdy;
        if ((by >> 8) < 1)      { by = 1 << 8;          bdy = -bdy; }
        if ((by >> 8) >= PG_H)  { by = (PG_H-1) << 8;   bdy = -bdy; }

        int cx = bx >> 8, cy = by >> 8;

        if (cx >= PG_W - 3 && bdx > 0) {
            if (cy >= player_y && cy < player_y + PAD_H) {
                bdx = -bdx; bdy += rng_range(-30, 30); bx = (PG_W - 4) << 8;
            } else if (cx >= PG_W - 1) {
                c_score++; bx = (PG_W/2)<<8; by = (PG_H/2)<<8; bdx=-180; bdy=(rng()&1)?120:-120;
            }
        }
        if (cx <= 3 && bdx < 0) {
            if (cy >= cpu_y && cy < cpu_y + PAD_H) {
                bdx = -bdx; bdy += rng_range(-30, 30); bx = 4 << 8;
            } else if (cx <= 1) {
                p_score++; bx = (PG_W/2)<<8; by = (PG_H/2)<<8; bdx=180; bdy=(rng()&1)?120:-120;
            }
        }
        if (bdy > 280) bdy = 280; if (bdy < -280) bdy = -280;
        if (bdy > -40 && bdy < 40) bdy = (bdy >= 0) ? 40 : -40;
        cx = bx >> 8; cy = by >> 8;

        for (int y = 1; y <= PG_H; y++) { gputch(1, y, COL_NORM, ' '); gputch(W-3, y, COL_NORM, ' '); }
        for (int i = 0; i < PAD_H; i++) {
            gputch(1,   cpu_y    + i + 1, COL_BLUE,  '|');
            gputch(W-3, player_y + i + 1, COL_GREEN, '|');
        }
        if (cx >= 0 && cx < PG_W && cy >= 0 && cy < PG_H) gputch(cx+1, cy+1, COL_WHITE, 'O');
        gnum(PG_W/2-6, 0, COL_RED,   c_score);
        gstr(PG_W/2-4, 0, COL_DARK,  " - ");
        gnum(PG_W/2+2, 0, COL_GREEN, p_score);
        if (p_score >= 10 || c_score >= 10) break;
        delay(20);
    }

    const char *winner = (p_score >= 10) ? "YOU WIN!" : (c_score >= 10) ? "CPU WINS" : "GAME OVER";
    uint8_t wc = (p_score >= 10) ? COL_GREEN : COL_RED;
    gstr(35, 12, wc,      winner);
    gstr(33, 13, COL_NORM, "press any key");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}



static int slen_g(const char *s) { int n=0; while(s[n]) n++; return n; }
static void hline(int row, char c) {
    gc(0, row, COL_DARK);
    for (int i = 0; i < W; i++) terminal_putchar(c);
}

void cmd_sysinfo(void) {
    terminal_clear();
    uint8_t hdr = terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    gc(0, 0, hdr);
    const char *title = "  stinger OS - System Information  ";
    int tl = slen_g(title);
    for (int i = 0; i < (W - tl) / 2; i++) terminal_putchar(' ');
    terminal_writestring(title);
    for (int i = 0; i < W; i++) terminal_putchar(' ');

    int row = 2;
    gstr(2, row++, COL_CYAN, "CPU");
    hline(row++, '-');

    uint32_t eax, ebx, ecx, edx;
    char vendor[13]; vendor[12] = 0;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(0));
    ((uint32_t*)vendor)[0] = ebx; ((uint32_t*)vendor)[1] = edx; ((uint32_t*)vendor)[2] = ecx;
    gstr(4, row, COL_NORM, "Vendor : "); gstr(13, row++, COL_WHITE, vendor);

    char brand[49]; brand[48] = 0;
    uint32_t b_max;
    __asm__ volatile("cpuid" : "=a"(b_max) : "a"(0x80000000) : "ebx","ecx","edx");
    if (b_max >= 0x80000004) {
        uint32_t *bp = (uint32_t*)brand;
        for (int leaf = 0; leaf < 3; leaf++) {
            __asm__ volatile("cpuid" : "=a"(bp[0]),"=b"(bp[1]),"=c"(bp[2]),"=d"(bp[3])
                             : "a"(0x80000002 + (uint32_t)leaf));
            bp += 4;
        }
        const char *bs = brand; while (*bs == ' ') bs++;
        gstr(4, row, COL_NORM, "Brand  : "); gstr(13, row++, COL_WHITE, bs);
    }

    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1));
    int family=(int)((eax>>8)&0xF), model=(int)((eax>>4)&0xF), stepping=(int)(eax&0xF);
    char tmp[32];
    gstr(4, row, COL_NORM, "Family : ");
    g_itoa(family, tmp); gstr(13, row, COL_WHITE, tmp);
    gstr(20, row, COL_NORM, "  Model: ");
    g_itoa(model, tmp); gstr(29, row, COL_WHITE, tmp);
    gstr(36, row, COL_NORM, "  Step: ");
    g_itoa(stepping, tmp); gstr(44, row++, COL_WHITE, tmp);

    int has_sse2=(edx>>26)&1, has_sse3=(ecx)&1, has_fpu=(edx)&1;
    gstr(4, row, COL_NORM, "Flags  : ");
    gstr(13, row, has_fpu  ? COL_GREEN : COL_DARK, "FPU ");
    gstr(17, row, has_sse2 ? COL_GREEN : COL_DARK, "SSE2 ");
    gstr(22, row++, has_sse3 ? COL_GREEN : COL_DARK, "SSE3");
    row++;

    gstr(2, row++, COL_CYAN, "MEMORY");
    hline(row++, '-');
    void *probe = kmalloc(1024 * 1024);
    gstr(4, row, COL_NORM, "Heap   : ");
    if (probe) { gstr(13, row++, COL_WHITE, ">= 1 MB (kmalloc heap healthy)"); kfree(probe); }
    else        { gstr(13, row++, COL_RED, "kmalloc returned NULL (heap exhausted?)"); }
    probe = kmalloc(4 * 1024 * 1024);
    gstr(4, row, COL_NORM, "       ");
    if (probe) { gstr(13, row++, COL_WHITE, ">= 4 MB available"); kfree(probe); }
    else        { gstr(13, row++, COL_DARK, "< 4 MB contiguous free"); }
    row++;

    gstr(2, row++, COL_CYAN, "PCI DEVICES");
    hline(row++, '-');
    int found = 0;
    for (uint8_t bus = 0; bus < 4 && row < H - 3; bus++) {
        for (uint8_t dev = 0; dev < 32 && row < H - 3; dev++) {
            uint32_t id = 0;
            { uint32_t addr = 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11);
              __asm__ volatile("outl %0,%1"::"a"(addr),"Nd"((uint16_t)0xCF8));
              __asm__ volatile("inl %1,%0":"=a"(id):"Nd"((uint16_t)0xCFC)); }
            if ((id & 0xFFFF) == 0xFFFF || id == 0) continue;
            uint16_t vid=(uint16_t)(id&0xFFFF), did=(uint16_t)(id>>16);
            uint32_t cls_reg = 0;
            { uint32_t addr = 0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|0x08;
              __asm__ volatile("outl %0,%1"::"a"(addr),"Nd"((uint16_t)0xCF8));
              __asm__ volatile("inl %1,%0":"=a"(cls_reg):"Nd"((uint16_t)0xCFC)); }
            uint8_t class_code=(uint8_t)(cls_reg>>24);
            const char *cname="Unknown";
            if(class_code==0x01)cname="Storage"; else if(class_code==0x02)cname="Network";
            else if(class_code==0x03)cname="Display"; else if(class_code==0x04)cname="Multimedia";
            else if(class_code==0x06)cname="Bridge"; else if(class_code==0x0C)cname="Serial Bus";
            static const char hc[]="0123456789ABCDEF";
            char vbuf[8],dbuf[8];
            vbuf[0]='0';vbuf[1]='x';vbuf[2]=hc[(vid>>12)&0xF];vbuf[3]=hc[(vid>>8)&0xF];vbuf[4]=hc[(vid>>4)&0xF];vbuf[5]=hc[vid&0xF];vbuf[6]=0;
            dbuf[0]='0';dbuf[1]='x';dbuf[2]=hc[(did>>12)&0xF];dbuf[3]=hc[(did>>8)&0xF];dbuf[4]=hc[(did>>4)&0xF];dbuf[5]=hc[did&0xF];dbuf[6]=0;
            gc(4, row, COL_DARK); terminal_printf("%02x:%02x ", bus, dev);
            gstr(10,row,COL_WHITE,vbuf); gstr(17,row,COL_DARK,":"); gstr(18,row,COL_WHITE,dbuf); gstr(25,row++,COL_NORM,cname);
            found++;
        }
    }
    if (!found) gstr(4, row++, COL_DARK, "(none found)");
    gstr(2, H - 1, COL_DARK, "press any key to exit");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}



void cmd_memtest(void) {
    terminal_clear();
    gstr(0, 0, terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), "  stinger OS - Memory Test                                                     ");

    int row = 2;
    gstr(2, row++, COL_CYAN, "Heap stress test (alloc -> fill -> verify -> free)");
    row++;
    static const int sizes[] = { 64, 256, 1024, 4096, 16384, 65536, 0 };
    int all_ok = 1;

    for (int si = 0; sizes[si]; si++) {
        int sz = sizes[si];
        gstr(4, row, COL_NORM, "Block "); gnum(10, row, COL_WHITE, sz); gstr(18, row, COL_NORM, " bytes ... ");
        uint8_t *buf = (uint8_t*)kmalloc((size_t)sz);
        if (!buf) { gstr(30, row++, COL_RED, "ALLOC FAIL"); all_ok=0; continue; }
        for (int i = 0; i < sz; i++) buf[i] = (uint8_t)(i ^ 0xA5);
        int ok = 1;
        for (int i = 0; i < sz; i++) if (buf[i] != (uint8_t)(i ^ 0xA5)) { ok=0; break; }
        kfree(buf);
        if (ok) gstr(30, row++, COL_GREEN, "OK");
        else  { gstr(30, row++, COL_RED, "DATA ERROR"); all_ok=0; }
        delay(30);
    }

    row++;
    gstr(4, row++, COL_NORM, "Fragmentation test (128 x 32B, free odd)...");

    void *ptrs[FRAG_N];
    int alloc_ok = 1;
    for (int i = 0; i < FRAG_N; i++) { ptrs[i]=kmalloc(32); if(!ptrs[i]){alloc_ok=0;break;} if(ptrs[i])((uint8_t*)ptrs[i])[0]=(uint8_t)i; }
    if (!alloc_ok) { gstr(4, row++, COL_RED, "alloc failed during frag test"); all_ok=0; }
    else {
        for (int i=1;i<FRAG_N;i+=2) kfree(ptrs[i]);
        int realloc_ok=1;
        for (int i=1;i<FRAG_N;i+=2) { ptrs[i]=kmalloc(32); if(!ptrs[i]){realloc_ok=0;break;} }
        for (int i=0;i<FRAG_N;i++) if(ptrs[i]) kfree(ptrs[i]);
        if (realloc_ok) gstr(4, row++, COL_GREEN, "Fragmentation: OK");
        else           { gstr(4, row++, COL_RED, "Fragmentation: realloc failed"); all_ok=0; }
    }

    row++;
    if (all_ok) gstr(4, row++, terminal_make_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN), " ALL TESTS PASSED ");
    else        gstr(4, row++, terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED), " SOME TESTS FAILED ");
    gstr(2, H-1, COL_DARK, "press any key");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}





void cmd_cpubench(void) {
    terminal_clear();
    gstr(0, 0, terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE),
         "  stinger OS - CPU Benchmark                                                    ");

    int row = 2;
    gstr(2, row++, COL_CYAN, "Integer throughput");
    hline(row++, '-');

    gstr(4, row, COL_NORM, "50M integer adds   ... ");
    volatile uint32_t acc = 0;
    uint32_t t0, t1;
    __asm__ volatile("rdtsc" : "=a"(t0) :: "edx");
    for (uint32_t i = 0; i < BENCH_ITERS; i++) acc += i;
    __asm__ volatile("rdtsc" : "=a"(t1) :: "edx");
    (void)acc;
    gnum(26, row, COL_WHITE, (int)((t1-t0)/1000000)); gstr(33, row++, COL_NORM, " Mcycles");

    gstr(4, row, COL_NORM, "50M integer muls   ... ");
    volatile uint32_t acc2 = 1;
    __asm__ volatile("rdtsc" : "=a"(t0) :: "edx");
    for (uint32_t i = 1; i < BENCH_ITERS; i++) acc2 = acc2 * i + 1;
    __asm__ volatile("rdtsc" : "=a"(t1) :: "edx");
    (void)acc2;
    gnum(26, row, COL_WHITE, (int)((t1-t0)/1000000)); gstr(33, row++, COL_NORM, " Mcycles");

    row++;
    gstr(2, row++, COL_CYAN, "Memory throughput");
    hline(row++, '-');


    uint8_t *buf = (uint8_t*)kmalloc(MB_SIZE);
    if (buf) {
        gstr(4, row, COL_NORM, "1MB sequential write ... ");
        __asm__ volatile("rdtsc" : "=a"(t0) :: "edx");
        for (int i = 0; i < MB_SIZE; i++) buf[i] = (uint8_t)i;
        __asm__ volatile("rdtsc" : "=a"(t1) :: "edx");
        gnum(28, row, COL_WHITE, (int)((t1-t0)/1000000)); gstr(35, row++, COL_NORM, " Mcycles");

        gstr(4, row, COL_NORM, "1MB sequential read  ... ");
        volatile uint32_t chk = 0;
        __asm__ volatile("rdtsc" : "=a"(t0) :: "edx");
        for (int i = 0; i < MB_SIZE; i++) chk += buf[i];
        __asm__ volatile("rdtsc" : "=a"(t1) :: "edx");
        (void)chk;
        gnum(28, row, COL_WHITE, (int)((t1-t0)/1000000)); gstr(35, row++, COL_NORM, " Mcycles");
        kfree(buf);
    } else {
        gstr(4, row++, COL_RED, "kmalloc(1MB) failed -- skipping memory bench");
    }

    row++;
    gstr(2, row++, COL_CYAN, "Branch prediction");
    hline(row++, '-');
    gstr(4, row, COL_NORM, "50M branch (random)  ... ");
    volatile uint32_t br_acc = 0;
    uint32_t br_seed = 0xDEADC0DE;
    __asm__ volatile("rdtsc" : "=a"(t0) :: "edx");
    for (uint32_t i = 0; i < BENCH_ITERS; i++) { br_seed=br_seed*1664525u+1013904223u; if(br_seed&1)br_acc++; }
    __asm__ volatile("rdtsc" : "=a"(t1) :: "edx");
    (void)br_acc;
    gnum(28, row, COL_WHITE, (int)((t1-t0)/1000000)); gstr(35, row++, COL_NORM, " Mcycles");

    row++;
    gstr(2,  row, COL_DARK, "Note: Mcycles = millions of TSC cycles. Lower = faster.");
    gstr(2, H-1, COL_DARK, "press any key");
    keyboard_getchar();
    terminal_clear();
    terminal_setcolor(COL_NORM);
}










static void gg_delay(int ms){
    for(int m=0;m<ms;m++) for(volatile int i=0;i<4000;i++);
}





void cmd_snake_gui(int wx, int wy, int ww, int wh){
    int cx = wx + 2, cy = wy + 34, cw = ww - 4, ch = wh - 36;
    if(cw < 40 || ch < 40) return;

    int cell = 16;
    int cols = cw / cell, rows = ch / cell;
    if(cols < 5) cols = 5;
    if(rows < 5) rows = 5;

    int ox = cx + (cw - cols * cell) / 2;
    int oy = cy + (ch - rows * cell) / 2;

    vbe_fill_rect(cx, cy, cw, ch, _GG_COL(10, 10, 20));
    vbe_fill_rect(ox-2, oy-2, cols*cell+4, 2,           _GG_COL(40,80,40));
    vbe_fill_rect(ox-2, oy+rows*cell, cols*cell+4, 2,   _GG_COL(40,80,40));
    vbe_fill_rect(ox-2, oy-2, 2, rows*cell+4,           _GG_COL(40,80,40));
    vbe_fill_rect(ox+cols*cell, oy-2, 2, rows*cell+4,   _GG_COL(40,80,40));

    int sx[SG_MAXLEN], sy[SG_MAXLEN];
    int slen=4, dx=1, dy=0, ndx=1, ndy=0;
    int fx=rng_range(0,cols-1), fy=rng_range(0,rows-1);
    int score=0, alive=1;

    for(int i=0;i<slen&&i<SG_MAXLEN;i++){ sx[i]=cols/2-i; sy[i]=rows/2; }



        char _sb[32]; int _si=0; int _sv=score; \
        if(!_sv){_sb[_si++]='0';}else{char _t[12];int _ti=0;while(_sv){_t[_ti++]='0'+_sv%10;_sv/=10;}while(_ti>0)_sb[_si++]=_t[--_ti];} \
        _sb[_si]=0; \
        vbe_fill_rect(ox,oy-20,cols*cell,16,_GG_COL(10,10,20)); \
        vbe_draw_char(ox,   oy-20,'S',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        vbe_draw_char(ox+8, oy-20,'c',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        vbe_draw_char(ox+16,oy-20,'o',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        vbe_draw_char(ox+24,oy-20,'r',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        vbe_draw_char(ox+32,oy-20,'e',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        vbe_draw_char(ox+40,oy-20,':',_GG_COL(0,220,80),_GG_COL(10,10,20)); \
        for(int _i=0;_sb[_i];_i++) vbe_draw_char(ox+50+_i*8,oy-20,_sb[_i],_GG_COL(255,255,255),_GG_COL(10,10,20)); \
    } while(0)

    SG_CELL(fx, fy, _GG_COL(220,60,60));
    SG_SCORE_DRAW();

    while(alive){
        if(keyboard_keyready()){
            char k=keyboard_poll();
            if(k=='q'||k=='Q'||k==27) break;
            if(k=='\x1b'){ char k2=keyboard_poll(); if(k2=='['){char k3=keyboard_poll();
                if(k3=='A'&&dy!=1){ndx=0;ndy=-1;} if(k3=='B'&&dy!=-1){ndx=0;ndy=1;}
                if(k3=='C'&&dx!=-1){ndx=1;ndy=0;} if(k3=='D'&&dx!=1){ndx=-1;ndy=0;} } }
            if((k=='w'||k=='W')&&dy!=1){ndx=0;ndy=-1;} if((k=='s'||k=='S')&&dy!=-1){ndx=0;ndy=1;}
            if((k=='d'||k=='D')&&dx!=-1){ndx=1;ndy=0;} if((k=='a'||k=='A')&&dx!=1){ndx=-1;ndy=0;}
        }
        dx=ndx; dy=ndy;
        SG_CELL(sx[slen-1],sy[slen-1],_GG_COL(10,10,20));
        for(int i=slen-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}
        sx[0]+=dx; sy[0]+=dy;
        if(sx[0]<0) sx[0]=cols-1; if(sx[0]>=cols) sx[0]=0;
        if(sy[0]<0) sy[0]=rows-1; if(sy[0]>=rows) sy[0]=0;
        for(int i=1;i<slen;i++) if(sx[0]==sx[i]&&sy[0]==sy[i]){alive=0;break;}
        if(!alive) break;
        if(sx[0]==fx&&sy[0]==fy){
            score+=10;
            if(slen<SG_MAXLEN){sx[slen]=sx[slen-1];sy[slen]=sy[slen-1];slen++;}
            do{ fx=rng_range(0,cols-1);fy=rng_range(0,rows-1);
                int ok=1; for(int i=0;i<slen;i++) if(sx[i]==fx&&sy[i]==fy){ok=0;break;} if(ok)break; }while(1);
            SG_SCORE_DRAW();
        }
        SG_CELL(sx[0],sy[0],_GG_COL(0,255,80));
        for(int i=1;i<slen;i++){ int g=180-i*2; if(g<60)g=60; SG_CELL(sx[i],sy[i],_GG_COL(0,g,40)); }
        SG_CELL(fx,fy,_GG_COL(220,60,60));
        vbe_flip_rect(wx,wy,ww,wh);
        gg_delay(100-(score/30>70?70:score/30));
    }

    int gox=ox+cols*cell/2-48, goy=oy+rows*cell/2-16;
    vbe_fill_rect(gox-8,goy-8,120,52,_GG_COL(20,0,0));
    const char *go="GAME OVER"; for(int i=0;go[i];i++) vbe_draw_char(gox+i*8,goy,go[i],_GG_COL(255,80,80),_GG_COL(20,0,0));
    const char *qa="Q to close"; for(int i=0;qa[i];i++) vbe_draw_char(gox+i*8,goy+20,qa[i],_GG_COL(160,160,160),_GG_COL(20,0,0));
    vbe_flip_rect(wx,wy,ww,wh);
    for(;;){ if(keyboard_keyready()){char k=keyboard_poll();if(k=='q'||k=='Q'||k==27)break;} gg_delay(20); }


}



static uint32_t vga_to_rgb(uint8_t vgac){
    switch(vgac){
    case VGA_COLOR_LIGHT_CYAN:    return _GG_COL(0,220,220);
    case VGA_COLOR_LIGHT_BROWN:   return _GG_COL(255,200,0);
    case VGA_COLOR_LIGHT_MAGENTA: return _GG_COL(200,0,200);
    case VGA_COLOR_LIGHT_GREEN:   return _GG_COL(0,220,60);
    case VGA_COLOR_LIGHT_RED:     return _GG_COL(220,60,60);
    case VGA_COLOR_LIGHT_BLUE:    return _GG_COL(60,120,255);
    case VGA_COLOR_WHITE:         return _GG_COL(255,255,255);
    default:                      return _GG_COL(180,180,180);
    }
}

void cmd_tetris_gui(int wx, int wy, int ww, int wh){
    int cx=wx+2, cy=wy+34, cw=ww-4, ch=wh-36;
    if(cw<80||ch<100) return;

    int cell=ch/(TH+2); if(cell<4)cell=4; if(cell>24)cell=24;
    int board_pw=TW*cell, board_ph=TH*cell;
    int stats_w=120;
    int bx_off=(cw-board_pw-stats_w)/2; if(bx_off<4)bx_off=4;
    int bx=cx+bx_off, by=cy+(ch-board_ph)/2;
    int sx2=bx+board_pw+16, sy2=by;

    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) t_board[y][x]=0;
    vbe_fill_rect(cx,cy,cw,ch,_GG_COL(12,10,18));
    vbe_fill_rect(bx-2,by-2,board_pw+4,2,         _GG_COL(60,60,100));
    vbe_fill_rect(bx-2,by+board_ph,board_pw+4,2,  _GG_COL(60,60,100));
    vbe_fill_rect(bx-2,by-2,2,board_ph+4,         _GG_COL(60,60,100));
    vbe_fill_rect(bx+board_pw,by-2,2,board_ph+4,  _GG_COL(60,60,100));






        for(int _y=0;_y<TH;_y++) for(int _x=0;_x<TW;_x++){ \
            if(t_board[_y][_x]) TC_CELL(_x,_y,vga_to_rgb((uint8_t)(t_board[_y][_x]-1))); \
            else                TC_EMPTY(_x,_y); } }while(0)


        uint32_t _col=(erase)?_GG_COL(18,14,28):vga_to_rgb(T_COLORS[p2]); \
        for(int _i=0;_i<4;_i++){ int _x=(px2)+T_PIECES[p2][r2][_i][0],_y=(py2)+T_PIECES[p2][r2][_i][1]; \
            if(_y>=0&&_y<TH&&_x>=0&&_x<TW){ if(erase)TC_EMPTY(_x,_y); else TC_CELL(_x,_y,_col); } } }while(0)


        vbe_fill_rect(sx2,sy2,stats_w-8,board_ph,_GG_COL(12,10,18)); \
        vbe_draw_char(sx2,   sy2,'N',_GG_COL(180,180,180),_GG_COL(12,10,18)); \
        vbe_draw_char(sx2+8, sy2,'E',_GG_COL(180,180,180),_GG_COL(12,10,18)); \
        vbe_draw_char(sx2+16,sy2,'X',_GG_COL(180,180,180),_GG_COL(12,10,18)); \
        vbe_draw_char(sx2+24,sy2,'T',_GG_COL(180,180,180),_GG_COL(12,10,18)); \
        int _nc=(cell>12)?12:cell; \
        for(int _i=0;_i<4;_i++){ int _px3=T_PIECES[np2][0][_i][0],_py3=T_PIECES[np2][0][_i][1]; \
            vbe_fill_rect(sx2+_px3*_nc+1,sy2+20+_py3*_nc+1,_nc-2,_nc-2,vga_to_rgb(T_COLORS[np2])); } \
        const char *_lbl[]={"Score","Level","Lines"}; int _vals[]={(sc2),(lv2),(ln2)}; \
        for(int _li=0;_li<3;_li++){ int _lby=sy2+80+_li*40; \
            for(int _ci=0;_lbl[_li][_ci];_ci++) vbe_draw_char(sx2+_ci*8,_lby,_lbl[_li][_ci],_GG_COL(120,120,180),_GG_COL(12,10,18)); \
            char _nb[12];int _ni=0;int _v2=_vals[_li]; \
            if(!_v2){_nb[_ni++]='0';}else{char _t[12];int _ti=0;while(_v2){_t[_ti++]='0'+_v2%10;_v2/=10;}while(_ti>0)_nb[_ni++]=_t[--_ti];} \
            _nb[_ni]=0; for(int _ci=0;_nb[_ci];_ci++) vbe_draw_char(sx2+_ci*8,_lby+14,_nb[_ci],_GG_COL(255,255,255),_GG_COL(12,10,18)); } }while(0)

    g_seed ^= 0x13371337;
    int score=0,level=1,lines_total=0;
    int piece=rng_range(0,6),rot=0,px=3,py2=-1;
    int next_piece=rng_range(0,6);
    int alive=1,tick=0,tick_max=30;

    TC_DRAW_BOARD();
    TC_DRAW_STATS(score,level,lines_total,next_piece);
    vbe_flip_rect(wx,wy,ww,wh);

    while(alive){
        if(keyboard_keyready()){
            char k=keyboard_poll();
            if(k=='q'||k=='Q'||k==27) break;
            if(k=='\x1b'){ char k2=keyboard_poll(); if(k2=='['){char k3=keyboard_poll();
                if(k3=='C'&&t_fits(piece,rot,px+1,py2)){TC_DRAW_PIECE(piece,rot,px,py2,1);px++;}
                if(k3=='D'&&t_fits(piece,rot,px-1,py2)){TC_DRAW_PIECE(piece,rot,px,py2,1);px--;}
                if(k3=='B'&&t_fits(piece,rot,px,py2+1)){TC_DRAW_PIECE(piece,rot,px,py2,1);py2++;}
                if(k3=='A'){TC_DRAW_PIECE(piece,rot,px,py2,1);while(t_fits(piece,rot,px,py2+1))py2++;tick=tick_max;} } }
            if(k=='z'||k=='Z'){int nr=(rot+3)%4;if(t_fits(piece,nr,px,py2)){TC_DRAW_PIECE(piece,rot,px,py2,1);rot=nr;}}
            if(k=='x'||k=='X'){int nr=(rot+1)%4;if(t_fits(piece,nr,px,py2)){TC_DRAW_PIECE(piece,rot,px,py2,1);rot=nr;}}
            if(k==' '){TC_DRAW_PIECE(piece,rot,px,py2,1);while(t_fits(piece,rot,px,py2+1))py2++;tick=tick_max;}
        }
        tick++;
        if(tick>=tick_max){
            tick=0;
            if(t_fits(piece,rot,px,py2+1)){ TC_DRAW_PIECE(piece,rot,px,py2,1); py2++; }
            else{
                for(int i=0;i<4;i++){ int lx=px+T_PIECES[piece][rot][i][0],ly=py2+T_PIECES[piece][rot][i][1]; if(ly>=0)t_board[ly][lx]=(uint8_t)(T_COLORS[piece]+1); }
                int cl=t_clear_lines(); lines_total+=cl;
                static const int pts[]={0,100,300,500,800};
                score+=pts[cl>4?4:cl]*level; level=1+lines_total/10;
                tick_max=32-level*2; if(tick_max<4)tick_max=4;
                piece=next_piece; next_piece=rng_range(0,6); px=3; py2=-1; rot=0;
                if(!t_fits(piece,rot,px,py2+1)) alive=0;
                TC_DRAW_BOARD(); TC_DRAW_STATS(score,level,lines_total,next_piece);
                vbe_flip_rect(wx,wy,ww,wh); continue;
            }
        }
        TC_DRAW_PIECE(piece,rot,px,py2,0);
        vbe_flip_rect(wx,wy,ww,wh);
        gg_delay(16);
    }

    int gox=bx+board_pw/2-40, goy=by+board_ph/2-10;
    vbe_fill_rect(gox-8,goy-8,104,44,_GG_COL(20,0,0));
    const char *go2="GAME OVER"; for(int i=0;go2[i];i++) vbe_draw_char(gox+i*8,goy,go2[i],_GG_COL(255,80,80),_GG_COL(20,0,0));
    const char *qa2="Q to close"; for(int i=0;qa2[i];i++) vbe_draw_char(gox+i*8,goy+20,qa2[i],_GG_COL(180,180,180),_GG_COL(20,0,0));
    vbe_flip_rect(wx,wy,ww,wh);
    for(;;){ if(keyboard_keyready()){char k=keyboard_poll();if(k=='q'||k=='Q'||k==27)break;} gg_delay(20); }





}



void cmd_pong_gui(int wx, int wy, int ww, int wh){
    int cx=wx+2, cy=wy+34, cw=ww-4, ch=wh-36;
    if(cw<120||ch<80) return;

    vbe_fill_rect(cx,cy,cw,ch,_GG_COL(8,8,16));
    for(int y=0;y<ch;y+=12) vbe_fill_rect(cx+cw/2-1,cy+y,2,6,_GG_COL(40,40,80));

    int pad_h=ch/6, pad_w=10, ball_sz=10;
    int player_x=cx+cw-pad_w-8, cpu_x=cx+8;
    int player_y=cy+(ch-pad_h)/2, cpu_y=cy+(ch-pad_h)/2;
    int p_score=0, c_score=0;
    int bx=(cx+cw/2)<<8, by=(cy+ch/2)<<8;
    int bdx=(rng()&1)?180:-180, bdy=(rng()&1)?120:-120;
    int alive=1;


        char _nb[8];int _ni=0;int _v=(v); \
        if(!_v){_nb[_ni++]='0';}else{char _t[8];int _ti=0;while(_v){_t[_ti++]='0'+_v%10;_v/=10;}while(_ti>0)_nb[_ni++]=_t[--_ti];} \
        _nb[_ni]=0; for(int _i=0;_nb[_i];_i++) vbe_draw_char((nx)+_i*8,(ny),_nb[_i],(col),_GG_COL(8,8,16)); }while(0)

    while(alive){
        if(keyboard_keyready()){
            char k=keyboard_poll(); if(k=='q'||k=='Q'||k==27) break;
            int spd=8;
            if((k=='w'||k=='W')&&player_y-spd>=cy)             player_y-=spd;
            if((k=='s'||k=='S')&&player_y+pad_h+spd<=cy+ch)   player_y+=spd;
            if(k=='\x1b'){char k2=keyboard_poll();if(k2=='['){char k3=keyboard_poll();
                if(k3=='A'&&player_y-spd>=cy)           player_y-=spd;
                if(k3=='B'&&player_y+pad_h+spd<=cy+ch) player_y+=spd;}}
        }
        int ball_cy=(by>>8)+ball_sz/2, cpu_mid=cpu_y+pad_h/2;
        if(ball_cy<cpu_mid-2&&cpu_y>cy+2)                cpu_y-=5;
        else if(ball_cy>cpu_mid+2&&cpu_y+pad_h<cy+ch-2) cpu_y+=5;

        int obx=bx>>8, oby=by>>8;
        vbe_fill_rect(obx,oby,ball_sz,ball_sz,_GG_COL(8,8,16));
        bx+=bdx; by+=bdy;
        if((by>>8)<cy){by=cy<<8;bdy=-bdy;}
        if((by>>8)+ball_sz>=cy+ch){by=(cy+ch-ball_sz)<<8;bdy=-bdy;}

        int cbx=bx>>8,cby=by>>8;
        if(bdx>0&&cbx+ball_sz>=player_x&&cbx<player_x+pad_w){
            if(cby+ball_sz>player_y&&cby<player_y+pad_h){bdx=-bdx;bdy+=rng_range(-30,30);bx=(player_x-ball_sz)<<8;}
            else if(cbx+ball_sz>player_x+pad_w+4){c_score++;bx=(cx+cw/2)<<8;by=(cy+ch/2)<<8;bdx=-180;bdy=(rng()&1)?120:-120;}}
        if(bdx<0&&cbx<=cpu_x+pad_w&&cbx+ball_sz>cpu_x){
            if(cby+ball_sz>cpu_y&&cby<cpu_y+pad_h){bdx=-bdx;bdy+=rng_range(-30,30);bx=(cpu_x+pad_w)<<8;}
            else if(cbx<cpu_x-4){p_score++;bx=(cx+cw/2)<<8;by=(cy+ch/2)<<8;bdx=180;bdy=(rng()&1)?120:-120;}}
        if(bdy>320)bdy=320;if(bdy<-320)bdy=-320;if(bdy>-50&&bdy<50)bdy=(bdy>=0)?50:-50;
        cbx=bx>>8;cby=by>>8;

        vbe_fill_rect(cpu_x,cy,pad_w,ch,_GG_COL(8,8,16));
        vbe_fill_rect(player_x,cy,pad_w,ch,_GG_COL(8,8,16));
        vbe_fill_rect(cpu_x,cpu_y,pad_w,pad_h,_GG_COL(60,120,255));
        vbe_fill_rect(player_x,player_y,pad_w,pad_h,_GG_COL(0,220,80));
        vbe_fill_rect(cbx,cby,ball_sz,ball_sz,_GG_COL(255,255,255));
        PONG_NUM(cx+cw/2-40,cy+4,c_score,_GG_COL(100,100,255));
        PONG_NUM(cx+cw/2+24,cy+4,p_score,_GG_COL(0,220,80));
        if(p_score>=10||c_score>=10) break;
        vbe_flip_rect(wx,wy,ww,wh);
        gg_delay(16);
    }

    const char *winner=(p_score>=10)?"YOU WIN!":(c_score>=10)?"CPU WINS":"GAME OVER";
    uint32_t wc=(p_score>=10)?_GG_COL(0,220,80):_GG_COL(220,60,60);
    int gox=cx+cw/2-32, goy=cy+ch/2-16;
    vbe_fill_rect(gox-8,goy-8,104,52,_GG_COL(20,10,20));
    for(int i=0;winner[i];i++) vbe_draw_char(gox+i*8,goy,winner[i],wc,_GG_COL(20,10,20));
    const char *qa3="Q to close"; for(int i=0;qa3[i];i++) vbe_draw_char(gox+i*8,goy+20,qa3[i],_GG_COL(180,180,180),_GG_COL(20,10,20));
    vbe_flip_rect(wx,wy,ww,wh);
    for(;;){ if(keyboard_keyready()){char k=keyboard_poll();if(k=='q'||k=='Q'||k==27)break;} gg_delay(20); }

}











static const uint32_t BLK_TOP[7] = {
    0, _GG_COL(140,140,140), _GG_COL(100,70,40), _GG_COL(60,160,30),
    _GG_COL(100,60,20), _GG_COL(40,100,30), _GG_COL(210,190,100),
};
static const uint32_t BLK_SIDE[7] = {
    0, _GG_COL(110,110,110), _GG_COL(100,70,40), _GG_COL(90,55,25),
    _GG_COL(80,50,20), _GG_COL(30,80,25), _GG_COL(190,170,90),
};





static uint8_t mc_chunk[MC_CY][MC_CZ][MC_CX];

static void mc_gen_terrain(void){
    for(int y=0;y<MC_CY;y++) for(int z=0;z<MC_CZ;z++) for(int x=0;x<MC_CX;x++){
        uint32_t h_seed=(uint32_t)(x*7+z*13+0xC0FFEE);
        h_seed=h_seed*1664525u+1013904223u;
        int ground=6+(int)((h_seed>>24)%4);
        if(y>ground) mc_chunk[y][z][x]=BLK_AIR;
        else if(y==ground) mc_chunk[y][z][x]=BLK_GRASS;
        else if(y>=ground-2) mc_chunk[y][z][x]=BLK_DIRT;
        else mc_chunk[y][z][x]=BLK_STONE;
    }
    int tx=8,tz=8,ty=MC_CY-1;
    while(ty>0&&mc_chunk[ty][tz][tx]==BLK_AIR) ty--;
    ty++;
    for(int h=0;h<4&&ty+h<MC_CY;h++) mc_chunk[ty+h][tz][tx]=BLK_WOOD;
    for(int ly=-1;ly<=1;ly++) for(int lz=-2;lz<=2;lz++) for(int lx=-2;lx<=2;lx++){
        int bx2=tx+lx,bz2=tz+lz,by2=ty+3+ly;
        if(bx2>=0&&bx2<MC_CX&&bz2>=0&&bz2<MC_CZ&&by2>=0&&by2<MC_CY&&mc_chunk[by2][bz2][bx2]==BLK_AIR)
            mc_chunk[by2][bz2][bx2]=BLK_LEAVES;
    }
}


static int32_t mc_sin(int32_t deg){
    int32_t a=deg&0xFF;
    int32_t s;
    if(a<64)     { s=(a*(128-a))/16; }
    else if(a<128){ int b=a-64;  s=((64-b)*(128-(64-b)))/16; }
    else if(a<192){ int b=a-128; s=-((b*(128-b))/16); }
    else           { int b=a-192; s=-(((64-b)*(128-(64-b)))/16); }
    return s;
}
static int32_t mc_cos(int32_t deg){ return mc_sin((deg+64)&0xFF); }

void cmd_minecraft_gui(int wx, int wy, int ww, int wh){
    int cx=wx+2, cy=wy+34, cw=ww-4, ch=wh-36;
    if(cw<120||ch<80) return;

    mc_gen_terrain();


    int _ssy=MC_CY-1; while(_ssy>0&&mc_chunk[_ssy][8][8]==BLK_AIR)_ssy--;
    int32_t px_f=(8<<8), py_f=((_ssy+2)<<8), pz_f=(8<<8);
    int32_t yaw=0, pitch=0;

    uint8_t held_block=BLK_STONE;
    const uint8_t hotbar[]={BLK_STONE,BLK_DIRT,BLK_GRASS,BLK_WOOD,BLK_LEAVES,BLK_SAND};
    int hotbar_sel=0;


    int render_h=ch-24;
    int half=render_h/2;

    int rh=render_h>0?render_h:1;







        int _hby=cy+ch-24; \
        vbe_fill_rect(cx,_hby,cw,24,_GG_COL(30,30,30)); \
        int _sw=24, _hbx=cx+cw/2-(6*_sw)/2; \
        for(int _i=0;_i<6;_i++){ \
            int _sx3=_hbx+_i*_sw; \
            uint32_t _border=(_i==hotbar_sel)?_GG_COL(255,200,0):_GG_COL(80,80,80); \
            vbe_fill_rect(_sx3,_hby+2,_sw-2,20,BLK_SIDE[hotbar[_i]]); \
            vbe_fill_rect(_sx3,_hby+2,_sw-2,1,_border); vbe_fill_rect(_sx3,_hby+21,_sw-2,1,_border); \
            vbe_fill_rect(_sx3,_hby+2,1,20,_border); vbe_fill_rect(_sx3+_sw-3,_hby+2,1,20,_border); } \
        int _chx=cx+cw/2,_chhy=cy+ch/2; \
        vbe_fill_rect(_chx-8,_chhy-1,16,2,_GG_COL(255,255,255)); \
        vbe_fill_rect(_chx-1,_chhy-8,2,16,_GG_COL(255,255,255)); \
        const char *_hint="WASD+QE=move  Mouse=look  LClick=break  RClick=place  1-6=block  Esc=quit"; \
        int _hlen=0; while(_hint[_hlen])_hlen++; \
        int _hx=cx+(cw-_hlen*8)/2; if(_hx<cx)_hx=cx; \
        vbe_fill_rect(_hx-4,cy+2,_hlen*8+8,14,_GG_COL(0,0,0)); \
        for(int _i=0;_hint[_i];_i++) vbe_draw_char(_hx+_i*8,cy+3,_hint[_i],_GG_COL(200,200,200),_GG_COL(0,0,0)); \
    } while(0)



        int32_t _cs=mc_cos(yaw), _ss=mc_sin(yaw); \
        vbe_fill_rect(cx,cy,cw,render_h,_GG_COL(0,0,0)); \
        for(int _sx3=0;_sx3<cw;_sx3++){ \
            int _cam=(_sx3-cw/2)*256/((cw/2)+1); \
            int32_t _rdx=(_cs*256+_ss*_cam)>>8, _rdz=(_ss*256-_cs*_cam)>>8; \
            int32_t _rx=px_f>>8, _rz2=pz_f>>8, _fx2=px_f&0xFF, _fz2=pz_f&0xFF; \
            int32_t _step_x=(_rdx>=0)?1:-1, _step_z=(_rdz>=0)?1:-1; \
            int32_t _td_x=(_rdx!=0)?(256*256/(_rdx<0?-_rdx:_rdx)):0x7FFF; \
            int32_t _td_z=(_rdz!=0)?(256*256/(_rdz<0?-_rdz:_rdz)):0x7FFF; \
            int32_t _side_x,_side_z; \
            if(_rdx==0){_side_x=0x7FFF;}else if(_rdx>0){_side_x=(256-_fx2)*256/_rdx;}else{_side_x=(_fx2>0)?(_fx2*256/-_rdx):_td_x;} \
            if(_rdz==0){_side_z=0x7FFF;}else if(_rdz>0){_side_z=(256-_fz2)*256/_rdz;}else{_side_z=(_fz2>0)?(_fz2*256/-_rdz):_td_z;} \
            int32_t _mx=_rx,_mz=_rz2; \
            int _hit=0,_hface=0; int32_t _pdist=1; int _hblk=BLK_AIR,_hy=0; \
            for(int _steps=0;_steps<32&&!_hit;_steps++){ \
                if(_side_x<_side_z){_pdist=_side_x;_side_x+=_td_x;_mx+=_step_x;_hface=0;} \
                else{_pdist=_side_z;_side_z+=_td_z;_mz+=_step_z;_hface=1;} \
                if(_mx<0||_mx>=MC_CX||_mz<0||_mz>=MC_CZ) break; \
                for(int _vy=MC_CY-1;_vy>=0;_vy--){ if(mc_chunk[_vy][_mz][_mx]!=BLK_AIR){ _hit=1;_hblk=mc_chunk[_vy][_mz][_mx];_hy=_vy;break; } } } \
            if(_hit&&_pdist>1){ \
                int _proj_h=(render_h*256)/_pdist; \
                if(_proj_h<1)_proj_h=1; \
                if(_proj_h>render_h*8)_proj_h=render_h*8; \
                 \
                int _cam_y=py_f>>8; \
                int _dtop=half-(_hy+1-_cam_y)*_proj_h-(pitch*3); \
                int _dbot=_dtop+_proj_h; \
                int _vtop = _dtop < 0 ? 0 : (_dtop > render_h ? render_h : _dtop); \
                int _vbot = _dbot < 0 ? 0 : (_dbot > render_h ? render_h : _dbot); \
                uint32_t _base = (!_hface) ? BLK_TOP[_hblk] : BLK_SIDE[_hblk]; \
                if(_hface==0){ \
                    int _r2=(int)((_base>>16)&0xFF)*3/4; \
                    int _g2=(int)((_base>>8)&0xFF)*3/4; \
                    int _b2=(int)(_base&0xFF)*3/4; \
                    _base=_GG_COL(_r2,_g2,_b2); \
                } \
                int _ds=_pdist>>8; \
                if(_ds>10)_ds=10; \
                int _fade=10-_ds; \
                int _r2=(int)((_base>>16)&0xFF)*(_fade+4)/14; \
                int _g2=(int)((_base>>8)&0xFF)*(_fade+4)/14; \
                int _b2=(int)(_base&0xFF)*(_fade+4)/14; \
                if(_r2>255)_r2=255; if(_g2>255)_g2=255; if(_b2>255)_b2=255; \
                uint32_t _wc=_GG_COL(_r2,_g2,_b2); \
                for(int _sy3=0;_sy3<render_h;_sy3++){ \
                    if(_sy3<_vtop) vbe_put_pixel(cx+_sx3,cy+_sy3,MC_SKY(_sy3)); \
                    else if(_sy3<_vbot) vbe_put_pixel(cx+_sx3,cy+_sy3,_wc); \
                    else { \
                        int _d=_sy3-(half-pitch*3); \
                        uint32_t _fc=_GG_COL(65-_d/6>0?65-_d/6:0,55,35); \
                        vbe_put_pixel(cx+_sx3,cy+_sy3,_fc); \
                    } \
                } \
            } else { \
                int _horizon=half-pitch*3; \
                for(int _sy3=0;_sy3<render_h;_sy3++){ \
                    uint32_t _col; \
                    if(_sy3<_horizon) _col=MC_SKY(_sy3); \
                    else{ int _d=_sy3-_horizon; _col=_GG_COL(65-_d/6>0?65-_d/6:0,55,35); } \
                    vbe_put_pixel(cx+_sx3,cy+_sy3,_col); } } } \
        MC_DRAW_HUD(); \
        vbe_flip_rect(wx,wy,ww,wh); \
    } while(0)

    MC_RENDER_FRAME();

    mouse_x=wx+ww/2; mouse_y=wy+wh/2;

    for(;;){
        int moved=0;

        while(keyboard_keyready()){
            char k=keyboard_poll();
            if(k=='\x1b'){
                if(keyboard_keyready()){
                    char k2=keyboard_poll();
                    if(k2=='['&&keyboard_keyready()){
                        char k3=keyboard_poll();
                        if(k3=='A'){pitch-=4;if(pitch<-24)pitch=-24;moved=1;}
                        if(k3=='B'){pitch+=4;if(pitch>24)pitch=24;moved=1;}

                        if(k3=='C'){yaw=(yaw-6+256)&0xFF;moved=1;}

                        if(k3=='D'){yaw=(yaw+6)&0xFF;moved=1;}
                    }
                } else { goto mc_quit; }
                continue;
            }
            int spd=20;

            if(k=='w'||k=='W'){ px_f+=(mc_cos(yaw)*spd)>>8; pz_f+=(mc_sin(yaw)*spd)>>8; moved=1; }
            if(k=='s'||k=='S'){ px_f-=(mc_cos(yaw)*spd)>>8; pz_f-=(mc_sin(yaw)*spd)>>8; moved=1; }

            if(k=='a'||k=='A'){ px_f-=(mc_sin(yaw)*spd)>>8; pz_f+=(mc_cos(yaw)*spd)>>8; moved=1; }
            if(k=='d'||k=='D'){ px_f+=(mc_sin(yaw)*spd)>>8; pz_f-=(mc_cos(yaw)*spd)>>8; moved=1; }
            if(k=='q'||k=='Q'){ py_f+=spd; moved=1; }
            if(k=='e'||k=='E'){ py_f-=spd; moved=1; }
            if(k>='1'&&k<='6'){ hotbar_sel=k-'1'; held_block=hotbar[hotbar_sel]; moved=1; }

            if((px_f>>8)<0)px_f=0; if((px_f>>8)>=MC_CX)px_f=(MC_CX-1)<<8;
            if((pz_f>>8)<0)pz_f=0; if((pz_f>>8)>=MC_CZ)pz_f=(MC_CZ-1)<<8;
            if((py_f>>8)<0)py_f=0; if((py_f>>8)>=MC_CY)py_f=(MC_CY-1)<<8;
        }

        mouse_poll_backends();
        {
            mouse_event_t ev;
            while(mouse_poll(&ev)){

                if(ev.dx!=0){ yaw=(int32_t)(yaw-ev.dx/MC_MOUSE_YAW_DIV+256)&0xFF; moved=1; }

                if(ev.dy!=0){ pitch+=ev.dy/MC_MOUSE_PITCH_DIV; if(pitch<-24)pitch=-24; if(pitch>24)pitch=24; moved=1; }
                if(ev.buttons&MOUSE_BTN_LEFT){
                    int tx2=(px_f>>8)+(int)((mc_cos(yaw)*3)>>8);
                    int tz2=(pz_f>>8)+(int)((mc_sin(yaw)*3)>>8);
                    int ty2=(py_f>>8);
                    if(tx2>=0&&tx2<MC_CX&&tz2>=0&&tz2<MC_CZ&&ty2>=0&&ty2<MC_CY)
                        mc_chunk[ty2][tz2][tx2]=BLK_AIR;
                    moved=1;
                }
                if(ev.buttons&MOUSE_BTN_RIGHT){
                    int tx2=(px_f>>8)+(int)((mc_cos(yaw)*3)>>8);
                    int tz2=(pz_f>>8)+(int)((mc_sin(yaw)*3)>>8);
                    int ty2=(py_f>>8)+1;
                    if(tx2>=0&&tx2<MC_CX&&tz2>=0&&tz2<MC_CZ&&ty2>=0&&ty2<MC_CY&&mc_chunk[ty2][tz2][tx2]==BLK_AIR)
                        mc_chunk[ty2][tz2][tx2]=held_block;
                    moved=1;
                }
                if(ev.dz!=0){ hotbar_sel=(hotbar_sel-ev.dz+6)%6; held_block=hotbar[hotbar_sel]; moved=1; }
            }
        }

        if(moved){ MC_RENDER_FRAME(); }
        gg_delay(16);
    }
mc_quit:;
    vbe_fill_rect(cx,cy,cw,ch,_GG_COL(12,12,20));
    vbe_flip_rect(wx,wy,ww,wh);





}




