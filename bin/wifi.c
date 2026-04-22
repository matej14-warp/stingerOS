

void bin_wifi(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: wifi scan|connect <ssid> [pass]|disconnect|status\n");return;}
    if(b_scmp(argv[1],"scan")==0){wifi_scan();for(int i=0;i<wifi_ap_count;i++)terminal_printf("  %s  ch%d  %ddBm\n",wifi_aps[i].ssid,wifi_aps[i].channel,wifi_aps[i].rssi);}
    else if(b_scmp(argv[1],"connect")==0&&argc>2)wifi_connect(argv[2],argc>3?argv[3]:"");
    else if(b_scmp(argv[1],"disconnect")==0)wifi_disconnect();
    else if(b_scmp(argv[1],"status")==0){char buf[128];wifi_status(buf,128);terminal_printf("%s\n",buf);}
    else terminal_printf("wifi: unknown subcommand\n");}



