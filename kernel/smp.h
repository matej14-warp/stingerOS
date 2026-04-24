/* scorpion OS - kernel/smp.h */
#ifndef SMP_H
#define SMP_H
#include <stdint.h>

#define SMP_MAX_CPUS  8
#define LAPIC_BASE    0xFEE00000u

/* LAPIC registers (byte offsets) */
#define LAPIC_ID      0x020
#define LAPIC_VER     0x030
#define LAPIC_TPR     0x080
#define LAPIC_EOI     0x0B0
#define LAPIC_SVR     0x0F0
#define LAPIC_ICR_LO  0x300
#define LAPIC_ICR_HI  0x310
#define LAPIC_LVT_TMR 0x320
#define LAPIC_TMRINITCNT 0x380
#define LAPIC_TMRCURCNT  0x390
#define LAPIC_TMRDIV     0x3E0

/* IPI delivery modes */
#define IPI_FIXED  0x00000000u
#define IPI_INIT   0x00000500u
#define IPI_SIPI   0x00000600u

typedef struct {
    uint8_t  apic_id;
    uint8_t  is_bsp;
    uint8_t  online;
    uint8_t  pad;
    uint32_t core_id;
} cpu_t;

extern cpu_t g_cpus[SMP_MAX_CPUS];
extern int   g_cpu_count;

/* AP work queue — function pointer itself is volatile (the pointer can change),
   not the return type (which caused the -Wignored-qualifiers warning).           */
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
void    smp_print_status(void);

/* Dispatch fn(arg) to first online AP; wait=1 blocks until done.
   Returns 0 on success, -1 if no AP available.                   */
int     smp_dispatch_ap(void (*fn)(void *), void *arg, int wait);

/* Returns 1 if an AP is currently busy with work, 0 if idle.     */
int     smp_is_busy(void);

/* AP entry point — called from trampoline, never returns */
void __attribute__((noreturn)) smp_ap_entry(void);

#endif /* SMP_H */
