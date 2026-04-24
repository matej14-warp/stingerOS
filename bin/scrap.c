#include "_bin_common.h"
static char scrap_buf[2048]="";
void bin_scrap(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("%s\n",scrap_buf[0]?scrap_buf:"(empty)");return;}
    if(b_scmp(argv[1],"clear")==0){scrap_buf[0]=0;terminal_printf("scrap cleared\n");}
    else if(b_scmp(argv[1],"save")==0&&argc>2){sfs_write_file(cwd,argv[2],(uint8_t*)scrap_buf,b_slen(scrap_buf));terminal_printf("saved to %s\n",argv[2]);}
    else{int l=b_slen(scrap_buf);b_sncpy(scrap_buf+l,argv[1],(int)(sizeof(scrap_buf)-l-2));b_sncpy(scrap_buf+l+b_slen(argv[1]),"\n",2);terminal_printf("added to scrap\n");}}