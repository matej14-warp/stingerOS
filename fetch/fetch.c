












static void scpy(char*d,const char*s){while((*d++=*s++));}
static int scmp(const char*a,const char*b){
    while(*a&&*a==*b){a++;b++;} return(unsigned char)*a-(unsigned char)*b;
}
static void scat(char*d,const char*s){
    while(*d) d++;
    while((*d++=*s++));
}


typedef struct {
    sfs_node_t *target_dir;
    int         file_count;
    char        has_setup;
} extract_ctx_t;

static void on_extracted_file(const char *fname, const uint8_t *data,
                               size_t len, void *userdata) {
    extract_ctx_t *ctx = (extract_ctx_t*)userdata;


    char path[256];
    scpy(path, fname);


    char *last_slash = NULL;
    for(char *p = path; *p; p++) if(*p=='/') last_slash=p;

    sfs_node_t *parent = ctx->target_dir;

    if(last_slash) {
        *last_slash = 0;
        const char *fname_only = last_slash + 1;


        char *seg = path;
        while(*seg) {
            char *next_slash = seg;
            while(*next_slash && *next_slash!='/') next_slash++;
            int had_slash = (*next_slash == '/');
            *next_slash = 0;

            sfs_node_t *child = sfs_find_child(parent, seg);
            if(!child) child = sfs_mkdir(parent, seg);
            if(!child) break;
            parent = child;

            if(!had_slash) break;
            seg = next_slash + 1;
        }


        if(*fname_only == 0) { ctx->file_count++; return; }
        sfs_write_file(parent, fname_only, data, len);

        if(scmp(fname_only, "setup-runtime.mbf")==0) ctx->has_setup=1;
    } else {

        if(path[0]==0) return;
        sfs_write_file(parent, path, data, len);
        if(scmp(path,"setup-runtime.mbf")==0) ctx->has_setup=1;
    }

    ctx->file_count++;
    terminal_printf("  extracted: %s (%u bytes)\n", fname, (unsigned)len);
}


extern char shell_getchar(void);

static int confirm(const char *prompt) {
    terminal_printf("%s [Y/n] ", prompt);
    char c = shell_getchar();
    terminal_putchar(c);
    terminal_putchar('\n');
    return (c=='Y'||c=='y'||c=='\r'||c=='\n');
}


int fetch_package(const char *pkg_name) {
    if(!pkg_name || !*pkg_name) {
        terminal_printf("fetch: usage: fetch <package>\n");
        return -1;
    }

    terminal_printf("\n");
    terminal_printf(":: stinger package manager\n");
    terminal_printf(":: fetching package: %s\n", pkg_name);
    terminal_printf(":: CDN: cdn.voided.uk\n\n");


    char url_path[256];
    scpy(url_path, "/fetch3/");
    scat(url_path, pkg_name);


    uint8_t *body = NULL;
    size_t   body_len = 0;
    terminal_printf(":: downloading %s...\n", url_path);

    int r = http_get("cdn.voided.uk", url_path, &body, &body_len);
    if(r != 0 || !body || body_len == 0) {
        terminal_printf("[fetch] error: could not download package '%s'\n",
                        pkg_name);
        terminal_printf("[fetch] is cdn.voided.uk reachable?\n");
        if(body) kfree(body);
        return -1;
    }

    terminal_printf(":: downloaded %u bytes\n", (unsigned)body_len);


    if(body_len < 4) {
        terminal_printf("[fetch] error: response is too short\n");
        kfree(body);
        return -1;
    }


    sfs_node_t *tmp_root = sfs_resolve(sfs_root(), "/tmp");
    if(!tmp_root) tmp_root = sfs_mkdir(sfs_root(), "tmp");


    sfs_node_t *old = sfs_find_child(tmp_root, pkg_name);
    if(old) sfs_delete(old);

    sfs_node_t *pkg_dir = sfs_mkdir(tmp_root, pkg_name);
    if(!pkg_dir) {
        terminal_printf("[fetch] error: could not create /tmp/%s\n", pkg_name);
        kfree(body);
        return -1;
    }


    sfs_write_file(tmp_root,  pkg_name, body, body_len);


    terminal_printf(":: extracting package contents...\n");
    extract_ctx_t ctx;
    ctx.target_dir = pkg_dir;
    ctx.file_count = 0;
    ctx.has_setup  = 0;

    int nfiles = zip_extract(body, body_len, on_extracted_file, &ctx);
    kfree(body);

    if(nfiles < 0) {
        terminal_printf("[fetch] error: ZIP extraction failed\n");
        return -1;
    }

    terminal_printf("\n:: package '%s' contains %d file(s)\n", pkg_name, nfiles);

    if(!ctx.has_setup) {
        terminal_printf("[fetch] warning: no setup-runtime.mbf found in package\n");
        terminal_printf("[fetch] files were extracted to /tmp/%s\n", pkg_name);
        return 0;
    }


    terminal_printf("\n");
    terminal_printf("Package name : %s\n", pkg_name);
    terminal_printf("Files        : %d\n", nfiles);
    terminal_printf("Installer    : setup-runtime.mbf\n\n");

    if(!confirm(":: Proceed with installation?")) {
        terminal_printf(":: Installation cancelled.\n");
        return 0;
    }

    terminal_printf("\n:: running setup-runtime.mbf...\n");
    terminal_printf("----------------------------------------\n");


    sfs_node_t *setup = sfs_find_child(pkg_dir, "setup-runtime.mbf");
    if(!setup || setup->type != SFS_FILE) {
        terminal_printf("[fetch] error: setup-runtime.mbf not found after extraction\n");
        return -1;
    }

    int mbf_ret = mbf_run((const char*)setup->data, setup->size, pkg_dir);

    terminal_printf("----------------------------------------\n");
    if(mbf_ret == 0)
        terminal_printf(":: Package '%s' installed successfully!\n", pkg_name);
    else
        terminal_printf(":: Installation script returned error %d\n", mbf_ret);

    return mbf_ret;
}

static char fetch_cdn[128] = "cdn.voided.uk";

void fetch_set_cdn(const char *host) {
    int i = 0;
    while(host[i] && i < 127) { fetch_cdn[i] = host[i]; i++; }
    fetch_cdn[i] = 0;
    terminal_printf("[fetch] CDN set to: %s\n", fetch_cdn);
}

void fetch_update_index(void) {
    terminal_printf("[fetch] fetching package index from %s...\n", fetch_cdn);
    uint8_t *body = NULL;
    size_t body_len = 0;
    int r = http_get(fetch_cdn, "/fetch3/INDEX", &body, &body_len);
    if(r != 0 || !body || body_len == 0) {
        terminal_printf("[fetch] could not fetch index\n");
        if(body) kfree(body);
        return;
    }

    sfs_node_t *tmp = sfs_resolve(sfs_root(), "/tmp");
    if(!tmp) tmp = sfs_mkdir(sfs_root(), "tmp");
    if(tmp) sfs_write_file(tmp, "fetch-index", body, body_len);
    terminal_printf("[fetch] index updated (%u bytes)\n", (unsigned)body_len);
    kfree(body);
}

void fetch_list_packages(void) {
    sfs_node_t *idx = sfs_resolve(sfs_root(), "/tmp/fetch-index");
    if(!idx || idx->type != SFS_FILE || !idx->data) {
        terminal_printf("[fetch] no index cached — run 'fetch -u' first\n");
        return;
    }
    terminal_printf("[fetch] available packages:\n");
    const char *p = (const char*)idx->data;
    const char *end = p + idx->size;
    while(p < end) {
        const char *line = p;
        while(p < end && *p != '\n') p++;
        int len = (int)(p - line);
        if(len > 0) {
            terminal_printf("  ");
            for(int i=0;i<len;i++) terminal_putchar(line[i]);
            terminal_putchar('\n');
        }
        if(p < end) p++;
    }
}




