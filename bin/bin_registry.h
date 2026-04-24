/* scorpion OS - bin/bin_registry.h
   Auto-generated master list of all /bin/ commands.
   Included by shell.c.  Each entry: { "cmdname", bin_funcname }
   The shell calls bin_dispatch(argv, argc, cwd) instead of the old
   giant if-else chain.                                            */

#ifndef BIN_REGISTRY_H
#define BIN_REGISTRY_H

#include "../fs/sfs.h"

typedef void (*bin_fn_t)(char**argv, int argc, sfs_node_t*cwd);

/* forward declarations */
void bin_ls(char**,int,sfs_node_t*);
void bin_cat(char**,int,sfs_node_t*);
void bin_echo(char**,int,sfs_node_t*);
void bin_pwd(char**,int,sfs_node_t*);
void bin_mkdir(char**,int,sfs_node_t*);
void bin_mkdir_p(char**,int,sfs_node_t*);
void bin_rm(char**,int,sfs_node_t*);
void bin_touch(char**,int,sfs_node_t*);
void bin_cp(char**,int,sfs_node_t*);
void bin_mv(char**,int,sfs_node_t*);
void bin_ln(char**,int,sfs_node_t*);
void bin_append(char**,int,sfs_node_t*);
void bin_tee(char**,int,sfs_node_t*);
void bin_stat(char**,int,sfs_node_t*);
void bin_find(char**,int,sfs_node_t*);
void bin_tree(char**,int,sfs_node_t*);
void bin_rev(char**,int,sfs_node_t*);
void bin_grep(char**,int,sfs_node_t*);
void bin_wc(char**,int,sfs_node_t*);
void bin_sort(char**,int,sfs_node_t*);
void bin_head(char**,int,sfs_node_t*);
void bin_tail(char**,int,sfs_node_t*);
void bin_hexdump(char**,int,sfs_node_t*);
void bin_strings(char**,int,sfs_node_t*);
void bin_crc32(char**,int,sfs_node_t*);
void bin_base64(char**,int,sfs_node_t*);
void bin_rot13(char**,int,sfs_node_t*);
void bin_toupper(char**,int,sfs_node_t*);
void bin_tolower(char**,int,sfs_node_t*);
void bin_nl(char**,int,sfs_node_t*);
void bin_cut(char**,int,sfs_node_t*);
void bin_tr(char**,int,sfs_node_t*);
void bin_uniq(char**,int,sfs_node_t*);
void bin_tac(char**,int,sfs_node_t*);
void bin_paste(char**,int,sfs_node_t*);
void bin_diff(char**,int,sfs_node_t*);
void bin_od(char**,int,sfs_node_t*);
void bin_xxd(char**,int,sfs_node_t*);
void bin_md5sum(char**,int,sfs_node_t*);
void bin_sha256sum(char**,int,sfs_node_t*);
void bin_ps(char**,int,sfs_node_t*);
void bin_top(char**,int,sfs_node_t*);
void bin_free(char**,int,sfs_node_t*);
void bin_meminfo(char**,int,sfs_node_t*);
void bin_uname(char**,int,sfs_node_t*);
void bin_df(char**,int,sfs_node_t*);
void bin_du(char**,int,sfs_node_t*);
void bin_lscpu(char**,int,sfs_node_t*);
void bin_dmesg(char**,int,sfs_node_t*);
void bin_uptime(char**,int,sfs_node_t*);
void bin_date(char**,int,sfs_node_t*);
void bin_sleep(char**,int,sfs_node_t*);
void bin_seq(char**,int,sfs_node_t*);
void bin_yes(char**,int,sfs_node_t*);
void bin_clear(char**,int,sfs_node_t*);
void bin_reset(char**,int,sfs_node_t*);
void bin_whoami(char**,int,sfs_node_t*);
void bin_id(char**,int,sfs_node_t*);
void bin_hostname(char**,int,sfs_node_t*);
void bin_chmod(char**,int,sfs_node_t*);
void bin_chown(char**,int,sfs_node_t*);
void bin_passwd(char**,int,sfs_node_t*);
void bin_adduser(char**,int,sfs_node_t*);
void bin_kill(char**,int,sfs_node_t*);
void bin_reboot(char**,int,sfs_node_t*);
void bin_halt(char**,int,sfs_node_t*);
void bin_ping(char**,int,sfs_node_t*);
void bin_ifconfig(char**,int,sfs_node_t*);
void bin_wget(char**,int,sfs_node_t*);
void bin_dhcp(char**,int,sfs_node_t*);
void bin_route(char**,int,sfs_node_t*);
void bin_nslookup(char**,int,sfs_node_t*);
void bin_dns(char**,int,sfs_node_t*);
void bin_ssh(char**,int,sfs_node_t*);
void bin_irc(char**,int,sfs_node_t*);
void bin_nc(char**,int,sfs_node_t*);
void bin_wifi(char**,int,sfs_node_t*);
void bin_fetch(char**,int,sfs_node_t*);
void bin_diskinfo(char**,int,sfs_node_t*);
void bin_sound(char**,int,sfs_node_t*);
void bin_lspci(char**,int,sfs_node_t*);
void bin_lsusb(char**,int,sfs_node_t*);
void bin_smpinfo(char**,int,sfs_node_t*);
void bin_acpi(char**,int,sfs_node_t*);
void bin_mbf(char**,int,sfs_node_t*);
void bin_more(char**,int,sfs_node_t*);
void bin_dllist(char**,int,sfs_node_t*);
void bin_dlopen(char**,int,sfs_node_t*);
void bin_fortune(char**,int,sfs_node_t*);
void bin_nyan(char**,int,sfs_node_t*);
void bin_matrix(char**,int,sfs_node_t*);
void bin_cowsay(char**,int,sfs_node_t*);
void bin_sl(char**,int,sfs_node_t*);
void bin_rickroll(char**,int,sfs_node_t*);
void bin_doge(char**,int,sfs_node_t*);
void bin_brainrot(char**,int,sfs_node_t*);
void bin_ohio(char**,int,sfs_node_t*);
void bin_ratio(char**,int,sfs_node_t*);
void bin_gigachad(char**,int,sfs_node_t*);
void bin_shrug(char**,int,sfs_node_t*);
void bin_tableflip(char**,int,sfs_node_t*);
void bin_yeet(char**,int,sfs_node_t*);
void bin_based(char**,int,sfs_node_t*);
void bin_boykisser(char**,int,sfs_node_t*);
void bin_doomscroll(char**,int,sfs_node_t*);
void bin_spinning(char**,int,sfs_node_t*);
void bin_ligma(char**,int,sfs_node_t*);
void bin_updog(char**,int,sfs_node_t*);
void bin_answer42(char**,int,sfs_node_t*);
void bin_calc(char**,int,sfs_node_t*);
void bin_which(char**,int,sfs_node_t*);
void bin_type(char**,int,sfs_node_t*);
void bin_watch(char**,int,sfs_node_t*);
void bin_time(char**,int,sfs_node_t*);
void bin_env_cmd_wrap(char**,int,sfs_node_t*);
void bin_printenv(char**,int,sfs_node_t*);
void bin_false(char**,int,sfs_node_t*);
void bin_true(char**,int,sfs_node_t*);
void bin_compress(char**,int,sfs_node_t*);
void bin_decompress(char**,int,sfs_node_t*);
void bin_banner(char**,int,sfs_node_t*);
void bin_color(char**,int,sfs_node_t*);
void bin_random(char**,int,sfs_node_t*);
void bin_ascii(char**,int,sfs_node_t*);
void bin_hexcalc(char**,int,sfs_node_t*);
void bin_scrap(char**,int,sfs_node_t*);
void bin_morse(char**,int,sfs_node_t*);
void bin_lolcat(char**,int,sfs_node_t*);
void bin_figlet(char**,int,sfs_node_t*);
void bin_pipes(char**,int,sfs_node_t*);
void bin_repeat(char**,int,sfs_node_t*);
void bin_yaoigui(char**,int,sfs_node_t*);
void bin_gcc(char**,int,sfs_node_t*);   /* gcc / cc — C compiler */

typedef struct { const char *name; bin_fn_t fn; } bin_entry_t;

static const bin_entry_t bin_table[] = {
    /* file commands */
    {"ls",       bin_ls},
    {"cat",      bin_cat},
    {"echo",     bin_echo},
    {"pwd",      bin_pwd},
    {"mkdir",    bin_mkdir_p},
    {"rm",       bin_rm},
    {"touch",    bin_touch},
    {"cp",       bin_cp},
    {"mv",       bin_mv},
    {"ln",       bin_ln},
    {"append",   bin_append},
    {"tee",      bin_tee},
    {"stat",     bin_stat},
    {"find",     bin_find},
    {"tree",     bin_tree},
    {"rev",      bin_rev},
    /* text tools */
    {"grep",     bin_grep},
    {"wc",       bin_wc},
    {"sort",     bin_sort},
    {"head",     bin_head},
    {"tail",     bin_tail},
    {"hexdump",  bin_hexdump},
    {"strings",  bin_strings},
    {"crc32",    bin_crc32},
    {"base64",   bin_base64},
    {"rot13",    bin_rot13},
    {"toupper",  bin_toupper},
    {"tolower",  bin_tolower},
    {"nl",       bin_nl},
    {"cut",      bin_cut},
    {"tr",       bin_tr},
    {"uniq",     bin_uniq},
    {"tac",      bin_tac},
    {"paste",    bin_paste},
    {"diff",     bin_diff},
    {"od",       bin_od},
    {"xxd",      bin_xxd},
    {"md5sum",   bin_md5sum},
    {"sha256sum",bin_sha256sum},
    /* system */
    {"ps",       bin_ps},
    {"top",      bin_top},
    {"free",     bin_free},
    {"meminfo",  bin_meminfo},
    {"uname",    bin_uname},
    {"df",       bin_df},
    {"du",       bin_du},
    {"lscpu",    bin_lscpu},
    {"dmesg",    bin_dmesg},
    {"uptime",   bin_uptime},
    {"date",     bin_date},
    {"sleep",    bin_sleep},
    {"seq",      bin_seq},
    {"yes",      bin_yes},
    {"clear",    bin_clear},
    {"cls",      bin_clear},
    {"reset",    bin_reset},
    {"more",     bin_more},
    {"less",     bin_more},
    /* users */
    {"whoami",   bin_whoami},
    {"id",       bin_id},
    {"hostname", bin_hostname},
    {"chmod",    bin_chmod},
    {"chown",    bin_chown},
    {"passwd",   bin_passwd},
    {"adduser",  bin_adduser},
    {"kill",     bin_kill},
    {"reboot",   bin_reboot},
    {"halt",     bin_halt},
    {"poweroff", bin_halt},
    {"shutdown", bin_halt},
    /* network */
    {"ping",     bin_ping},
    {"ifconfig", bin_ifconfig},
    {"wget",     bin_wget},
    {"curl",     bin_wget},
    {"dhcp",     bin_dhcp},
    {"route",    bin_route},
    {"nslookup", bin_nslookup},
    {"dns",      bin_dns},
    {"ssh",      bin_ssh},
    {"irc",      bin_irc},
    {"nc",       bin_nc},
    {"wifi",     bin_wifi},
    {"fetch",    bin_fetch},
    /* disk */
    {"diskinfo", bin_diskinfo},
    /* hardware */
    {"sound",    bin_sound},
    {"beep",     bin_sound},
    {"lspci",    bin_lspci},
    {"lsusb",    bin_lsusb},
    {"smpinfo",  bin_smpinfo},
    {"acpi",     bin_acpi},
    /* compiler */
    {"gcc",      bin_gcc},
    {"cc",       bin_gcc},
    /* scripting */
    {"mbf",      bin_mbf},
    {"dllist",   bin_dllist},
    {"dlopen",   bin_dlopen},
    /* fun */
    {"fortune",  bin_fortune},
    {"nyan",     bin_nyan},
    {"matrix",   bin_matrix},
    {"cowsay",   bin_cowsay},
    {"sl",       bin_sl},
    {"rickroll", bin_rickroll},
    {"rick",     bin_rickroll},
    {"doge",     bin_doge},
    {"wow",      bin_doge},
    {"brainrot", bin_brainrot},
    {"skibidi",  bin_brainrot},
    {"ohio",     bin_ohio},
    {"ratio",    bin_ratio},
    {"gigachad", bin_gigachad},
    {"chad",     bin_gigachad},
    {"shrug",    bin_shrug},
    {"tableflip",bin_tableflip},
    {"flip",     bin_tableflip},
    {"yeet",     bin_yeet},
    {"based",    bin_based},
    {"boykisser",bin_boykisser},
    {"nya",      bin_boykisser},
    {"doomscroll",bin_doomscroll},
    {"spin",     bin_spinning},
    {"speed",    bin_spinning},
    {"ligma",    bin_ligma},
    {"updog",    bin_updog},
    {"42",       bin_answer42},
    {"answer",   bin_answer42},
    /* QoL */
    {"calc",     bin_calc},
    {"which",    bin_which},
    {"type",     bin_type},
    {"watch",    bin_watch},
    {"time",     bin_time},
    {"env",       bin_env_cmd_wrap},
    {"printenv",  bin_printenv},
    {"false",    bin_false},
    {"true",     bin_true},
    {"compress", bin_compress},
    {"decompress",bin_decompress},
    {"banner",   bin_banner},
    {"color",    bin_color},
    {"colour",   bin_color},
    {"random",   bin_random},
    {"rand",     bin_random},
    {"ascii",    bin_ascii},
    {"hexcalc",  bin_hexcalc},
    {"scrap",    bin_scrap},
    {"morse",    bin_morse},
    {"lolcat",   bin_lolcat},
    {"figlet",   bin_figlet},
    {"pipes",    bin_pipes},
    {"repeat",   bin_repeat},
    {"explorer", bin_yaoigui},
    {NULL, NULL}
};

static inline int bin_dispatch(char**argv, int argc, sfs_node_t*cwd){
    if(!argv||argc==0) return 0;
    for(int i=0;bin_table[i].name;i++){
        const char*a=bin_table[i].name,*b=argv[0];
        while(*a&&*a==*b){a++;b++;}
        if(!*a&&!*b){bin_table[i].fn(argv,argc,cwd);return 1;}
    }
    return 0;
}

#endif /* BIN_REGISTRY_H */
