#ifndef __GS_USB_H__
#define __GS_USB_H__
#include <stdint.h>

#include "usb_core.h"

#define GS_CAN_FEATURE_LISTEN_ONLY (1 << 0)
#define GS_CAN_FEATURE_LOOP_BACK (1 << 1)
#define GS_CAN_FEATURE_TRIPLE_SAMPLE (1 << 2)
#define GS_CAN_FEATURE_ONE_SHOT (1 << 3)
#define GS_CAN_FEATURE_FD (1 << 8)
#define GS_CAN_FEATURE_BT_CONST_EXT (1 << 10)
#define GS_CAN_FEATURE_BERR_REPORTING (1 << 12)
/* GS_USB vendor requests (match kernel/rust protocol order) */
enum {
    GS_USB_BREQ_HOST_FORMAT = 0,
    GS_USB_BREQ_BITTIMING,
    GS_USB_BREQ_MODE,
    GS_USB_BREQ_BERR,
    GS_USB_BREQ_BT_CONST,
    GS_USB_BREQ_DEVICE_CONFIG,
    GS_USB_BREQ_TIMESTAMP,
    GS_USB_BREQ_IDENTIFY,
    GS_USB_BREQ_GET_USER_ID,
    GS_USB_BREQ_QUIRK_CANTACT_PRO_DATA_BITTIMING = GS_USB_BREQ_GET_USER_ID,
    GS_USB_BREQ_SET_USER_ID,
    GS_USB_BREQ_DATA_BITTIMING,
    GS_USB_BREQ_BT_CONST_EXT,
    GS_USB_BREQ_SET_TERMINATION,
    GS_USB_BREQ_GET_TERMINATION,
    GS_USB_BREQ_GET_STATE,
};
/* ===== Device info ===== */
struct gs_usb_device_config {
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t icount; /* number of CAN interfaces */
    uint32_t sw_version;
    uint32_t hw_version;
} __attribute__((packed));

/* ===== Bit timing capability ===== */
struct gs_usb_bittiming_const {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;
} __attribute__((packed));

struct gs_device_bt_const_extended {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;

    uint32_t dtseg1_min;
    uint32_t dtseg1_max;
    uint32_t dtseg2_min;
    uint32_t dtseg2_max;
    uint32_t dsjw_max;
    uint32_t dbrp_min;
    uint32_t dbrp_max;
    uint32_t dbrp_inc;
} __attribute__((packed));

struct gs_device_bittiming {
    uint32_t prop_seg;
    uint32_t phase_seg1;
    uint32_t phase_seg2;
    uint32_t sjw;
    uint32_t brp;
} __attribute__((packed));

struct gs_device_state {
    uint32_t state;
    uint32_t rxerr;
    uint32_t txerr;
} __attribute__((packed));

#define GS_CAN_MODE_RESET 0
#define GS_CAN_MODE_START 1
#define GS_CAN_MODE_FD (1 << 8)

#define GS_CAN_STATE_ERROR_ACTIVE 0
#define GS_CAN_STATE_ERROR_WARNING 1
#define GS_CAN_STATE_ERROR_PASSIVE 2
#define GS_CAN_STATE_BUS_OFF 3
#define GS_CAN_STATE_STOPPED 4
#define GS_CAN_STATE_SLEEPING 5

#define GS_CAN_TERMINATION_STATE_OFF 0
#define GS_CAN_TERMINATION_STATE_ON 1

/* SocketCAN-style CAN flags */
#define CAN_EFF_FLAG 0x80000000U
#define CAN_RTR_FLAG 0x40000000U
#define CAN_ERR_FLAG 0x20000000U

#define GS_CAN_FLAG_FD (1 << 1)
#define GS_CAN_FLAG_BRS (1 << 2)
#define GS_CAN_FLAG_ESI (1 << 3)


#define NUM_CAN_CHANNELS 2
#if NUM_CAN_CHANNELS > 3
#error "NUM_CAN_CHANNELS max is 3 for gs_usb"
#endif

/* Bulk data frame (classic/FD) */
struct gs_host_frame {
    uint32_t echo_id;
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t channel;
    uint8_t flags;
    uint8_t reserved;
    uint8_t data[64];
} __attribute__((packed));

int usb_handle_gs_usb_request(const usb_setup_pkt_t *req, const uint8_t *data, uint16_t len);
void gs_usb_handle_bulk_out(uint16_t len);
extern const usb_app_ops_t gs_usb_ops;
#endif
