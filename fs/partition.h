




















typedef struct {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t size_lba;
} __attribute__((packed)) mbr_part_t;

typedef struct {
    uint8_t   bootstrap[446];
    mbr_part_t entries[4];
    uint16_t  signature;
} __attribute__((packed)) mbr_t;

typedef struct {
    int      drive;
    int      index;
    uint8_t  type;
    uint32_t lba_start;
    uint32_t size_lba;
    int      bootable;
    int      mounted;
    char     label[12];
} part_entry_t;

int         partition_detect_all(part_entry_t *out, int max);
int         partition_create(int drive, int index, uint8_t type,
                              uint32_t lba_start, uint32_t size_mb);
int         partition_delete(int drive, int index);
const char *partition_type_name(uint8_t type);
void        partition_print_table(const part_entry_t *parts, int count);






