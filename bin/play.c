





static sfs_node_t *resolve_media(const char *path, sfs_node_t *cwd) {

    if (path[0] == '/') {
        return sfs_resolve(sfs_root(), path);
    }


    sfs_node_t *n = sfs_find_child(cwd, path);
    if (n) return n;


    sfs_node_t *modules = sfs_resolve(sfs_root(), "/modules");
    if (modules) {
        n = sfs_find_child(modules, path);
        if (n) return n;
    }

    return NULL;
}

void bin_play(char **argv, int argc, sfs_node_t *cwd) {
    if (argc < 2) {
        terminal_printf("usage: play <file.saf|file.svf>\n");
        terminal_printf("  .saf  stinger Audio Format  (PC speaker)\n");
        terminal_printf("  .svf  stinger Video Format  (framebuffer)\n");
        return;
    }

    const char *path = argv[1];
    sfs_node_t *file = resolve_media(path, cwd);

    if (!file) {
        terminal_printf("play: file not found: %s\n", path);
        terminal_printf("  (searched cwd and /modules/)\n");
        return;
    }

    if (file->type == SFS_DIR) {
        terminal_printf("play: '%s' is a directory\n", path);
        return;
    }


    const char *name = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') name = p + 1;

    media_play(file, name);
}




