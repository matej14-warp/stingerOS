/* scorpion OS - drivers/ax201.h
   Intel Wi-Fi 6 AX201 (CNVi / PCIe) driver header.
   The AX201 is a CNVi device: it shows up on PCIe as vendor 0x8086,
   device 0x02F0 (Ice Lake) or 0x43F0 (Tiger Lake) and a handful of
   other device IDs.  Communication goes through the IWL transport layer
   which programs the device via MMIO CSR registers and a shared
   TX/RX DMA ring set, then uploads firmware from the SFS.            */

#ifndef AX201_H
#define AX201_H
#include <stdint.h>

/* PCI IDs we recognise */
#define AX201_VENDOR       0x8086
/* common AX201 device IDs */
#define AX201_DEV_ICL      0x02F0   /* Ice Lake */
#define AX201_DEV_TGL      0x43F0   /* Tiger Lake */
#define AX201_DEV_ADL      0x51F0   /* Alder Lake */
#define AX201_DEV_RPL      0x7AF0   /* Raptor Lake */
#define AX201_DEV_CNL      0x9DF0   /* Cannon Lake */
#define AX201_DEV_CML      0xA0F0   /* Comet Lake */

/* CSR register offsets */
#define IWL_CSR_BASE        0x000
#define IWL_CSR_HW_IF_CONFIG 0x000
#define IWL_CSR_INT_COALESCING 0x004
#define IWL_CSR_INT         0x008
#define IWL_CSR_INT_MASK    0x00C
#define IWL_CSR_FH_INT      0x010
#define IWL_CSR_RESET       0x020
#define IWL_CSR_GP_CNTRL    0x024
#define IWL_CSR_HW_REV      0x028
#define IWL_CSR_GIO_CHICKEN_BITS 0x100

/* Reset / init bits */
#define IWL_CSR_RESET_SW        (1<<7)
#define IWL_CSR_RESET_MASTER_DIS (1<<8)
#define IWL_CSR_GP_CNTRL_RFKILL  (1<<27)
#define IWL_CSR_GP_CNTRL_INIT_DONE (1<<0)

/* Firmware file path on SFS (place iwlwifi-ax201-*.ucode here) */
#define AX201_FW_PATH   "/firmware/iwlwifi-ax201.ucode"
#define AX201_FW_MAGIC  0x5A4D3C2B

/* Public API */
int  ax201_probe(void);          /* returns 0 if device found & init OK */
int  ax201_scan(void);           /* trigger a scan; fills wifi_aps[] */
int  ax201_connect(const char *ssid, const char *pass);
int  ax201_disconnect(void);
int  ax201_status(char *buf, int buflen);
void ax201_irq_handler(void);    /* called from PCI MSI/INTx handler  */

#endif /* AX201_H */
