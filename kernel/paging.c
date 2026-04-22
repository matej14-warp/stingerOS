






static uint32_t __attribute__((aligned(4096)))
    kernel_page_directory[PAGE_TABLE_ENTRIES];

static uint32_t __attribute__((aligned(4096)))
    kernel_page_tables[MAX_PAGE_TABLES][PAGE_TABLE_ENTRIES];

static int num_page_tables = 0;



static uint32_t mmio_vaddr_next = MMIO_VADDR_BASE;

static uint32_t get_or_alloc_page_table(uint32_t virt_addr)
{
    uint32_t pd_index = (virt_addr >> 22) & 0x3FF;

    if (kernel_page_directory[pd_index] & PTE_PRESENT)
        return kernel_page_directory[pd_index] & PAGE_MASK;

    if (num_page_tables >= MAX_PAGE_TABLES) {
        terminal_printf("[paging] ERROR: page table pool exhausted\n");
        return 0;
    }

    uint32_t pt_phys = (uint32_t)&kernel_page_tables[num_page_tables];
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
        kernel_page_tables[num_page_tables][i] = 0;
    kernel_page_directory[pd_index] = pt_phys | (PTE_PRESENT | PTE_WRITE);
    num_page_tables++;
    return pt_phys;
}

int paging_map_region(uint32_t phys_addr, uint32_t virt_addr,
                      size_t size, uint32_t flags)
{
    phys_addr &= PAGE_MASK;
    virt_addr &= PAGE_MASK;
    size = (size + PAGE_SIZE - 1) & PAGE_MASK;
    uint32_t pages = size / PAGE_SIZE;

    terminal_printf("[paging] map phys 0x%08x -> virt 0x%08x (%u pages flags=0x%x)\n",
                    phys_addr, virt_addr, pages, flags);

    for (uint32_t i = 0; i < pages; i++) {
        uint32_t cv = virt_addr + (i * PAGE_SIZE);
        uint32_t cp = phys_addr + (i * PAGE_SIZE);
        uint32_t pt_phys = get_or_alloc_page_table(cv);
        if (!pt_phys) return -1;
        uint32_t *pt    = (uint32_t *)pt_phys;
        uint32_t pt_idx = (cv >> 12) & 0x3FF;
        pt[pt_idx] = cp | flags;
    }

    paging_flush_tlb();
    return 0;
}

int paging_map_mmio(uint32_t phys_addr, size_t size, uint8_t **virt_addr_out)
{
    if (!virt_addr_out) return -1;

    uint32_t sz = (uint32_t)((size + PAGE_SIZE - 1) & PAGE_MASK);
    uint32_t virt = mmio_vaddr_next;
    mmio_vaddr_next += sz;

    if (mmio_vaddr_next > MMIO_VADDR_LIMIT) {
        terminal_printf("[paging] ERROR: MMIO VA pool exhausted\n");
        return -1;
    }

    if (paging_map_region(phys_addr, virt, size, MMIO_PTE_FLAGS) != 0)
        return -1;

    *virt_addr_out = (uint8_t *)virt;
    terminal_printf("[paging] MMIO phys=0x%08x -> virt=0x%08x size=0x%x\n",
                    phys_addr, virt, (uint32_t)size);
    return 0;
}

static void pat_init_wc(void)
{
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x277));

    hi = (hi & 0xFFFFFF00u) | 0x01u;
    __asm__ volatile("wrmsr" :: "c"(0x277), "a"(lo), "d"(hi));
}

int paging_map_wc(uint32_t phys_addr, size_t size, uint8_t **virt_addr_out)
{
    if (!virt_addr_out) return -1;
    pat_init_wc();

    uint32_t sz = (uint32_t)((size + PAGE_SIZE - 1) & PAGE_MASK);
    uint32_t virt = mmio_vaddr_next;
    mmio_vaddr_next += sz;

    if (mmio_vaddr_next > MMIO_VADDR_LIMIT) {
        terminal_printf("[paging] ERROR: MMIO VA pool exhausted\n");
        return -1;
    }

    if (paging_map_region(phys_addr, virt, size, WC_PTE_FLAGS) != 0)
        return -1;

    *virt_addr_out = (uint8_t *)virt;
    terminal_printf("[paging] WC phys=0x%08x -> virt=0x%08x size=0x%x\n",
                    phys_addr, virt, (uint32_t)size);
    return 0;
}

void paging_init(uint32_t mem_kb)
{
    (void)mem_kb;

    terminal_printf("[paging] mapping full 4 GB identity...\n");

    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
        kernel_page_directory[i] = 0;

    for (int pt_idx = 0; pt_idx < MAX_PAGE_TABLES; pt_idx++) {
        uint32_t *pt = kernel_page_tables[pt_idx];

        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            uint32_t phys = ((uint32_t)pt_idx << 22) | ((uint32_t)i << 12);
            pt[i] = phys | (PTE_PRESENT | PTE_WRITE);
        }

        uint32_t pt_phys = (uint32_t)pt;
        kernel_page_directory[pt_idx] = pt_phys | (PTE_PRESENT | PTE_WRITE);
    }

    num_page_tables = MAX_PAGE_TABLES;

    terminal_printf("[paging] 4 GB identity-mapped (%d page tables)\n",
                    MAX_PAGE_TABLES);
}

void paging_enable(void)
{
    uint32_t pd_addr = (uint32_t)&kernel_page_directory;

    __asm__ __volatile__(
        "movl  %%cr4, %%ecx\n\t"
        "andl  $~(1<<7), %%ecx\n\t"
        "movl  %%ecx, %%cr4\n\t"
        "movl  %0, %%cr3\n\t"
        "movl  %%cr0, %%eax\n\t"
        "orl   $0x80010001, %%eax\n\t"
        "movl  %%eax, %%cr0\n\t"
        "xorl  %%eax, %%eax\n\t"
        "cpuid\n\t"
        :
        : "r"(pd_addr)
        : "%eax", "%ebx", "%ecx", "%edx", "memory"
    );
}

uint32_t paging_get_cr3(void)
{
    uint32_t cr3;
    __asm__ volatile("movl %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void paging_set_cr3(uint32_t cr3)
{
    __asm__ volatile("movl %0, %%cr3" : : "r"(cr3) : "memory");
}

void paging_flush_tlb(void)
{
    uint32_t cr0;
    __asm__ volatile("movl %%cr0, %0" : "=r"(cr0));
    if (cr0 & 0x80000000u) {
        uint32_t cr4;
        __asm__ volatile("movl %%cr4, %0" : "=r"(cr4));
        if (cr4 & (1u << 7)) {
            __asm__ volatile(
                "movl %%cr4, %%eax\n\t"
                "andl $~(1<<7), %%eax\n\t"
                "movl %%eax, %%cr4\n\t"
                "orl  $(1<<7), %%eax\n\t"
                "movl %%eax, %%cr4\n\t"
                ::: "%eax", "memory"
            );
        } else {
            uint32_t cr3 = paging_get_cr3();
            paging_set_cr3(cr3);
        }
    }
}

uint32_t paging_get_phys(uint32_t virt) {
    uint32_t pd_idx = (virt >> 22) & 0x3FF;
    if (!(kernel_page_directory[pd_idx] & PTE_PRESENT)) return 0;

    uint32_t *pt = (uint32_t *)(kernel_page_directory[pd_idx] & PAGE_MASK);
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;

    return (pt[pt_idx] & PAGE_MASK) | (virt & 0xFFF);
}




