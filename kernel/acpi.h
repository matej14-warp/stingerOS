/* scorpion OS - kernel/acpi.h */
#ifndef ACPI_H
#define ACPI_H
#include <stdint.h>

int  acpi_init(void);
void acpi_enable(void);
void acpi_poweroff(void);
void acpi_reboot(void);

/* S5 sleep state package — filled by acpi_init */
extern uint16_t g_acpi_pm1a_cnt;
extern uint16_t g_acpi_slp_typa;
extern uint16_t g_acpi_slp_typb;
extern int      g_acpi_ready;

#endif /* ACPI_H */
