

























typedef struct {
    uint8_t  apic_id;
    uint8_t  is_bsp;
    uint8_t  online;
    uint8_t  pad;
    uint32_t core_id;
} cpu_t;

extern cpu_t g_cpus[SMP_MAX_CPUS];
extern int   g_cpu_count;


typedef void (*ap_work_fn_t)(void *);
extern volatile ap_work_fn_t  g_ap_work_fn;
extern void * volatile        g_ap_work_arg;
extern volatile int           g_ap_work_done;

void    lapic_init(void);
void    lapic_eoi(void);
uint8_t lapic_get_id(void);
void    lapic_send_ipi(uint8_t apic_id, uint32_t icr_lo);

int     smp_detect(void);
int     smp_boot_ap(uint8_t apic_id, uint32_t trampoline_page);
void    smp_boot_ap_start(uint8_t apic_id, uint32_t trampoline_page);
int     smp_boot_ap_finish(uint8_t apic_id);
void    smp_print_status(void);


int     smp_dispatch_ap(void (*fn)(void *), void *arg, int wait);


int     smp_is_busy(void);


typedef struct {
    void (*fn)(void *arg, int start, int end);
    void *arg;
    volatile int next_iteration;
    int total_iterations;
    volatile int remaining_cpus;
    int grain_size;
} parallel_task_t;

void smp_parallel_for(void (*fn)(void *, int, int), void *arg, int total_iterations);


void __attribute__((noreturn)) smp_ap_entry(void);






