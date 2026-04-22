


void bin_diskinfo(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("ATA drives:\n");
    for(int i=0;i<4;i++){uint32_t s=ata_drive_sectors(i);if(s)terminal_printf("  hd%c: %u MB\n",'a'+i,s/2048);}
    terminal_printf("AHCI drives:\n");
    for(int i=0;i<8;i++){uint64_t s=ahci_drive_sectors(i);if(s)terminal_printf("  sata%d: %llu MB  %s\n",i,(unsigned long long)(s/2048),ahci_drive_model(i));}}



