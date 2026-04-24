/* scorpion OS - net/irc.c
   Minimal IRC client (RFC 1459)                              */

#include "irc.h"
#include "../drivers/net.h"
#include "../drivers/keyboard.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

static size_t k_strlen(const char *s){size_t n=0;while(s[n])n++;return n;}
static void   k_strcpy(char *d,const char *s){while((*d++=*s++));}
static int    k_strncmp(const char*a,const char*b,size_t n){
    while(n&&*a&&*a==*b){a++;b++;n--;}return n?(unsigned char)*a-(unsigned char)*b:0;}

static void irc_send(tcp_socket_t *sock, const char *line) {
    size_t len = k_strlen(line);
    tcp_send(sock, (const uint8_t*)line, len);
}

/* Parse a line from the server and pretty-print it */
static void irc_display(const char *line) {
    /* PING :server → PONG */
    if (k_strncmp(line, "PING", 4) == 0) return; /* handled elsewhere */

    /* :nick!user@host PRIVMSG #chan :message */
    const char *p = line;
    char nick[64] = {0};
    if (*p == ':') {
        p++;
        int ni = 0;
        while (*p && *p != '!' && *p != ' ' && ni < 63) nick[ni++] = *p++;
        nick[ni] = 0;
    }
    /* Skip to command */
    while (*p && *p != ' ') p++;
    while (*p == ' ') p++;

    if (k_strncmp(p, "PRIVMSG", 7) == 0) {
        p += 7; while (*p == ' ') p++;
        /* Skip to channel name */
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        /* skip the target field */
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        if (*p == ':') p++;
        terminal_writestring("\x1b[36m"); /* cyan */
        terminal_writestring("<");
        terminal_writestring(nick);
        terminal_writestring("> ");
        terminal_writestring("\x1b[0m");
        terminal_writestring(p);
        terminal_putchar('\n');
    } else if (k_strncmp(p, "JOIN", 4) == 0) {
        terminal_writestring("\x1b[32m*** ");
        terminal_writestring(nick);
        terminal_writestring(" has joined\x1b[0m\n");
    } else if (k_strncmp(p, "PART", 4) == 0 || k_strncmp(p, "QUIT", 4) == 0) {
        terminal_writestring("\x1b[31m*** ");
        terminal_writestring(nick);
        terminal_writestring(" has left\x1b[0m\n");
    } else if (k_strncmp(p, "NOTICE", 6) == 0 || k_strncmp(p, "001", 3) == 0
            || k_strncmp(p, "372", 3) == 0 || k_strncmp(p, "375", 3) == 0) {
        terminal_writestring("\x1b[33m-- ");
        terminal_writestring(line);
        terminal_writestring("\x1b[0m\n");
    }
}

int irc_connect(const char *server_ip, const char *nick, const char *channel) {
    uint8_t ip[4] = {0,0,0,0}; int oct = 0; const char *p = server_ip;
    while (*p && oct < 4) {
        int n = 0; while (*p >= '0' && *p <= '9') n = n*10 + (*p++)-'0';
        ip[oct++] = (uint8_t)n; if (*p == '.') p++;
    }

    terminal_printf("[irc] connecting to %d.%d.%d.%d:6667\n",
        ip[0],ip[1],ip[2],ip[3]);
    tcp_socket_t *sock = tcp_connect(ip, 6667);
    if (!sock) { terminal_printf("[irc] connection failed\n"); return -1; }

    /* Registration */
    {
        char buf[128];
        /* NICK */
        char *b = buf; const char *s = "NICK "; while(*s) *b++=*s++; s=nick; while(*s) *b++=*s++; *b++='\r'; *b++='\n'; *b=0;
        irc_send(sock, buf);
        /* USER */
        b = buf; s = "USER "; while(*s) *b++=*s++; s=nick; while(*s) *b++=*s++;
        s = " 0 * :Scorpion OS user\r\n"; while(*s) *b++=*s++; *b=0;
        irc_send(sock, buf);
    }

    terminal_printf("[irc] registered as %s\n", nick);
    terminal_printf("[irc] joining %s\n", channel);
    terminal_printf("[irc] type /quit to disconnect, or just type to send to channel\n\n");

    /* Wait a moment for server welcome */
    { volatile int i=0; while(i<0x800000) i++; }

    /* Join channel */
    {
        char buf[128]; char *b = buf;
        const char *s = "JOIN "; while(*s) *b++=*s++; s=channel; while(*s) *b++=*s++;
        *b++='\r'; *b++='\n'; *b=0;
        irc_send(sock, buf);
    }

    /* Receive/send loop */
    char input[512]; int input_pos = 0;
    uint8_t net_buf[4096];

    while (1) {
        /* Non-blocking keyboard */
        char c = keyboard_poll();
        if (c) {
            if (c == '\n' || c == '\r') {
                input[input_pos] = 0;
                if (input[0] == '/') {
                    if (input[1]=='q' && input[2]=='u') { /* /quit */
                        irc_send(sock, "QUIT :bye\r\n");
                        break;
                    } else if (input[1]=='m' && input[2]=='e') {
                        /* /me action */
                        char buf[512];
                        char *b = buf; const char *s = "PRIVMSG "; while(*s) *b++=*s++;
                        s=channel; while(*s) *b++=*s++;
                        s=" :\x01" "ACTION "; while(*s) *b++=*s++;
                        s=input+4; while(*s) *b++=*s++;
                        *b++='\x01'; *b++='\r'; *b++='\n'; *b=0;
                        irc_send(sock, buf);
                    }
                } else if (input_pos > 0) {
                    /* Send as PRIVMSG */
                    char buf[512]; char *b = buf;
                    const char *s = "PRIVMSG "; while(*s) *b++=*s++;
                    s = channel; while(*s) *b++=*s++;
                    *b++=' '; *b++=':';
                    s = input; while(*s) *b++=*s++;
                    *b++='\r'; *b++='\n'; *b=0;
                    irc_send(sock, buf);
                    /* Echo locally */
                    terminal_writestring("\x1b[36m<");
                    terminal_writestring(nick);
                    terminal_writestring("> \x1b[0m");
                    terminal_writestring(input);
                    terminal_putchar('\n');
                }
                input_pos = 0;
                terminal_putchar('\n');
            } else if (c == '\b') {
                if (input_pos > 0) {
                    input_pos--;
                    terminal_putchar('\b'); terminal_putchar(' '); terminal_putchar('\b');
                }
            } else if (c >= 32 && input_pos < 510) {
                input[input_pos++] = c;
                terminal_putchar(c);
            }
        }

        /* Non-blocking network receive */
        int n = tcp_recv(sock, net_buf, sizeof(net_buf)-1, 10);
        if (n > 0) {
            net_buf[n] = 0;
            /* Split on \r\n and process each line */
            char *line = (char*)net_buf;
            for (int i = 0; i < n; i++) {
                if (net_buf[i] == '\n') {
                    net_buf[i] = 0;
                    if (i > 0 && net_buf[i-1] == '\r') net_buf[i-1] = 0;
                    /* Handle PING */
                    if (line[0]=='P'&&line[1]=='I'&&line[2]=='N'&&line[3]=='G') {
                        char pong[64]; char *b=pong;
                        const char *s="PONG "; while(*s) *b++=*s++;
                        s=line+5; while(*s) *b++=*s++;
                        *b++='\r'; *b++='\n'; *b=0;
                        irc_send(sock, pong);
                    } else {
                        irc_display(line);
                    }
                    line = (char*)net_buf + i + 1;
                }
            }
        } else if (n == 0) {
            terminal_printf("[irc] connection closed by server\n");
            break;
        }
    }

    tcp_close(sock);
    terminal_printf("[irc] disconnected\n");
    return 0;
}
