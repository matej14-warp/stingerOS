












static uint32_t bitbuf = 0;
static int      bitcnt = 0;
static const uint8_t *zin;
static size_t   zin_pos, zin_len;
static uint8_t *zout;
static size_t   zout_pos, zout_cap;

static void zreset(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap) {
    bitbuf=0; bitcnt=0; zin=in; zin_pos=0; zin_len=inlen;
    zout=out; zout_pos=0; zout_cap=outcap;
}

static uint32_t zbits(int n) {
    while (bitcnt < n) {
        if (zin_pos < zin_len) bitbuf |= (uint32_t)zin[zin_pos++] << bitcnt;
        bitcnt += 8;
    }
    uint32_t v = bitbuf & ((1u << n) - 1);
    bitbuf >>= n; bitcnt -= n;
    return v;
}


static int decode_lit_fixed(void) {

    uint32_t code = 0;
    for (int i = 0; i < 7; i++) code = (code << 1) | zbits(1);
    if (code <= 23)       return (int)(256 + code);
    code = (code << 1) | zbits(1);
    if (code >= 0x30 && code <= 0xBF) return (int)(code - 0x30);
    if (code >= 0xC0 && code <= 0xC7) return (int)(280 + code - 0xC0);
    code = (code << 1) | zbits(1);
    return (int)(144 + code - 0x190);
}

static const uint16_t len_base[]  = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const uint8_t  len_extra[] = {0,0,0,0,0,0,0,0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0};
static const uint16_t dist_base[] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const uint8_t  dist_extra[]= {0,0,0,0,1,1,2,2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13};

static int inflate_fixed_block(void) {
    for (;;) {
        int sym = decode_lit_fixed();
        if (sym < 0) return -1;
        if (sym == 256) return 0;
        if (sym < 256) {
            if (zout_pos < zout_cap) zout[zout_pos++] = (uint8_t)sym;
        } else {
            int li = sym - 257;
            if (li < 0 || li >= 29) return -1;
            uint32_t length = len_base[li] + zbits(len_extra[li]);
            uint32_t dc = zbits(5);

            uint32_t dc_r = 0;
            for (int i=0;i<5;i++) dc_r=(dc_r<<1)|(dc>>i&1);
            if (dc_r >= 30) return -1;
            uint32_t dist = dist_base[dc_r] + zbits(dist_extra[dc_r]);
            for (uint32_t i = 0; i < length; i++) {
                if (zout_pos < zout_cap && zout_pos >= dist)
                    zout[zout_pos] = zout[zout_pos - dist];
                zout_pos++;
            }
        }
    }
}

static int zlib_inflate(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap) {
    if (inlen < 2) return -1;

    zreset(in + 2, inlen - 2, out, outcap);
    for (;;) {
        int bfinal = (int)zbits(1);
        int btype  = (int)zbits(2);
        if (btype == 0) {

            bitcnt = 0; bitbuf = 0;
            if (zin_pos + 4 > zin_len) return -1;
            uint16_t len  = (uint16_t)(zin[zin_pos] | (zin[zin_pos+1]<<8)); zin_pos+=2;
             zin_pos+=2;
            for (uint16_t i=0;i<len;i++) {
                if (zout_pos < zout_cap && zin_pos < zin_len)
                    zout[zout_pos++] = zin[zin_pos++];
            }
        } else if (btype == 1) {
            if (inflate_fixed_block() < 0) return -1;
        } else {

            return -1;
        }
        if (bfinal) break;
    }
    return (int)zout_pos;
}


static uint32_t png_read_u32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}


static uint8_t paeth(uint8_t a, uint8_t b, uint8_t c) {
    int p = (int)a + b - c;
    int pa = p-(int)a; if(pa<0) pa=-pa;
    int pb = p-(int)b; if(pb<0) pb=-pb;
    int pc = p-(int)c; if(pc<0) pc=-pc;
    if (pa<=pb && pa<=pc) return a;
    if (pb<=pc) return b;
    return c;
}

int png_decode(const uint8_t *data, size_t len, rgba_t **out,
               uint32_t *width, uint32_t *height) {
    if (!data || len < 8) return -1;


    uint32_t w=0,h=0,depth=0,ctype=0;
    uint8_t *idat_raw = (uint8_t*)kmalloc(len);
    if (!idat_raw) return -1;
    size_t idat_len = 0;


    size_t pos = 8;
    while (pos + 12 <= len) {
        uint32_t chlen  = png_read_u32(data+pos);   pos+=4;
        uint32_t chtype = png_read_u32(data+pos);   pos+=4;
        const uint8_t *chdata = data+pos;           pos+=chlen;
                                           pos+=4;
        if (pos > len) break;

        if (chtype == PNG_CHUNK_IHDR) {
            if (chlen < 13) { kfree(idat_raw); return -1; }
            w=png_read_u32(chdata); h=png_read_u32(chdata+4);
            depth=chdata[8]; ctype=chdata[9];
        } else if (chtype == PNG_CHUNK_IDAT) {
            if (idat_len + chlen <= len) {
                for (uint32_t i=0;i<chlen;i++) idat_raw[idat_len++]=chdata[i];
            }
        } else if (chtype == PNG_CHUNK_IEND) {
            break;
        }
    }

    if (!w||!h||!idat_len) { kfree(idat_raw); return -1; }


    int channels;
    switch(ctype) {
    case 0: channels=1; break;
    case 2: channels=3; break;
    case 3: channels=1; break;
    case 4: channels=2; break;
    case 6: channels=4; break;
    default: kfree(idat_raw); return -1;
    }
    if (depth != 8) { kfree(idat_raw); return -1; }

    int stride = (int)w * channels + 1;
    uint8_t *raw = (uint8_t*)kmalloc((size_t)stride * h);
    if (!raw) { kfree(idat_raw); return -1; }

    int rv = zlib_inflate(idat_raw, idat_len, raw, (size_t)stride*h);
    kfree(idat_raw);
    if (rv < 0) { kfree(raw); return -1; }

    rgba_t *pixels = (rgba_t*)kmalloc(w*h*sizeof(rgba_t));
    if (!pixels) { kfree(raw); return -1; }


    for (uint32_t y=0;y<h;y++) {
        uint8_t *row  = raw + y * stride;
        uint8_t *prev = (y>0) ? raw + (y-1)*stride + 1 : NULL;
        uint8_t ftype = row[0];
        uint8_t *s    = row+1;
        int bpp = channels;

        for (int x=0;x<(int)w*bpp;x++) {
            uint8_t a = (x>=bpp) ? s[x-bpp] : 0;
            uint8_t b = prev ? prev[x] : 0;
            uint8_t c = (prev && x>=bpp) ? prev[x-bpp] : 0;
            switch(ftype) {
            case 0: break;
            case 1: s[x]=(uint8_t)(s[x]+a); break;
            case 2: s[x]=(uint8_t)(s[x]+b); break;
            case 3: s[x]=(uint8_t)(s[x]+(a+b)/2); break;
            case 4: s[x]=(uint8_t)(s[x]+paeth(a,b,c)); break;
            }
        }

        for (uint32_t x=0;x<w;x++) {
            rgba_t *px = &pixels[y*w+x];
            switch(ctype) {
            case 0: px->r=px->g=px->b=s[x]; px->a=255; break;
            case 2: px->r=s[x*3]; px->g=s[x*3+1]; px->b=s[x*3+2]; px->a=255; break;
            case 4: px->r=px->g=px->b=s[x*2]; px->a=s[x*2+1]; break;
            case 6: px->r=s[x*4]; px->g=s[x*4+1]; px->b=s[x*4+2]; px->a=s[x*4+3]; break;
            default: px->r=px->g=px->b=0; px->a=255; break;
            }
        }
    }

    kfree(raw);
    *out = pixels; *width=w; *height=h;
    return 0;
}



uint8_t rgba_to_vga16(rgba_t p) {

    if (p.a < 128) return 0;

    static const uint8_t pal[16][3] = {
        {0,0,0},{0,0,170},{0,170,0},{0,170,170},
        {170,0,0},{170,0,170},{170,85,0},{170,170,170},
        {85,85,85},{85,85,255},{85,255,85},{85,255,255},
        {255,85,85},{255,85,255},{255,255,85},{255,255,255}
    };
    int best=0, bestd=0x7FFFFFFF;
    for (int i=0;i<16;i++) {
        int dr=(int)p.r-pal[i][0], dg=(int)p.g-pal[i][1], db=(int)p.b-pal[i][2];
        int d=dr*dr+dg*dg+db*db;
        if(d<bestd){bestd=d;best=i;}
    }
    return (uint8_t)best;
}

void png_render_ansi(const rgba_t *pixels, uint32_t w, uint32_t h) {

    uint32_t disp_w = w > 78 ? 78 : w;
    uint32_t disp_h = h > 44 ? 44 : h;


    static const uint8_t fg_codes[16] = {30,34,32,36,31,35,33,37,90,94,92,96,91,95,93,97};
    static const uint8_t bg_codes[16] = {40,44,42,46,41,45,43,47,100,104,102,106,101,105,103,107};

    terminal_writestring("\x1b[2J\x1b[H");

    for (uint32_t py = 0; py < disp_h; py += 2) {
        for (uint32_t px = 0; px < disp_w; px++) {

            uint32_t sx = px * w / disp_w;
            uint32_t sy_top = py     * h / disp_h;
            uint32_t sy_bot = (py+1) * h / disp_h;

            rgba_t top_px = pixels[sy_top * w + sx];
            rgba_t bot_px = (sy_bot < h) ? pixels[sy_bot * w + sx] : top_px;

            uint8_t top_col = rgba_to_vga16(top_px);
            uint8_t bot_col = rgba_to_vga16(bot_px);


            terminal_printf("\x1b[%d;%dm\xe2\x96\x84",
                            fg_codes[bot_col], bg_codes[top_col]);
        }
        terminal_writestring("\x1b[0m\n");
    }
    terminal_writestring("\x1b[0m");
}

void cmd_view_png(const char *path) {
    sfs_node_t *node = sfs_resolve(sfs_root(), path);
    if (!node || node->type != SFS_FILE) {
        terminal_printf("view: file not found: %s\n", path);
        return;
    }

    terminal_printf("Loading %s (%u bytes)...\n", path, (unsigned)node->size);

    rgba_t *pixels = NULL;
    uint32_t w=0, h=0;

    if (png_decode((const uint8_t*)node->data, node->size, &pixels, &w, &h) < 0) {
        terminal_printf("view: failed to decode PNG (unsupported format or corrupt file)\n");
        terminal_printf("      supported: 8-bit RGB and RGBA PNG only\n");
        return;
    }

    terminal_printf("Image: %ux%u pixels\n", w, h);
    png_render_ansi(pixels, w, h);
    kfree(pixels);
    terminal_printf("\nPress Enter...");
    keyboard_getchar();
    terminal_clear();
}


static const char *palette_names[16] = {
    "Black","Blue","Green","Cyan","Red","Magenta","Brown","LtGrey",
    "DkGrey","LtBlue","LtGreen","LtCyan","LtRed","LtMag","Yellow","White"
};
static const uint8_t palette_fg[16] = {30,34,32,36,31,35,33,37,90,94,92,96,91,95,93,97};
static const uint8_t palette_bg[16] = {40,44,42,46,41,45,43,47,100,104,102,106,101,105,103,107};

static void canvas_redraw(canvas_t *c) {
    terminal_clear();
    terminal_writestring("\x1b[H");


    terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    terminal_printf("  draw — %s%s  [arrows=move  SPACE=paint  0-9=color  s=save  q=quit]  ",
                    c->filename[0] ? c->filename : "untitled",
                    c->modified ? "*" : "");
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_putchar('\n');


    for (int y = 0; y < CANVAS_H; y++) {
        terminal_writestring("  ");
        for (int x = 0; x < CANVAS_W; x++) {
            uint8_t col = c->pixels[y][x];
            int is_cursor = (x == c->cursor_x && y == c->cursor_y);
            if (is_cursor) {
                terminal_printf("\x1b[%dm\x1b[%dm[]", palette_fg[15], palette_bg[col]);
            } else {
                terminal_printf("\x1b[%dm  ", palette_bg[col]);
            }
        }
        terminal_writestring("\x1b[0m\n");
    }


    terminal_writestring("\x1b[0m  Palette: ");
    for (int i = 0; i < 16; i++) {
        if (i == c->color)
            terminal_printf("\x1b[%dm[%X]\x1b[0m", palette_bg[i], i);
        else
            terminal_printf("\x1b[%dm %X \x1b[0m", palette_bg[i], i);
    }
    terminal_printf("  Current: \x1b[%dm  \x1b[0m %s  Pos: %d,%d\n",
                    palette_bg[c->color], palette_names[c->color],
                    c->cursor_x, c->cursor_y);
}

int canvas_save(canvas_t *c) {

    const char *fname = c->filename[0] ? c->filename : "drawing.ppm";

    size_t ppm_size = 20 + (size_t)CANVAS_W * CANVAS_H * 3;
    uint8_t *buf = (uint8_t*)kmalloc(ppm_size + 64);
    if (!buf) return -1;


    static const uint8_t vga_rgb[16][3] = {
        {0,0,0},{0,0,170},{0,170,0},{0,170,170},
        {170,0,0},{170,0,170},{170,85,0},{170,170,170},
        {85,85,85},{85,85,255},{85,255,85},{85,255,255},
        {255,85,85},{255,85,255},{255,255,85},{255,255,255}
    };


    int bi = 0;
    const char *hdr_str = "P6\n40 20\n255\n";
    for (const char *h = hdr_str; *h; h++) buf[bi++] = (uint8_t)*h;
    for (int y=0;y<CANVAS_H;y++) for (int x=0;x<CANVAS_W;x++) {
        uint8_t ci = c->pixels[y][x];
        buf[bi++]=vga_rgb[ci][0]; buf[bi++]=vga_rgb[ci][1]; buf[bi++]=vga_rgb[ci][2];
    }

    sfs_write_file(sfs_root(), fname, buf, (size_t)bi);
    kfree(buf);
    c->modified = 0;
    terminal_printf("\nSaved to %s (%d bytes)\n", fname, bi);
    return 0;
}

int canvas_load(canvas_t *c, const char *filename) {
    sfs_node_t *node = sfs_resolve(sfs_root(), filename);
    if (!node || node->type != SFS_FILE) return -1;

    for (int y=0;y<CANVAS_H;y++) for (int x=0;x<CANVAS_W;x++) c->pixels[y][x]=0;
    return 0;
}

void cmd_draw(const char *filename) {
    canvas_t *c = (canvas_t*)kmalloc(sizeof(canvas_t));
    if (!c) { terminal_printf("draw: out of memory\n"); return; }
    for (int y=0;y<CANVAS_H;y++) for(int x=0;x<CANVAS_W;x++) c->pixels[y][x]=0;
    c->cursor_x=0; c->cursor_y=0; c->color=15; c->modified=0;

    if (filename && filename[0]) {
        size_t fl = 0; while(filename[fl]) fl++;
        for (size_t i=0;i<fl&&i<63;i++) c->filename[i]=filename[i];
        c->filename[fl]=0;
        canvas_load(c, filename);
    } else {
        c->filename[0]=0;
    }

    canvas_redraw(c);

    while (1) {
        char ch = keyboard_getchar();

        if (ch == 'q' || ch == 'Q') {
            if (c->modified) {
                terminal_printf("\nUnsaved changes. Save? [y/n]: ");
                char r = keyboard_getchar();
                terminal_putchar(r); terminal_putchar('\n');
                if (r=='y'||r=='Y') canvas_save(c);
            }
            break;
        }
        if (ch == 's' || ch == 'S') {
            if (!c->filename[0]) {
                terminal_printf("\nSave as: ");

                int fi=0; char nc2;
                while ((nc2=keyboard_getchar())!='\n'&&nc2!='\r'&&fi<63) {
                    c->filename[fi++]=nc2; terminal_putchar(nc2);
                }
                c->filename[fi]=0;
            }
            canvas_save(c);
            canvas_redraw(c);
            continue;
        }
        if (ch == ' ') {
            c->pixels[c->cursor_y][c->cursor_x] = (uint8_t)c->color;
            c->modified = 1;
            canvas_redraw(c);
            continue;
        }

        if (ch>='0'&&ch<='9') { c->color=ch-'0'; canvas_redraw(c); continue; }
        if (ch>='a'&&ch<='f') { c->color=10+(ch-'a'); canvas_redraw(c); continue; }


        if (ch == 0x1b) {
            char c2 = keyboard_getchar();
            if (c2 == '[') {
                char c3 = keyboard_getchar();
                if (c3=='A'&&c->cursor_y>0)          c->cursor_y--;
                if (c3=='B'&&c->cursor_y<CANVAS_H-1) c->cursor_y++;
                if (c3=='C'&&c->cursor_x<CANVAS_W-1) c->cursor_x++;
                if (c3=='D'&&c->cursor_x>0)          c->cursor_x--;
                canvas_redraw(c);
            }
        }
    }

    kfree(c);
    terminal_clear();
}



