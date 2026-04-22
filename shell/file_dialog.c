











static int slen(const char *s) { int n=0; while(s[n]) n++; return n; }
static void scpy(char *d, const char *s) { while((*d++=*s++)); }
static void sncpy(char *d, const char *s, int n) {
    int i=0; while(i<n-1 && s[i]) { d[i]=s[i]; i++; } d[i]=0;
}

static void draw_dialog_header(const char *title) {
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
    terminal_printf("\x1b[2J\x1b[H");
    terminal_printf(" %-78s\n", title);
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static int scmp(const char *a, const char *b) {
    while(*a && *b && *a==*b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

file_dialog_result_t file_dialog_open(sfs_node_t *start_dir, const char *title) {
    file_dialog_result_t result;
    result.result = 0;
    result.path[0] = 0;
    result.filename[0] = 0;

    sfs_node_t *current_dir = start_dir ? start_dir : sfs_root();
    if (!current_dir) return result;

    while (1) {
        draw_dialog_header(title);


        char path_buf[256];
        sfs_path(current_dir, path_buf, sizeof(path_buf));
        terminal_printf(" Path: %s\n\n", path_buf);


        sfs_node_t *entries[MAX_FILES];
        int entry_count = 0;
        int dir_count = 0;


        terminal_printf(" [0] ..\n");
        if (current_dir->parent) {
            entries[entry_count++] = current_dir->parent;
            dir_count++;
        }


        for (sfs_node_t *child = current_dir->child; child && entry_count < MAX_FILES; child = child->next) {
            char marker = (child->type == SFS_DIR) ? '/' : ' ';
            terminal_printf(" [%d] %s%c\n", entry_count + 1, child->name, marker);
            entries[entry_count++] = child;
            if (child->type == SFS_DIR) dir_count++;
        }

        terminal_printf("\n Open: [d]irect path  [c]ancel  Select [0-%d]: ", entry_count);
        char c = keyboard_getchar();
        terminal_putchar(c);
        terminal_putchar('\n');

        if (c == 'c' || c == 'C') {
            result.result = 0;
            return result;
        }

        if (c == 'd' || c == 'D') {
            terminal_printf(" Enter file path: ");
            int len = 0;
            char buf[256];
            while ((c = keyboard_getchar()) != '\n' && c != '\r' && len < 255) {
                if (c == '\b' || c == 127) {
                    if (len > 0) len--;
                } else {
                    buf[len++] = c;
                    terminal_putchar(c);
                }
            }
            buf[len] = 0;
            terminal_putchar('\n');


            sfs_node_t *node = sfs_resolve(sfs_root(), buf);
            if (node && node->type == SFS_FILE) {
                sfs_path(node, result.path, sizeof(result.path));
                result.result = 1;
                return result;
            } else {
                terminal_printf(" File not found. Press any key...\n");
                keyboard_getchar();
            }
            continue;
        }

        if (c >= '0' && c < '0' + entry_count) {
            int idx = c - '0';
            if (idx == 0 && current_dir->parent) {
                current_dir = current_dir->parent;
                continue;
            }
            if (idx > 0 && idx <= entry_count) {
                sfs_node_t *selected = entries[idx - 1];
                if (selected->type == SFS_DIR) {
                    current_dir = selected;
                    continue;
                } else if (selected->type == SFS_FILE) {
                    sfs_path(selected, result.path, sizeof(result.path));
                    sncpy(result.filename, selected->name, sizeof(result.filename));
                    result.result = 1;
                    return result;
                }
            }
        }
    }

    return result;
}

file_dialog_result_t file_dialog_save(sfs_node_t *start_dir, const char *title, const char *default_name) {
    file_dialog_result_t result;
    result.result = 0;
    result.path[0] = 0;
    result.filename[0] = 0;

    sfs_node_t *current_dir = start_dir ? start_dir : sfs_root();
    if (!current_dir) return result;

    while (1) {
        draw_dialog_header(title);

        char path_buf[256];
        sfs_path(current_dir, path_buf, sizeof(path_buf));
        terminal_printf(" Path: %s\n\n", path_buf);

        sfs_node_t *entries[MAX_FILES];
        int entry_count = 0;

        terminal_printf(" [0] ..\n");
        if (current_dir->parent) {
            entries[entry_count++] = current_dir->parent;
        }

        for (sfs_node_t *child = current_dir->child; child && entry_count < MAX_FILES; child = child->next) {
            if (child->type == SFS_DIR) {
                terminal_printf(" [%d] %s/\n", entry_count + 1, child->name);
                entries[entry_count++] = child;
            }
        }

        terminal_printf("\n Filename [%s]: ", default_name);
        int len = 0;
        char buf[256];
        char c;
        while ((c = keyboard_getchar()) != '\n' && c != '\r' && len < 255) {
            if (c == '\b' || c == 127) {
                if (len > 0) len--;
            } else {
                buf[len++] = c;
                terminal_putchar(c);
            }
        }
        terminal_putchar('\n');

        if (len == 0) {
            scpy(buf, default_name);
            len = slen(default_name);
        }
        buf[len] = 0;

        terminal_printf(" Save to: %s/%s [y/n/d/c]: ", path_buf, buf);
        c = keyboard_getchar();
        terminal_putchar(c);
        terminal_putchar('\n');

        if (c == 'y' || c == 'Y') {
            sncpy(result.filename, buf, sizeof(result.filename));
            scpy(result.path, path_buf);
            result.result = 1;
            return result;
        }

        if (c == 'c' || c == 'C') {
            return result;
        }

        if (c == 'd' || c == 'D') {
            terminal_printf(" Choose directory [0-%d]: ", entry_count);
            c = keyboard_getchar();
            terminal_putchar(c);
            terminal_putchar('\n');

            if (c >= '0' && c < '0' + entry_count) {
                int idx = c - '0';
                if (idx == 0 && current_dir->parent) {
                    current_dir = current_dir->parent;
                }
                if (idx > 0 && idx <= entry_count) {
                    if (entries[idx - 1]->type == SFS_DIR) {
                        current_dir = entries[idx - 1];
                    }
                }
            }
        }
    }

    return result;
}




