#include "usb_core.h"

#include "usb_desc.h"

volatile ep0_state_t ep0_state = EP0_IDLE;
static uint8_t ep0_pending_address = 0;
static uint8_t usb_configuration = 0;
volatile usb_setup_pkt_t ep0_last_setup;
volatile uint16_t ep0_out_len = 0;

/* EP0 buffers */

volatile uint8_t *ep0_tx_ptr;
volatile uint16_t ep0_tx_len;
volatile uint8_t ep0_rx_buf[USB_EP0_BUF_SIZE]={0};

/* EP1 buffers */
volatile uint8_t ep1_tx_buf[USB_EP1_BUF_SIZE] = {0};
volatile uint8_t ep1_rx_buf[USB_EP1_BUF_SIZE] = {0};
static volatile uint8_t ep1_in_busy = 0;
static uint8_t ep1_pending_buf[USB_EP1_BUF_SIZE];
static volatile uint16_t ep1_pending_len = 0;
__attribute__((weak)) const usb_app_ops_t *usb_app_ops = NULL;

/* ---------- EP0 SETUP entry ---------- */
void usb_ep0_setup(const usb_setup_pkt_t *req) {
    ep0_last_setup = *req;
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    switch (req->bmRequestType & 0x60) {
        case USB_REQ_TYPE_STANDARD:
            usb_ep0_handle_standard(req);
            return;

        case USB_REQ_TYPE_CLASS:
        case USB_REQ_TYPE_VENDOR:
            /* Handle OUT (class/vendor) requests with data stage */
            if ((req->bmRequestType & 0x80) == 0x00 && req->wLength > 0) {
                ep0_state = EP0_DATA_OUT;
                ep0_out_len = (req->wLength > USB_EP0_BUF_SIZE) ? USB_EP0_BUF_SIZE : req->wLength;
                HAL_PCD_EP_Receive(&hpcd_USB_DRD_FS, 0x00, (uint8_t *) ep0_rx_buf, ep0_out_len);
                return;
            }
            if ((req->bmRequestType & 0x60) == USB_REQ_TYPE_CLASS) {
                if (usb_app_ops && usb_app_ops->class_handler &&
                    usb_app_ops->class_handler(req, NULL, 0) == 0) {
                    return;
                }
            } else {
                if (usb_app_ops && usb_app_ops->vendor_handler &&
                    usb_app_ops->vendor_handler(req, NULL, 0) == 0) {
                    return;
                }
            }

            /* Unknown class/vendor request */
            usb_ep0_stall();
            return;

        default:
            usb_ep0_stall();
            return;
    }
}

void usb_ep0_send(uint8_t *buf, uint16_t len) {
    if (len == 0) {
        usb_ep0_ack();
        return;
    }

    ep0_state = EP0_DATA_IN;
    ep0_tx_ptr = buf;
    ep0_tx_len = len;

    uint16_t pkt = (len > USB_EP0_BUF_SIZE) ? USB_EP0_BUF_SIZE : len;
    HAL_PCD_EP_Transmit(&hpcd_USB_DRD_FS, 0x80, ep0_tx_ptr, pkt);
    ep0_tx_ptr += pkt;
    ep0_tx_len -= pkt;
}

/* ---------- Standard Requests ---------- */
void usb_ep0_handle_standard(const usb_setup_pkt_t *req) {
    uint8_t recipient = req->bmRequestType & 0x1F;
    switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR: {
            uint8_t desc_type = req->wValue >> 8;
            uint8_t desc_idx = req->wValue & 0xFF;
            const uint8_t *desc = NULL;
            uint16_t len = 0;

            switch (desc_type) {
                case USB_DESC_DEVICE:
                    desc = usb_device_desc;
                    len = USB_DEVICE_DESC_SIZE;
                    break;

                case USB_DESC_CONFIGURATION:
                    desc = usb_config_desc;
                    len = USB_CONFIG_DESC_SIZE;
                    break;

                case USB_DESC_STRING:
                    desc = usb_get_string_desc(desc_idx, &len);
                    break;

                default:
                    usb_ep0_stall();
                    return;
            }

            if (desc == NULL) {
                usb_ep0_stall();
                return;
            }

            if (req->wLength < len) {
                len = req->wLength;
            }

            usb_ep0_send((uint8_t *) desc, len);
            return;
        }

        case USB_REQ_SET_ADDRESS:
            ep0_pending_address = (uint8_t) (req->wValue & 0x7F);
            usb_ep0_ack();
            return;

        case USB_REQ_GET_STATUS: {
            uint16_t status = 0x0000;
            /* Device / Interface / Endpoint: return 0 (no wakeup, not halted) */
            usb_ep0_send((uint8_t *) &status, 2);
            return;
        }

        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE: {
            /* ENDPOINT_HALT for endpoint; DEVICE_REMOTE_WAKEUP for device */
            if (recipient == 0x02) {
                if (req->wValue != 0x0000) {
                    usb_ep0_stall();
                    return;
                }
                uint8_t ep_addr = (uint8_t) (req->wIndex & 0xFF);
                if (req->bRequest == USB_REQ_SET_FEATURE) {
                    HAL_PCD_EP_SetStall(&hpcd_USB_DRD_FS, ep_addr);
                } else {
                    HAL_PCD_EP_ClrStall(&hpcd_USB_DRD_FS, ep_addr);
                }
                usb_ep0_ack();
                return;
            }
            if (recipient == 0x00) {
                /* Accept remote wakeup feature without changing state */
                if (req->wValue == 0x0001) {
                    usb_ep0_ack();
                    return;
                }
            }
            usb_ep0_stall();
            return;
        }



        case USB_REQ_SET_CONFIGURATION: {
            uint8_t cfg = req->wValue & 0xFF;

            if (cfg == 1) {
                HAL_PCD_EP_Open(&hpcd_USB_DRD_FS, 0x01, 64, USB_EP_TYPE_BULK);
                HAL_PCD_EP_Open(&hpcd_USB_DRD_FS, 0x81, 64, USB_EP_TYPE_BULK);

                /* Prime EP1 OUT to receive data */
                HAL_PCD_EP_Receive(&hpcd_USB_DRD_FS, 0x01, (uint8_t *) ep1_rx_buf, USB_EP1_BUF_SIZE);
            } else if (cfg == 0) {
                HAL_PCD_EP_Close(&hpcd_USB_DRD_FS, 0x01);
                HAL_PCD_EP_Close(&hpcd_USB_DRD_FS, 0x81);
            } else {
                usb_ep0_stall();
                return;
            }

            usb_configuration = cfg;
            usb_ep0_ack();
            return;
        }

        case USB_REQ_GET_CONFIGURATION:
            usb_ep0_send(&usb_configuration, 1);
            return;

        default:
            usb_ep0_stall();
            return;
    }
}

int usb_ep1_send(const uint8_t *buf, uint16_t len) {
    if (len > USB_EP1_BUF_SIZE) {
        len = USB_EP1_BUF_SIZE;
    }
    if (ep1_in_busy) {
        memcpy(ep1_pending_buf, buf, len);
        ep1_pending_len = len;
        return 1;
    }
    memcpy((uint8_t *) ep1_tx_buf, buf, len);
    ep1_in_busy = 1;
    HAL_PCD_EP_Transmit(&hpcd_USB_DRD_FS, 0x81, (uint8_t *) ep1_tx_buf, len);
    return 0;
}

void usb_ep1_tx_complete(void) {
    if (ep1_pending_len > 0) {
        uint16_t len = ep1_pending_len;
        memcpy((uint8_t *) ep1_tx_buf, ep1_pending_buf, len);
        ep1_pending_len = 0;
        ep1_in_busy = 1;
        HAL_PCD_EP_Transmit(&hpcd_USB_DRD_FS, 0x81, (uint8_t *) ep1_tx_buf, len);
        return;
    }
    ep1_in_busy = 0;
}

void usb_ep0_handle_out_data(uint16_t len) {
    uint8_t type = ep0_last_setup.bmRequestType & 0x60;
    if (type == USB_REQ_TYPE_CLASS) {
        if (usb_app_ops && usb_app_ops->class_handler) {
            (void)usb_app_ops->class_handler((const usb_setup_pkt_t *)&ep0_last_setup,
                                             (const uint8_t *) ep0_rx_buf,
                                             len);
        }
    } else if (type == USB_REQ_TYPE_VENDOR) {
        if (usb_app_ops && usb_app_ops->vendor_handler) {
            (void)usb_app_ops->vendor_handler((const usb_setup_pkt_t *)&ep0_last_setup,
                                              (const uint8_t *) ep0_rx_buf,
                                              len);
        }
    }
}

void usb_ep0_ack(void) {
    /* Send zero-length packet on EP0 IN (Status stage) */
    ep0_state = EP0_STATUS;
    HAL_PCD_EP_Transmit(&hpcd_USB_DRD_FS, 0x80, NULL, 0);
}

/* ---------- STALL ---------- */
void usb_ep0_stall(void) {
    HAL_PCD_EP_SetStall(&hpcd_USB_DRD_FS, 0x00);
    HAL_PCD_EP_SetStall(&hpcd_USB_DRD_FS, 0x80);
}

void usb_ep0_apply_pending_address(void) {
    if (ep0_pending_address != 0) {
        HAL_PCD_SetAddress(&hpcd_USB_DRD_FS, ep0_pending_address);
        ep0_pending_address = 0;
    }
}

void usb_core_reset_state(void) {
    ep0_pending_address = 0;
    usb_configuration = 0;
}
