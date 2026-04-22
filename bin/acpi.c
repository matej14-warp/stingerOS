

void bin_acpi(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc>1&&b_scmp(argv[1],"poweroff")==0){terminal_printf("powering off...\n");acpi_poweroff();}
    else if(argc>1&&b_scmp(argv[1],"reboot")==0){terminal_printf("rebooting...\n");acpi_reboot();}
    else terminal_printf("ACPI: %s\nPM1a_CNT: 0x%04x\n",g_acpi_ready?"ready":"not available",g_acpi_pm1a_cnt);}



