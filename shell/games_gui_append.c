

/* ================================================================
   GUI GAMES — VBE FRAMEBUFFER RENDERERS
   These are called directly from yaoigui.c with the window client
   rectangle.  They take over the framebuffer inside that rect for
   the duration of the game, flipping only the window region.
   ================================================================ */

#include "../kernel/vbe.h"
#include "../drivers/mouse.h"

/* ---- colour macro (same as yaoigui) ---- */
#ifndef _GG_COL
#define _GG_COL(r,g,b) ((uint32_t)(((unsigned)(r)<<16)|((unsigned)(g)<<8)|(unsigned)(b)))
#endif

/* ---- tiny delay for GUI games ---- */
static void gg_delay(int ms){
    for(int m=0;m<ms;m++) for(volatile int i=0;i<4000;i++);
}

/* ---- rng (shared with terminal games via same g_seed) ---- */
/* g_seed already defined above; just use rng() / rng_range() */

/* ================================================================
   SNAKE GUI
   Renders coloured blocks directly into the window client area.
   Arrow keys / WASD.  Q / Escape quits.
   ================================================================ */

#define SGW_MAX 160   /* max cells wide */
#define SGH_MAX  90   /* max cells tall */
#define SG_MAXLEN 2000

void cmd_snake_gui(int wx, int wy, int ww, int wh){
    /* title bar already drawn by yaoigui; wx/wy/ww/wh = full window rect.
       Compute client area below 32-px title bar. */
    int cx = wx + 2;
    int cy = wy + 34;     /* TITLE_H = 32, +2 border */
    int cw = ww - 4;
    int ch = wh - 36;
    if(cw < 40 || ch < 40) return;

    /* cell size: target ~20 cols / 15 rows minimum */
    int cell = 16;
    int cols = cw / cell;
    int rows = ch / cell;
    if(cols < 5) cols = 5;
    if(rows < 5) rows = 5;

    /* canvas origin — centred */
    int ox = cx + (cw - cols * cell) / 2;
    int oy = cy + (ch - rows * cell) / 2;

    /* background */
    vbe_fill_rect(cx, cy, cw, ch, _GG_COL(10, 10, 20));

    /* grid border */
    vbe_fill_rect(ox - 2, oy - 2, cols * cell + 4, 2,             _GG_COL(40,80,40));
    vbe_fill_rect(ox - 2, oy + rows * cell, cols * cell + 4, 2,   _GG_COL(40,80,40));
    vbe_fill_rect(ox - 2, oy - 2, 2,             rows * cell + 4, _GG_COL(40,80,40));
    vbe_fill_rect(ox + cols * cell, oy - 2, 2,   rows * cell + 4, _GG_COL(40,80,40));

    /* state */
    int sx[SG_MAXLEN], sy[SG_MAXLEN];
    int slen = 4;
    int dx = 1, dy = 0;
    int ndx = 1, ndy = 0;
    int fx = rng_range(0, cols - 1);
    int fy = rng_range(0, rows - 1);
    int score = 0;
    int alive = 1;

    /* place initial snake */
    for(int i = 0; i < slen && i < SG_MAXLEN; i++){
        sx[i] = cols / 2 - i;
        sy[i] = rows / 2;
    }

    /* helper: draw one cell */
    #define SG_CELL(gx,gy,col) vbe_fill_rect(ox+(gx)*cell+1, oy+(gy)*cell+1, cell-2, cell-2, col)

    /* draw initial food */
    SG_CELL(fx, fy, _GG_COL(220, 60, 60));

    /* score text helper */
    char sg_sbuf[32];
    int sg_si;
    #define SG_SCORE_DRAW() do { \
        sg_si = 0; \
        { int v = score; if(!v){ sg_sbuf[sg_si++]='0'; } \
          else { char t[12]; int ti=0; while(v){ t[ti++]='0'+v%10; v/=10; } \
                 while(ti>0) sg_sbuf[sg_si++]=t[--ti]; } } \
        sg_sbuf[sg_si] = 0; \
        vbe_fill_rect(ox, oy - 20, cols * cell, 16, _GG_COL(10,10,20)); \
        vbe_draw_char(ox, oy - 20, 'S', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        vbe_draw_char(ox+8, oy-20, 'c', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        vbe_draw_char(ox+16,oy-20,'o', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        vbe_draw_char(ox+24,oy-20,'r', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        vbe_draw_char(ox+32,oy-20,'e', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        vbe_draw_char(ox+40,oy-20,':', _GG_COL(0,220,80), _GG_COL(10,10,20)); \
        for(int _sci=0;sg_sbuf[_sci];_sci++) \
            vbe_draw_char(ox+50+_sci*8, oy-20, sg_sbuf[_sci], _GG_COL(255,255,255), _GG_COL(10,10,20)); \
    } while(0)

    SG_SCORE_DRAW();

    while(alive){
        /* input */
        if(keyboard_keyready()){
            char k = keyboard_poll();
            if(k == 'q' || k == 'Q' || k == 27) break;
            if(k == '\x1b'){
                char k2 = keyboard_poll();
                if(k2 == '['){
                    char k3 = keyboard_poll();
                    if(k3 == 'A' && dy != 1)  { ndx = 0; ndy = -1; }
                    if(k3 == 'B' && dy != -1) { ndx = 0; ndy =  1; }
                    if(k3 == 'C' && dx != -1) { ndx = 1; ndy =  0; }
                    if(k3 == 'D' && dx != 1)  { ndx =-1; ndy =  0; }
                }
            }
            if((k=='w'||k=='W') && dy!=1)  { ndx=0; ndy=-1; }
            if((k=='s'||k=='S') && dy!=-1) { ndx=0; ndy= 1; }
            if((k=='d'||k=='D') && dx!=-1) { ndx=1; ndy= 0; }
            if((k=='a'||k=='A') && dx!=1)  { ndx=-1;ndy= 0; }
        }
        dx = ndx; dy = ndy;

        /* erase tail */
        SG_CELL(sx[slen-1], sy[slen-1], _GG_COL(10,10,20));

        /* move */
        for(int i = slen-1; i > 0; i--){ sx[i]=sx[i-1]; sy[i]=sy[i-1]; }
        sx[0] += dx; sy[0] += dy;

        /* wall wrap (no death on wall — modern snake style) */
        if(sx[0] < 0)     sx[0] = cols - 1;
        if(sx[0] >= cols) sx[0] = 0;
        if(sy[0] < 0)     sy[0] = rows - 1;
        if(sy[0] >= rows) sy[0] = 0;

        /* self collision */
        for(int i = 1; i < slen; i++)
            if(sx[0]==sx[i] && sy[0]==sy[i]){ alive=0; break; }
        if(!alive) break;

        /* food */
        if(sx[0]==fx && sy[0]==fy){
            score += 10;
            if(slen < SG_MAXLEN){
                sx[slen]=sx[slen-1]; sy[slen]=sy[slen-1]; slen++;
            }
            do {
                fx=rng_range(0,cols-1); fy=rng_range(0,rows-1);
                int ok=1;
                for(int i=0;i<slen;i++) if(sx[i]==fx&&sy[i]==fy){ok=0;break;}
                if(ok) break;
            } while(1);
            SG_SCORE_DRAW();
        }

        /* draw */
        /* head: bright green */
        SG_CELL(sx[0], sy[0], _GG_COL(0, 255, 80));
        /* body: darker green gradient */
        for(int i = 1; i < slen; i++){
            int g = 180 - i * 2; if(g < 60) g = 60;
            SG_CELL(sx[i], sy[i], _GG_COL(0, g, 40));
        }
        /* food */
        SG_CELL(fx, fy, _GG_COL(220, 60, 60));

        /* partial flip of window region */
        vbe_flip_rect(wx, wy, ww, wh);

        gg_delay(100 - (score / 30 > 70 ? 70 : score / 30));
    }

    /* game over overlay */
    int gox = ox + cols * cell / 2 - 48;
    int goy = oy + rows * cell / 2 - 16;
    vbe_fill_rect(gox - 8, goy - 8, 120, 52, _GG_COL(20,0,0));
    vbe_fill_rect(gox - 8, goy - 8, 120, 2,  _GG_COL(220,60,60));
    vbe_fill_rect(gox - 8, goy+44, 120, 2,   _GG_COL(220,60,60));
    const char *go = "GAME OVER";
    for(int i=0; go[i]; i++) vbe_draw_char(gox+i*8, goy, go[i], _GG_COL(255,80,80), _GG_COL(20,0,0));
    const char *qa = "Q to close";
    for(int i=0; qa[i]; i++) vbe_draw_char(gox+i*8, goy+20, qa[i], _GG_COL(160,160,160), _GG_COL(20,0,0));
    vbe_flip_rect(wx, wy, ww, wh);
    /* wait for Q/Esc */
    for(;;){
        if(keyboard_keyready()){ char k=keyboard_poll(); if(k=='q'||k=='Q'||k==27) break; }
        gg_delay(20);
    }
    #undef SG_CELL
    #undef SG_SCORE_DRAW
}

/* ================================================================
   TETRIS GUI
   Pieces rendered as coloured blocks in the window client area.
   Controls: arrows (left/right/soft-drop/hard-drop), Z/X rotate, Q quit.
   ================================================================ */

/* Reuse T_PIECES / T_COLORS from terminal tetris above */

/* Map VGA color index → 32-bit RGB for GUI */
static uint32_t vga_to_rgb(uint8_t vgac){
    switch(vgac){
    case VGA_COLOR_LIGHT_CYAN:     return _GG_COL(0,220,220);
    case VGA_COLOR_LIGHT_BROWN:    return _GG_COL(255,200,0);
    case VGA_COLOR_LIGHT_MAGENTA:  return _GG_COL(200,0,200);
    case VGA_COLOR_LIGHT_GREEN:    return _GG_COL(0,220,60);
    case VGA_COLOR_LIGHT_RED:      return _GG_COL(220,60,60);
    case VGA_COLOR_LIGHT_BLUE:     return _GG_COL(60,120,255);
    case VGA_COLOR_WHITE:          return _GG_COL(255,255,255);
    default:                       return _GG_COL(180,180,180);
    }
}

void cmd_tetris_gui(int wx, int wy, int ww, int wh){
    int cx = wx + 2;
    int cy = wy + 34;
    int cw = ww - 4;
    int ch = wh - 36;
    if(cw < 80 || ch < 100) return;

    /* cell size to fit TW×TH board */
    int cell = ch / (TH + 2);
    if(cell < 4) cell = 4;
    if(cell > 24) cell = 24;

    int board_pw = TW * cell;
    int board_ph = TH * cell;
    /* board left edge — leave room on right for stats */
    int stats_w = 120;
    int bx_off = (cw - board_pw - stats_w) / 2;
    if(bx_off < 4) bx_off = 4;
    int bx = cx + bx_off;
    int by = cy + (ch - board_ph) / 2;

    /* clear board */
    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) t_board[y][x]=0;

    /* clear bg */
    vbe_fill_rect(cx, cy, cw, ch, _GG_COL(12, 10, 18));

    /* board border */
    vbe_fill_rect(bx-2, by-2, board_pw+4, 2,            _GG_COL(60,60,100));
    vbe_fill_rect(bx-2, by+board_ph, board_pw+4, 2,     _GG_COL(60,60,100));
    vbe_fill_rect(bx-2, by-2, 2,            board_ph+4, _GG_COL(60,60,100));
    vbe_fill_rect(bx+board_pw, by-2, 2,     board_ph+4, _GG_COL(60,60,100));

    /* draw helpers */
    #define TC_CELL(gx,gy,col) vbe_fill_rect(bx+(gx)*cell+1, by+(gy)*cell+1, cell-2, cell-2, col)
    #define TC_EMPTY(gx,gy)    vbe_fill_rect(bx+(gx)*cell, by+(gy)*cell, cell, cell, _GG_COL(18,14,28))

    auto void draw_board_gui(void);
    void draw_board_gui(void){
        for(int y=0;y<TH;y++)
            for(int x=0;x<TW;x++){
                if(t_board[y][x]) TC_CELL(x,y, vga_to_rgb((uint8_t)(t_board[y][x]-1)));
                else               TC_EMPTY(x,y);
            }
    }

    auto void draw_piece_gui(int piece,int rot,int px2,int py2,int erase);
    void draw_piece_gui(int piece,int rot,int px2,int py2,int erase){
        uint32_t col = erase ? _GG_COL(18,14,28) : vga_to_rgb(T_COLORS[piece]);
        for(int i=0;i<4;i++){
            int x=px2+T_PIECES[piece][rot][i][0];
            int y=py2+T_PIECES[piece][rot][i][1];
            if(y>=0 && y<TH && x>=0 && x<TW){
                if(erase) TC_EMPTY(x,y);
                else      TC_CELL(x,y,col);
            }
        }
    }

    /* stats panel */
    int sx2 = bx + board_pw + 16;
    int sy2 = by;

    auto void draw_stats(int score2, int level2, int lines2, int next_piece2);
    void draw_stats(int score2, int level2, int lines2, int next_piece2){
        vbe_fill_rect(sx2, sy2, stats_w - 8, board_ph, _GG_COL(12,10,18));
        /* NEXT label */
        vbe_draw_char(sx2, sy2,    'N',_GG_COL(180,180,180),_GG_COL(12,10,18));
        vbe_draw_char(sx2+8,sy2,   'E',_GG_COL(180,180,180),_GG_COL(12,10,18));
        vbe_draw_char(sx2+16,sy2,  'X',_GG_COL(180,180,180),_GG_COL(12,10,18));
        vbe_draw_char(sx2+24,sy2,  'T',_GG_COL(180,180,180),_GG_COL(12,10,18));
        /* next preview: 4x4 block at sy2+20 */
        int nc = (cell > 12) ? 12 : cell;
        for(int i=0;i<4;i++){
            int px3=T_PIECES[next_piece2][0][i][0];
            int py3=T_PIECES[next_piece2][0][i][1];
            vbe_fill_rect(sx2+px3*nc+1, sy2+20+py3*nc+1, nc-2, nc-2, vga_to_rgb(T_COLORS[next_piece2]));
        }
        /* score/level/lines */
        const char *labels[]={"Score","Level","Lines"};
        int vals[]={score2,level2,lines2};
        for(int li=0;li<3;li++){
            int lby=sy2+80+li*40;
            for(int ci=0;labels[li][ci];ci++)
                vbe_draw_char(sx2+ci*8, lby, labels[li][ci], _GG_COL(120,120,180), _GG_COL(12,10,18));
            char nb[12]; int ni=0;
            int v2=vals[li];
            if(!v2){nb[ni++]='0';}else{char t[12];int ti=0;while(v2){t[ti++]='0'+v2%10;v2/=10;}while(ti>0)nb[ni++]=t[--ti];}
            nb[ni]=0;
            for(int ci=0;nb[ci];ci++)
                vbe_draw_char(sx2+ci*8, lby+14, nb[ci], _GG_COL(255,255,255), _GG_COL(12,10,18));
        }
    }

    g_seed ^= 0x13371337;
    int score = 0, level = 1, lines_total = 0;
    int piece = rng_range(0,6), rot = 0, px = 3, py2 = -1;
    int next_piece = rng_range(0,6);
    int alive = 1;
    int tick = 0, tick_max = 30;

    draw_board_gui();
    draw_stats(score, level, lines_total, next_piece);
    vbe_flip_rect(wx, wy, ww, wh);

    while(alive){
        if(keyboard_keyready()){
            char k = keyboard_poll();
            if(k=='q'||k=='Q'||k==27) break;
            if(k=='\x1b'){
                char k2=keyboard_poll();
                if(k2=='['){
                    char k3=keyboard_poll();
                    if(k3=='C' && t_fits(piece,rot,px+1,py2)) { draw_piece_gui(piece,rot,px,py2,1); px++; }
                    if(k3=='D' && t_fits(piece,rot,px-1,py2)) { draw_piece_gui(piece,rot,px,py2,1); px--; }
                    if(k3=='B' && t_fits(piece,rot,px,py2+1)){ draw_piece_gui(piece,rot,px,py2,1); py2++; }
                    if(k3=='A'){  /* hard drop */
                        draw_piece_gui(piece,rot,px,py2,1);
                        while(t_fits(piece,rot,px,py2+1)) py2++;
                        tick=tick_max;
                    }
                }
            }
            if(k=='z'||k=='Z'){ int nr=(rot+3)%4; if(t_fits(piece,nr,px,py2)){draw_piece_gui(piece,rot,px,py2,1);rot=nr;} }
            if(k=='x'||k=='X'){ int nr=(rot+1)%4; if(t_fits(piece,nr,px,py2)){draw_piece_gui(piece,rot,px,py2,1);rot=nr;} }
            if(k==' '){ draw_piece_gui(piece,rot,px,py2,1); while(t_fits(piece,rot,px,py2+1))py2++; tick=tick_max; }
        }

        tick++;
        if(tick>=tick_max){
            tick=0;
            if(t_fits(piece,rot,px,py2+1)){
                draw_piece_gui(piece,rot,px,py2,1); py2++;
            } else {
                /* lock */
                for(int i=0;i<4;i++){
                    int lx=px+T_PIECES[piece][rot][i][0];
                    int ly=py2+T_PIECES[piece][rot][i][1];
                    if(ly>=0) t_board[ly][lx]=(uint8_t)(T_COLORS[piece]+1);
                }
                int cl=t_clear_lines();
                lines_total+=cl;
                static const int pts[]={0,100,300,500,800};
                score+=pts[cl>4?4:cl]*level;
                level=1+lines_total/10;
                tick_max=32-level*2; if(tick_max<4)tick_max=4;
                piece=next_piece; next_piece=rng_range(0,6);
                px=3; py2=-1; rot=0;
                if(!t_fits(piece,rot,px,py2+1)) alive=0;
                draw_board_gui();
                draw_stats(score,level,lines_total,next_piece);
                vbe_flip_rect(wx,wy,ww,wh);
                continue;
            }
        }

        draw_piece_gui(piece,rot,px,py2,0);
        vbe_flip_rect(wx,wy,ww,wh);
        gg_delay(16);
    }

    /* game over */
    int gox=bx+board_pw/2-40;
    int goy=by+board_ph/2-10;
    vbe_fill_rect(gox-8,goy-8,104,44,_GG_COL(20,0,0));
    const char *go2="GAME OVER";
    for(int i=0;go2[i];i++) vbe_draw_char(gox+i*8,goy,go2[i],_GG_COL(255,80,80),_GG_COL(20,0,0));
    const char *qa2="Q to close";
    for(int i=0;qa2[i];i++) vbe_draw_char(gox+i*8,goy+20,qa2[i],_GG_COL(180,180,180),_GG_COL(20,0,0));
    vbe_flip_rect(wx,wy,ww,wh);
    for(;;){ if(keyboard_keyready()){char k=keyboard_poll();if(k=='q'||k=='Q'||k==27)break;} gg_delay(20); }
    #undef TC_CELL
    #undef TC_EMPTY
}

/* ================================================================
   PONG GUI
   Two paddles + ball rendered as coloured blocks.
   Player = right paddle (W/S or arrows).  CPU tracks ball.
   First to 10 points wins.
   ================================================================ */

void cmd_pong_gui(int wx, int wy, int ww, int wh){
    int cx = wx + 2;
    int cy = wy + 34;
    int cw = ww - 4;
    int ch = wh - 36;
    if(cw < 120 || ch < 80) return;

    /* clear bg */
    vbe_fill_rect(cx, cy, cw, ch, _GG_COL(8, 8, 16));
    /* centre line dots */
    for(int y=0; y<ch; y+=12)
        vbe_fill_rect(cx+cw/2-1, cy+y, 2, 6, _GG_COL(40,40,80));

    int pad_h   = ch / 6;       /* paddle height in pixels */
    int pad_w   = 10;
    int ball_sz = 10;

    /* player on right (x = cx+cw-pad_w-8), CPU on left (x = cx+8) */
    int player_x = cx + cw - pad_w - 8;
    int cpu_x    = cx + 8;

    /* y positions = top of paddle */
    int player_y = cy + (ch - pad_h) / 2;
    int cpu_y    = cy + (ch - pad_h) / 2;
    int p_score  = 0, c_score  = 0;

    /* ball position (×256 sub-pixel) */
    int bx = (cx + cw/2) << 8;
    int by = (cy + ch/2) << 8;
    int bdx = (rng()&1) ?  180 : -180;
    int bdy = (rng()&1) ?  120 : -120;

    int alive = 1;

    /* number draw helper */
    #define PONG_NUM(nx,ny,v,col) do { \
        char _nb[8]; int _ni=0; int _v=(v); \
        if(!_v){_nb[_ni++]='0';}else{char _t[8];int _ti=0;while(_v){_t[_ti++]='0'+_v%10;_v/=10;}while(_ti>0)_nb[_ni++]=_t[--_ti];} \
        _nb[_ni]=0; \
        for(int _i=0;_nb[_i];_i++) vbe_draw_char((nx)+_i*8,(ny),_nb[_i],(col),_GG_COL(8,8,16)); \
    } while(0)

    while(alive){
        /* input */
        if(keyboard_keyready()){
            char k=keyboard_poll();
            if(k=='q'||k=='Q'||k==27) break;
            int spd=8;
            if((k=='w'||k=='W') && player_y-spd>=cy)               player_y-=spd;
            if((k=='s'||k=='S') && player_y+pad_h+spd<=cy+ch)     player_y+=spd;
            if(k=='\x1b'){
                char k2=keyboard_poll();
                if(k2=='['){
                    char k3=keyboard_poll();
                    if(k3=='A' && player_y-spd>=cy)             player_y-=spd;
                    if(k3=='B' && player_y+pad_h+spd<=cy+ch)   player_y+=spd;
                }
            }
        }

        /* CPU AI */
        int ball_cy = (by>>8) + ball_sz/2;
        int cpu_mid  = cpu_y + pad_h/2;
        if(ball_cy < cpu_mid - 2 && cpu_y > cy+2)                 cpu_y -= 5;
        else if(ball_cy > cpu_mid + 2 && cpu_y+pad_h < cy+ch-2)  cpu_y += 5;

        /* erase old ball */
        int obx=(bx>>8), oby=(by>>8);
        vbe_fill_rect(obx, oby, ball_sz, ball_sz, _GG_COL(8,8,16));

        /* move */
        bx+=bdx; by+=bdy;

        /* top/bottom bounce */
        if((by>>8)<cy){ by=cy<<8; bdy=-bdy; }
        if((by>>8)+ball_sz>=cy+ch){ by=(cy+ch-ball_sz)<<8; bdy=-bdy; }

        int cbx=bx>>8, cby=by>>8;

        /* player paddle collision */
        if(bdx>0 && cbx+ball_sz>=player_x && cbx<player_x+pad_w){
            if(cby+ball_sz>player_y && cby<player_y+pad_h){
                bdx=-bdx;
                bdy+=rng_range(-30,30);
                bx=(player_x-ball_sz)<<8;
            } else if(cbx+ball_sz>player_x+pad_w+4){
                c_score++;
                bx=(cx+cw/2)<<8; by=(cy+ch/2)<<8;
                bdx=-180; bdy=(rng()&1)?120:-120;
            }
        }

        /* cpu paddle collision */
        if(bdx<0 && cbx<=cpu_x+pad_w && cbx+ball_sz>cpu_x){
            if(cby+ball_sz>cpu_y && cby<cpu_y+pad_h){
                bdx=-bdx;
                bdy+=rng_range(-30,30);
                bx=(cpu_x+pad_w)<<8;
            } else if(cbx<cpu_x-4){
                p_score++;
                bx=(cx+cw/2)<<8; by=(cy+ch/2)<<8;
                bdx=180; bdy=(rng()&1)?120:-120;
            }
        }

        /* clamp bdy */
        if(bdy>320)bdy=320; if(bdy<-320)bdy=-320;
        if(bdy>-50&&bdy<50)bdy=(bdy>=0)?50:-50;

        cbx=bx>>8; cby=by>>8;

        /* erase old paddles */
        vbe_fill_rect(cpu_x,    cy,  pad_w, ch, _GG_COL(8,8,16));
        vbe_fill_rect(player_x, cy,  pad_w, ch, _GG_COL(8,8,16));

        /* draw paddles */
        vbe_fill_rect(cpu_x,    cpu_y,    pad_w, pad_h, _GG_COL(60,120,255));
        vbe_fill_rect(player_x, player_y, pad_w, pad_h, _GG_COL(0,220,80));

        /* draw ball */
        vbe_fill_rect(cbx, cby, ball_sz, ball_sz, _GG_COL(255,255,255));

        /* scores */
        PONG_NUM(cx+cw/2-40, cy+4, c_score, _GG_COL(100,100,255));
        PONG_NUM(cx+cw/2+24, cy+4, p_score, _GG_COL(0,220,80));

        if(p_score>=10 || c_score>=10) break;

        vbe_flip_rect(wx,wy,ww,wh);
        gg_delay(16);
    }

    /* result */
    const char *winner = (p_score>=10)?"YOU WIN!":(c_score>=10)?"CPU WINS":"GAME OVER";
    uint32_t wc = (p_score>=10)?_GG_COL(0,220,80):_GG_COL(220,60,60);
    int gox=cx+cw/2-32;
    int goy=cy+ch/2-16;
    vbe_fill_rect(gox-8,goy-8,104,52,_GG_COL(20,10,20));
    for(int i=0;winner[i];i++) vbe_draw_char(gox+i*8,goy,winner[i],wc,_GG_COL(20,10,20));
    const char *qa3="Q to close";
    for(int i=0;qa3[i];i++) vbe_draw_char(gox+i*8,goy+20,qa3[i],_GG_COL(180,180,180),_GG_COL(20,10,20));
    vbe_flip_rect(wx,wy,ww,wh);
    for(;;){ if(keyboard_keyready()){char k=keyboard_poll();if(k=='q'||k=='Q'||k==27)break;} gg_delay(20); }
    #undef PONG_NUM
}

/* ================================================================
   MINECRAFT GUI — Raycaster-style voxel world
   - 16×16×16 chunk of blocks (stone, dirt, grass, wood, leaves, air)
   - WASD = move (horizontal), Q/E = up/down
   - Mouse look (dx from mouse driver)
   - Left click = break block under crosshair
   - Right click = place selected block
   - 1-6 hotbar to pick block type
   - Escape = quit
   ================================================================ */

/* Block types */
#define BLK_AIR    0
#define BLK_STONE  1
#define BLK_DIRT   2
#define BLK_GRASS  3
#define BLK_WOOD   4
#define BLK_LEAVES 5
#define BLK_SAND   6

/* colours per block face (top, side, bottom) */
static const uint32_t BLK_TOP[7] = {
    0,                         /* AIR */
    _GG_COL(140,140,140),      /* STONE */
    _GG_COL(100,70,40),        /* DIRT */
    _GG_COL(60,160,30),        /* GRASS top */
    _GG_COL(100,60,20),        /* WOOD */
    _GG_COL(40,100,30),        /* LEAVES */
    _GG_COL(210,190,100),      /* SAND */
};
static const uint32_t BLK_SIDE[7] = {
    0,
    _GG_COL(110,110,110),
    _GG_COL(100,70,40),
    _GG_COL(90,55,25),         /* grass side = dirt-ish */
    _GG_COL(80,50,20),
    _GG_COL(30,80,25),
    _GG_COL(190,170,90),
};

#define MC_CX 16
#define MC_CY 16
#define MC_CZ 16

static uint8_t mc_chunk[MC_CY][MC_CZ][MC_CX];

/* simple terrain gen */
static void mc_gen_terrain(void){
    for(int y=0;y<MC_CY;y++)
        for(int z=0;z<MC_CZ;z++)
            for(int x=0;x<MC_CX;x++){
                /* height varies per column */
                uint32_t h_seed = (uint32_t)(x*7+z*13+0xC0FFEE);
                h_seed=h_seed*1664525u+1013904223u;
                int ground = 6 + (int)((h_seed>>24)%4); /* 6..9 */
                if(y > ground){
                    mc_chunk[y][z][x]=BLK_AIR;
                } else if(y == ground){
                    mc_chunk[y][z][x]=BLK_GRASS;
                } else if(y >= ground-2){
                    mc_chunk[y][z][x]=BLK_DIRT;
                } else {
                    mc_chunk[y][z][x]=BLK_STONE;
                }
            }
    /* small tree at (8,8) */
    int tx=8,tz=8;
    /* find surface */
    int ty=MC_CY-1; while(ty>0&&mc_chunk[ty][tz][tx]==BLK_AIR)ty--;
    ty++; /* one above surface */
    for(int h=0;h<4&&ty+h<MC_CY;h++) mc_chunk[ty+h][tz][tx]=BLK_WOOD;
    /* leaves crown */
    for(int ly=-1;ly<=1;ly++)
        for(int lz=-2;lz<=2;lz++)
            for(int lx=-2;lx<=2;lx++){
                int bx2=tx+lx, bz2=tz+lz, by2=ty+3+ly;
                if(bx2>=0&&bx2<MC_CX&&bz2>=0&&bz2<MC_CZ&&by2>=0&&by2<MC_CY)
                    if(mc_chunk[by2][bz2][bx2]==BLK_AIR)
                        mc_chunk[by2][bz2][bx2]=BLK_LEAVES;
            }
}

/* Fixed-point trig helpers */
/* We use a simple look-up-less approach with integer math */
/* sin/cos via polynomial approximation (Q8.8) */
static int32_t mc_sin(int32_t deg){
    /* deg in units of (1/64) radians stored as int; range 0..2π×64 */
    /* Use small table: 64 values per quarter circle */
    /* Actually just use a 256-entry table built at init */
    /* For a hobby OS, let's use a simpler inline poly */
    /* map deg to 0..255 (full circle = 256 units), angle in units of 2π/256 */
    int32_t a = deg & 0xFF;   /* 0..255 */
    /* sin approximation for a in [0,255] → result in [-256,256] */
    /* Use: sin(2πa/256) ≈ mirrored parabola */
    int32_t s;
    if(a < 64){      s =  (a * (128 - a)) / 16; }
    else if(a < 128){ int b=a-64; s = ((64-b)*(128-(64-b)))/16; }
    else if(a < 192){ int b=a-128; s = -((b*(128-b))/16); }
    else {            int b=a-192; s = -(((64-b)*(128-(64-b)))/16); }
    return s; /* range roughly ±256 */
}
static int32_t mc_cos(int32_t deg){ return mc_sin((deg+64)&0xFF); }

void cmd_minecraft_gui(int wx, int wy, int ww, int wh){
    int cx = wx + 2;
    int cy = wy + 34;
    int cw = ww - 4;
    int ch = wh - 36;
    if(cw < 120 || ch < 80) return;

    mc_gen_terrain();

    /* Player position (×256 sub-block) */
    int32_t px_f = (8 << 8);   /* fixed-point x */
    int32_t py_f = (11 << 8);  /* y = one above ground */
    int32_t pz_f = (8 << 8);
    int32_t yaw  = 0;           /* 0..255, angle units */
    int32_t pitch= 0;           /* −32..+32, look up/down */

    uint8_t held_block = BLK_STONE;
    const uint8_t hotbar[] = {BLK_STONE,BLK_DIRT,BLK_GRASS,BLK_WOOD,BLK_LEAVES,BLK_SAND};
    int hotbar_sel = 0;

    /* raycaster render into window */
    /* We render at half resolution then upscale for speed */
    int rw = cw / 2;
    int rh = ch / 2;
    if(rw < 2) rw = 2;
    if(rh < 2) rh = 2;

    /* sky/floor gradient colours */
    #define MC_SKY(row) _GG_COL(80 + (row)*80/rh, 140+(row)*60/rh, 200+(row)*30/rh)
    #define MC_FLOOR(row) _GG_COL(60,50,30)

    /* ---- HUD draw ---- */
    auto void draw_hud(void);
    void draw_hud(void){
        /* hotbar strip at bottom of window */
        int hby = cy + ch - 24;
        vbe_fill_rect(cx, hby, cw, 24, _GG_COL(30,30,30));
        int slot_w = 24;
        int hbx = cx + cw/2 - (6*slot_w)/2;
        for(int i=0;i<6;i++){
            int sx3=hbx+i*slot_w;
            uint32_t border=(i==hotbar_sel)?_GG_COL(255,200,0):_GG_COL(80,80,80);
            vbe_fill_rect(sx3, hby+2, slot_w-2, 20, BLK_SIDE[hotbar[i]]);
            /* border */
            vbe_fill_rect(sx3,    hby+2,  slot_w-2, 1, border);
            vbe_fill_rect(sx3,    hby+21, slot_w-2, 1, border);
            vbe_fill_rect(sx3,    hby+2,  1, 20, border);
            vbe_fill_rect(sx3+slot_w-3, hby+2, 1, 20, border);
        }
        /* crosshair */
        int chx=cx+cw/2, chhy=cy+ch/2;
        vbe_fill_rect(chx-8, chhy-1, 16, 2, _GG_COL(255,255,255));
        vbe_fill_rect(chx-1, chhy-8, 2, 16, _GG_COL(255,255,255));
        /* WASD hint */
        const char *hint="WASD+QE=move | LClick=break | RClick=place | 1-6=block | Esc=quit";
        int hlen=0; while(hint[hlen]) hlen++;
        int hx=cx+(cw-hlen*8)/2; if(hx<cx)hx=cx;
        vbe_fill_rect(hx-4, cy+2, hlen*8+8, 14, _GG_COL(0,0,0));
        for(int i=0;hint[i];i++) vbe_draw_char(hx+i*8, cy+3, hint[i], _GG_COL(200,200,200), _GG_COL(0,0,0));
    }

    /* ---- raycaster frame ---- */
    auto void render_frame(void);
    void render_frame(void){
        int32_t cs = mc_cos(yaw);  /* cos(yaw), range ~±256 */
        int32_t ss = mc_sin(yaw);  /* sin(yaw) */

        int render_h = ch - 24; /* leave hotbar */

        for(int sy3=0; sy3<render_h; sy3++){
            /* vertical look offset */
            int half = render_h / 2;
            int vert = sy3 - half - pitch * 4;

            for(int sx3=0; sx3<cw; sx3++){
                /* ray direction */
                int cam = (sx3 - cw/2) * 256 / (cw / 2);  /* -256..+256 */
                int32_t rdx = (cs * 256 + ss * cam) >> 8;
                int32_t rdz = (ss * 256 - cs * cam) >> 8;  /* Y is up, Z forward */

                /* Flat projection: determine if this ray hits sky or floor first,
                   then DDA march for block intersection */
                if(rdx == 0 && rdz == 0){ vbe_put_pixel(cx+sx3, cy+sy3, MC_SKY(sy3)); continue; }

                /* DDA ray march in XZ plane */
                int32_t rx = px_f >> 8;  /* block coords */
                int32_t ry_base = py_f >> 8;
                int32_t rz = pz_f >> 8;
                /* sub-block fractional */
                int32_t fx2 = px_f & 0xFF;
                int32_t fz2 = pz_f & 0xFF;

                /* vertical ray: pitch adjusts which Y layer we render */
                /* Simplified: for each column pixel, cast horizontal ray and
                   then interpolate Y from vert offset */

                uint32_t hit_col = 0;
                int hit = 0;
                int dist = 0;
                int face = 0; /* 0=X side, 1=Z side, 2=top */

                /* DDA */
                int32_t step_x = (rdx > 0) ? 1 : -1;
                int32_t step_z = (rdz > 0) ? 1 : -1;

                /* tDelta — how far along ray to cross one cell */
                int32_t td_x = (rdx != 0) ? (256 * 256 / (rdx < 0 ? -rdx : rdx)) : 0x7FFF;
                int32_t td_z = (rdz != 0) ? (256 * 256 / (rdz < 0 ? -rdz : rdz)) : 0x7FFF;

                /* initial distances to first cell boundaries */
                int32_t side_x = (rdx > 0) ? ((256 - fx2) * 256 / (rdx > 0 ? rdx : -rdx)) :
                                               (fx2 * 256 / (rdx > 0 ? rdx : -rdx));
                int32_t side_z = (rdz > 0) ? ((256 - fz2) * 256 / (rdz > 0 ? rdz : -rdz)) :
                                               (fz2 * 256 / (rdz > 0 ? rdz : -rdz));

                int32_t march_x = rx, march_z = rz;

                for(int steps=0; steps<24; steps++){
                    /* choose smaller side */
                    if(side_x < side_z){
                        side_x += td_x;
                        march_x += step_x;
                        face = 0;
                        dist = steps;
                    } else {
                        side_z += td_z;
                        march_z += step_z;
                        face = 1;
                        dist = steps;
                    }
                    if(march_x<0||march_x>=MC_CX||march_z<0||march_z>=MC_CZ) break;

                    /* check all Y levels — pick the one in view based on vert */
                    for(int vy=MC_CY-1;vy>=0;vy--){
                        if(mc_chunk[vy][march_z][march_x] != BLK_AIR){
                            /* project block to screen Y */
                            int by3 = vy;
                            int proj_y = (by3 - ry_base) * 128 / (dist+1) + half;
                            /* check if current screen row is within this block face */
                            int block_screen_h = 128 / (dist + 1);
                            if(block_screen_h < 1) block_screen_h = 1;
                            if(sy3 >= proj_y && sy3 < proj_y + block_screen_h){
                                uint8_t blk = mc_chunk[vy][march_z][march_x];
                                /* shade by face and distance */
                                uint32_t base_col = (face==0) ? BLK_SIDE[blk] :
                                                    (face==1) ? BLK_SIDE[blk] : BLK_TOP[blk];
                                /* darken by distance */
                                int d2 = dist+1; if(d2>8)d2=8;
                                int r2=(int)((base_col>>16)&0xFF) * (8-d2+4) / 12;
                                int g2=(int)((base_col>>8)&0xFF)  * (8-d2+4) / 12;
                                int b2=(int)( base_col    &0xFF)  * (8-d2+4) / 12;
                                if(r2<0)r2=0; if(r2>255)r2=255;
                                if(g2<0)g2=0; if(g2>255)g2=255;
                                if(b2<0)b2=0; if(b2>255)b2=255;
                                hit_col = _GG_COL(r2,g2,b2);
                                hit = 1;
                            }
                            break;
                        }
                    }
                    if(hit) break;
                }

                if(!hit){
                    /* sky / floor */
                    if(sy3 < render_h/2 + pitch*4)
                        hit_col = MC_SKY(sy3);
                    else
                        hit_col = _GG_COL(80-(sy3-render_h/2)/4,65,40);
                }
                vbe_put_pixel(cx+sx3, cy+sy3, hit_col);
            }
        }
        draw_hud();
        vbe_flip_rect(wx, wy, ww, wh);
    }

    /* initial render */
    render_frame();

    /* ---- input + game loop ---- */
    int last_mx = mouse_x, last_my = mouse_y;

    for(;;){
        int moved = 0;
        /* keyboard */
        while(keyboard_keyready()){
            char k = keyboard_poll();
            if(k==27){ goto mc_quit; }
            /* WASD movement */
            int spd = 20; /* sub-block units */
            if(k=='w'||k=='W'){
                px_f += (mc_cos(yaw)*spd)>>8;
                pz_f -= (mc_sin(yaw)*spd)>>8;
                moved=1;
            }
            if(k=='s'||k=='S'){
                px_f -= (mc_cos(yaw)*spd)>>8;
                pz_f += (mc_sin(yaw)*spd)>>8;
                moved=1;
            }
            if(k=='a'||k=='A'){
                px_f += (mc_sin(yaw)*spd)>>8;
                pz_f += (mc_cos(yaw)*spd)>>8;
                moved=1;
            }
            if(k=='d'||k=='D'){
                px_f -= (mc_sin(yaw)*spd)>>8;
                pz_f -= (mc_cos(yaw)*spd)>>8;
                moved=1;
            }
            if(k=='q'||k=='Q'){ py_f += spd; moved=1; }
            if(k=='e'||k=='E'){ py_f -= spd; moved=1; }
            /* hotbar */
            if(k>='1'&&k<='6'){ hotbar_sel=k-'1'; held_block=hotbar[hotbar_sel]; moved=1; }
            /* look via arrow keys too */
            if(k=='\x1b'){
                char k2=keyboard_poll();
                if(k2=='['){
                    char k3=keyboard_poll();
                    if(k3=='A'){ pitch-=4; if(pitch<-24)pitch=-24; moved=1; }
                    if(k3=='B'){ pitch+=4; if(pitch>24)pitch=24; moved=1; }
                    if(k3=='C'){ yaw=(yaw+6)&0xFF; moved=1; }
                    if(k3=='D'){ yaw=(yaw-6+256)&0xFF; moved=1; }
                }
            }
            /* clamp position */
            if((px_f>>8)<0) px_f=0;
            if((px_f>>8)>=MC_CX) px_f=(MC_CX-1)<<8;
            if((pz_f>>8)<0) pz_f=0;
            if((pz_f>>8)>=MC_CZ) pz_f=(MC_CZ-1)<<8;
            if((py_f>>8)<0) py_f=0;
            if((py_f>>8)>=MC_CY) py_f=(MC_CY-1)<<8;
        }

        /* mouse look */
        mouse_poll_backends();
        {
            mouse_event_t ev;
            while(mouse_poll(&ev)){
                if(ev.dx!=0){ yaw=(int32_t)(yaw + ev.dx/4)&0xFF; moved=1; }
                if(ev.dy!=0){ pitch-=ev.dy/4; if(pitch<-24)pitch=-24; if(pitch>24)pitch=24; moved=1; }
                /* left click = break */
                if(ev.buttons & MOUSE_BTN_LEFT){
                    /* simple: break block 3 units ahead */
                    int tx2=(px_f>>8)+(int)((mc_cos(yaw)*3)>>8);
                    int tz2=(pz_f>>8)-(int)((mc_sin(yaw)*3)>>8);
                    int ty2=(py_f>>8);
                    if(tx2>=0&&tx2<MC_CX&&tz2>=0&&tz2<MC_CZ&&ty2>=0&&ty2<MC_CY)
                        mc_chunk[ty2][tz2][tx2]=BLK_AIR;
                    moved=1;
                }
                /* right click = place */
                if(ev.buttons & MOUSE_BTN_RIGHT){
                    int tx2=(px_f>>8)+(int)((mc_cos(yaw)*3)>>8);
                    int tz2=(pz_f>>8)-(int)((mc_sin(yaw)*3)>>8);
                    int ty2=(py_f>>8)+1;
                    if(tx2>=0&&tx2<MC_CX&&tz2>=0&&tz2<MC_CZ&&ty2>=0&&ty2<MC_CY&&
                       mc_chunk[ty2][tz2][tx2]==BLK_AIR)
                        mc_chunk[ty2][tz2][tx2]=held_block;
                    moved=1;
                }
            }
        }

        if(moved){
            render_frame();
        }
        gg_delay(16);
    }
mc_quit:;
    /* clear window area back to bg */
    vbe_fill_rect(cx, cy, cw, ch, _GG_COL(12,12,20));
    vbe_flip_rect(wx, wy, ww, wh);
    #undef MC_SKY
    #undef MC_FLOOR
}
