#include "usb_desc.h"

const uint8_t usb_device_desc[] = {
    0x12,                // bLength
    USB_DESC_TYPE_DEVICE,// bDescriptorType
    0x00,
    0x02,// bcdUSB = USB 2.00
    0x00,// bDeviceClass (must be 0)
    0x00,// bDeviceSubClass
    0x00,// bDeviceProtocol
    0x40,// bMaxPacketSize0 = 64
    LOBYTE(USB_VID),
    HIBYTE(USB_VID),
    LOBYTE(USB_PID),
    HIBYTE(USB_PID),
    0x00,
    0x01,// bcdDevice
    0x01,// iManufacturer
    0x02,// iProduct
    0x03,// iSerialNumber
    0x01 // bNumConfigurations
};
const uint8_t usb_config_desc[] = {
    /* Configuration Descriptor */
    0x09,                                                      // bLength
    USB_DESC_TYPE_CONFIGURATION,                               // bDescriptorType
    LOBYTE(USB_CONFIG_DESC_SIZE), HIBYTE(USB_CONFIG_DESC_SIZE),// wTotalLength
    0x01,                                                      // bNumInterfaces
    0x01,                                                      // bConfigurationValue
    0x00,                                                      // iConfiguration
    0x80,                                                      // bmAttributes (Bus powered)
    0x32,                                                      // bMaxPower (100mA)

    /* Interface Descriptor */
    0x09,                   // bLength
    USB_DESC_TYPE_INTERFACE,// bDescriptorType
    0x00,                   // bInterfaceNumber
    0x00,                   // bAlternateSetting
    0x02,                   // bNumEndpoints = 2
    0xFF,                   // bInterfaceClass (Vendor Specific)
    0x00,                   // bInterfaceSubClass
    0x00,                   // bInterfaceProtocol
    0x00,                   // iInterface

    /* Endpoint OUT Descriptor */
    0x07,                  // bLength
    USB_DESC_TYPE_ENDPOINT,// bDescriptorType
    0x01,                  // bEndpointAddress = EP1 OUT
    0x02,                  // bmAttributes = Bulk
    0x40, 0x00,            // wMaxPacketSize = 64
    0x00,                  // bInterval (ignored)

    /* Endpoint IN Descriptor */
    0x07,                  // bLength
    USB_DESC_TYPE_ENDPOINT,// bDescriptorType
    0x81,                  // EP1 IN
    0x02,                  // Bulk
    0x40, 0x00,            // wMaxPacketSize = 64
    0x00                   // bInterval (ignored)
};
/* ========== String 0: Language ID ========== */
/* English (US) = 0x0409 */
const uint8_t usb_lang_id_desc[] = {0x04, USB_DESC_TYPE_STRING, 0x09, 0x04};

/* ========== String 1: Manufacturer ========== */
const uint8_t usb_manufacturer_desc[] = {2 + 2 * 6, USB_DESC_TYPE_STRING, 'O', 0, 'p', 0, 'e', 0, 'n', 0, 'A', 0, 'I',
                                         0};

/* ========== String 2: Product ========== */
const uint8_t usb_product_desc[] = {
    2 + 2 * 7, USB_DESC_TYPE_STRING, 'U', 0, 'S', 0, 'B', 0, '-', 0, 'C', 0, 'A', 0, 'N', 0};

/* ========== String 3: Serial Number ========== */
const uint8_t usb_serial_desc[] = {
    2 + 2 * 8, USB_DESC_TYPE_STRING, '0', 0, '0', 0, '0', 0, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0};

/* ========== Descriptor selector ========== */
const uint8_t *usb_get_string_desc(uint8_t index, uint16_t *len) {
    const uint8_t *desc = NULL;

    switch (index) {
        case 0: desc = usb_lang_id_desc; break;
        case 1: desc = usb_manufacturer_desc; break;
        case 2: desc = usb_product_desc; break;
        case 3: desc = usb_serial_desc; break;
        default: return NULL;
    }

    *len = desc[0];
    return desc;
}
