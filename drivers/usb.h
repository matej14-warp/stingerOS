/* scorpion OS - drivers/usb.h */
#ifndef USB_H
#define USB_H
#include <stdint.h>

int  usb_init(void);
void usb_scan_devices(void);
int  usb_get_device_count(void);
void usb_list_devices(void);

#endif /* USB_H */
