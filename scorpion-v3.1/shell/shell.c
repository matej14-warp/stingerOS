/* scorpion OS - shell/shell.c  v3.1
   New commands vs v2: chmod, chown, passwd, hostname, df, du,
                       ssh, irc, fetch --update, adduser       */

#include "shell.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include "../fs/sfs.h"
#include "../fs/partition.h"
#include "../fs/sfs_disk.h"
#include "../drivers/keyboard.h"
#include "../drivers/wifi.h"
#include "../drivers/ata.h"
#include "../fetch/fetch.h"
#include "../mbf/mbf.h"
#include "../drivers/net.h"
#include "../auth/auth.h"
#include "../net/ssh.h"
#include "../net/irc.h"
#include <stdint.h>
#include <stddef.h>

/* ---- helpers ---- */
static size_t slen(const char*s){size_t n=0;while(s[n])n++;return n;}
static int scmp(const char*a,const char*b){
    while(*a&&*a==*b){a++;b++;} return(unsigned char)*a-(unsigned char)*b;}
static int sncmp(const char*a,const char*b,size_t n){
    while(n&&*a&&*a==*b){a++;b++;n--;}
    return n?(unsigned char)*a-(unsigned char)*b:0;}
static void scpy(char*d,const char*s){while((*d++=*s++));}
static int sisspace(char c){return c==' '||c=='\t';}
static int satoi(const char *s){int n=0;while(*s>='0'&&*s<='9')n=n*10+(*s++)-'0';return n;}
static unsigned long satoul_hex(const char *s){
    unsigned long n=0;
    if(s[0]=='0'&&(s[1]=='x'||s[1]=='X')) s+=2;
    while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F')){
        n<<=4;
        if(*s>='0'&&*s<='9') n+=*s-'0';
        else if(*s>='a'&&*s<='f') n+=*s-'a'+10;
        else n+=*s-'A'+10;
        s++;
    }
    return n;
}
static unsigned long satoul_octal(const char *s){
    unsigned long n=0;
    while(*s>='0'&&*s<='7') n=n*8+(unsigned long)(*s++)-'0';
    return n;
}

static int tokenize(char *line, char **argv, int max){
    int n=0; char *p=line;
    while(*p&&n<max){
        while(*p&&sisspace(*p))p++;
        if(!*p) break;
        argv[n++]=p;
        while(*p&&!sisspace(*p))p++;
        if(*p)*p++=0;
    }
    return n;
}

/* ---- Shell state ---- */
static sfs_node_t *cwd;
static char        input_buf[512];
static part_entry_t g_parts[MAX_PARTITIONS];
static int          g_part_count = 0;

char shell_getchar(void) { return keyboard_getchar(); }

/* ---- readline variants ---- */
static int readline_ex(char *buf, size_t maxlen, int mask){
    size_t pos=0;
    while(1){
        char c=keyboard_getchar();
        if(c=='\n'||c=='\r'){buf[pos]=0;terminal_putchar('\n');return(int)pos;}
        else if(c=='\b'){
            if(pos>0){pos--;terminal_putchar('\b');terminal_putchar(' ');terminal_putchar('\b');}
        } else if(c>=32&&c<127&&pos<maxlen-1){
            buf[pos++]=c; terminal_putchar(mask?'*':c);
        }
    }
}
static int readline(char *buf, size_t maxlen){ return readline_ex(buf,maxlen,0); }
static int readline_masked(char *buf, size_t maxlen){ return readline_ex(buf,maxlen,1); }

/* ---- ls callback ---- */
static void ls_cb(sfs_node_t *node, void *userdata){
    (void)userdata;
    /* mode string */
    uint16_t m = node->mode ? node->mode : (node->type==SFS_DIR ? 0755 : 0644);
    char ms[11]; ms[0]=node->type==SFS_DIR?'d':'-';
    ms[1]=(m&0400)?'r':'-'; ms[2]=(m&0200)?'w':'-'; ms[3]=(m&0100)?'x':'-';
    ms[4]=(m&0040)?'r':'-'; ms[5]=(m&0020)?'w':'-'; ms[6]=(m&0010)?'x':'-';
    ms[7]=(m&0004)?'r':'-'; ms[8]=(m&0002)?'w':'-'; ms[9]=(m&0001)?'x':'-';
    ms[10]=0;
    terminal_printf("%s %4u  ", ms, (unsigned)node->uid);
    if(node->type==SFS_DIR){
        terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
        terminal_writestring(node->name); terminal_putchar('/');
    } else {
        terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE,VGA_COLOR_BLACK));
        terminal_writestring(node->name);
    }
    size_t l=slen(node->name)+(node->type==SFS_DIR?1:0);
    for(size_t i=l;i<18;i++) terminal_putchar(' ');
    if(node->type==SFS_FILE) terminal_printf("%u\n",(unsigned)node->size);
    else terminal_putchar('\n');
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
}

/* ============================================================
   ORIGINAL COMMANDS
   ============================================================ */
static void cmd_ls(char **argv, int argc){
    sfs_node_t *dir=cwd;
    if(argc>=2){dir=sfs_resolve(cwd,argv[1]);
        if(!dir){terminal_printf("ls: no such directory: %s\n",argv[1]);return;}
        if(dir->type!=SFS_DIR){terminal_printf("ls: not a directory: %s\n",argv[1]);return;}
    }
    sfs_list(dir,ls_cb,NULL);
}
static void cmd_cd(char **argv, int argc){
    if(argc<2){cwd=sfs_root();return;}
    sfs_node_t *n=sfs_resolve(cwd,argv[1]);
    if(!n){terminal_printf("cd: no such directory: %s\n",argv[1]);return;}
    if(n->type!=SFS_DIR){terminal_printf("cd: not a directory: %s\n",argv[1]);return;}
    cwd=n;
}
static void cmd_pwd(void){char buf[512];sfs_path(cwd,buf,sizeof(buf));terminal_writestring(buf);terminal_putchar('\n');}
static void cmd_cat(char **argv, int argc){
    if(argc<2){terminal_printf("cat: missing filename\n");return;}
    sfs_node_t *n=sfs_resolve(cwd,argv[1]);
    if(!n){terminal_printf("cat: no such file: %s\n",argv[1]);return;}
    if(n->type!=SFS_FILE){terminal_printf("cat: is a directory\n");return;}
    if(n->data&&n->size>0) terminal_write((const char*)n->data,n->size);
    terminal_putchar('\n');
}
static void cmd_mkdir_sh(char **argv, int argc){
    if(argc<2){terminal_printf("mkdir: missing name\n");return;}
    if(!sfs_mkdir(cwd,argv[1])) terminal_printf("mkdir: failed (exists or OOM)\n");
}
static void cmd_rm(char **argv, int argc){
    if(argc<2){terminal_printf("rm: missing path\n");return;}
    sfs_node_t *n=sfs_resolve(cwd,argv[1]);
    if(!n){terminal_printf("rm: not found: %s\n",argv[1]);return;}
    if(sfs_delete(n)<0) terminal_printf("rm: cannot delete root\n");
}
static void cmd_echo(char **argv, int argc){
    for(int i=1;i<argc;i++){terminal_writestring(argv[i]);if(i<argc-1)terminal_putchar(' ');}
    terminal_putchar('\n');
}
static void cmd_write(char **argv, int argc){
    if(argc<3){terminal_printf("write: write <file> <content>\n");return;}
    char buf[512]; int pos=0;
    for(int i=2;i<argc;i++){
        for(char*p=argv[i];*p&&pos<510;p++) buf[pos++]=*p;
        if(i<argc-1&&pos<510) buf[pos++]=' ';
    }
    buf[pos++]='\n'; buf[pos]=0;
    sfs_write_file(cwd,argv[1],(const uint8_t*)buf,(size_t)pos);
    terminal_printf("wrote %d bytes\n",pos);
}
static void cmd_reboot(void){
    terminal_printf("Rebooting...\n");
    uint8_t tmp; do{__asm__ volatile("inb $0x64,%0":"=a"(tmp));}while(tmp&2);
    __asm__ volatile("outb %0,$0x64"::"a"((uint8_t)0xFE));
    __asm__ volatile("cli; hlt");
}
static void cmd_halt(void){terminal_printf("System halted.\n");__asm__ volatile("cli; hlt");}
static void cmd_mbf_run(char **argv, int argc){
    if(argc<2){terminal_printf("mbf: missing script path\n");return;}
    mbf_run_file(argv[1],cwd);
}

/* ============================================================
   DISK COMMANDS (from v2, kept intact)
   ============================================================ */
static void cmd_diskinfo(void){
    terminal_printf("ATA drives:\n");
    for(int d=0;d<4;d++){uint32_t s=ata_drive_sectors(d);
        if(s) terminal_printf("  hd%c: %u sectors (%u MB)\n",'a'+d,s,s/2048);}
}
static void cmd_lspart(void){
    if(g_part_count==0){terminal_printf("no partitions found\n");return;}
    partition_print_table(g_parts,g_part_count);
}
static void cmd_mkpart(char **argv, int argc){
    if(argc<6){terminal_printf("usage: mkpart <d> <i> <type_hex> <lba> <mb>\n");return;}
    int drive=satoi(argv[1]),idx=satoi(argv[2]);
    uint8_t type=(uint8_t)satoul_hex(argv[3]);
    uint32_t start=(uint32_t)satoi(argv[4]),mb=(uint32_t)satoi(argv[5]);
    if(partition_create(drive,idx,type,start,mb)!=0)
        terminal_printf("error: partition_create failed\n");
    else terminal_printf("done.\n");
    g_part_count=partition_detect_all(g_parts,MAX_PARTITIONS);
}
static void cmd_rmpart(char **argv, int argc){
    if(argc<3){terminal_printf("usage: rmpart <d> <i>\n");return;}
    if(partition_delete(satoi(argv[1]),satoi(argv[2]))!=0)
        terminal_printf("error\n");
    else terminal_printf("deleted\n");
    g_part_count=partition_detect_all(g_parts,MAX_PARTITIONS);
}
static void cmd_fdisk(char **argv, int argc){
    (void)argc;(void)argv;
    terminal_printf("fdisk  n=new  d=delete  p=print  w=write&exit  q=quit\n");
    terminal_printf("drive [0-3]: ");
    char buf[8]; readline(buf,sizeof(buf));
    int drive=satoi(buf);
    while(1){
        terminal_printf("fdisk(hd%c)> ",'a'+drive);
        char c=keyboard_getchar(); terminal_putchar(c); terminal_putchar('\n');
        if(c=='p'){ partition_print_table(g_parts,g_part_count); }
        else if(c=='n'){
            char ibuf[4],tbuf[4],sbuf[12],mbbuf[12];
            terminal_printf("idx(0-3): ");  readline(ibuf,4);
            terminal_printf("type(hex): "); readline(tbuf,4);
            terminal_printf("start LBA: "); readline(sbuf,12);
            terminal_printf("size MB: ");   readline(mbbuf,12);
            if(partition_create(drive,satoi(ibuf),(uint8_t)satoul_hex(tbuf),
                                (uint32_t)satoi(sbuf),(uint32_t)satoi(mbbuf))==0)
                terminal_printf("created\n"); else terminal_printf("error\n");
        } else if(c=='d'){
            char ibuf[4]; terminal_printf("idx: "); readline(ibuf,4);
            if(partition_delete(drive,satoi(ibuf))==0) terminal_printf("deleted\n");
            else terminal_printf("error\n");
        } else if(c=='w'){g_part_count=partition_detect_all(g_parts,MAX_PARTITIONS);terminal_printf("done\n");return;}
        else if(c=='q'){return;}
        else terminal_printf("unknown\n");
    }
}
static void cmd_format(char **argv, int argc){
    if(argc<3){terminal_printf("usage: format <d> <i>\n");return;}
    int drive=satoi(argv[1]),idx=satoi(argv[2]);
    for(int i=0;i<g_part_count;i++){
        if(g_parts[i].drive==drive&&g_parts[i].index==idx){
            uint8_t buf[512]; for(int j=0;j<512;j++)buf[j]=0;
            buf[0]='S';buf[1]='F';buf[2]='S';buf[3]=0x01;buf[510]=0x55;buf[511]=0xAA;
            if(ata_write_sectors(drive,g_parts[i].lba_start,1,buf)==0)
                terminal_printf("formatted\n");
            else terminal_printf("write error\n");
            return;
        }
    }
    terminal_printf("partition not found\n");
}
static void cmd_install(void){
    terminal_printf("run: mbf /etc/installer.mbf\n");
    terminal_printf("  (or use the interactive 'install' wizard below)\n\n");
    if(g_part_count==0){terminal_printf("no partitions found\n");return;}
    partition_print_table(g_parts,g_part_count);
    char dbuf[4],ibuf[4];
    terminal_printf("drive [0-3]: "); readline(dbuf,4);
    terminal_printf("partition index [1-4]: "); readline(ibuf,4);
    int drive=satoi(dbuf),idx=satoi(ibuf);
    int pi=-1;
    for(int i=0;i<g_part_count;i++) if(g_parts[i].drive==drive&&g_parts[i].index==idx){pi=i;break;}
    if(pi<0){terminal_printf("partition not found\n");return;}
    terminal_printf("installing to /dev/hd%c%d...\n",'a'+drive,idx);
    terminal_printf("[1/4] formatting...\n");
    {uint8_t buf[512];for(int j=0;j<512;j++)buf[j]=0;
     buf[0]='S';buf[1]='F';buf[2]='S';buf[3]=0x01;buf[510]=0x55;buf[511]=0xAA;
     if(ata_write_sectors(drive,g_parts[pi].lba_start,1,buf)!=0){terminal_printf("failed\n");return;}}
    terminal_printf("[2/4] creating directory structure...\n");
    sfs_node_t *r=sfs_mkdir(sfs_root(),"install_root");
    if(r){sfs_mkdir(r,"boot");sfs_mkdir(r,"bin");sfs_mkdir(r,"etc");}
    terminal_printf("[3/4] writing grub.cfg...\n");
    const char *gcfg="set timeout=3\nset default=0\nmenuentry 'Scorpion OS'{\n    multiboot /boot/kernel.elf\n    boot\n}\n";
    if(r){sfs_node_t *bd=sfs_find_child(r,"boot");
          if(bd) sfs_write_file(bd,"grub.cfg",(const uint8_t*)gcfg,slen(gcfg));}
    terminal_printf("[4/4] patching MBR...\n");
    {uint8_t mbr_buf[512];
     if(ata_read_sectors(drive,0,1,mbr_buf)==0){
         mbr_buf[444]='S';mbr_buf[445]='C';ata_write_sectors(drive,0,1,mbr_buf);}}
    terminal_printf("installation complete!\n");
}

/* ============================================================
   SYNC COMMANDS (from v2)
   ============================================================ */
static void cmd_setlabel(char **argv, int argc){
    if(argc<4){terminal_printf("usage: setlabel <d> <i> <label>\n");return;}
    int drive=satoi(argv[1]),idx=satoi(argv[2]);
    for(int i=0;i<g_part_count;i++){
        if(g_parts[i].drive==drive&&g_parts[i].index==idx){
            int li=0; for(char*p=argv[3];*p&&li<11;p++) g_parts[i].label[li++]=(char)*p; g_parts[i].label[li]=0;
            terminal_printf("label set: /dev/hd%c%d -> \"%s\"\n",'a'+drive,idx,g_parts[i].label);
            return;
        }
    }
    terminal_printf("partition not found\n");
}
static void cmd_sync_shell(void){ cmd_sync(g_parts,g_part_count); }
static void cmd_diskload(void){
    terminal_printf("replace in-memory filesystem? [y/N] ");
    char c=keyboard_getchar(); terminal_putchar(c); terminal_putchar('\n');
    if(c!='y'&&c!='Y'){terminal_printf("cancelled\n");return;}
    cmd_mount_disk(g_parts,g_part_count);
    cwd=sfs_root();
}

/* ============================================================
   NETWORK COMMANDS (from v2)
   ============================================================ */
static void cmd_ifconfig(void){
    net_config_t cfg; net_get_config(&cfg);
    if(!cfg.configured){terminal_printf("eth0: not configured\n");return;}
    terminal_printf("eth0:  IP   %d.%d.%d.%d\n",cfg.ip[0],cfg.ip[1],cfg.ip[2],cfg.ip[3]);
    terminal_printf("       mask %d.%d.%d.%d\n",cfg.mask[0],cfg.mask[1],cfg.mask[2],cfg.mask[3]);
    terminal_printf("       gw   %d.%d.%d.%d\n",cfg.gw[0],cfg.gw[1],cfg.gw[2],cfg.gw[3]);
    terminal_printf("       MAC  %x:%x:%x:%x:%x:%x\n",
        cfg.mac[0],cfg.mac[1],cfg.mac[2],cfg.mac[3],cfg.mac[4],cfg.mac[5]);
}
static void cmd_ping(char **argv, int argc){
    if(argc<2){terminal_printf("usage: ping <ip|localhost>\n");return;}
    uint8_t ip[4]={0,0,0,0};
    /* FIX: resolve "localhost" to 127.0.0.1 */
    if(scmp(argv[1],"localhost")==0){
        ip[0]=127; ip[1]=0; ip[2]=0; ip[3]=1;
    } else {
        int oct=0; const char *p=argv[1];
        while(*p&&oct<4){int n=0;while(*p>='0'&&*p<='9')n=n*10+(*p++)-'0';ip[oct++]=(uint8_t)n;if(*p=='.')p++;}
    }
    terminal_printf("PING %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
    /* FIX: loopback (127.x.x.x) never hits the wire — always reachable */
    if(ip[0]==127){
        terminal_printf("reachable (loopback)\n"); return;
    }
    /* FIX: pinging our own IP is always reachable, no ARP needed */
    net_config_t self; net_get_config(&self);
    if(self.configured && ip[0]==self.ip[0]&&ip[1]==self.ip[1]&&ip[2]==self.ip[2]&&ip[3]==self.ip[3]){
        terminal_printf("reachable (self, MAC %x:%x:%x:%x:%x:%x)\n",
            self.mac[0],self.mac[1],self.mac[2],self.mac[3],self.mac[4],self.mac[5]);
        return;
    }
    uint8_t mac[6];
    if(net_arp_resolve(ip,mac))
        terminal_printf("reachable (MAC %x:%x:%x:%x:%x:%x)\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    else terminal_printf("unreachable\n");
}
static void cmd_wget(char **argv, int argc){
    if(argc<2){terminal_printf("usage: wget <url> [dest]\n");return;}
    const char *url=argv[1];
    if(sncmp(url,"http://",7)!=0){terminal_printf("only http:// supported\n");return;}
    const char *host=url+7; const char *path="/"; const char *sl=host;
    while(*sl&&*sl!='/') sl++;
    if(*sl=='/') path=sl;
    char hostbuf[128]; int hi=0;
    while(host<sl&&hi<127) hostbuf[hi++]=(char)*host++;
    hostbuf[hi]=0;
    terminal_printf("downloading http://%s%s...\n",hostbuf,path);
    uint8_t *body=NULL; size_t blen=0;
    if(http_get(hostbuf,path,&body,&blen)!=0){terminal_printf("failed\n");return;}
    terminal_printf("downloaded %u bytes\n",(unsigned)blen);
    const char *dest=argc>=3?argv[2]:path;
    const char *fname=dest; for(const char *q=dest;*q;q++) if(*q=='/') fname=q+1;
    if(!*fname) fname="index.html";
    sfs_node_t *tmp=sfs_resolve(sfs_root(),"/tmp");
    if(!tmp) tmp=sfs_mkdir(sfs_root(),"tmp");
    sfs_write_file(tmp,fname,body,blen);
    terminal_printf("saved to /tmp/%s\n",fname);
    if(body) kfree(body);
}
static void cmd_wifi(char **argv, int argc){
    if(argc<2){terminal_printf("usage: wifi <scan|connect|disconnect|status>\n");return;}
    if(scmp(argv[1],"scan")==0){
        terminal_printf("scanning...\n");
        wifi_net_t nets[WIFI_MAX_NETS]; int n=wifi_scan(nets,WIFI_MAX_NETS);
        if(n<0){terminal_printf("no WiFi adapter\n");return;}
        terminal_printf("found %d network(s)\n",n);
        if(n>0) wifi_print_nets(nets,n);
    } else if(scmp(argv[1],"connect")==0){
        if(argc<3){terminal_printf("wifi connect <ssid> [pass]\n");return;}
        if(wifi_connect(argv[2],argc>=4?argv[3]:"")==0) terminal_printf("connected\n");
        else terminal_printf("failed\n");
    } else if(scmp(argv[1],"disconnect")==0){ wifi_disconnect(); terminal_printf("disconnected\n"); }
    else if(scmp(argv[1],"status")==0){
        if(!wifi_active){terminal_printf("no WiFi driver\n");return;}
        terminal_printf("driver: %s (%s)\n",wifi_active->name,wifi_active->chipset);
        terminal_printf("status: %s\n",wifi_active->is_connected()?"connected":"disconnected");
    } else terminal_printf("unknown subcommand\n");
}

/* ============================================================
   NEW PHASE 1/3 COMMANDS
   ============================================================ */

/* chmod <octal> <path> */
static void cmd_chmod(char **argv, int argc){
    if(argc<3){terminal_printf("usage: chmod <octal_mode> <path>\n");return;}
    sfs_node_t *n=sfs_resolve(cwd,argv[2]);
    if(!n){terminal_printf("chmod: not found: %s\n",argv[2]);return;}
    uint16_t mode=(uint16_t)satoul_octal(argv[1]);
    sfs_chmod(n,mode);
    terminal_printf("mode set to %04o on %s\n",(unsigned)mode,argv[2]);
}

/* chown <uid> <path> */
static void cmd_chown(char **argv, int argc){
    if(argc<3){terminal_printf("usage: chown <uid> <path>\n");return;}
    sfs_node_t *n=sfs_resolve(cwd,argv[2]);
    if(!n){terminal_printf("chown: not found: %s\n",argv[2]);return;}
    sfs_chown(n,(uint32_t)satoi(argv[1]),n->gid);
    terminal_printf("owner set to %d\n",satoi(argv[1]));
}

/* passwd [username]  — change password */
static void cmd_passwd(char **argv, int argc){
    const char *target = (argc>=2) ? argv[1] : current_user.username;
    /* Only root can change other users */
    if(scmp(target,current_user.username)!=0 && current_user.uid!=0){
        terminal_printf("passwd: permission denied\n"); return;
    }
    terminal_printf("New password for %s: ", target);
    char pw1[128]; readline_masked(pw1,128);
    terminal_printf("Retype new password: ");
    char pw2[128]; readline_masked(pw2,128);
    /* compare */
    if(scmp(pw1,pw2)!=0){terminal_printf("Passwords do not match.\n");return;}
    if(auth_add_user(target,pw1,0,0,"/root","/bin/sh")==0)
        terminal_printf("Password updated.\n");
    else
        terminal_printf("Error updating password.\n");
}

/* adduser <username> */
static void cmd_adduser(char **argv, int argc){
    if(argc<2){terminal_printf("usage: adduser <username>\n");return;}
    if(current_user.uid!=0){terminal_printf("permission denied (must be root)\n");return;}
    terminal_printf("Password for %s: ", argv[1]);
    char pw[128]; readline_masked(pw,128);
    char home[64]; home[0]='/'; home[1]='h'; home[2]='o'; home[3]='m'; home[4]='e'; home[5]='/';
    int i=6; for(char *p=argv[1];*p&&i<62;p++) home[i++]=*p; home[i]=0;
    sfs_node_t *homedir=sfs_resolve(sfs_root(),"/home");
    if(!homedir) homedir=sfs_mkdir(sfs_root(),"home");
    if(homedir) sfs_mkdir(homedir,argv[1]);
    static uint32_t next_uid=1000;
    if(auth_add_user(argv[1],pw,next_uid,next_uid,home,"/bin/sh")==0){
        terminal_printf("user %s created (uid=%u)\n",argv[1],next_uid);
        next_uid++;
    } else terminal_printf("failed\n");
}

/* hostname [new_name] */
static void cmd_hostname(char **argv, int argc){
    if(argc>=2){
        auth_set_hostname(argv[1]);
        terminal_printf("hostname set to %s\n",argv[1]);
    } else {
        char h[64]; auth_get_hostname(h,64);
        terminal_printf("%s\n",h);
    }
}

/* uname -a */
static void cmd_uname(char **argv, int argc){
    char h[64]; auth_get_hostname(h,64);
    terminal_printf("scorpion OS 3.1 %s x86 SMP 2025 #1 GNU/SFS\n",h);
    (void)argv;(void)argc;
}

/* df — disk usage of SYNC_TARGET partition */
static void cmd_df(void){
    terminal_printf("Filesystem        Size    Used   Avail  Use%%  Mounted\n");
    /* RAM filesystem stats */
    uint32_t nodes=0, bytes=0;
    sfs_du(sfs_root(),&nodes,&bytes);
    terminal_printf("scorpionfs (ram)  4096KB  %4uKB %4uKB  %u%%   /\n",
        bytes/1024, (4096*1024-bytes)/1024,
        bytes/(4096*1024/100+1));
    /* ATA partitions */
    for(int i=0;i<g_part_count;i++){
        uint32_t mb=g_parts[i].size_lba/2048;
        terminal_printf("/dev/hd%c%d        %4uMB  %-6s %-6s  ---   -\n",
            'a'+g_parts[i].drive, g_parts[i].index,
            mb, "-", "-");
    }
}

/* du [path] — disk usage of a directory */
static void cmd_du(char **argv, int argc){
    sfs_node_t *target = (argc>=2) ? sfs_resolve(cwd,argv[1]) : cwd;
    if(!target){terminal_printf("du: not found: %s\n",argv[1]);return;}
    uint32_t nodes=0, bytes=0;
    sfs_du(target,&nodes,&bytes);
    char path[128]; sfs_path(target,path,128);
    terminal_printf("%u bytes\t%u nodes\t%s\n",bytes,nodes,path);
}

/* free */
static void cmd_free(void){terminal_printf("heap: ~%u KB free\n",(unsigned)(kheap_free()/1024));}

/* ssh <ip> [user] */
static void cmd_ssh(char **argv, int argc){
    if(argc<2){terminal_printf("usage: ssh <ip> [username]\n");return;}
    const char *user=argc>=3?argv[2]:current_user.username;
    terminal_printf("Password for %s@%s: ", user, argv[1]);
    char pw[128]; readline_masked(pw,128);
    ssh_connect(argv[1], user, pw);
}

/* irc <server_ip> [nick] [channel] */
static void cmd_irc(char **argv, int argc){
    if(argc<2){terminal_printf("usage: irc <server_ip> [nick] [#channel]\n");return;}
    const char *nick=argc>=3?argv[2]:"scorpion";
    const char *chan=argc>=4?argv[3]:"#scorpion";
    irc_connect(argv[1], nick, chan);
}

/* fetch --update  OR  fetch <pkg> */
static void cmd_fetch(char **argv, int argc){
    if(argc<2){terminal_printf("fetch: missing argument\n");return;}
    if(scmp(argv[1],"--update")==0){
        terminal_printf(":: checking for kernel updates...\n");
        uint8_t *body=NULL; size_t blen=0;
        if(http_get("cdn.voided.uk","/fetch3/scorpion-kernel",&body,&blen)==0 && body && blen>4){
            terminal_printf(":: update found (%u bytes)\n",(unsigned)blen);
            terminal_printf(":: saving to /tmp/kernel-update.zip\n");
            sfs_node_t *tmp=sfs_resolve(sfs_root(),"/tmp");
            if(!tmp) tmp=sfs_mkdir(sfs_root(),"tmp");
            sfs_write_file(tmp,"kernel-update.zip",body,blen);
            terminal_printf(":: run 'fetch scorpion-kernel' to install\n");
        } else {
            terminal_printf(":: no update available or CDN unreachable\n");
        }
        if(body) kfree(body);
    } else {
        fetch_package(argv[1]);
    }
}

/* fstab — read /etc/fstab and display */
static void cmd_fstab(void){
    sfs_node_t *etc=sfs_resolve(sfs_root(),"/etc");
    if(etc){sfs_node_t *f=sfs_find_child(etc,"fstab");
        if(f&&f->data){terminal_write((const char*)f->data,f->size);return;}}
    terminal_printf("(no /etc/fstab found)\n");
}

/* ============================================================
   HELP
   ============================================================ */
static void cmd_help(void){
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK));
    terminal_writestring(
        "scorpion OS v3.1 commands:\n\n"
        "  Files:    ls  cd  pwd  cat  mkdir  rm  write  echo\n"
        "  Perms:    chmod <octal> <path>  chown <uid> <path>\n"
        "  Users:    passwd [user]  adduser <user>  hostname [name]\n"
        "  System:   uname  free  df  du [path]  fstab  mbf <script>\n"
        "  Disk:     diskinfo  lspart  fdisk  mkpart  rmpart  format  install\n"
        "  Sync:     setlabel <d> <i> <label>  sync  diskload\n"
        "  WiFi:     wifi scan|connect <ssid> [pass]|disconnect|status\n"
        "  Network:  ifconfig  ping <ip>  wget <url>\n"
        "            ssh <ip> [user]\n"
        "            irc <ip> [nick] [#chan]\n"
        "  Pkgs:     fetch <pkg>  fetch --update\n"
        "  Boot:     clear  reboot  halt  help\n"
    );
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
}

/* ============================================================
   DISPATCH
   ============================================================ */
static void execute(char **argv, int argc){
    if(argc==0) return;
    if(scmp(argv[0],"ls"      )==0){ cmd_ls(argv,argc);        return; }
    if(scmp(argv[0],"cd"      )==0){ cmd_cd(argv,argc);        return; }
    if(scmp(argv[0],"pwd"     )==0){ cmd_pwd();                return; }
    if(scmp(argv[0],"cat"     )==0){ cmd_cat(argv,argc);       return; }
    if(scmp(argv[0],"mkdir"   )==0){ cmd_mkdir_sh(argv,argc);  return; }
    if(scmp(argv[0],"rm"      )==0){ cmd_rm(argv,argc);        return; }
    if(scmp(argv[0],"echo"    )==0){ cmd_echo(argv,argc);      return; }
    if(scmp(argv[0],"write"   )==0){ cmd_write(argv,argc);     return; }
    if(scmp(argv[0],"mbf"     )==0){ cmd_mbf_run(argv,argc);   return; }
    if(scmp(argv[0],"clear"   )==0){ terminal_clear();         return; }
    if(scmp(argv[0],"help"    )==0){ cmd_help();               return; }
    if(scmp(argv[0],"reboot"  )==0){ cmd_reboot();             return; }
    if(scmp(argv[0],"halt"    )==0){ cmd_halt();               return; }
    if(scmp(argv[0],"diskinfo")==0){ cmd_diskinfo();           return; }
    if(scmp(argv[0],"lspart"  )==0){ cmd_lspart();             return; }
    if(scmp(argv[0],"fdisk"   )==0){ cmd_fdisk(argv,argc);     return; }
    if(scmp(argv[0],"mkpart"  )==0){ cmd_mkpart(argv,argc);    return; }
    if(scmp(argv[0],"rmpart"  )==0){ cmd_rmpart(argv,argc);    return; }
    if(scmp(argv[0],"format"  )==0){ cmd_format(argv,argc);    return; }
    if(scmp(argv[0],"install" )==0){ cmd_install();            return; }
    if(scmp(argv[0],"setlabel")==0){ cmd_setlabel(argv,argc);  return; }
    if(scmp(argv[0],"sync"    )==0){ cmd_sync_shell();         return; }
    if(scmp(argv[0],"diskload")==0){ cmd_diskload();           return; }
    if(scmp(argv[0],"ifconfig")==0){ cmd_ifconfig();           return; }
    if(scmp(argv[0],"ping"    )==0){ cmd_ping(argv,argc);      return; }
    if(scmp(argv[0],"wget"    )==0){ cmd_wget(argv,argc);      return; }
    if(scmp(argv[0],"wifi"    )==0){ cmd_wifi(argv,argc);      return; }
    if(scmp(argv[0],"chmod"   )==0){ cmd_chmod(argv,argc);     return; }
    if(scmp(argv[0],"chown"   )==0){ cmd_chown(argv,argc);     return; }
    if(scmp(argv[0],"passwd"  )==0){ cmd_passwd(argv,argc);    return; }
    if(scmp(argv[0],"adduser" )==0){ cmd_adduser(argv,argc);   return; }
    if(scmp(argv[0],"hostname")==0){ cmd_hostname(argv,argc);  return; }
    if(scmp(argv[0],"uname"   )==0){ cmd_uname(argv,argc);     return; }
    if(scmp(argv[0],"df"      )==0){ cmd_df();                 return; }
    if(scmp(argv[0],"du"      )==0){ cmd_du(argv,argc);        return; }
    if(scmp(argv[0],"free"    )==0){ cmd_free();               return; }
    if(scmp(argv[0],"ssh"     )==0){ cmd_ssh(argv,argc);       return; }
    if(scmp(argv[0],"irc"     )==0){ cmd_irc(argv,argc);       return; }
    if(scmp(argv[0],"fetch"   )==0){ cmd_fetch(argv,argc);     return; }
    if(scmp(argv[0],"fstab"   )==0){ cmd_fstab();              return; }

    /* /bin lookup */
    sfs_node_t *bin=sfs_resolve(sfs_root(),"/bin");
    if(bin){
        sfs_node_t *prog=sfs_find_child(bin,argv[0]);
        if(prog&&prog->type==SFS_FILE&&prog->size>0){
            mbf_run((const char*)prog->data,prog->size,cwd); return;
        }
    }
    terminal_printf("scorpion: command not found: %s\n",argv[0]);
    terminal_printf("          type 'help' for available commands\n");
}

static void print_banner(void){
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_RED,VGA_COLOR_BLACK));
    terminal_writestring(
        "\n"
        "  ███████  ██████  ██████  ██████  ██████  ██  ██████  ███    ██ \n"
        "  ██      ██      ██    ██ ██   ██ ██   ██ ██ ██    ██ ████   ██ \n"
        "  ███████ ██      ██    ██ ██████  ██████  ██ ██    ██ ██ ██  ██ \n"
        "       ██ ██      ██    ██ ██   ██ ██      ██ ██    ██ ██  ██ ██ \n"
        "  ███████  ██████  ██████  ██   ██ ██      ██  ██████  ██   ████ \n"
        "\n"
    );
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
    terminal_writestring("  scorpion OS v3.1 | 32-bit | ScorpionFS | ANSI | SSH | IRC\n");
    terminal_writestring("  type 'help' for commands\n\n");
}

void shell_run(void){
    cwd = sfs_root();
    g_part_count = partition_detect_all(g_parts, MAX_PARTITIONS);
    if(g_part_count>0)
        terminal_printf("[shell] detected %d partition(s)\n", g_part_count);

    print_banner();

    while(1){
        char path[128]; sfs_path(cwd,path,sizeof(path));
        /* Prompt: user@hostname:path# */
        char hostname[64]; auth_get_hostname(hostname,64);
        terminal_writestring("\x1b[32m");
        terminal_writestring(current_user.username[0] ? current_user.username : "root");
        terminal_writestring("\x1b[0m@\x1b[36m");
        terminal_writestring(hostname);
        terminal_writestring("\x1b[0m:\x1b[34m");
        terminal_writestring(path);
        terminal_writestring("\x1b[0m");
        terminal_writestring(current_user.uid==0 ? "# " : "$ ");

        readline(input_buf, sizeof(input_buf));
        if(!input_buf[0]) continue;

        char *argv[32]; int argc=tokenize(input_buf,argv,32);
        execute(argv,argc);
    }
}
