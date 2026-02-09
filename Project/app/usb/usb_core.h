#ifndef __USB_CORE_H__
#define __USB_CORE_H__
#include <stdint.h>
#include <string.h>

#include "stm32g0xx_hal.h"

/* USB request types */
#define USB_REQ_TYPE_STANDARD 0x00
#define USB_REQ_TYPE_CLASS 0x20
#define USB_REQ_TYPE_VENDOR 0x40

/* USB standard requests */
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_SET_CONFIG 0x09

/* Descriptor types */
#define USB_DESC_DEVICE 0x01
#define USB_DESC_CONFIGURATION 0x02
#define USB_DESC_STRING 0x03

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_pkt_t;

typedef int (*usb_class_handler_t)(const usb_setup_pkt_t *req, const uint8_t *data, uint16_t len);
typedef int (*usb_vendor_handler_t)(const usb_setup_pkt_t *req, const uint8_t *data, uint16_t len);
typedef void (*usb_ep1_out_handler_t)(uint16_t rx_len);

typedef struct {
    usb_class_handler_t class_handler;
    usb_vendor_handler_t vendor_handler;
    usb_ep1_out_handler_t ep1_out;
} usb_app_ops_t;

extern const usb_app_ops_t *usb_app_ops;

/* EP0 state */
typedef enum { EP0_IDLE = 0, EP0_DATA_IN, EP0_DATA_OUT, EP0_STATUS } ep0_state_t;

/* Buffer definitions */
#define USB_EP0_BUF_SIZE 64
#define USB_EP1_BUF_SIZE 128

/* EP0 buffers */

extern volatile uint8_t *ep0_tx_ptr;
extern volatile uint16_t ep0_tx_len;
extern volatile uint8_t ep0_rx_buf[USB_EP0_BUF_SIZE];
extern volatile usb_setup_pkt_t ep0_last_setup;
extern volatile uint16_t ep0_out_len;

/* EP1 buffers */
extern volatile uint8_t ep1_tx_buf[USB_EP1_BUF_SIZE];
extern volatile uint8_t ep1_rx_buf[USB_EP1_BUF_SIZE];

/* EP0 state */
extern volatile ep0_state_t ep0_state;
extern PCD_HandleTypeDef hpcd_USB_DRD_FS;
void usb_ep0_setup(const usb_setup_pkt_t *req);
void usb_ep0_handle_standard(const usb_setup_pkt_t *req);
void usb_ep0_send(uint8_t *buf, uint16_t len);
void usb_ep0_ack(void);
void usb_ep0_handle_out_data(uint16_t len);


int usb_ep1_send(const uint8_t *buf, uint16_t len);
void usb_ep1_tx_complete(void);

void usb_ep0_stall(void);
void usb_ep0_apply_pending_address(void);
void usb_core_reset_state(void);
#endif
