#ifndef __USB_DESC_H__
#define __USB_DESC_H__

#include "usb_def.h"
#include <stddef.h>
#include <stdint.h>

#define USB_CONFIG_DESC_SIZE (9 + 9 + 7 + 7)
#define USB_DEVICE_DESC_SIZE  18

extern const uint8_t usb_device_desc[];
extern const uint8_t usb_config_desc[];

extern const uint8_t usb_lang_id_desc[];
extern const uint8_t usb_manufacturer_desc[];
extern const uint8_t usb_product_desc[];
extern const uint8_t usb_serial_desc[];

const uint8_t *usb_get_string_desc(uint8_t index, uint16_t *len);
#endif