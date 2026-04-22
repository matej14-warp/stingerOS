










static void kzeromem(void *d, int n){uint8_t*p=d;while(n--)p[n]=0;}
static void kcopymem(void *d,const void*s,int n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}

int partition_detect_all(part_entry_t *out, int max){
    int total=0;
    for(int d=0;d<MAX_DRIVES&&total<max;d++){
        uint8_t buf[512];
        if(ata_read_sectors(d,0,1,buf)!=0) continue;
        mbr_t *mbr=(mbr_t*)buf;
        if(mbr->signature!=MBR_SIG) continue;
        for(int i=0;i<4&&total<max;i++){
            mbr_part_t *e=&mbr->entries[i];
            if(e->type==PART_TYPE_EMPTY) continue;
            out[total].drive    =d;
            out[total].index    =i+1;
            out[total].type     =e->type;
            out[total].lba_start=e->lba_start;
            out[total].size_lba =e->size_lba;
            out[total].bootable =(e->status==0x80);
            out[total].mounted  =0;
            out[total].label[0] =0;
            total++;
        }
    }
    return total;
}

int partition_create(int drive, int index, uint8_t type,
                      uint32_t lba_start, uint32_t size_mb){
    if(index<0||index>3) return -1;
    uint8_t buf[512];
    if(ata_read_sectors(drive,0,1,buf)!=0) return -1;
    mbr_t *mbr=(mbr_t*)buf;
    if(mbr->signature!=MBR_SIG){
        kzeromem(buf,512);
        mbr->signature=MBR_SIG;
    }

    lba_start=((lba_start+SECS_PER_MB-1)/SECS_PER_MB)*SECS_PER_MB;
    uint32_t size_lba=size_mb*SECS_PER_MB;

    mbr_part_t *e=&mbr->entries[index];
    kzeromem(e,sizeof(*e));
    e->status   =0x00;
    e->type     =type;
    e->lba_start=lba_start;
    e->size_lba =size_lba;

    e->chs_first[0]=0xFE; e->chs_first[1]=0xFF; e->chs_first[2]=0xFF;
    e->chs_last[0] =0xFE; e->chs_last[1] =0xFF; e->chs_last[2] =0xFF;

    return ata_write_sectors(drive,0,1,buf);
}

int partition_delete(int drive, int index){
    if(index<0||index>3) return -1;
    uint8_t buf[512];
    if(ata_read_sectors(drive,0,1,buf)!=0) return -1;
    mbr_t *mbr=(mbr_t*)buf;
    if(mbr->signature!=MBR_SIG) return -1;
    kzeromem(&mbr->entries[index],sizeof(mbr_part_t));
    return ata_write_sectors(drive,0,1,buf);
}

const char *partition_type_name(uint8_t type){
    switch(type){
        case PART_TYPE_EMPTY:  return "Empty";
        case PART_TYPE_FAT12:  return "FAT12";
        case PART_TYPE_FAT16:  return "FAT16";
        case PART_TYPE_EXT:    return "Extended";
        case PART_TYPE_FAT16B: return "FAT16B";
        case PART_TYPE_NTFS:   return "NTFS";
        case PART_TYPE_FAT32:  return "FAT32";
        case PART_TYPE_FAT32X: return "FAT32 LBA";
        case PART_TYPE_LINUX:  return "Linux";
        case PART_TYPE_SWAP:   return "Linux Swap";
        case PART_TYPE_SFS:    return "stingerFS";
        default:               return "Unknown";
    }
}

void partition_print_table(const part_entry_t *parts, int count){
    terminal_printf("  %-12s %s  %-10s  %-10s  %-8s  %s\n",
        "Device","Boot","Start","End","Size","Type");
    terminal_printf("  %-12s %s  %-10s  %-10s  %-8s  %s\n",
        "------------","----","----------","----------","--------","----------");
    for(int i=0;i<count;i++){
        uint32_t end=parts[i].lba_start+parts[i].size_lba-1;
        uint32_t mb =parts[i].size_lba/SECS_PER_MB;
        terminal_printf("  /dev/hd%c%d    %s   %-10u  %-10u  %4u MB   %s\n",
            'a'+parts[i].drive, parts[i].index,
            parts[i].bootable ? "*" : " ",
            parts[i].lba_start, end, mb,
            partition_type_name(parts[i].type));
    }
}




