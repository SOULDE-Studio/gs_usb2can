/**
  **************************************************************************
  * @file     download_protocol.h
  * @brief    Download protocol definitions for 8-byte fixed packet
  *           Compatible with CAN bus, inspired by YMODEM protocol
  **************************************************************************
  */

#ifndef __DOWNLOAD_PROTOCOL_H
#define __DOWNLOAD_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* ========================================
   Protocol frame format (8 bytes fixed)
   ========================================
   
   Byte 0: Command type (CMD)
   Byte 1: Sequence number low byte (SEQ_L)
   Byte 2: Sequence number high byte (SEQ_H)
   Byte 3: Data length (0-5 bytes, 0-5)
   Byte 4-7: Data payload (up to 5 bytes)
   
   Total: 8 bytes (CAN compatible)
   ======================================== */

/* ========================================
   Protocol command types
   ======================================== */
typedef enum {
    CMD_START = 0x01, /* Start transfer: contains file size and CRC32 */
    CMD_DATA = 0x02,  /* Data packet */
    CMD_END = 0x03,   /* End of transfer */
    CMD_INFO = 0x04,  /* Request bootloader info */
    CMD_ACK = 0x06,   /* Acknowledge (ACK) */
    CMD_NAK = 0x15,   /* Negative acknowledge (NAK) */
    CMD_CANCEL = 0x18 /* Cancel transfer */
} ProtocolCmd_t;
typedef enum Status {
    STATUS_OK = 0,
    STATUS_ERROR = 1,
    STATUS_INVALID = 2,
    STATUS_BUSY = 3,
    STATUS_VERIFY_FAILED = 4,
    STATUS_COMPLETE = 5,
} ProtocolStatus_t;
/* ========================================
   Protocol frame structure
   ======================================== */
typedef struct {
    uint8_t cmd;      /* Command type */
    uint16_t seq;     /* Sequence number (0-65535) */
    uint8_t data_len; /* Data length (0-4) */
    uint8_t data[4];  /* Data payload (up to 4 bytes) */
} ProtocolFrame_t;

/* ========================================
   Protocol state machine
   ======================================== */
typedef enum {
    PROTOCOL_STATE_IDLE,      /* Waiting for START command */
    PROTOCOL_STATE_RECEIVING, /* Receiving data packets */
    PROTOCOL_STATE_COMPLETE,  /* Transfer complete */
    PROTOCOL_STATE_ERROR      /* Error occurred */
} ProtocolState_t;
/* ========================================
   Protocol info types
   ======================================== */
typedef enum {
    INFO_TYPE_VERSION = 0x01, /* Bootloader version */
    INFO_TYPE_SERIES = 0x02,  /* Bootloader series */
    INFO_TYPE_SPEC = 0x03,    /* Bootloader specific */
} ProtocolInfoType_t;
/* ========================================
   Protocol configuration
   ======================================== */
#define PROTOCOL_FRAME_SIZE (8U)         /* Fixed 8-byte frame */
#define PROTOCOL_MAX_DATA_PER_FRAME (4U) /* Maximum data bytes per frame */
#define PROTOCOL_MAX_SEQ (65535U)        /* Maximum sequence number */
#define PROTOCOL_TIMEOUT_MS (5000U)      /* Frame timeout (5 seconds) */
#define PROTOCOL_MAX_RETRIES (3U)        /* Maximum retry attempts */

typedef struct ProtocolController {
    ProtocolState_t protocol_state;
    uint16_t expected_seq;    /* Expected next sequence number */
    uint32_t firmware_size;   /* Firmware size from START command */
    uint32_t firmware_crc;    /* Firmware CRC32 from START command */
    uint32_t received_bytes;  /* Total bytes received */
    uint32_t last_frame_time; /* Timestamp of last received frame */
    uint16_t retry_count;     /* Retry counter */
    bool start_size_received; /* Flag to track START size reception */
    uint32_t system_time_ms;  /* System time counter  */
} ProtocolController_t;
extern ProtocolController_t controller;
/* ========================================
   Protocol function prototypes
   ======================================== */

/**
  * @brief  Initialize protocol handler
  * @retval download_status_t: DOWNLOAD_OK on success
  */
ProtocolStatus_t protocol_init(ProtocolController_t *controller);

/**
  * @brief  Deinitialize protocol handler
  * @retval download_status_t: DOWNLOAD_OK on success
  */
ProtocolStatus_t protocol_deinit(ProtocolController_t *controller);

/**
  * @brief  Reset protocol state machine
  * @retval download_status_t: DOWNLOAD_OK on success
  */
ProtocolStatus_t protocol_reset(ProtocolController_t *controller);

/**
  * @brief  Check if RX buffer has a complete frame
  * @retval bool: True if buffer contains a valid frame
  */
bool isBufferOk(ProtocolController_t *controller);

/**
  * @brief  Process received 8-byte frame

  * @retval download_status_t: Protocol status
  */
ProtocolStatus_t protocol_process_frame(ProtocolController_t *controller);

/**
  * @brief  Send ACK frame
  * @param  seq: Sequence number to acknowledge
  * @retval download_status_t: DOWNLOAD_OK on success
  */
ProtocolStatus_t protocol_send_ack(uint16_t seq);

/**
  * @brief  Send NAK frame
  * @param  seq: Sequence number to reject (0xFFFF for general NAK)
  * @retval download_status_t: DOWNLOAD_OK on success
  */
ProtocolStatus_t protocol_send_nak(uint16_t seq);
ProtocolStatus_t protocol_send_version(uint16_t seq);
ProtocolStatus_t protocol_send_series(uint16_t seq);
ProtocolStatus_t protocol_send_spec(uint16_t seq);
/**
  * @brief  Get current protocol state
  * @retval ProtocolState_t: Current state
  */
ProtocolState_t protocol_get_state(ProtocolController_t *controller);

/**
  * @brief  Get received firmware size (after START command)
  * @param  size: Pointer to store firmware size
  * @retval download_status_t: DOWNLOAD_OK on success
  */
uint32_t protocol_get_firmware_size(ProtocolController_t *controller);

/**
  * @brief  Get received firmware CRC32 (after START command)
  * @param  crc: Pointer to store CRC32 value
  * @retval download_status_t: DOWNLOAD_OK on success
  */
uint32_t protocol_get_firmware_crc(ProtocolController_t *controller);

/**
  * @brief  Update protocol system time (should be called periodically)
  * @param  ms: Milliseconds to add
  */
void protocol_update_time(ProtocolController_t *controller, uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __DOWNLOAD_PROTOCOL_H */
