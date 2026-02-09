#include "usb_core.h"

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd == &hpcd_USB_DRD_FS) {
        usb_setup_pkt_t setup;
        memcpy(&setup, hpcd->Setup, sizeof(setup));
        /* Reset EP0 state on each SETUP */
        ep0_tx_len = 0;
        ep0_tx_ptr = NULL;
        ep0_state = EP0_IDLE;
        usb_ep0_setup(&setup);
    }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    if (epnum == 1) {
        usb_ep1_tx_complete();
        return;
    }
    if (epnum != 0) {
        return;
    }

    if (ep0_tx_len > 0) {
        uint16_t pkt = (ep0_tx_len > USB_EP0_BUF_SIZE) ? USB_EP0_BUF_SIZE : ep0_tx_len;
        HAL_PCD_EP_Transmit(hpcd, 0x80, (uint8_t *) ep0_tx_ptr, pkt);
        ep0_tx_ptr += pkt;
        ep0_tx_len -= pkt;
        if (ep0_tx_len == 0) {
            /* Prepare for status OUT stage */
            HAL_PCD_EP_Receive(hpcd, 0x00, (uint8_t *) ep0_rx_buf, 0);
            ep0_state = EP0_STATUS;
        }
        return;
    }

    if (ep0_state == EP0_DATA_IN) {
        /* Data stage completed (single packet), prepare for status OUT stage */
        HAL_PCD_EP_Receive(hpcd, 0x00, (uint8_t *) ep0_rx_buf, 0);
        ep0_state = EP0_STATUS;
        return;
    }

    /* Status stage completed (IN ZLP) */
    if (ep0_state == EP0_STATUS) {
        usb_ep0_apply_pending_address();
        ep0_state = EP0_IDLE;
    }
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    if (hpcd != &hpcd_USB_DRD_FS) {
        return;
    }

    if (epnum == 0) {
        if (ep0_state == EP0_DATA_OUT) {
            /* Data OUT stage done, send status IN */
            uint16_t rx = HAL_PCD_EP_GetRxCount(hpcd, 0x00);
            usb_ep0_handle_out_data(rx);
            usb_ep0_ack();

            return;
        }
        /* Status OUT stage done */

        ep0_state = EP0_IDLE;
        return;
    }

    if (epnum == 1) {
        /* EP1 OUT: re-arm for next packet */
        uint16_t rx = HAL_PCD_EP_GetRxCount(hpcd, 0x01);
        if (usb_app_ops && usb_app_ops->ep1_out) {
            usb_app_ops->ep1_out(rx);
        }
        HAL_PCD_EP_Receive(hpcd, 0x01, (uint8_t *) ep1_rx_buf, USB_EP1_BUF_SIZE);
    }
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd != &hpcd_USB_DRD_FS) {
        return;
    }

    /* Re-init EP0 on bus reset */
    HAL_PCD_EP_Open(hpcd, 0x00, USB_EP0_BUF_SIZE, EP_TYPE_CTRL);
    HAL_PCD_EP_Open(hpcd, 0x80, USB_EP0_BUF_SIZE, EP_TYPE_CTRL);
    HAL_PCD_EP_Receive(hpcd, 0x00, (uint8_t *) ep0_rx_buf, USB_EP0_BUF_SIZE);

    ep0_tx_len = 0;
    ep0_tx_ptr = NULL;
    ep0_state = EP0_IDLE;
    usb_core_reset_state();
}
