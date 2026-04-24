#include "_bin_common.h"
#include "../fetch/fetch.h"
void bin_fetch(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: fetch <pkg>|--list|--update|--cdn <host>\n");return;}
    if(b_scmp(argv[1],"--list")==0){fetch_list_packages();}
    else if(b_scmp(argv[1],"--update")==0){fetch_update_index();}
    else if(b_scmp(argv[1],"--cdn")==0&&argc>2){fetch_set_cdn(argv[2]);}
    else{terminal_printf("Fetching package: %s\n",argv[1]);fetch_package(argv[1]);}}