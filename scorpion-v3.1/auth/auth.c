/* scorpion OS - auth/auth.c
   User authentication, /etc/passwd management, login prompt  */

#include "auth.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include "../drivers/keyboard.h"
#include "../fs/sfs.h"
#include <stdint.h>
#include <stddef.h>

/* ---- string helpers ---- */
static size_t k_strlen(const char *s){size_t n=0;while(s[n])n++;return n;}
static int    k_strcmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static void   k_strcpy(char *d,const char *s){while((*d++=*s++));}
static void   k_strncpy(char *d,const char *s,size_t n){size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]=0;}
static void   k_memset(void *d,uint8_t v,size_t n){uint8_t*p=d;while(n--)*p++=v;}

/* uint32 → 8 hex chars */
static void u32_to_hex(uint32_t v, char *out) {
    for (int i = 7; i >= 0; i--) {
        int d = (int)(v & 0xF);
        out[i] = (char)(d < 10 ? '0'+d : 'a'+(d-10));
        v >>= 4;
    }
    out[8] = 0;
}

/* 8 hex chars → uint32 */
static uint32_t hex_to_u32(const char *s) {
    uint32_t v = 0;
    for (int i = 0; i < 8 && s[i]; i++) {
        v <<= 4;
        char c = s[i];
        if (c >= '0' && c <= '9') v += (uint32_t)(c-'0');
        else if (c >= 'a' && c <= 'f') v += (uint32_t)(c-'a'+10);
        else if (c >= 'A' && c <= 'F') v += (uint32_t)(c-'A'+10);
    }
    return v;
}

/* ============================================================
   Password hashing: djb2 XOR fold to 32 bits
   ============================================================ */
uint32_t auth_hash_password(const char *password) {
    uint32_t h = 5381;
    unsigned char c;
    while ((c = (unsigned char)*password++) != 0)
        h = ((h << 5) + h) ^ (uint32_t)c;
    /* XOR upper and lower 16 bits back together for compactness */
    h = h ^ (h >> 16);
    return h;
}

/* ============================================================
   Session globals
   ============================================================ */
auth_user_t current_user;
int         user_logged_in = 0;

/* ============================================================
   /etc/passwd parsing
   Format: username:hash8hex:uid:gid:home:shell\n
   ============================================================ */

/* Parse one field from a colon-delimited line, advance *p past the ':' */
static void parse_field(const char **p, char *out, size_t maxlen) {
    size_t i = 0;
    while (**p && **p != ':' && **p != '\n' && i < maxlen-1)
        out[i++] = *(*p)++;
    out[i] = 0;
    if (**p == ':') (*p)++;
}

static uint32_t parse_u32(const char **p) {
    uint32_t v = 0;
    while (**p >= '0' && **p <= '9') v = v*10 + (uint32_t)(*(*p)++ - '0');
    if (**p == ':') (*p)++;
    return v;
}

int auth_verify(const char *username, const char *password, auth_user_t *out) {
    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (!etc) return 0;
    sfs_node_t *pw_file = sfs_find_child(etc, "passwd");
    if (!pw_file || pw_file->type != SFS_FILE || !pw_file->data) return 0;

    uint32_t input_hash = auth_hash_password(password);

    const char *data = (const char*)pw_file->data;
    size_t len = pw_file->size;
    size_t pos = 0;

    while (pos < len) {
        /* Read one line */
        char line[256]; size_t li = 0;
        while (pos < len && data[pos] != '\n' && li < 255)
            line[li++] = data[pos++];
        line[li] = 0;
        if (pos < len && data[pos] == '\n') pos++;
        if (line[0] == '#' || li == 0) continue;

        const char *p = line;
        char uname[AUTH_NAME_MAX];
        char hash_str[16];
        char home[AUTH_HOME_MAX];
        char shell[AUTH_SHELL_MAX];

        parse_field(&p, uname,    AUTH_NAME_MAX);
        parse_field(&p, hash_str, 16);
        uint32_t stored_uid = parse_u32(&p);
        uint32_t stored_gid = parse_u32(&p);
        parse_field(&p, home,  AUTH_HOME_MAX);
        parse_field(&p, shell, AUTH_SHELL_MAX);

        if (k_strcmp(uname, username) != 0) continue;

        /* Locked account */
        if (hash_str[0] == '!') return 0;

        uint32_t stored_hash = hex_to_u32(hash_str);
        if (stored_hash != input_hash) return 0;

        /* Success */
        if (out) {
            k_strncpy(out->username, uname,  AUTH_NAME_MAX);
            k_strncpy(out->home,     home,   AUTH_HOME_MAX);
            k_strncpy(out->shell,    shell,  AUTH_SHELL_MAX);
            out->uid    = stored_uid;
            out->gid    = stored_gid;
            out->locked = 0;
        }
        return 1;
    }
    return 0;
}

int auth_add_user(const char *username, const char *password,
                  uint32_t uid, uint32_t gid,
                  const char *home, const char *shell) {
    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (!etc) return -1;

    /* Build new line */
    char hash_str[9];
    u32_to_hex(auth_hash_password(password), hash_str);

    /* uid/gid to string */
    char uid_s[12], gid_s[12];
    {
        uint32_t v=uid; int i=0;
        if(v==0){uid_s[i++]='0';}else{char tmp[12];int ti=0;while(v){tmp[ti++]=(char)('0'+v%10);v/=10;}while(ti--)uid_s[i++]=tmp[ti];}
        uid_s[i]=0;
    }
    {
        uint32_t v=gid; int i=0;
        if(v==0){gid_s[i++]='0';}else{char tmp[12];int ti=0;while(v){tmp[ti++]=(char)('0'+v%10);v/=10;}while(ti--)gid_s[i++]=tmp[ti];}
        gid_s[i]=0;
    }

    /* Assemble line: username:hash:uid:gid:home:shell\n */
    char new_line[256];
    size_t nl = 0;
    #define APPEND(str) do { const char *_s=(str); while(*_s && nl<254) new_line[nl++]=*_s++; } while(0)
    APPEND(username); new_line[nl++]=':';
    APPEND(hash_str); new_line[nl++]=':';
    APPEND(uid_s);    new_line[nl++]=':';
    APPEND(gid_s);    new_line[nl++]=':';
    APPEND(home);     new_line[nl++]=':';
    APPEND(shell);    new_line[nl++]='\n';
    new_line[nl] = 0;
    #undef APPEND

    /* Read existing content */
    sfs_node_t *pw_file = sfs_find_child(etc, "passwd");
    size_t old_len = (pw_file && pw_file->data) ? pw_file->size : 0;
    size_t new_total = old_len + nl;

    char *buf = (char*)kmalloc(new_total + 1);
    if (!buf) return -1;

    if (old_len && pw_file->data) {
        for (size_t i = 0; i < old_len; i++) buf[i] = ((char*)pw_file->data)[i];
    }
    for (size_t i = 0; i < nl; i++) buf[old_len + i] = new_line[i];
    buf[new_total] = 0;

    sfs_write_file(etc, "passwd", (const uint8_t*)buf, new_total);
    kfree(buf);
    return 0;
}

/* ============================================================
   Hostname
   ============================================================ */
void auth_get_hostname(char *buf, size_t len) {
    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (etc) {
        sfs_node_t *hn = sfs_find_child(etc, "hostname");
        if (hn && hn->data && hn->size > 0) {
            size_t copy = hn->size < len-1 ? hn->size : len-1;
            for (size_t i = 0; i < copy; i++) buf[i] = ((char*)hn->data)[i];
            /* Strip trailing newline */
            while (copy > 0 && (buf[copy-1]=='\n'||buf[copy-1]=='\r')) copy--;
            buf[copy] = 0;
            return;
        }
    }
    k_strncpy(buf, "scorpion", len);
}

int auth_set_hostname(const char *name) {
    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (!etc) return -1;
    char buf[128];
    size_t i = 0;
    while (name[i] && i < 126) { buf[i] = name[i]; i++; }
    buf[i++] = '\n'; buf[i] = 0;
    sfs_write_file(etc, "hostname", (const uint8_t*)buf, i);
    return 0;
}

/* ============================================================
   auth_init_etc: seed /etc with defaults if missing
   ============================================================ */
void auth_init_etc(void) {
    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (!etc) etc = sfs_mkdir(sfs_root(), "etc");
    if (!etc) return;

    /* /etc/hostname */
    if (!sfs_find_child(etc, "hostname"))
        sfs_write_file(etc, "hostname", (const uint8_t*)"scorpion\n", 9);

    /* /etc/passwd — root with password "root" */
    if (!sfs_find_child(etc, "passwd")) {
        auth_add_user("root", "root", 0, 0, "/root", "/bin/sh");
    }

    /* /root home dir */
    sfs_node_t *root_home = sfs_resolve(sfs_root(), "/root");
    if (!root_home) sfs_mkdir(sfs_root(), "root");
}

/* ============================================================
   login_prompt: blocks until valid credentials entered
   ============================================================ */

/* Masked readline: echoes '*' instead of characters */
static int readline_masked(char *buf, size_t maxlen) {
    size_t pos = 0;
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n' || c == '\r') {
            buf[pos] = 0;
            terminal_putchar('\n');
            return (int)pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
        } else if (c >= 32 && c < 127 && pos < maxlen-1) {
            buf[pos++] = c;
            terminal_putchar('*');
        }
    }
}

/* Normal (unmasked) readline for username */
static int readline_plain(char *buf, size_t maxlen) {
    size_t pos = 0;
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n' || c == '\r') {
            buf[pos] = 0;
            terminal_putchar('\n');
            return (int)pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
        } else if (c >= 32 && c < 127 && pos < maxlen-1) {
            buf[pos++] = c;
            terminal_putchar(c);
        }
    }
}

void login_prompt(void) {
    char hostname[64];
    auth_get_hostname(hostname, sizeof(hostname));

    /* Max 3 failed attempts, then lock for a bit and retry */
    while (1) {
        int attempts = 0;
        while (attempts < 3) {
            /* Print login banner */
            terminal_writestring("\x1b[32m");   /* green */
            terminal_writestring(hostname);
            terminal_writestring(" login: ");
            terminal_writestring("\x1b[0m");

            char username[AUTH_NAME_MAX];
            readline_plain(username, AUTH_NAME_MAX);
            if (!username[0]) continue;

            terminal_writestring("Password: ");
            char password[128];
            readline_masked(password, 128);

            if (auth_verify(username, password, &current_user)) {
                user_logged_in = 1;
                /* Clear password from stack ASAP */
                k_memset(password, 0, sizeof(password));
                terminal_writestring("\x1b[32m");
                terminal_writestring("Login successful.\n");
                terminal_writestring("\x1b[0m");
                return;
            }

            k_memset(password, 0, sizeof(password));
            attempts++;
            terminal_writestring("\x1b[31mLogin incorrect.\x1b[0m\n");
        }

        /* 3 failures — spin for a few seconds */
        terminal_writestring("Too many failures. Try again in a moment.\n");
        for (volatile int i = 0; i < 0x8000000; i++);
    }
}
