#include "gs_usb.h"

#include "fdcan.h"
#include <string.h>

#include "usb_core.h"// 你自己的 EP0 API

/* ================= Device capability ================= */
/**
 * struct gs_device_config - Configuration describing the gs_usb compatible device
 * @icount: number of CAN interfaces - 1
 * @sw_version: software version - usually 2
 * @hw_version: hardware version - usually 1
 *
 */
static const struct gs_usb_device_config gs_dev_cfg = {
    .reserved1 = 0,
    .reserved2 = 0,
    .reserved3 = 0,
    .icount = NUM_CAN_CHANNELS - 1,
    .sw_version = 0x00010000, /* v1.0 */
    .hw_version = 0x00010000,
};

/* ================= Bit timing capability ================= */

static const struct gs_usb_bittiming_const gs_bt_const = {
    .feature = GS_CAN_FEATURE_LISTEN_ONLY | GS_CAN_FEATURE_LOOP_BACK | GS_CAN_FEATURE_TRIPLE_SAMPLE |
               GS_CAN_FEATURE_ONE_SHOT | GS_CAN_FEATURE_BERR_REPORTING | GS_CAN_FEATURE_FD |
               GS_CAN_FEATURE_BT_CONST_EXT,
    .fclk_can = 60000000,
    .tseg1_min = 1,
    .tseg1_max = 256,
    .tseg2_min = 1,
    .tseg2_max = 128,
    .sjw_max = 128,
    .brp_min = 1,
    .brp_max = 512,
    .brp_inc = 1,
};

static const struct gs_device_bt_const_extended gs_bt_const_ext = {
    .feature = GS_CAN_FEATURE_LISTEN_ONLY | GS_CAN_FEATURE_LOOP_BACK | GS_CAN_FEATURE_TRIPLE_SAMPLE |
               GS_CAN_FEATURE_ONE_SHOT | GS_CAN_FEATURE_BERR_REPORTING | GS_CAN_FEATURE_FD |
               GS_CAN_FEATURE_BT_CONST_EXT,
    .fclk_can = 60000000,
    .tseg1_min = 1,
    .tseg1_max = 256,
    .tseg2_min = 1,
    .tseg2_max = 128,
    .sjw_max = 128,
    .brp_min = 1,
    .brp_max = 512,
    .brp_inc = 1,
    .dtseg1_min = 1,
    .dtseg1_max = 32,
    .dtseg2_min = 1,
    .dtseg2_max = 16,
    .dsjw_max = 16,
    .dbrp_min = 1,
    .dbrp_max = 32,
    .dbrp_inc = 1,
};

/* EP0 temp buffer for vendor IN responses */
static uint8_t gs_ep0_buf[128];
static uint8_t gs_can_started[NUM_CAN_CHANNELS] = {0};
static uint8_t gs_fd_enabled[NUM_CAN_CHANNELS] = {0};

static FDCAN_HandleTypeDef *gs_usb_get_can(uint8_t channel) {
    if (channel == 0) {
        return &hfdcan1;
    }
#if NUM_CAN_CHANNELS > 1
    if (channel == 1) {
        return &hfdcan2;
    }
#endif
#if NUM_CAN_CHANNELS > 2
    if (channel == 2) {
        return &hfdcan3;
    }
#endif
    return NULL;
}

static int gs_usb_apply_bittiming(uint8_t channel,
                                  FDCAN_HandleTypeDef *hcan,
                                  const struct gs_device_bittiming *bt,
                                  uint8_t is_data) {
    if (hcan == NULL || bt == NULL) {
        return -1;
    }


    if (channel < NUM_CAN_CHANNELS && gs_can_started[channel]) {
        (void)HAL_FDCAN_Stop(hcan);
    }

    if (is_data) {
        hcan->Init.DataPrescaler = (uint32_t) bt->brp;
        hcan->Init.DataSyncJumpWidth = (uint32_t) bt->sjw;
        hcan->Init.DataTimeSeg1 = (uint32_t) (bt->prop_seg + bt->phase_seg1);
        hcan->Init.DataTimeSeg2 = (uint32_t) bt->phase_seg2;
    } else {
        hcan->Init.NominalPrescaler = (uint32_t) bt->brp;
        hcan->Init.NominalSyncJumpWidth = (uint32_t) bt->sjw;
        hcan->Init.NominalTimeSeg1 = (uint32_t) (bt->prop_seg + bt->phase_seg1);
        hcan->Init.NominalTimeSeg2 = (uint32_t) bt->phase_seg2;
    }

    if (HAL_FDCAN_Init(hcan) != HAL_OK) {
        return -1;
    }

    return 0;
}

static uint8_t gs_usb_dlc_to_len(uint32_t dlc) {
    switch (dlc) {
        case FDCAN_DLC_BYTES_0: return 0;
        case FDCAN_DLC_BYTES_1: return 1;
        case FDCAN_DLC_BYTES_2: return 2;
        case FDCAN_DLC_BYTES_3: return 3;
        case FDCAN_DLC_BYTES_4: return 4;
        case FDCAN_DLC_BYTES_5: return 5;
        case FDCAN_DLC_BYTES_6: return 6;
        case FDCAN_DLC_BYTES_7: return 7;
        case FDCAN_DLC_BYTES_8: return 8;
        case FDCAN_DLC_BYTES_12: return 12;
        case FDCAN_DLC_BYTES_16: return 16;
        case FDCAN_DLC_BYTES_20: return 20;
        case FDCAN_DLC_BYTES_24: return 24;
        case FDCAN_DLC_BYTES_32: return 32;
        case FDCAN_DLC_BYTES_48: return 48;
        case FDCAN_DLC_BYTES_64: return 64;
        default: return 8;
    }
}

static uint32_t gs_usb_len_to_dlc(uint8_t len) {
    switch (len) {
        case 0: return FDCAN_DLC_BYTES_0;
        case 1: return FDCAN_DLC_BYTES_1;
        case 2: return FDCAN_DLC_BYTES_2;
        case 3: return FDCAN_DLC_BYTES_3;
        case 4: return FDCAN_DLC_BYTES_4;
        case 5: return FDCAN_DLC_BYTES_5;
        case 6: return FDCAN_DLC_BYTES_6;
        case 7: return FDCAN_DLC_BYTES_7;
        case 8: return FDCAN_DLC_BYTES_8;
        case 12: return FDCAN_DLC_BYTES_12;
        case 16: return FDCAN_DLC_BYTES_16;
        case 20: return FDCAN_DLC_BYTES_20;
        case 24: return FDCAN_DLC_BYTES_24;
        case 32: return FDCAN_DLC_BYTES_32;
        case 48: return FDCAN_DLC_BYTES_48;
        case 64: return FDCAN_DLC_BYTES_64;
        default: return FDCAN_DLC_BYTES_8;
    }
}

static void gs_usb_ep0_send_padded(const usb_setup_pkt_t *req, const void *data, uint16_t data_len) {
    uint16_t len = req->wLength;
    if (len > sizeof(gs_ep0_buf)) {
        len = sizeof(gs_ep0_buf);
    }
    memset(gs_ep0_buf, 0, len);
    if (data != NULL && data_len > 0) {
        uint16_t copy_len = (data_len < len) ? data_len : len;
        memcpy(gs_ep0_buf, data, copy_len);
    }
    usb_ep0_send(gs_ep0_buf, len);
}

/* ================= Vendor request handler ================= */

int usb_handle_gs_usb_request(const usb_setup_pkt_t *req, const uint8_t *data, uint16_t len) {
    /* bmRequestType:
     *  IN  = 0xC0
     *  OUT = 0x40
     */
    switch (req->bRequest) {
        case GS_USB_BREQ_DEVICE_CONFIG:
            gs_usb_ep0_send_padded(req, &gs_dev_cfg, sizeof(gs_dev_cfg));
            return 0;

        case GS_USB_BREQ_BT_CONST:
            gs_usb_ep0_send_padded(req, &gs_bt_const, sizeof(gs_bt_const));
            return 0;

        case GS_USB_BREQ_BT_CONST_EXT:
            gs_usb_ep0_send_padded(req, &gs_bt_const_ext, sizeof(gs_bt_const_ext));
            return 0;

        case GS_USB_BREQ_GET_STATE: {
            struct gs_device_state st = {0};
            uint8_t channel = (uint8_t)(req->wIndex & 0xFF);
            if (channel >= NUM_CAN_CHANNELS) {
                return -1;
            }
            st.state = gs_can_started[channel] ? GS_CAN_STATE_ERROR_ACTIVE : GS_CAN_STATE_STOPPED;
            st.rxerr = 0;
            st.txerr = 0;
            gs_usb_ep0_send_padded(req, &st, sizeof(st));
            return 0;
        }

        case GS_USB_BREQ_GET_TERMINATION: {
            uint32_t term = GS_CAN_TERMINATION_STATE_OFF;
            gs_usb_ep0_send_padded(req, &term, sizeof(term));
            return 0;
        }

        case GS_USB_BREQ_GET_USER_ID: {
            uint32_t id = 0;
            gs_usb_ep0_send_padded(req, &id, sizeof(id));
            return 0;
        }

        case GS_USB_BREQ_TIMESTAMP: {
            static uint32_t ts = 0;
            gs_usb_ep0_send_padded(req, &ts, sizeof(ts));
            return 0;
        }

        case GS_USB_BREQ_IDENTIFY:
        case GS_USB_BREQ_BERR:
        case GS_USB_BREQ_SET_USER_ID:
        case GS_USB_BREQ_SET_TERMINATION:
            usb_ep0_ack();
            return 0;

        case GS_USB_BREQ_MODE:
        
            if (len >= 8 && data != NULL) {
                uint32_t mode = 0;
                uint32_t flags = 0;
                memcpy(&mode, data, sizeof(mode));
                memcpy(&flags, data + 4, sizeof(flags));

                uint8_t channel = (uint8_t)(req->wIndex & 0xFF);
                FDCAN_HandleTypeDef *hcan = gs_usb_get_can(channel);
                if (hcan == NULL || channel >= NUM_CAN_CHANNELS) {
                    return -1;
                }

                gs_fd_enabled[channel] = (flags & GS_CAN_MODE_FD) ? 1 : 0;
                
                if (mode == GS_CAN_MODE_START && !gs_can_started[channel]) {
                    FDCAN_FilterTypeDef filter = {0};
                    filter.FilterIndex = 0;
                    filter.FilterType = FDCAN_FILTER_MASK;
                    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
                    filter.FilterID1 = 0;
                    filter.FilterID2 = 0;
                    filter.IdType = FDCAN_STANDARD_ID;
                    (void)HAL_FDCAN_ConfigFilter(hcan, &filter);

                    filter.FilterIndex = 1;
                    filter.IdType = FDCAN_EXTENDED_ID;
                    (void)HAL_FDCAN_ConfigFilter(hcan, &filter);

                    if (gs_fd_enabled[channel]) {
                        hcan->Init.FrameFormat = FDCAN_FRAME_FD_NO_BRS;
                    } else {
                        hcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
                    }

                    (void)HAL_FDCAN_Init(hcan);
                    (void)HAL_FDCAN_Start(hcan);
                    (void)HAL_FDCAN_ActivateNotification(hcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
                    gs_can_started[channel] = 1;
                } else if (mode == GS_CAN_MODE_RESET && gs_can_started[channel]) {
                    (void)HAL_FDCAN_Stop(hcan);
                    gs_can_started[channel] = 0;
                }
            }

            return 0;

        case GS_USB_BREQ_BITTIMING:
            if (len >= sizeof(struct gs_device_bittiming) && data != NULL) {
                struct gs_device_bittiming bt;
                memcpy(&bt, data, sizeof(bt));
                uint8_t channel = (uint8_t) (req->wIndex & 0xFF);
                if (channel >= NUM_CAN_CHANNELS) {
                    return -1;
                }
                (void)gs_usb_apply_bittiming(channel, gs_usb_get_can(channel), &bt, 0);
            }
            return 0;

        case GS_USB_BREQ_DATA_BITTIMING:
            if (len >= sizeof(struct gs_device_bittiming) && data != NULL) {
                struct gs_device_bittiming bt;
                memcpy(&bt, data, sizeof(bt));
                uint8_t channel = (uint8_t) (req->wIndex & 0xFF);
                if (channel >= NUM_CAN_CHANNELS) {
                    return -1;
                }
                (void)gs_usb_apply_bittiming(channel, gs_usb_get_can(channel), &bt, 1);
            }
            return 0;

        case GS_USB_BREQ_HOST_FORMAT:
            return 0;

        default:
            return -1;
    }
}

void gs_usb_handle_bulk_out(uint16_t len) {
    if (len < (sizeof(struct gs_host_frame) - 64 + 8)) {
        return;
    }

    const struct gs_host_frame *frm = (const struct gs_host_frame *) ep1_rx_buf;
    FDCAN_HandleTypeDef *hcan = gs_usb_get_can(frm->channel);
    if (hcan == NULL) {
        return;
    }

    uint32_t can_id = frm->can_id;
    if (can_id & CAN_ERR_FLAG) {
        return;
    }

    FDCAN_TxHeaderTypeDef tx = {0};
    uint8_t is_fd = (frm->flags & GS_CAN_FLAG_FD) ? 1 : 0;
    uint8_t is_brs = (frm->flags & GS_CAN_FLAG_BRS) ? 1 : 0;
    tx.IdType = (can_id & CAN_EFF_FLAG) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    tx.Identifier = (can_id & CAN_EFF_FLAG) ? (can_id & 0x1FFFFFFFU) : (can_id & 0x7FFU);
    tx.TxFrameType = (can_id & CAN_RTR_FLAG) ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    tx.DataLength = gs_usb_len_to_dlc(frm->can_dlc);
    tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx.BitRateSwitch = is_brs ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    tx.FDFormat = is_fd ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
    tx.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx.MessageMarker = 0;

    uint8_t data_bytes[64] = {0};
    uint8_t payload_len = gs_usb_dlc_to_len(tx.DataLength);
    uint8_t copy_len = (payload_len > 64) ? 64 : payload_len;
    memcpy(data_bytes, frm->data, copy_len);

    if (HAL_FDCAN_AddMessageToTxFifoQ(hcan, &tx, data_bytes) == HAL_OK) {
        /* Echo back as TX complete */
        uint16_t out_len = 16U + payload_len;
        usb_ep1_send((const uint8_t *) frm, out_len);
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
        return;
    }

    FDCAN_RxHeaderTypeDef rx = {0};
    uint8_t data[64] = {0};
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx, data) != HAL_OK) {
        return;
    }

    struct gs_host_frame frm = {0};
    frm.echo_id = 0xFFFFFFFFU;
    frm.can_id = rx.Identifier;
    if (rx.IdType == FDCAN_EXTENDED_ID) {
        frm.can_id |= CAN_EFF_FLAG;
    }
    if (rx.RxFrameType == FDCAN_REMOTE_FRAME) {
        frm.can_id |= CAN_RTR_FLAG;
    }
    frm.can_dlc = gs_usb_dlc_to_len(rx.DataLength);
    frm.channel = (hfdcan == &hfdcan1) ? 0 : 1;
    frm.flags = 0;
    frm.reserved = 0;
    uint8_t payload_len = gs_usb_dlc_to_len(rx.DataLength);
    memcpy(frm.data, data, payload_len > 64 ? 64 : payload_len);

    frm.flags = 0;
    if (rx.FDFormat == FDCAN_FD_CAN) {
        frm.flags |= GS_CAN_FLAG_FD;
    }
    if (rx.BitRateSwitch == FDCAN_BRS_ON) {
        frm.flags |= GS_CAN_FLAG_BRS;
    }

    usb_ep1_send((const uint8_t *) &frm, 16U + payload_len);
}

const usb_app_ops_t gs_usb_ops = {
    .class_handler = usb_handle_gs_usb_request,
    .vendor_handler = usb_handle_gs_usb_request,
    .ep1_out = gs_usb_handle_bulk_out,
};

const usb_app_ops_t *usb_app_ops = &gs_usb_ops;
