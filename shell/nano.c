/* scorpion OS - shell/nano.c
   Minimal text editor.
   Features: open/create file, edit, Ctrl+S save, Ctrl+X exit, Ctrl+W find,
             Ctrl+K cut line, Ctrl+U paste line, arrow navigation,
             Home/End, Page Up/Down, status bar with filename + modified flag. */

#include "nano.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include "../drivers/keyboard.h"
#include "../fs/sfs.h"
#include <stdint.h>
#include <stddef.h>

/* dynamic: computed from VBE screen size at runtime */
static int NANO_COLS = 80;
static int NANO_ROWS = 23;
#define NANO_MAXLINES 1024
#define NANO_LINELEN  256

static char  *lines[NANO_MAXLINES];
static int    line_count = 0;
static int    cur_row = 0, cur_col = 0;
static int    view_top = 0;
static int    modified = 0;
static char   nano_filename[128];
static char   clipboard[NANO_LINELEN];

/* ---- helpers ---- */
static int  slen(const char *s){int n=0;while(s[n])n++;return n;}
static void scpy(char *d,const char *s){while((*d++=*s++));}
static void sncpy(char *d,const char *s,int n){int i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]=0;}
static void smove(char *d,const char *s,int n){
    /* memmove equivalent */
    if(d<s){for(int i=0;i<n;i++)d[i]=s[i];}
    else{for(int i=n-1;i>=0;i--)d[i]=s[i];}
}

static char *line_alloc(void) {
    char *l = (char*)kmalloc(NANO_LINELEN);
    if (l) l[0] = 0;
    return l;
}

static void free_lines(void) {
    for (int i=0;i<line_count;i++) { if(lines[i]) kfree(lines[i]); lines[i]=NULL; }
    line_count = 0;
}

/* ---- rendering ---- */
static void status_bar(void) {
    terminal_setcolor(terminal_make_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    /* top bar */
    terminal_printf("\x1b[1;1H");
    for (int i=0;i<NANO_COLS;i++) terminal_putchar(' ');
    terminal_printf("\x1b[1;1H");
    terminal_printf("  nano | %s%s", nano_filename[0]?nano_filename:"(new)", modified?" *":"");
    /* bottom bar */
    terminal_setcolor(terminal_make_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    terminal_printf("\x1b[25;1H");
    for (int i=0;i<NANO_COLS;i++) terminal_putchar(' ');
    terminal_printf("\x1b[25;1H");
    terminal_printf("  ^S Save  ^X Exit  ^W Find  ^K Cut  ^U Paste  Ln %d/%d  Col %d",
        cur_row+1, line_count, cur_col+1);
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static void redraw(void) {
    terminal_writestring("\x1b[2J\x1b[H");
    status_bar();
    for (int i=0;i<NANO_ROWS;i++) {
        int li = view_top + i;
        terminal_printf("\x1b[%d;1H", i+2);  /* rows 2..24 */
        terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        if (li < line_count) {
            char *l = lines[li];
            int len = slen(l);
            for (int c=0;c<NANO_COLS-1&&c<len;c++) terminal_putchar(l[c]);
        }
        terminal_writestring("\x1b[K");  /* clear to end of line */
    }
    /* Position cursor */
    int screen_row = cur_row - view_top + 2;
    terminal_printf("\x1b[%d;%dH", screen_row, cur_col+1);
}

/* ---- file I/O ---- */
static void load_file(const char *path, sfs_node_t *cwd) {
    sfs_node_t *node = sfs_resolve(cwd, path);
    if (!node || node->type != SFS_FILE || !node->data) return;
    const char *data = (const char*)node->data;
    size_t sz = node->size;
    free_lines();
    size_t pos = 0;
    while (pos <= sz && line_count < NANO_MAXLINES) {
        char *l = line_alloc();
        if (!l) break;
        int li = 0;
        while (pos < sz && data[pos] != '\n' && li < NANO_LINELEN-1)
            l[li++] = data[pos++];
        l[li] = 0;
        if (pos < sz && data[pos] == '\n') pos++;
        lines[line_count++] = l;
    }
    if (!line_count) { lines[0]=line_alloc(); line_count=1; }
}

static void save_file(sfs_node_t *cwd) {
    /* Compute total size */
    size_t total = 0;
    for (int i=0;i<line_count;i++) total += slen(lines[i]) + 1;
    uint8_t *buf = (uint8_t*)kmalloc(total+1);
    if (!buf) { terminal_printf("\x1b[25;1H  ERROR: out of memory"); return; }
    size_t pos = 0;
    for (int i=0;i<line_count;i++) {
        int l = slen(lines[i]);
        for (int j=0;j<l;j++) buf[pos++]=(uint8_t)lines[i][j];
        buf[pos++]='\n';
    }
    sfs_write_file(cwd, nano_filename, buf, pos);
    kfree(buf);
    modified = 0;
}

/* ---- edit operations ---- */
static void insert_char(char c) {
    if (cur_row >= NANO_MAXLINES) return;
    char *l = lines[cur_row];
    int len = slen(l);
    if (len >= NANO_LINELEN-1) return;
    /* shift right */
    for (int i=len;i>=cur_col;i--) l[i+1]=l[i];
    l[cur_col++] = c;
    modified = 1;
}

static void delete_char(void) {  /* backspace */
    if (cur_col > 0) {
        char *l = lines[cur_row];
        int len = slen(l);
        smove(l+cur_col-1, l+cur_col, len-cur_col+1);
        cur_col--;
        modified = 1;
    } else if (cur_row > 0) {
        /* merge with previous line */
        char *prev = lines[cur_row-1];
        char *cur  = lines[cur_row];
        int plen = slen(prev);
        int clen = slen(cur);
        cur_col = plen;
        if (plen+clen < NANO_LINELEN-1) {
            scpy(prev+plen, cur);
        }
        kfree(cur);
        for (int i=cur_row;i<line_count-1;i++) lines[i]=lines[i+1];
        lines[--line_count]=NULL;
        cur_row--;
        modified = 1;
    }
}

static void insert_newline(void) {
    if (line_count >= NANO_MAXLINES) return;
    char *l = lines[cur_row];
    char *newl = line_alloc();
    if (!newl) return;
    /* copy remainder to new line */
    scpy(newl, l+cur_col);
    l[cur_col] = 0;
    /* shift lines down */
    for (int i=line_count;i>cur_row+1;i--) lines[i]=lines[i-1];
    lines[cur_row+1] = newl;
    line_count++;
    cur_row++;
    cur_col = 0;
    modified = 1;
}

/* ---- search ---- */
static void do_find(void) {
    terminal_setcolor(terminal_make_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    terminal_printf("\x1b[25;1H  Find: ");
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    char query[64]; int qi=0; query[0]=0;
    while (1) {
        char c = keyboard_getchar();
        if (c=='\n'||c=='\r') break;
        if ((c=='\b'||c==127)&&qi>0) { qi--; query[qi]=0; }
        else if (c>=32&&qi<63) { query[qi++]=c; query[qi]=0; }
        terminal_printf("\x1b[25;8H%s\x1b[K", query);
    }
    if (!query[0]) return;
    int qlen = slen(query);
    for (int r=cur_row;r<line_count;r++) {
        int start = (r==cur_row) ? cur_col+1 : 0;
        char *l = lines[r];
        int llen = slen(l);
        for (int c=start;c<=llen-qlen;c++) {
            int match=1;
            for (int k=0;k<qlen;k++) if(l[c+k]!=query[k]){match=0;break;}
            if (match) {
                cur_row=r; cur_col=c;
                if (cur_row < view_top) view_top=cur_row;
                if (cur_row >= view_top+NANO_ROWS) view_top=cur_row-NANO_ROWS+1;
                return;
            }
        }
    }
    terminal_printf("\x1b[25;1H  Not found: %s", query);
    keyboard_getchar();
}

/* ---- main entry ---- */
void nano_open(const char *filename, sfs_node_t *cwd) {
    /* compute fullscreen dimensions from VBE */
    extern int vbe_screen_width, vbe_screen_height;
    if (vbe_screen_width > 0)  NANO_COLS = vbe_screen_width / 8;
    if (vbe_screen_height > 0) NANO_ROWS = vbe_screen_height / 16 - 2; /* 2 status rows */
    if (NANO_COLS < 40) NANO_COLS = 40;
    if (NANO_ROWS < 5)  NANO_ROWS = 5;

    free_lines();
    cur_row=0; cur_col=0; view_top=0; modified=0;
    clipboard[0]=0;

    if (filename && filename[0]) {
        sncpy(nano_filename, filename, 128);
        load_file(filename, cwd);
    } else {
        nano_filename[0]=0;
        lines[0]=line_alloc();
        line_count=1;
    }
    if (!line_count) { lines[0]=line_alloc(); line_count=1; }

    redraw();

    while (1) {
        char c = keyboard_getchar();

        /* Ctrl keys */
        if (c == 0x13) { /* Ctrl-S */
            if (!nano_filename[0]) {
                terminal_printf("\x1b[25;1H  Save as: ");
                int fi=0;
                while ((c=keyboard_getchar())!='\n'&&c!='\r'&&fi<127) {
                    nano_filename[fi++]=c; terminal_putchar(c);
                }
                nano_filename[fi]=0;
            }
            save_file(cwd);
            redraw(); continue;
        }
        if (c == 0x18) { /* Ctrl-X */
            if (modified) {
                terminal_printf("\x1b[25;1H  Save before exit? [y/n]: ");
                char r2 = keyboard_getchar();
                if (r2=='y'||r2=='Y') {
                    if (!nano_filename[0]) {
                        terminal_printf("\n  Save as: ");
                        int fi=0;
                        while ((r2=keyboard_getchar())!='\n'&&r2!='\r'&&fi<127) {
                            nano_filename[fi++]=r2; terminal_putchar(r2);
                        }
                        nano_filename[fi]=0;
                    }
                    save_file(cwd);
                }
            }
            break;
        }
        if (c == 0x17) { do_find(); redraw(); continue; } /* Ctrl-W */
        if (c == 0x0B) { /* Ctrl-K: cut line */
            if (cur_row < line_count) {
                sncpy(clipboard, lines[cur_row], NANO_LINELEN);
                kfree(lines[cur_row]);
                for (int i=cur_row;i<line_count-1;i++) lines[i]=lines[i+1];
                lines[--line_count]=NULL;
                if (cur_row>=line_count&&line_count>0) cur_row=line_count-1;
                cur_col=0; modified=1;
            }
            redraw(); continue;
        }
        if (c == 0x15) { /* Ctrl-U: paste line */
            if (clipboard[0] && line_count < NANO_MAXLINES) {
                char *nl = line_alloc();
                if (nl) {
                    scpy(nl, clipboard);
                    for (int i=line_count;i>cur_row;i--) lines[i]=lines[i-1];
                    lines[cur_row]=nl; line_count++;
                    cur_row++; modified=1;
                }
            }
            redraw(); continue;
        }

        /* Arrow keys: ESC [ A/B/C/D */
        if (c == 0x1b) {
            char c2=keyboard_getchar();
            if (c2=='[') {
                char c3=keyboard_getchar();
                if (c3=='A') { /* up */
                    if (cur_row>0) { cur_row--;
                        int ll=slen(lines[cur_row]);
                        if(cur_col>ll)cur_col=ll;
                    }
                } else if (c3=='B') { /* down */
                    if (cur_row<line_count-1) { cur_row++;
                        int ll=slen(lines[cur_row]);
                        if(cur_col>ll)cur_col=ll;
                    }
                } else if (c3=='C') { /* right */
                    int ll=slen(lines[cur_row]);
                    if (cur_col<ll) cur_col++;
                    else if (cur_row<line_count-1) { cur_row++; cur_col=0; }
                } else if (c3=='D') { /* left */
                    if (cur_col>0) cur_col--;
                    else if (cur_row>0) { cur_row--; cur_col=slen(lines[cur_row]); }
                } else if (c3=='H') { cur_col=0; }  /* Home */
                else if (c3=='F') { cur_col=slen(lines[cur_row]); } /* End */
                else if (c3=='5') { keyboard_getchar(); /* PgUp */
                    cur_row-=NANO_ROWS; if(cur_row<0)cur_row=0; cur_col=0;
                } else if (c3=='6') { keyboard_getchar(); /* PgDn */
                    cur_row+=NANO_ROWS; if(cur_row>=line_count)cur_row=line_count-1; cur_col=0;
                }
            }
        } else if (c=='\n'||c=='\r') {
            insert_newline();
        } else if (c=='\b'||c==127) {
            delete_char();
        } else if (c=='\t') {
            for (int i=0;i<4;i++) insert_char(' ');
        } else if (c>=32&&c<127) {
            insert_char(c);
        }

        /* Adjust view */
        if (cur_row < view_top) view_top=cur_row;
        if (cur_row >= view_top+NANO_ROWS) view_top=cur_row-NANO_ROWS+1;

        redraw();
    }

    free_lines();
    terminal_clear();
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}
