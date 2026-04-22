

static void execute(char **argv, int argc);
void pipe_execute(const char *cmd1, const char *cmd2);

static size_t slen(const char *s) {
  size_t n = 0;
  while (s[n])
    n++;
  return n;
}
static int scmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}
static int sncmp(const char *a, const char *b, size_t n) {
  while (n && *a && *a == *b) {
    a++;
    b++;
    n--;
  }
  return n ? (unsigned char)*a - (unsigned char)*b : 0;
}
static void scpy(char *d, const char *s) {
  while ((*d++ = *s++))
    ;
}
static void sncpy(char *d, const char *s, size_t n) {
  size_t i = 0;
  while (i < n - 1 && s[i]) {
    d[i] = s[i];
    i++;
  }
  d[i] = 0;
}
static int sisspace(char c) { return c == ' ' || c == '\t'; }
static int sisdigit(char c) { return c >= '0' && c <= '9'; }
static int satoi(const char *s) {
  int n = 0, neg = 0;
  if (*s == '-') {
    neg = 1;
    s++;
  }
  while (sisdigit(*s))
    n = n * 10 + (*s++) - '0';
  return neg ? -n : n;
}
static unsigned long satoul_hex(const char *s) {
  unsigned long n = 0;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;
  while ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') ||
         (*s >= 'A' && *s <= 'F')) {
    n <<= 4;
    if (*s >= '0' && *s <= '9')
      n += *s - '0';
    else if (*s >= 'a' && *s <= 'f')
      n += *s - 'a' + 10;
    else
      n += *s - 'A' + 10;
    s++;
  }
  return n;
}
static unsigned long satoul_octal(const char *s) {
  unsigned long n = 0;
  while (*s >= '0' && *s <= '7')
    n = n * 8 + (unsigned long)(*s++) - '0';
  return n;
}

static int tokenize(char *line, char **argv, int max) {
  int n = 0;
  char *p = line;
  while (*p && n < max) {
    while (*p && sisspace(*p))
      p++;
    if (!*p)
      break;
    argv[n++] = p;
    while (*p && !sisspace(*p))
      p++;
    if (*p)
      *p++ = 0;
  }
  return n;
}

static sfs_node_t *cwd;
static sfs_node_t *prev_cwd;
static char input_buf[512];
part_entry_t g_shell_parts[MAX_PARTITIONS];
int g_shell_part_count = 0;

static char custom_prompt[128] = "";
void shell_set_prompt(const char *p) { sncpy(custom_prompt, p, 128); }

static char history_buf[HISTORY_MAX][512];
static int history_count = 0;
static void history_add(const char *line) {
  if (!line || !line[0])
    return;
  if (history_count > 0 &&
      scmp(history_buf[(history_count - 1) % HISTORY_MAX], line) == 0)
    return;
  sncpy(history_buf[history_count % HISTORY_MAX], line, 512);
  history_count++;
}
static const char *history_get(int n) {

  if (history_count == 0)
    return NULL;
  int idx = history_count - 1 - n;
  if (idx < 0)
    return NULL;
  if (history_count > HISTORY_MAX && idx < history_count - HISTORY_MAX)
    return NULL;
  return history_buf[idx % HISTORY_MAX];
}

static struct {
  char name[32];
  char val[128];
} aliases[ALIAS_MAX];
static int alias_count = 0;
const char *alias_resolve(const char *name) {
  for (int i = 0; i < alias_count; i++)
    if (scmp(aliases[i].name, name) == 0)
      return aliases[i].val;
  return NULL;
}

static struct {
  char key[32];
  char val[128];
} env_vars[ENV_MAX];
int env_count = 0;
void env_set(const char *k, const char *v) {
  for (int i = 0; i < env_count; i++) {
    if (scmp(env_vars[i].key, k) == 0) {
      sncpy(env_vars[i].val, v, 128);
      return;
    }
  }
  if (env_count < ENV_MAX) {
    sncpy(env_vars[env_count].key, k, 32);
    sncpy(env_vars[env_count].val, v, 128);
    env_count++;
  }
}
const char *env_get(const char *k) {
  for (int i = 0; i < env_count; i++)
    if (scmp(env_vars[i].key, k) == 0)
      return env_vars[i].val;
  return "";
}
void env_unset(const char *k) {
  for (int i = 0; i < env_count; i++) {
    if (scmp(env_vars[i].key, k) == 0) {
      for (int j = i; j < env_count - 1; j++)
        env_vars[j] = env_vars[j + 1];
      env_count--;
      return;
    }
  }
}
void cmd_env_cmd(void) {
  for (int i = 0; i < env_count; i++)
    terminal_printf("%s=%s\n", env_vars[i].key, env_vars[i].val);
}

static sfs_node_t *dirstack[DIRSTACK_MAX];
static int dirstack_top = 0;

char shell_getchar(void) { return keyboard_getchar(); }

static void rl_redraw(const char *buf, size_t len, size_t cur, int mask) {
  terminal_writestring("\x1b[u");
  terminal_writestring("\x1b[K");
  for (size_t i = 0; i < len; i++)
    terminal_putchar(mask ? '*' : buf[i]);
  if (cur < len) {
    size_t back = len - cur;
    terminal_printf("\x1b[%dD", (int)back);
  }
}

static const char *tab_cmds[] = {
    "acpi",    "adduser",   "alias",    "append",     "ascii",      "base64",
    "based",   "banner",    "beep",     "boykisser",  "brainrot",   "calc",
    "cat",     "cd",        "chad",     "chmod",      "chown",      "clear",
    "cls",     "color",     "colour",   "compress",   "cowsay",     "cp",
    "crc32",   "curl",      "cut",      "date",       "decompress", "df",
    "dhcp",    "diff",      "diskinfo", "diskload",   "dllist",     "dlopen",
    "dmesg",   "dns",       "doge",     "doomscroll", "draw",       "du",
    "du",      "echo",      "emacs",    "env",        "exit",       "export",
    "false",   "fdisk",     "fetch",    "figlet",     "find",       "flip",
    "fortune", "free",      "fstab",    "gigachad",   "grep",       "groups",
    "halt",    "head",      "help",     "hexcalc",    "hexdump",    "hi",
    "history", "hostname",  "id",       "ifconfig",   "ip",         "install",
    "irc",     "kill",      "less",     "ligma",      "lmao",       "ln",
    "lolcat",  "logout",    "ls",       "lscpu",      "lspci",      "lsusb",
    "make",    "man",       "matrix",   "mbf",        "md5sum",     "meminfo",
    "memtest", "mkdir",     "mkpart",   "modules",    "more",       "morse",
    "mv",      "nano",      "nc",       "netcfg",     "nl",         "nslookup",
    "nyan",    "od",        "ohio",     "passwd",     "paste",      "pipes",
    "ping",    "play",      "popd",     "poweroff",   "printenv",   "ps",
    "pushd",   "pwd",       "random",   "rand",       "ratio",      "reboot",
    "repeat",  "reset",     "rev",      "rickroll",   "rick",       "rm",
    "rmpart",  "rot13",     "route",    "rtl-cfg",    "run",        "scrap",
    "seq",     "sha256sum", "shrug",    "shutdown",   "skibidi",    "sl",
    "sleep",   "smpinfo",   "sort",     "sound",      "speed",      "spin",
    "ssh",     "stat",      "strings",  "tac",        "tableflip",  "tail",
    "tee",     "time",      "top",      "touch",      "toupper",    "tolower",
    "tr",      "tree",      "true",     "type",       "unalias",    "uniq",
    "unset",   "uname",     "updog",    "uptime",     "watch",      "wc",
    "wget",    "which",     "whoami",   "wifi",       "xxd",        "yaoigui",
    "yeet",    "yes",       "42",       "explorer",   NULL};

static int tab_common_prefix(char matches[][64], int n) {
  if (n == 0)
    return 0;
  int len = 0;
  while (1) {
    char c = matches[0][len];
    if (!c)
      break;
    int ok = 1;
    for (int i = 1; i < n; i++)
      if (matches[i][len] != c) {
        ok = 0;
        break;
      }
    if (!ok)
      break;
    len++;
  }
  return len;
}

static size_t tab_complete(char *buf, size_t len, size_t cur, size_t maxlen,
                           size_t *cur_out) {
  size_t wstart = cur;
  while (wstart > 0 && buf[wstart - 1] != ' ')
    wstart--;
  size_t wlen = cur - wstart;
  char word[128];
  int wi = 0;
  for (size_t i = wstart; i < cur && wi < 127; i++) {
    word[wi++] = buf[i];
  }
  word[wi] = 0;
  int is_cmd = 1;
  for (size_t i = 0; i < wstart; i++)
    if (buf[i] != ' ') {
      is_cmd = 0;
      break;
    }
  char matches[64][64];
  int nm = 0;
  if (is_cmd) {
    for (int i = 0; tab_cmds[i] && nm < 64; i++) {
      const char *c = tab_cmds[i];
      int match = 1;
      for (int j = 0; j < wi; j++)
        if (c[j] != word[j]) {
          match = 0;
          break;
        }
      if (match && nm < 64) {
        int k = 0;
        while (c[k] && k < 63)
          matches[nm][k] = c[k], k++;
        matches[nm][k] = 0;
        nm++;
      }
    }
  } else {
    sfs_node_t *dir = cwd;
    char dirpart[128] = "";
    char filepart[128] = "";
    int slash = -1;
    for (int i = wi - 1; i >= 0; i--)
      if (word[i] == '/') {
        slash = i;
        break;
      }
    if (slash >= 0) {
      sncpy(dirpart, word, (size_t)slash + 1);
      scpy(filepart, word + slash + 1);
      sfs_node_t *d2 = sfs_resolve(cwd, dirpart);
      if (d2 && d2->type == SFS_DIR)
        dir = d2;
    } else
      scpy(filepart, word);
    size_t fplen = slen(filepart);
    for (sfs_node_t *c = dir->child; c && nm < 64; c = c->next) {
      int match = 1;
      for (size_t k = 0; k < fplen; k++)
        if (c->name[k] != filepart[k]) {
          match = 0;
          break;
        }
      if (match) {
        int k = 0;
        if (slash >= 0) {
          sncpy(matches[nm], dirpart, 64);
          k = (int)slen(matches[nm]);
        }
        while (c->name[k - (slash >= 0 ? (int)slen(dirpart) : 0)] && k < 63)
          matches[nm][k] = c->name[k - (slash >= 0 ? (int)slen(dirpart) : 0)],
          k++;
        matches[nm][k] = 0;
        nm++;
      }
    }
  }
  if (nm == 0)
    return len;
  if (nm == 1) {
    size_t ml = slen(matches[0]);
    for (size_t i = wstart; i < wstart + ml && i < maxlen - 1; i++)
      buf[i] = matches[0][i - wstart];
    if (wstart + ml < maxlen - 1 &&
        (is_cmd || (sfs_resolve(cwd, matches[0]) &&
                    sfs_resolve(cwd, matches[0])->type == SFS_DIR)))
      buf[wstart + ml] = ' ', ml++;
    len = wstart + ml;
    *cur_out = len;
  } else {
    int cp = tab_common_prefix(matches, nm);
    if (cp > (int)wlen) {
      for (int i = 0; i < cp && wstart + (size_t)i < maxlen - 1; i++)
        buf[wstart + i] = matches[0][i];
      len = wstart + (size_t)cp;
      *cur_out = len;
    } else {
      terminal_putchar('\n');
      for (int i = 0; i < nm; i++) {
        terminal_writestring(matches[i]);
        terminal_putchar(' ');
      }
      terminal_putchar('\n');
      *cur_out = cur;
    }
  }
  return len;
}

int readline(char *buf, int maxlen) {
  size_t len = 0, cur = 0;
  buf[0] = 0;

  terminal_writestring("\x1b[s");
  int hist_offset = -1;
  char original_buf[512] = "";

  while (1) {
    char c = keyboard_getchar();

    if (c == '\r' || c == '\n') {
      terminal_putchar('\n');
      buf[len] = 0;
      return (int)len;
    }

    if (c == '\b' || c == 127) {
      if (cur > 0) {
        for (size_t i = cur - 1; i < len - 1; i++)
          buf[i] = buf[i + 1];
        len--;
        cur--;
        rl_redraw(buf, len, cur, 0);
      }
      continue;
    }

    if (c == 4 && len == 0) {
      buf[0] = 0;
      return -1;
    }

    if (c == 9) {
      size_t nc = cur;
      len = tab_complete(buf, (size_t)maxlen - 1, cur, (size_t)maxlen, &nc);
      cur = nc;
      rl_redraw(buf, len, cur, 0);
      continue;
    }

    if (c == 27) {
      char c2 = keyboard_getchar();
      if (c2 == '[') {
        char c3 = keyboard_getchar();
        if (c3 == 'A') {
          if (hist_offset == -1)
            sncpy(original_buf, buf, 512);
          hist_offset++;
          const char *h = history_get(hist_offset);
          if (h) {
            sncpy(buf, h, (size_t)maxlen);
            len = slen(buf);
            cur = len;
          } else {
            hist_offset--;
          }
          rl_redraw(buf, len, cur, 0);
        } else if (c3 == 'B') {
          if (hist_offset > 0) {
            hist_offset--;
            const char *h = history_get(hist_offset);
            if (h) {
              sncpy(buf, h, (size_t)maxlen);
              len = slen(buf);
              cur = len;
            }
          } else if (hist_offset == 0) {
            hist_offset = -1;
            sncpy(buf, original_buf, (size_t)maxlen);
            len = slen(buf);
            cur = len;
          }
          rl_redraw(buf, len, cur, 0);
        } else if (c3 == 'C') {
          if (cur < len) {
            cur++;
            rl_redraw(buf, len, cur, 0);
          }
        } else if (c3 == 'D') {
          if (cur > 0) {
            cur--;
            rl_redraw(buf, len, cur, 0);
          }
        } else if (c3 == 'H' || c3 == '1') {
          cur = 0;
          rl_redraw(buf, len, cur, 0);
        } else if (c3 == 'F' || c3 == '4') {
          cur = len;
          rl_redraw(buf, len, cur, 0);
        } else if (c3 == '3') {
          char c4 = keyboard_getchar();
          if (c4 == '~' && cur < len) {
            for (size_t i = cur; i < len - 1; i++)
              buf[i] = buf[i + 1];
            len--;
            rl_redraw(buf, len, cur, 0);
          }
        }
      }
      continue;
    }

    if (c == 1) {
      cur = 0;
      rl_redraw(buf, len, cur, 0);
      continue;
    }
    if (c == 5) {
      cur = len;
      rl_redraw(buf, len, cur, 0);
      continue;
    }
    if (c == 3) {
      terminal_writestring("^C\n");
      buf[0] = 0;
      return 0;
    }
    if (c == 11) {
      len = cur;
      buf[len] = 0;
      rl_redraw(buf, len, cur, 0);
      continue;
    }
    if (c == 21) {
      len = 0;
      cur = 0;
      buf[0] = 0;
      rl_redraw(buf, len, cur, 0);
      continue;
    }

    if (c < 32)
      continue;
    if (len < (size_t)maxlen - 1) {
      for (size_t i = len; i > cur; i--)
        buf[i] = buf[i - 1];
      buf[cur++] = c;
      len++;
      rl_redraw(buf, len, cur, 0);
    }
  }
}

int readline_masked(char *buf, int maxlen) {
  size_t len = 0;
  buf[0] = 0;
  terminal_writestring("\x1b[s");
  while (1) {
    char c = keyboard_getchar();
    if (c == '\r' || c == '\n') {
      terminal_putchar('\n');
      buf[len] = 0;
      return (int)len;
    }
    if (c == '\b' || c == 127) {
      if (len > 0) {
        len--;
        rl_redraw(buf, len, len, 1);
      }
      continue;
    }
    if (c < 32 || (int)len >= (int)maxlen - 1)
      continue;
    buf[len++] = c;
    rl_redraw(buf, len, len, 1);
  }
}

void paged_write(const char *data, size_t sz) {
  int lines = 0;
  size_t i = 0;
  while (i < sz) {
    terminal_putchar(data[i]);
    if (data[i] == '\n') {
      lines++;
      if (lines >= 23) {
        terminal_printf("-- More -- (space=next page q=quit any=continue)");
        char c = keyboard_getchar();
        terminal_putchar('\n');
        if (c == 'q')
          return;
        if (c == ' ')
          lines = 0;
        else
          lines = 0;
      }
    }
    i++;
  }
}

static void ls_cb(sfs_node_t *n, void *_) {
  (void)_;
  if (!n)
    return;
  int is_dir = (n->type == SFS_DIR);
  if (is_dir)
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  else if (n->mode && (n->mode & 0111))
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  else
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  terminal_writestring(n->name);
  if (is_dir)
    terminal_putchar('/');
  terminal_putchar('\n');
  terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static void tree_node(sfs_node_t *n, int d) {
  if (!n)
    return;
  for (int i = 0; i < d; i++)
    terminal_writestring(i == d - 1 ? "├── " : "│   ");
  if (n->type == SFS_DIR) {
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring(n->name);
    terminal_putchar('/');
    terminal_putchar('\n');
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (sfs_node_t *c = n->child; c; c = c->next)
      tree_node(c, d + 1);
  } else {
    terminal_writestring(n->name);
    terminal_putchar('\n');
  }
}

static int konami_state = 0;
static const char konami_seq[] = {'A', 'A', 'B', 'B', 'D',
                                  'C', 'D', 'C', 'b', 'a'};
static void check_konami(char c) {
  if (c == konami_seq[konami_state])
    konami_state++;
  else
    konami_state = 0;
  if (konami_state >= (int)slen(konami_seq)) {
    konami_state = 0;
    terminal_clear();
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_printf(
        "\n  +30 LIVES ACTIVATED\n  Konami Code accepted! stinger OS is now in "
        "GOD MODE.\n  (Not really. But nice try.)\n\n");
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  }
}

static void cmd_cd(char **argv, int argc) {
  if (argc < 2) {
    prev_cwd = cwd;
    cwd = sfs_root();
    return;
  }
  if (scmp(argv[1], "-") == 0) {
    if (!prev_cwd) {
      terminal_printf("cd: no previous directory\n");
      return;
    }
    sfs_node_t *tmp = cwd;
    cwd = prev_cwd;
    prev_cwd = tmp;
    char p[256];
    sfs_path(cwd, p, sizeof(p));
    terminal_printf("%s\n", p);
    return;
  }
  sfs_node_t *n = sfs_resolve(cwd, argv[1]);
  if (!n) {
    terminal_printf("cd: not found: %s\n", argv[1]);
    return;
  }
  if (n->type != SFS_DIR) {
    terminal_printf("cd: not a directory: %s\n", argv[1]);
    return;
  }
  prev_cwd = cwd;
  cwd = n;
}

static void cmd_pushd(char **argv, int argc) {
  if (argc < 2) {
    terminal_printf("usage: pushd <dir>\n");
    return;
  }
  sfs_node_t *n = sfs_resolve(cwd, argv[1]);
  if (!n || n->type != SFS_DIR) {
    terminal_printf("pushd: not a directory: %s\n", argv[1]);
    return;
  }
  if (dirstack_top < DIRSTACK_MAX) {
    dirstack[dirstack_top++] = cwd;
    cwd = n;
    char p[256];
    sfs_path(cwd, p, sizeof(p));
    terminal_printf("%s\n", p);
  } else
    terminal_printf("pushd: stack full\n");
}

static void cmd_popd(void) {
  if (dirstack_top == 0) {
    terminal_printf("popd: directory stack empty\n");
    return;
  }
  cwd = dirstack[--dirstack_top];
  char p[256];
  sfs_path(cwd, p, sizeof(p));
  terminal_printf("%s\n", p);
}

static void cmd_dirs(void) {
  char p[256];
  sfs_path(cwd, p, sizeof(p));
  terminal_printf("%s", p);
  for (int i = dirstack_top - 1; i >= 0; i--) {
    sfs_path(dirstack[i], p, sizeof(p));
    terminal_printf(" %s", p);
  }
  terminal_putchar('\n');
}

static void cmd_export(char **argv, int argc) {
  if (argc < 2) {
    cmd_env_cmd();
    return;
  }
  for (int i = 1; i < argc; i++) {
    char *eq = argv[i];
    while (*eq && *eq != '=')
      eq++;
    if (*eq == '=') {
      char key[32];
      sncpy(key, argv[i], (size_t)(eq - argv[i]) + 1);
      key[eq - argv[i]] = 0;
      env_set(key, eq + 1);
    } else if (i + 1 < argc) {
      const char *k = argv[i];
      i++;
      env_set(k, argv[i]);
    } else
      terminal_printf("%s=%s\n", argv[i], env_get(argv[i]));
  }
}

static void cmd_alias(char **argv, int argc) {
  if (argc < 2) {
    for (int i = 0; i < alias_count; i++)
      terminal_printf("alias %s='%s'\n", aliases[i].name, aliases[i].val);
    return;
  }
  char *eq = argv[1];
  while (*eq && *eq != '=')
    eq++;
  if (*eq == '=') {
    char key[32];
    sncpy(key, argv[1], (size_t)(eq - argv[1]) + 1);
    key[eq - argv[1]] = 0;
    const char *v = eq + 1;
    if (*v == '\'')
      v++;
    char val[128];
    sncpy(val, v, 128);
    size_t vl = slen(val);
    if (vl && val[vl - 1] == '\'')
      val[vl - 1] = 0;
    for (int i = 0; i < alias_count; i++)
      if (scmp(aliases[i].name, key) == 0) {
        sncpy(aliases[i].val, val, 128);
        return;
      }
    if (alias_count < ALIAS_MAX) {
      sncpy(aliases[alias_count].name, key, 32);
      sncpy(aliases[alias_count].val, val, 128);
      alias_count++;
    }
  } else if (argc >= 3) {
    for (int i = 0; i < alias_count; i++)
      if (scmp(aliases[i].name, argv[1]) == 0) {
        sncpy(aliases[i].val, argv[2], 128);
        return;
      }
    if (alias_count < ALIAS_MAX) {
      sncpy(aliases[alias_count].name, argv[1], 32);
      sncpy(aliases[alias_count].val, argv[2], 128);
      alias_count++;
    }
  }
}

static void cmd_unalias(char **argv, int argc) {
  if (argc < 2) {
    terminal_printf("unalias: missing name\n");
    return;
  }
  for (int i = 0; i < alias_count; i++) {
    if (scmp(aliases[i].name, argv[1]) == 0) {
      for (int j = i; j < alias_count - 1; j++)
        aliases[j] = aliases[j + 1];
      alias_count--;
      terminal_printf("removed '%s'\n", argv[1]);
      return;
    }
  }
  terminal_printf("unalias: no alias '%s'\n", argv[1]);
}

static void cmd_history_sh(char **argv, int argc) {
  (void)argv;
  (void)argc;
  int start = history_count > HISTORY_MAX ? history_count - HISTORY_MAX : 0;
  for (int i = start; i < history_count; i++)
    terminal_printf("%4d  %s\n", i + 1, history_buf[i % HISTORY_MAX]);
}

static void cmd_help(void) {
  terminal_setcolor(
      terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  terminal_writestring(
      "stinger OS v4.0  --  'man <topic>' for details  [1/3]\n\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  terminal_writestring(
      "  files     ls cd pwd cat more touch mkdir rm cp mv ln tree find stat\n"
      "  text      grep wc sort head tail rev hexdump xxd base64 rot13 nl cut "
      "tr\n"
      "            uniq tac paste diff od strings crc32 md5sum sha256sum "
      "toupper tolower\n"
      "  editor    nano / write / vi  (^S save  ^X exit  ^W find)\n"
      "  system    ps top free df du meminfo uname uptime dmesg lscpu date "
      "sleep seq\n"
      "  users     whoami id passwd adduser hostname chmod chown kill\n"
      "  disk      diskinfo lspart fdisk mkpart format install fstab sync\n"
      "  network   ifconfig ping wget dhcp route nc ssh irc nslookup dns wifi\n"
      "  hardware  lspci lsusb smpinfo acpi sound beep\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
  terminal_writestring("  -- press any key for page 2 --");
  keyboard_getchar();
  terminal_putchar('\n');

  terminal_setcolor(
      terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  terminal_writestring("stinger OS v4.0  [2/3]\n\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  terminal_writestring(
      "  tools     calc hexcalc ascii morse banner figlet lolcat color random\n"
      "            compress decompress which type watch time repeat scrap\n"
      "  shell     cd [-|dir]  pushd popd dirs  alias unalias  export unset\n"
      "            history  !! !N  pipe: cmd|cmd  redirect: cmd > file\n"
      "            prompt <fmt>  (\\u=user \\h=host \\w=cwd)\n"
      "  env       env export unset printenv alias history\n"
      "  libs      dllist dlopen <lib>  run <module>\n"
      "  apps      make <src.c>  make list  make remove <n>  modules\n"
      "  script    mbf <script>\n"
      "  draw      draw [file]   view <file.png>\n"
      "  pkgs      fetch <pkg>  fetch --list  fetch --update  fetch --cdn "
      "<host>\n"
      "  signals   kill -l  kill -<sig> <pid>  cmd1 | cmd2\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
  terminal_writestring("  -- press any key for page 3 --");
  keyboard_getchar();
  terminal_putchar('\n');

  terminal_setcolor(
      terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  terminal_writestring("stinger OS v4.0  [3/3] -- fun stuff\n\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  terminal_writestring(
      "  gui       yaoigui (compositor)  explorer (file manager)\n"
      "  fun       nyan matrix cowsay fortune sl 42 rickroll doge\n"
      "            brainrot ohio ratio gigachad shrug tableflip yeet based\n"
      "            boykisser doomscroll spin ligma updog pipes lolcat figlet "
      "banner\n"
      "  games     snake tetris pong\n"
      "  bench     sysinfo memtest cpubench\n"
      "  boot      clear reset reboot halt poweroff\n");
  terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static void cmd_man(char **argv, int argc) {
  if (argc < 2) {
    terminal_printf("usage: man <topic>\ntopics: files text editor system "
                    "users disk network dns wifi hardware sound tools shell "
                    "env libs apps script draw pkgs signals fun\n");
    return;
  }
  const char *t = argv[1];
  if (scmp(t, "files") == 0)
    terminal_printf(
        "FILES:\n  ls [-la]    list dir\n  cat/more/less  view\n  head/tail "
        "[-N]  lines\n  touch/mkdir [-p]/rm [-r]  create/delete\n  cp/mv/ln  "
        "copy/move/link\n  tree/find [-name]/stat\n  append/tee/echo  write\n  "
        "chmod/chown  permissions\n");
  else if (scmp(t, "text") == 0)
    terminal_printf(
        "TEXT:\n  grep [-ivnc] <pat> <file>\n  wc  sort [-rnu]  rev  tac  nl\n "
        " hexdump  xxd  strings  od\n  base64 [-d]  rot13  crc32  md5sum  "
        "sha256sum\n  cut -d <d> -f <n>  tr <from> <to> <file>\n  uniq [-c]  "
        "diff  paste  head  tail\n  toupper tolower\n");
  else if (scmp(t, "tools") == 0)
    terminal_printf(
        "TOOLS:\n  calc <a> <op> <b>      math (+/-


static void cmd_lspart(void) {
      if (g_shell_part_count == 0) {
        terminal_printf("no partitions\n");
        return;
      }
      partition_print_table(g_shell_parts, g_shell_part_count);
}
static void cmd_mkpart(char **argv, int argc) {
      if (argc < 6) {
        terminal_printf("usage: mkpart <d> <i> <type_hex> <lba> <mb>\n");
        return;
      }
      int drive = satoi(argv[1]), idx = satoi(argv[2]);
      uint8_t type = (uint8_t)satoul_hex(argv[3]);
      uint32_t start = (uint32_t)satoi(argv[4]), mb = (uint32_t)satoi(argv[5]);
      if (partition_create(drive, idx, type, start, mb) != 0)
        terminal_printf("error\n");
      else
        terminal_printf("done.\n");
      g_shell_part_count = partition_detect_all(g_shell_parts, MAX_PARTITIONS);
}
static void cmd_rmpart(char **argv, int argc) {
      if (argc < 3) {
        terminal_printf("usage: rmpart <d> <i>\n");
        return;
      }
      if (partition_delete(satoi(argv[1]), satoi(argv[2])) != 0)
        terminal_printf("error\n");
      else
        terminal_printf("deleted\n");
      g_shell_part_count = partition_detect_all(g_shell_parts, MAX_PARTITIONS);
}
static void cmd_format(char **argv, int argc) {
      if (argc < 3) {
        terminal_printf("usage: format <drive> <partition>\n");
        return;
      }
      int drive = satoi(argv[1]), idx = satoi(argv[2]);
      for (int i = 0; i < g_shell_part_count; i++) {
        if (g_shell_parts[i].drive == drive && g_shell_parts[i].index == idx) {
          uint8_t buf[512];
          for (int j = 0; j < 512; j++)
            buf[j] = 0;
          buf[0] = 'S';
          buf[1] = 'F';
          buf[2] = 'S';
          buf[3] = 0x01;
          buf[510] = 0x55;
          buf[511] = 0xAA;
          int r;
          if (drive >= SATA_DRIVE_BASE)
            r = ahci_write_sectors(drive - SATA_DRIVE_BASE,
                                   (uint64_t)g_shell_parts[i].lba_start, 1,
                                   buf);
          else
            r = ata_write_sectors(drive, g_shell_parts[i].lba_start, 1, buf);
          if (r == 0)
            terminal_printf("formatted\n");
          else
            terminal_printf("write error\n");
          return;
        }
      }
      terminal_printf("partition not found\n");
}
static void cmd_sync_sh(void) {
      cmd_sync(g_shell_parts, g_shell_part_count); }
static void cmd_diskload_sh(void) {
      terminal_printf("replace RAM fs? [y/N] ");
      char c = keyboard_getchar();
      terminal_putchar(c);
      terminal_putchar('\n');
      if (c != 'y' && c != 'Y') {
        terminal_printf("cancelled\n");
        return;
      }
      cmd_mount_disk(g_shell_parts, g_shell_part_count);
      cwd = sfs_root();
}
static void cmd_setlabel(char **argv, int argc) {
      if (argc < 4) {
        terminal_printf("usage: setlabel <d> <i> <label>\n");
        return;
      }
      int drive = satoi(argv[1]), idx = satoi(argv[2]);
      for (int i = 0; i < g_shell_part_count; i++) {
        if (g_shell_parts[i].drive == drive && g_shell_parts[i].index == idx) {
          int li = 0;
          for (char *p = argv[3]; *p && li < 11; p++)
            g_shell_parts[i].label[li++] = (char)*p;
          g_shell_parts[i].label[li] = 0;
          terminal_printf("label set\n");
          return;
        }
      }
      terminal_printf("not found\n");
}
static void cmd_install(void) {
      terminal_clear();
      terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
      terminal_printf(
          "\n  +-------------------------------------------------+\n");
      terminal_printf("  |  stinger OS Installer (Clone Mode)             |\n");
      terminal_printf(
          "  +-------------------------------------------------+\n\n");

      if (g_shell_part_count == 0) {
        terminal_printf("  No partitions detected!\n");
        return;
      }

      partition_print_table(g_shell_parts, g_shell_part_count);

      char s_drive[4], s_idx[4], d_drive[4], d_idx[4];

      terminal_printf("\n  --- 1. Select SOURCE (USB) Partition ---\n");
      terminal_printf("  Source drive no: ");
      readline(s_drive, 4);
      terminal_printf("  Source partition index: ");
      readline(s_idx, 4);

      terminal_printf("\n  --- 2. Select DESTINATION (HDD) Partition ---\n");
      terminal_printf("  Must be >= size of source partition.\n");
      terminal_printf("  Dest drive no: ");
      readline(d_drive, 4);
      terminal_printf("  Dest partition index: ");
      readline(d_idx, 4);

      int sd = satoi(s_drive), si = satoi(s_idx);
      int dd = satoi(d_drive), di = satoi(d_idx);

      int src_pi = -1, dst_pi = -1;
      for (int i = 0; i < g_shell_part_count; i++) {
        if (g_shell_parts[i].drive == sd && g_shell_parts[i].index == si)
          src_pi = i;
        if (g_shell_parts[i].drive == dd && g_shell_parts[i].index == di)
          dst_pi = i;
      }

      if (src_pi < 0 || dst_pi < 0) {
        terminal_printf("  Invalid partition selection.\n");
        return;
      }

      part_entry_t *ps = &g_shell_parts[src_pi];
      part_entry_t *pd = &g_shell_parts[dst_pi];

      if (pd->size_lba < ps->size_lba) {
        terminal_printf(
            "  ERROR: Destination partition is smaller than source!\n");
        return;
      }

      terminal_printf("\n  Cloning %u sectors...\n", ps->size_lba);
      int s_sata = (sd >= SATA_DRIVE_BASE),
          s_port = s_sata ? sd - SATA_DRIVE_BASE : sd;
      int d_sata = (dd >= SATA_DRIVE_BASE),
          d_port = d_sata ? dd - SATA_DRIVE_BASE : dd;

      uint8_t copy_buf[512 * 8];
      uint32_t chunks = ps->size_lba / 8;
      uint32_t rem = ps->size_lba % 8;

      for (uint32_t c = 0; c < chunks; c++) {
        int read_ok = s_sata ? ahci_read_sectors(s_port, ps->lba_start + c * 8,
                                                 8, copy_buf) == 0
                             : ata_read_sectors(s_port, ps->lba_start + c * 8,
                                                8, copy_buf) == 0;
        if (!read_ok) {
          terminal_printf("  Read error at chunk %u\n", c);
          return;
        }

        int write_ok = d_sata
                           ? ahci_write_sectors(d_port, pd->lba_start + c * 8,
                                                8, copy_buf) == 0
                           : ata_write_sectors(d_port, pd->lba_start + c * 8, 8,
                                               copy_buf) == 0;
        if (!write_ok) {
          terminal_printf("  Write error at chunk %u\n", c);
          return;
        }

        if (c > 0 && c % 1024 == 0)
          terminal_printf("  Progress: %u MB copied...\r", (c * 8) / 2048);
      }

      if (rem > 0) {
        uint32_t off = chunks * 8;
        int read_ok = s_sata ? ahci_read_sectors(s_port, ps->lba_start + off,
                                                 rem, copy_buf) == 0
                             : ata_read_sectors(s_port, ps->lba_start + off,
                                                rem, copy_buf) == 0;
        if (read_ok) {
          if (d_sata)
            ahci_write_sectors(d_port, pd->lba_start + off, rem, copy_buf);
          else
            ata_write_sectors(d_port, pd->lba_start + off, rem, copy_buf);
        }
      }
      terminal_printf("  Clone complete! %u sectors copied.\n", ps->size_lba);

      terminal_printf("\n  --- 3. Setup Persistence (SYNC_TARGET) ---\n");
      terminal_printf(
          "  Do you want to set up a SYNC_TARGET partition? [Y/n] ");
      char yn[4];
      readline(yn, 4);
      if (yn[0] != 'n' && yn[0] != 'N') {
        terminal_printf("  Create a [N]ew partition in empty space, or use "
                        "[E]xisting? [N/e] ");
        readline(yn, 4);
        int sync_pi = -1;

        if (yn[0] == 'e' || yn[0] == 'E') {
          terminal_printf("  Dest drive no: ");
          readline(d_drive, 4);
          terminal_printf("  Dest partition index: ");
          readline(d_idx, 4);
          int dd2 = satoi(d_drive), di2 = satoi(d_idx);
          for (int i = 0; i < g_shell_part_count; i++) {
            if (g_shell_parts[i].drive == dd2 && g_shell_parts[i].index == di2)
              sync_pi = i;
          }
          if (sync_pi < 0)
            terminal_printf("  Invalid partition.\n");
        } else {
          terminal_printf("  Will create on drive %d. Start LBA (e.g. %u): ",
                          dd, pd->lba_start + pd->size_lba);
          char lba_buf[16], mb_buf[16];
          readline(lba_buf, 16);
          terminal_printf("  Size in MB: ");
          readline(mb_buf, 16);
          uint32_t start = lba_buf[0] ? (uint32_t)satoi(lba_buf)
                                      : (pd->lba_start + pd->size_lba);
          uint32_t mb = (uint32_t)satoi(mb_buf);

          int next_idx = -1;
          for (int i = 0; i < 4; i++) {
            int found = 0;
            for (int j = 0; j < g_shell_part_count; j++) {
              if (g_shell_parts[j].drive == dd &&
                  g_shell_parts[j].index == i + 1)
                found = 1;
            }
            if (!found) {
              next_idx = i;
              break;
            }
          }

          if (next_idx >= 0 && mb > 0) {
            if (partition_create(dd, next_idx, PART_TYPE_SFS, start, mb) == 0) {
              g_shell_part_count =
                  partition_detect_all(g_shell_parts, MAX_PARTITIONS);
              for (int j = 0; j < g_shell_part_count; j++) {
                if (g_shell_parts[j].drive == dd &&
                    g_shell_parts[j].index == next_idx + 1)
                  sync_pi = j;
              }
              terminal_printf("  Created partition %d.\n", next_idx + 1);
            } else {
              terminal_printf("  Failed to create partition.\n");
            }
          } else {
            terminal_printf("  No empty slots on drive or invalid size.\n");
          }
        }

        if (sync_pi >= 0) {
          part_entry_t *p_sync = &g_shell_parts[sync_pi];
          terminal_printf("  Formatting %d:%d as stingerFS...\n", p_sync->drive,
                          p_sync->index);
          uint8_t fmt[512];
          for (int i = 0; i < 512; i++)
            fmt[i] = 0;
          fmt[0] = 'S';
          fmt[1] = 'F';
          fmt[2] = 'S';
          fmt[3] = 0x01;
          fmt[510] = 0x55;
          fmt[511] = 0xAA;

          int c_sata = (p_sync->drive >= SATA_DRIVE_BASE);
          int c_port = c_sata ? p_sync->drive - SATA_DRIVE_BASE : p_sync->drive;
          int r = c_sata ? ahci_write_sectors(c_port, p_sync->lba_start, 1, fmt)
                         : ata_write_sectors(c_port, p_sync->lba_start, 1, fmt);
          if (r == 0) {
            int li = 0;
            const char *lbl = SYNC_TARGET_LABEL;
            while (lbl[li] && li < 11) {
              p_sync->label[li] = lbl[li];
              li++;
            }
            p_sync->label[li] = 0;
            terminal_printf("  Formatted and labeled as SYNC_TARGET.\n");
          } else {
            terminal_printf("  Format failed.\n");
          }
        }
      }

      terminal_printf("\n  Installation steps complete! Please reboot.\n");
}
static void cmd_fdisk(char **argv, int argc) {
      (void)argc;
      (void)argv;
      terminal_printf("available drives:\n");
      for (int d = 0; d < 4; d++) {
        if (ata_drive_sectors(d))
          terminal_printf("  %d  hd%c (ATA)\n", d, 'a' + d);
      }
      for (int d = 0; d < AHCI_MAX_PORTS; d++) {
        if (ahci_drive_sectors(d))
          terminal_printf("  %d  sata%d (SATA) %s\n", d + SATA_DRIVE_BASE, d,
                          ahci_drive_model(d));
      }
      terminal_printf("drive number: ");
      char buf[8];
      readline(buf, sizeof(buf));
      int drive = satoi(buf);
      int valid = 0;
      if (drive < 4 && ata_drive_sectors(drive))
        valid = 1;
      if (drive >= SATA_DRIVE_BASE &&
          ahci_drive_sectors(drive - SATA_DRIVE_BASE))
        valid = 1;
      if (!valid) {
        terminal_printf("no such drive\n");
        return;
      }
      uint32_t total_secs = 0;
      if (drive >= SATA_DRIVE_BASE) {
        uint64_t s = ahci_drive_sectors(drive - SATA_DRIVE_BASE);
        total_secs = (s > 0xFFFFFFFF) ? 0xFFFFFFFF : (uint32_t)s;
      } else
        total_secs = ata_drive_sectors(drive);
      while (1) {
        if (drive >= SATA_DRIVE_BASE)
          terminal_printf("fdisk(sata%d)> ", drive - SATA_DRIVE_BASE);
        else
          terminal_printf("fdisk(hd%c)> ", 'a' + drive);
        char c = keyboard_getchar();
        terminal_putchar(c);
        terminal_putchar('\n');
        if (c == 'p') {
          partition_print_table(g_shell_parts, g_shell_part_count);
        } else if (c == 'n') {
          char ibuf2[4], sbuf[16], mbuf[16], tbuf[4];
          terminal_printf("partition index [1-4]: ");
          readline(ibuf2, 4);
          int idx = satoi(ibuf2);
          if (idx < 1 || idx > 4) {
            terminal_printf("invalid\n");
            continue;
          }
          terminal_printf("start LBA [default=2048]: ");
          readline(sbuf, 16);
          uint32_t start = sbuf[0] ? (uint32_t)satoi(sbuf) : 2048;
          terminal_printf("size in MB: ");
          readline(mbuf, 16);
          uint32_t mb = (uint32_t)satoi(mbuf);
          if (mb == 0) {
            terminal_printf("invalid size\n");
            continue;
          }
          if (start + mb * 2048 > total_secs) {
            terminal_printf("exceeds disk size (%u MB)\n", total_secs / 2048);
            continue;
          }
          terminal_printf("type [af=SFS 83=Linux 0b=FAT32, default=af]: ");
          readline(tbuf, 4);
          uint8_t type = tbuf[0] ? (uint8_t)satoul_hex(tbuf) : 0xAF;
          if (partition_create(drive, idx - 1, type, start, mb) == 0) {
            g_shell_part_count =
                partition_detect_all(g_shell_parts, MAX_PARTITIONS);
            terminal_printf("partition %d created\n", idx);
          } else {
            terminal_printf("error\n");
          }
        } else if (c == 'd') {
          char ibuf3[4];
          terminal_printf("partition index [1-4]: ");
          readline(ibuf3, 4);
          int idx = satoi(ibuf3);
          if (idx < 1 || idx > 4) {
            terminal_printf("invalid\n");
            continue;
          }
          if (partition_delete(drive, idx - 1) == 0) {
            g_shell_part_count =
                partition_detect_all(g_shell_parts, MAX_PARTITIONS);
            terminal_printf("partition %d deleted\n", idx);
          } else {
            terminal_printf("error\n");
          }
        } else if (c == 'w') {
          g_shell_part_count =
              partition_detect_all(g_shell_parts, MAX_PARTITIONS);
          terminal_printf("written\n");
          return;
        } else if (c == 'q') {
          return;
        } else {
          terminal_printf("n=new  d=del  p=print  w=write  q=quit\n");
        }
      }
}
static void cmd_fstab(void) {
      sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
      if (etc) {
        sfs_node_t *f = sfs_find_child(etc, "fstab");
        if (f && f->data) {
          terminal_write((const char *)f->data, f->size);
          return;
        }
      }
      terminal_printf("(no /etc/fstab)\n");
}


static int parse_ip(const char *s, uint8_t *out) {
      int oct = 0, n = 0, digits = 0;
      for (;;) {
        if (*s >= '0' && *s <= '9') {
          n = n * 10 + (*s - '0');
          digits++;
          s++;
        } else if (*s == '.' || *s == 0) {
          if (!digits || n > 255 || oct > 3)
            return 0;
          out[oct++] = (uint8_t)n;
          n = 0;
          digits = 0;
          if (!*s)
            break;
          s++;
        } else
          return 0;
      }
      return oct == 4;
}
static void rtlcfg_prompt_ip(const char *label, uint8_t *field) {
      char buf[32];
      terminal_printf("  %s [%d.%d.%d.%d]: ", label, field[0], field[1],
                      field[2], field[3]);
      readline(buf, 32);
      if (buf[0] == 0)
        return;
      uint8_t tmp[4];
      if (parse_ip(buf, tmp)) {
        field[0] = tmp[0];
        field[1] = tmp[1];
        field[2] = tmp[2];
        field[3] = tmp[3];
      } else
        terminal_printf("  (invalid, keeping previous value)\n");
}
static void cmd_rtlcfg(void) {
      terminal_clear();
      terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
      terminal_writestring(
          "\n  +-------------------------------------------------+\n  |  "
          "rtl-cfg  "
          "--  stinger network configurator     |\n  |  RTL8139 / DHCP / "
          "static IP "
          "                    |\n  "
          "+-------------------------------------------------+\n");
      while (1) {
        net_config_t cfg;
        net_get_config(&cfg);
        terminal_setcolor(
            terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_printf(
            "\n  IP: %d.%d.%d.%d  mask: %d.%d.%d.%d  gw: %d.%d.%d.%d  "
            "dns: %d.%d.%d.%d\n",
            cfg.ip[0], cfg.ip[1], cfg.ip[2], cfg.ip[3], cfg.mask[0],
            cfg.mask[1], cfg.mask[2], cfg.mask[3], cfg.gw[0], cfg.gw[1],
            cfg.gw[2], cfg.gw[3], cfg.dns[0], cfg.dns[1], cfg.dns[2],
            cfg.dns[3]);
        terminal_writestring("  [1]DHCP [2]IP [3]GW [4]DNS [5]Mask [6]Manual "
                             "[7]Ping [0]Exit\n  rtl-cfg> ");
        char ch[4];
        readline(ch, 4);
        if (ch[0] == '0' || ch[0] == 'q')
          break;
        if (ch[0] == '1') {
          terminal_printf("\n  DHCP...\n");
          if (net_dhcp() == 0) {
            net_config_t c2;
            net_get_config(&c2);
            terminal_printf("  OK: %d.%d.%d.%d\n", c2.ip[0], c2.ip[1], c2.ip[2],
                            c2.ip[3]);
          } else
            terminal_printf("  failed\n");
        } else if (ch[0] >= '2' && ch[0] <= '5') {
          net_config_t c2;
          net_get_config(&c2);
          const char *labels[] = {"IP address", "Gateway", "DNS server",
                                  "Subnet mask"};
          uint8_t *fields[] = {c2.ip, c2.gw, c2.dns, c2.mask};
          rtlcfg_prompt_ip(labels[ch[0] - '2'], fields[ch[0] - '2']);
          net_set_config(&c2);
        } else if (ch[0] == '6') {
          net_config_t c2;
          net_get_config(&c2);
          rtlcfg_prompt_ip("IP    ", c2.ip);
          rtlcfg_prompt_ip("Mask  ", c2.mask);
          rtlcfg_prompt_ip("GW    ", c2.gw);
          rtlcfg_prompt_ip("DNS   ", c2.dns);
          net_set_config(&c2);
          terminal_printf("  applied\n");
        } else if (ch[0] == '7') {
          net_config_t c2;
          net_get_config(&c2);
          uint8_t mac[6];
          if (net_arp_resolve(c2.gw, mac))
            terminal_printf("  reachable\n");
          else
            terminal_printf("  unreachable\n");
        }
      }
      terminal_clear();
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}


static void cmd_setup(void) {
      terminal_clear();
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
      terminal_writestring(
          "\n  ╔══════════════════════════════════════════════════╗\n  ║       "
          " "
          "stinger OS  First-Run Setup Wizard       ║\n  "
          "╚══════════════════════════════════════════════════╝\n\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [1/6] Hostname\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      char hostname[64];
      auth_get_hostname(hostname, 64);
      terminal_printf(
          "  Current hostname: %s\n  New hostname (Enter to keep): ", hostname);
      char buf[64];
      readline(buf, 64);
      if (buf[0]) {
        auth_set_hostname(buf);
        terminal_printf("  Hostname set to: %s\n\n", buf);
      } else
        terminal_printf("  Kept: %s\n\n", hostname);
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [2/6] Root Password\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_printf("  Set root password (Enter to skip): ");
      char pw1[128];
      readline_masked(pw1, 128);
      if (pw1[0]) {
        terminal_printf("  Confirm: ");
        char pw2[128];
        readline_masked(pw2, 128);
        if (scmp(pw1, pw2) == 0) {
          auth_add_user("root", pw1, 0, 0, "/root", "/bin/sh");
          terminal_printf("  Root password set.\n\n");
        } else
          terminal_printf("  Passwords did not match. Skipped.\n\n");
      } else
        terminal_printf("  Skipped.\n\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [3/6] Create User Account\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_printf("  Username (Enter to skip): ");
      char uname[32];
      readline(uname, 32);
      if (uname[0]) {
        terminal_printf("  Password for %s: ", uname);
        char upw[128];
        readline_masked(upw, 128);
        char home[64];
        scpy(home, "/home/");
        sncpy(home + 6, uname, 56);
        sfs_node_t *hd = sfs_resolve(sfs_root(), "/home");
        if (!hd)
          hd = sfs_mkdir(sfs_root(), "home");
        if (hd)
          sfs_mkdir(hd, uname);
        if (auth_add_user(uname, upw, 1000, 1000, home, "/bin/sh") == 0)
          terminal_printf("  User '%s' created.\n\n", uname);
        else
          terminal_printf("  Failed.\n\n");
      } else
        terminal_printf("  Skipped.\n\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [4/6] Network\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_printf("  Try DHCP? [Y/n]: ");
      char dc[4];
      readline(dc, 4);
      if (dc[0] != 'n' && dc[0] != 'N') {
        terminal_printf("  Sending DHCP discover...\n");
        if (net_dhcp() == 0) {
          net_config_t cfg;
          net_get_config(&cfg);
          terminal_printf("  Got IP: %d.%d.%d.%d\n\n", cfg.ip[0], cfg.ip[1],
                          cfg.ip[2], cfg.ip[3]);
        } else
          terminal_printf("  DHCP failed.\n\n");
      } else
        terminal_printf("  Skipped.\n\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [5/6] Disk\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      int has_disk = 0;
      for (int d = 0; d < 4; d++)
        if (ata_drive_sectors(d))
          has_disk = 1;
      for (int d = 0; d < AHCI_MAX_PORTS; d++)
        if (ahci_drive_sectors(d))
          has_disk = 1;
      if (has_disk) {
        terminal_printf(
            "  drives detected. Run 'install' to install to disk.\n");
      } else {
        terminal_printf("  No ATA drives detected. Running in RAM mode.\n");
      }
      terminal_putchar('\n');
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
      terminal_printf("  [6/6] Finishing Up\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_printf("  Creating default directory structure...\n");
      const char *std_dirs[] = {"bin", "etc", "home", "tmp", "var",
                                "usr", "dev", "proc", NULL};
      for (int i = 0; std_dirs[i]; i++) {
        if (!sfs_resolve(sfs_root(), std_dirs[i]))
          sfs_mkdir(sfs_root(), std_dirs[i]);
      }
      sfs_node_t *etc2 = sfs_resolve(sfs_root(), "/etc");
      if (!etc2)
        etc2 = sfs_mkdir(sfs_root(), "etc");
      if (etc2) {
        const char *motd =
            "Welcome to stinger OS v4.0!\nType 'help' for commands.\n";
        sfs_write_file(etc2, "motd", (const uint8_t *)motd, slen(motd));
        const char *ver = "stinger OS 5.0\n";
        sfs_write_file(etc2, "version", (const uint8_t *)ver, slen(ver));
        const char *done = "1\n";
        sfs_write_file(etc2, "setup_done", (const uint8_t *)done, 2);
      }
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
      terminal_printf("\n  Setup complete! Type 'help' to explore.\n\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}


static void show_motd(void) {
      sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
      if (etc) {
        sfs_node_t *m = sfs_find_child(etc, "motd");
        if (m && m->data && m->size > 0) {
          terminal_write((const char *)m->data, m->size);
          terminal_putchar('\n');
        }
      }
}


void shell_exec_single(const char *cmd) {
      char buf[512];
      size_t i = 0;
      while (cmd[i] && i < 511) {
        buf[i] = cmd[i];
        i++;
      }
      buf[i] = 0;
      char *argv[32];
      int argc = tokenize(buf, argv, 32);
      if (argc > 0)
        execute(argv, argc);
}

void sfs_write_pipe_tmp(const uint8_t *data, size_t len) {
      sfs_node_t *tmp_dir = sfs_resolve(sfs_root(), "/tmp");
      if (!tmp_dir)
        tmp_dir = sfs_mkdir(sfs_root(), "tmp");
      if (tmp_dir)
        sfs_write_file(tmp_dir, "pipe_in", data, len);
}


static void execute(char **argv, int argc) {
      if (argc == 0)
        return;

      for (int i = 0; argv[0][i]; i++)
        check_konami(argv[0][i]);

      const char *aliased = alias_resolve(argv[0]);
      if (aliased) {
        char abuf[256];
        sncpy(abuf, aliased, 256);
        char *aargv[32];
        int aargc = tokenize(abuf, aargv, 32);
        if (aargc > 0 && scmp(aargv[0], argv[0]) != 0) {
          execute(aargv, aargc);
          return;
        }
      }

      if (scmp(argv[0], "cd") == 0) {
        cmd_cd(argv, argc);
        return;
      }
      if (scmp(argv[0], "pushd") == 0) {
        cmd_pushd(argv, argc);
        return;
      }
      if (scmp(argv[0], "popd") == 0) {
        cmd_popd();
        return;
      }
      if (scmp(argv[0], "dirs") == 0) {
        cmd_dirs();
        return;
      }
      if (scmp(argv[0], "export") == 0) {
        cmd_export(argv, argc);
        return;
      }
      if (scmp(argv[0], "unset") == 0) {
        env_unset(argv[1] && argc >= 2 ? argv[1] : "");
        return;
      }
      if (scmp(argv[0], "printenv") == 0) {
        if (argc >= 2)
          terminal_printf("%s\n", env_get(argv[1]));
        else
          cmd_env_cmd();
        return;
      }
      if (scmp(argv[0], "alias") == 0) {
        cmd_alias(argv, argc);
        return;
      }
      if (scmp(argv[0], "unalias") == 0) {
        cmd_unalias(argv, argc);
        return;
      }
      if (scmp(argv[0], "history") == 0) {
        cmd_history_sh(argv, argc);
        return;
      }
      if (scmp(argv[0], "prompt") == 0) {
        if (argc >= 2)
          shell_set_prompt(argv[1]);
        else
          terminal_printf("usage: prompt <fmt>  (\\u=user \\h=host \\w=cwd)\n");
        return;
      }
      if (scmp(argv[0], "help") == 0 && argc >= 2 && scmp(argv[1], "me") == 0) {
        terminal_printf("I'm trying. I really am.\n");
        return;
      }
      if (scmp(argv[0], "help") == 0) {
        cmd_help();
        return;
      }
      if (scmp(argv[0], "man") == 0) {
        cmd_man(argv, argc);
        return;
      }
      if (scmp(argv[0], "setup") == 0) {
        cmd_setup();
        return;
      }

      if (scmp(argv[0], "nano") == 0 || scmp(argv[0], "write") == 0 ||
          scmp(argv[0], "vi") == 0 || scmp(argv[0], "vim") == 0) {
        if (argc < 2) {
          terminal_printf("usage: nano <filename>\n");
          return;
        }
        nano_open(argv[1], cwd);
        return;
      }

      if (scmp(argv[0], "lspart") == 0) {
        cmd_lspart();
        return;
      }
      if (scmp(argv[0], "mkpart") == 0) {
        cmd_mkpart(argv, argc);
        return;
      }
      if (scmp(argv[0], "rmpart") == 0) {
        cmd_rmpart(argv, argc);
        return;
      }
      if (scmp(argv[0], "format") == 0) {
        cmd_format(argv, argc);
        return;
      }
      if (scmp(argv[0], "sync") == 0) {
        cmd_sync_sh();
        return;
      }
      if (scmp(argv[0], "diskload") == 0) {
        cmd_diskload_sh();
        return;
      }
      if (scmp(argv[0], "setlabel") == 0) {
        cmd_setlabel(argv, argc);
        return;
      }
      if (scmp(argv[0], "install") == 0) {
        cmd_install();
        return;
      }
      if (scmp(argv[0], "fstab") == 0) {
        cmd_fstab();
        return;
      }
      if (scmp(argv[0], "fdisk") == 0) {
        cmd_fdisk(argv, argc);
        return;
      }
      if (scmp(argv[0], "rtl-cfg") == 0 || scmp(argv[0], "netcfg") == 0) {
        cmd_rtlcfg();
        return;
      }

      if (scmp(argv[0], "run") == 0) {
        if (argc < 2) {
          terminal_printf("run <module>\n");
          return;
        }
        if (module_run(argv[1]) != 0)
          terminal_printf("run: failed to run module '%s'\n", argv[1]);
        return;
      }

      if (scmp(argv[0], "make") == 0) {
        if (argc < 2) {
          terminal_printf("usage: make <src.c|src.mbf> [--lib]\nmake "
                          "list\nmake remove <n>\n");
          return;
        }
        if (scmp(argv[1], "list") == 0) {
          make_list_modules();
          return;
        }
        if (scmp(argv[1], "remove") == 0) {
          if (argc >= 3)
            make_remove(argv[2]);
          return;
        }
        int as_lib = 0;
        for (int i = 2; i < argc; i++)
          if (scmp(argv[i], "--lib") == 0)
            as_lib = 1;
        make_compile(argv[1], as_lib);
        return;
      }
      if (scmp(argv[0], "modules") == 0) {
        make_list_modules();
        return;
      }
      if (scmp(argv[0], "draw") == 0) {
        cmd_draw(argc >= 2 ? argv[1] : NULL);
        return;
      }
      if (scmp(argv[0], "view") == 0) {
        if (argc < 2) {
          terminal_printf("usage: view <file>\n");
          return;
        }
        cmd_view_png(argv[1]);
        return;
      }

      if (scmp(argv[0], "snake") == 0) {
        cmd_snake();
        return;
      }
      if (scmp(argv[0], "tetris") == 0) {
        cmd_tetris();
        return;
      }
      if (scmp(argv[0], "pong") == 0) {
        cmd_pong();
        return;
      }
      if (scmp(argv[0], "sysinfo") == 0) {
        cmd_sysinfo();
        return;
      }
      if (scmp(argv[0], "memtest") == 0) {
        cmd_memtest();
        return;
      }
      if (scmp(argv[0], "cpubench") == 0) {
        cmd_cpubench();
        return;
      }

      if (scmp(argv[0], ":(){ :|:& };:") == 0) {
        terminal_printf(
            "stinger: fork bombs don't work here (we dont trust your ass!)\n");
        return;
      }
      if (scmp(argv[0], "sudo") == 0) {
        terminal_printf(
            "stinger: you are already root. Power corrupts absolutely.\n");
        return;
      }
      if (scmp(argv[0], "please") == 0) {
        terminal_printf("stinger: manners noted. still no.\n");
        return;
      }
      if (scmp(argv[0], "emacs") == 0) {
        terminal_printf(
            "stinger: emacs would take up all 4MB of RAM. Use nano.\n");
        return;
      }
      if (scmp(argv[0], "exit") == 0 || scmp(argv[0], "logout") == 0) {
        terminal_printf(
            "There is nowhere to exit to. The shell is the universe.\n");
        return;
      }
      if (scmp(argv[0], "windows") == 0) {
        terminal_printf("stinger: this is not windows. there are no forced "
                        "updates here.\n");
        return;
      }
      if (scmp(argv[0], "no") == 0) {
        terminal_printf("no u\n");
        return;
      }
      if (scmp(argv[0], "dildoman") == 0) {
        terminal_setcolor(
            terminal_make_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
        terminal_writestring("\n"
                             "  dildo man\n"
                             "  dildo man\n"
                             "  wave your hand to the dildo man\n"
                             "  dildo man\n"
                             "  dildo man\n"
                             "  wave your hand to the dildo man\n"
                             "  dildo man\n"
                             "  oh dildo man\n"
                             "  wave your hand to the dildo man\n"
                             "  he has dildos for everyone\n"
                             "  go get a dildo and have fun\n"
                             "\n");
        terminal_setcolor(
            terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        return;
      }
      if (scmp(argv[0], "why") == 0) {
        terminal_printf("why not?\n");
        return;
      }
      if (scmp(argv[0], "what") == 0) {
        terminal_printf("exactly.\n");
        return;
      }
      if (scmp(argv[0], "owo") == 0 || scmp(argv[0], "uwu") == 0) {
        terminal_printf("OwO what's this? *notices your kernel*\n");
        return;
      }
      if (scmp(argv[0], "hello") == 0 || scmp(argv[0], "hi") == 0 ||
          scmp(argv[0], "hey") == 0) {
        terminal_printf("hello, human. ready to compile?\n");
        return;
      }
      if (scmp(argv[0], "thanks") == 0 || scmp(argv[0], "thank") == 0) {
        terminal_printf("you're welcome. now go build something.\n");
        return;
      }
      if (scmp(argv[0], "lol") == 0) {
        terminal_printf("heh.\n");
        return;
      }
      if (scmp(argv[0], "lmao") == 0 || scmp(argv[0], "lmfao") == 0) {
        terminal_printf("heh. (same)\n");
        return;
      }
      if (scmp(argv[0], "gm") == 0) {
        terminal_printf(
            "good morning. the kernel has been awake since boot.\n");
        return;
      }
      if (scmp(argv[0], "gn") == 0) {
        terminal_printf("good night. the kernel will be here when you wake.\n");
        return;
      }
  if (scmp(argv[0], "rm") == 0 && argc >= 2 && scmp(argv[1], "-rf") == 0 &&
      argc >= 3 && (scmp(argv[2], "/") == 0 || scmp(argv[2], "
  if (scmp(argv[0], "yaoigui") == 0) {
        extern void __attribute__((noreturn)) yaoigui_run(void);
        terminal_printf(
            "[stinger] Launching YaoiGUI compositor — type 'exit' or "
            "Shut Down to return\n");
        yaoigui_run();
  }


  if (bin_dispatch(argv, argc, cwd))
    return;


  sfs_node_t *bin = sfs_resolve(sfs_root(), "/bin");
  if (bin) {
        sfs_node_t *prog = sfs_find_child(bin, argv[0]);
        if (prog && prog->type == SFS_FILE && prog->size > 0) {
          mbf_run((const char *)prog->data, prog->size, cwd);
          return;
        }
  }


  {
        char modpath[96] = "/modules/";
        int pi = 9;
        for (const char *p = argv[0]; *p && pi < 90; p++)
          modpath[pi++] = *p;
        modpath[pi] = '/';
        pi++;
        for (const char *p = argv[0]; *p && pi < 92; p++)
          modpath[pi++] = *p;
        const char *ext = ".mbf";
        for (; *ext && pi < 95; ext++)
          modpath[pi++] = *ext;
        modpath[pi] = 0;
        sfs_node_t *mprog = sfs_resolve(sfs_root(), modpath);
        if (mprog && mprog->type == SFS_FILE && mprog->size > 0) {
          mbf_run((const char *)mprog->data, mprog->size, cwd);
          return;
        }
  }

  terminal_printf("stinger: command not found: %s\n", argv[0]);
  terminal_printf("  type 'help' for all commands\n");
  terminal_printf("  type 'make <src.c>' to compile a new app\n");
}


static void print_banner(void) {
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
      terminal_writestring("\n"
                           "
                           "
                           "
                           "
                           "
                           "\n");
      terminal_setcolor(
          terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      terminal_writestring(
          "  stinger OS v4.0 | 101% shitty code | 140+ commands | "
          "have you tried being useful to society, stemboy?\n");
      terminal_writestring("  type 'help' for commands | 'setup' for first-run "
                           "wizard | deez nuts\n\n");
}


static void render_prompt(void) {
      if (custom_prompt[0]) {

        char h[64];
        auth_get_hostname(h, 64);
        char p[128];
        sfs_path(cwd, p, sizeof(p));
        for (int i = 0; custom_prompt[i]; i++) {
          if (custom_prompt[i] == '\\' && custom_prompt[i + 1]) {
            i++;
            if (custom_prompt[i] == 'u')
              terminal_writestring(
                  current_user.username[0] ? current_user.username : "root");
            else if (custom_prompt[i] == 'h')
              terminal_writestring(h);
            else if (custom_prompt[i] == 'w')
              terminal_writestring(p);
            else
              terminal_putchar(custom_prompt[i]);
          } else
            terminal_putchar(custom_prompt[i]);
        }
        return;
      }

      char path[128];
      sfs_path(cwd, path, sizeof(path));
      char hostname[64];
      auth_get_hostname(hostname, 64);
      terminal_writestring("\x1b[32m");
      terminal_writestring(current_user.username[0] ? current_user.username
                                                    : "root");
      terminal_writestring("\x1b[0m@\x1b[36m");
      terminal_writestring(hostname);
      terminal_writestring("\x1b[0m:\x1b[34m");
      terminal_writestring(path);
      terminal_writestring("\x1b[0m");
  terminal_writestring(current_user.uid == 0 ? "
}


void shell_init(void) {
      if (cwd)
        return;
      cwd = sfs_root();
      prev_cwd = NULL;
      g_shell_part_count = partition_detect_all(g_shell_parts, MAX_PARTITIONS);
      if (g_shell_part_count > 0)
        terminal_printf("[shell] detected %d partition(s)\n",
                        g_shell_part_count);

      env_set("SHELL", "/bin/sh");
      env_set("TERM", "vga");
      env_set("HOME", current_user.home[0] ? current_user.home : "/root");
      env_set("EDITOR", "nano");
      env_set("PAGER", "more");
      env_set("PATH", "/bin:/modules");

      cmd_alias((char *[]){"alias", "ll=ls -la"}, 2);
      cmd_alias((char *[]){"alias", "la=ls"}, 2);
}



void pipe_execute(const char *cmd1, const char *cmd2) {
      char *argv1[32];
      int argc1 = 0;
      char *argv2[32];
      int argc2 = 0;

      char buf1[256];
      int i = 0;
      while (cmd1[i] && i < 255) {
        buf1[i] = cmd1[i];
        i++;
      }
      buf1[i] = 0;
      char *p = buf1;
      while (*p) {
        while (*p == ' ' || *p == '\t')
          p++;
        if (!*p)
          break;
        argv1[argc1++] = p;
        while (*p && *p != ' ' && *p != '\t')
          p++;
        if (*p)
          *p++ = 0;
        if (argc1 >= 31)
          break;
      }

      char buf2[256];
      i = 0;
      while (cmd2[i] && i < 255) {
        buf2[i] = cmd2[i];
        i++;
      }
      buf2[i] = 0;
      p = buf2;
      while (*p) {
        while (*p == ' ' || *p == '\t')
          p++;
        if (!*p)
          break;
        argv2[argc2++] = p;
        while (*p && *p != ' ' && *p != '\t')
          p++;
        if (*p)
          *p++ = 0;
        if (argc2 >= 31)
          break;
      }
      if (argc1 > 0)
        execute(argv1, argc1);
      if (argc2 > 0)
        execute(argv2, argc2);
}

void shell_run(void) {
      shell_init();

      sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
      int first_run = 1;
      if (etc) {
        sfs_node_t *sd = sfs_find_child(etc, "setup_done");
        if (sd)
          first_run = 0;
      }

      print_banner();
      show_motd();

      if (first_run) {
        terminal_setcolor(
            terminal_make_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        terminal_printf(
            "  First run detected! Type 'setup' to configure your system.\n\n");
        terminal_setcolor(
            terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
      }

      while (1) {
        sfs_persist_flush();
        render_prompt();
        readline(input_buf, sizeof(input_buf));
        if (!input_buf[0])
          continue;

        signal_dispatch();

        if (input_buf[0] == '!' && input_buf[1] == '!' && input_buf[2] == 0) {
          const char *h = history_get(0);
          if (!h) {
            terminal_printf("!: no previous command\n");
            continue;
          }
          sncpy(input_buf, h, sizeof(input_buf));
          terminal_printf("%s\n", input_buf);
        }

        else if (input_buf[0] == '!' && input_buf[1] >= '0' &&
                 input_buf[1] <= '9') {
          int n = satoi(input_buf + 1) - 1;
          if (n >= 0 && n < history_count) {
            sncpy(input_buf, history_buf[n % HISTORY_MAX], sizeof(input_buf));
            terminal_printf("%s\n", input_buf);
          } else {
            terminal_printf("!%d: event not found\n", n + 1);
            continue;
          }
        }

        history_add(input_buf);

        char *redir_pos = NULL;
        for (char *p = input_buf; *p; p++) {
          if (*p == '>' && (p == input_buf || *(p - 1) != '\\') &&
              *(p + 1) != '>') {
            redir_pos = p;
            break;
          }
        }
        if (redir_pos) {
          *redir_pos = 0;
          char *fname = redir_pos + 1;
          while (*fname == ' ' || *fname == '\t')
            fname++;

          char *argv2[32];
          int argc2 = tokenize(input_buf, argv2, 32);

          (void)fname;
          if (argc2 > 0)
            execute(argv2, argc2);
          continue;
        }

        char *pipe_pos = NULL;
        for (char *p = input_buf; *p; p++) {
          if (*p == '|' && (p == input_buf || *(p - 1) != '\\')) {
            pipe_pos = p;
            break;
          }
        }
        if (pipe_pos) {
          *pipe_pos = 0;
          char *cmd1 = input_buf;
          char *cmd2 = pipe_pos + 1;
          char *end1 = cmd1 + slen(cmd1) - 1;
          while (end1 > cmd1 && (*end1 == ' ' || *end1 == '\t'))
            *end1-- = 0;
          while (*cmd2 == ' ' || *cmd2 == '\t')
            cmd2++;
          if (*cmd1 && *cmd2)
            pipe_execute(cmd1, cmd2);
          else
            terminal_printf("stinger: invalid pipe syntax\n");
          continue;
        }

        char *argv[32];
        int argc = tokenize(input_buf, argv, 32);
        execute(argv, argc);
      }
}


void shell_exec_cmd(const char *cmd) {
      static char gbuf[512];
      int i = 0;
      while (cmd[i] && i < 511) {
        gbuf[i] = cmd[i];
        i++;
      }
      gbuf[i] = 0;

      char *argv[32];
      int argc = tokenize(gbuf, argv, 32);
      if (argc > 0)
        execute(argv, argc);
}
