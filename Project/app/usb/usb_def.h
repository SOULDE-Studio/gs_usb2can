#ifndef __USB_DEF_H__
#define __USB_DEF_H__

#ifndef LOBYTE
#define LOBYTE(x) ((uint8_t)((x) & 0xFF))
#endif

#ifndef HIBYTE
#define HIBYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#endif
/* ================= USB Descriptor Types ================= */

#define USB_DESC_TYPE_DEVICE 0x01
#define USB_DESC_TYPE_CONFIGURATION 0x02
#define USB_DESC_TYPE_STRING 0x03
#define USB_DESC_TYPE_INTERFACE 0x04
#define USB_DESC_TYPE_ENDPOINT 0x05

/* ================= USB Standard Requests ================= */

#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_DESCRIPTOR 0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09

/* ================= USB Request Type ================= */

#define USB_REQ_TYPE_STANDARD 0x00
#define USB_REQ_TYPE_CLASS 0x20
#define USB_REQ_TYPE_VENDOR 0x40

/* ================= USB Endpoint Type ================= */

#define USB_EP_TYPE_CONTROL 0x00
#define USB_EP_TYPE_ISO 0x01
#define USB_EP_TYPE_BULK 0x02
#define USB_EP_TYPE_INTERRUPT 0x03

/* ================= USB Directions ================= */

#define USB_DIR_OUT 0x00
#define USB_DIR_IN 0x80

/* ================= USB Descriptor Lengths ================= */

#define USB_LEN_DEV_DESC 18
#define USB_LEN_CFG_DESC 9
#define USB_LEN_IF_DESC 9
#define USB_LEN_EP_DESC 7

/* ================= Vendor ID / Product ID ================= */

/*
 * ⚠️ 注意：
 *  - 测试阶段可用开源 VID/PID（如 CandleLight）
 *  - 正式产品必须换成你自己的 VID
 */

#define USB_VID 0x1D50 /* OpenMoko */
#define USB_PID 0x606F /* CandleLight GS_USB */
#endif
