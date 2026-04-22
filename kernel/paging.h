



























void paging_init(uint32_t mem_kb);
void paging_enable(void);
int  paging_map_mmio(uint32_t phys_addr, size_t size, uint8_t **virt_addr);
int  paging_map_region(uint32_t phys_addr, uint32_t virt_addr, size_t size, uint32_t flags);
int  paging_map_wc(uint32_t phys_addr, size_t size, uint8_t **virt_addr_out);

uint32_t paging_get_cr3(void);
void paging_set_cr3(uint32_t cr3);
void paging_flush_tlb(void);

uint32_t paging_get_phys(uint32_t virt_addr);






