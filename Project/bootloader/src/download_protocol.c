/**
  **************************************************************************
  * @file     download_protocol.c
  * @brief    Download protocol implementation for 8-byte fixed packet
  *           Compatible with CAN bus, inspired by YMODEM protocol
  **************************************************************************
  */

#include "download_protocol.h"

#include <string.h>

#include "bootloader_config.h"
#include "hw_interface.h"
#include "usbd_cdc_if.h"
ProtocolController_t controller;
/* ========================================
   Private defines
   ======================================== */

/* ========================================
   Private variables
   ======================================== */

/* ========================================
   Private function prototypes
   ======================================== */
void parse_frame(ProtocolController_t *controller, ProtocolFrame_t *parsed);
ProtocolStatus_t handle_start_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame);
ProtocolStatus_t handle_data_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame);
ProtocolStatus_t handle_end_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame);
ProtocolStatus_t handle_info_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame);
ProtocolStatus_t prepareFlash(ProtocolController_t *controller) {
    ProtocolStatus_t status = STATUS_OK;
    status = (ProtocolStatus_t) hw_flash_erase_range(APPLICATION_FLASH_START, APPLICATION_FLASH_END);
    return status;
}
bool isBufferOk(ProtocolController_t *controller);
bool verify_firmware(uint32_t expected_crc, uint32_t firmware_size);
uint32_t get_system_time_ms(ProtocolController_t *controller);

/* ========================================
   Protocol initialization
   ======================================== */

ProtocolStatus_t protocol_init(ProtocolController_t *controller) {
    controller->protocol_state = PROTOCOL_STATE_IDLE;
    controller->expected_seq = 0;
    controller->firmware_size = 0;
    controller->firmware_crc = 0;
    controller->received_bytes = 0;

    controller->retry_count = 0;
    controller->start_size_received = false;
    controller->system_time_ms = 0;

    return STATUS_OK;
}

ProtocolStatus_t protocol_deinit(ProtocolController_t *controller) {
    protocol_reset(controller);
    return STATUS_OK;
}

ProtocolStatus_t protocol_reset(ProtocolController_t *controller) {
    controller->protocol_state = PROTOCOL_STATE_IDLE;
    controller->expected_seq = 0;
    controller->firmware_size = 0;
    controller->firmware_crc = 0;
    controller->received_bytes = 0;

    controller->retry_count = 0;
    controller->start_size_received = false;

    return STATUS_OK;
}
bool isBufferOk(ProtocolController_t *controller) { return userRXBuffer.count >= 8; }
/* ========================================
   Protocol state machine
   ======================================== */

ProtocolStatus_t protocol_process_frame(ProtocolController_t *controller) {
    ProtocolFrame_t parsed;
    ProtocolStatus_t status = STATUS_OK;

    if (!isBufferOk(controller)) { return status; }

    /* Parse frame */
    parse_frame(controller, &parsed);

    /* Update last frame time */
    controller->last_frame_time = get_system_time_ms(controller);

    /* Handle different commands based on state */
    switch (controller->protocol_state) {
        case PROTOCOL_STATE_IDLE:
            if (parsed.cmd == CMD_START) {
                status = handle_start_cmd(controller, &parsed);
                /* If START frame 1 received but not frame 2, state remains IDLE */
                /* Next frame can be START frame 2 or DATA frame */
            } else if (parsed.cmd == CMD_DATA) {
                /* If we received size but not CRC32, and now receive DATA, start download */
                if (controller->start_size_received && parsed.seq == 0) {
                    /* Start download without CRC32 */
                    status = prepareFlash(controller);
                    if (status == STATUS_OK) {
                        controller->protocol_state = PROTOCOL_STATE_RECEIVING;
                        controller->expected_seq = 0;
                        controller->received_bytes = 0;
                        controller->retry_count = 0;

                        controller->start_size_received = false;
                        /* Process this DATA frame */
                        status = handle_data_cmd(controller, &parsed);
                    } else {
                        protocol_send_nak(0xFFFF);
                        // CDC_Transmit_FS((uint8_t *) "Failed to prepare flash\r\n", 25);
                        controller->protocol_state = PROTOCOL_STATE_ERROR;
                        controller->start_size_received = false;
                    }
                } else {
                    /* Unexpected DATA frame */
                    protocol_send_nak(0xFFFF);
                    // CDC_Transmit_FS((uint8_t *) "Invalid DATA frame\r\n", 21);
                    controller->protocol_state = PROTOCOL_STATE_ERROR;
                }
            } else if (parsed.cmd == CMD_CANCEL) {
                controller->start_size_received = false;
                protocol_reset(controller);
                status = STATUS_OK;
            } else if (parsed.cmd == CMD_INFO) {
                status = handle_info_cmd(controller, &parsed);
            } else {
                /* Unexpected command, send NAK */
                protocol_send_nak(0xFFFF);
                // CDC_Transmit_FS((uint8_t *) "Invalid command\r\n", 17);
                status = STATUS_INVALID;
            }
            break;

        case PROTOCOL_STATE_RECEIVING:
            if (parsed.cmd == CMD_DATA) {
                status = handle_data_cmd(controller, &parsed);
            } else if (parsed.cmd == CMD_END) {
                status = handle_end_cmd(controller, &parsed);
            } else if (parsed.cmd == CMD_CANCEL) {
                protocol_reset(controller);
                status = STATUS_OK;
            } else {
                /* Unexpected command, send NAK */
                protocol_send_nak(controller->expected_seq);
                // CDC_Transmit_FS((uint8_t *) "Invalid command\r\n", 17);
                status = STATUS_INVALID;
            }
            break;

        case PROTOCOL_STATE_COMPLETE:
            /* If CRC32 was provided, verify firmware */
            if (controller->firmware_crc != 0) {
                status = verify_firmware(controller->firmware_crc, controller->firmware_size);
                if (status != STATUS_OK) {
                    controller->protocol_state = PROTOCOL_STATE_ERROR;
                    hw_gpio_toggle();
                    return STATUS_VERIFY_FAILED;
                } else {
                    hw_system_reset();
                    status = STATUS_COMPLETE;
                }
            }
            break;
        case PROTOCOL_STATE_ERROR:
            /* Ignore frames in these states */
            hw_system_reset();
            break;
    }

    return status;
}
/* ========================================
   Frame parsing
   ======================================== */

void parse_frame(ProtocolController_t *controller, ProtocolFrame_t *parsed) {
    userRXBuffer.count -= 8;

    parsed->cmd = userRXBuffer.data[userRXBuffer.head];
    parsed->seq = ((uint16_t) userRXBuffer.data[userRXBuffer.head + 2] << 8)
        | userRXBuffer.data[userRXBuffer.head + 1]; /* Little-endian */
    parsed->data_len = userRXBuffer.data[userRXBuffer.head + 3];

    if (parsed->data_len > PROTOCOL_MAX_DATA_PER_FRAME) { parsed->data_len = PROTOCOL_MAX_DATA_PER_FRAME; }
    // CDC_Transmit_FS(userRXBuffer.data + userRXBuffer.head, 8);
    /* Copy data payload */
    for (uint8_t i = 0; i < parsed->data_len; i++) { parsed->data[i] = userRXBuffer.data[userRXBuffer.head + 4 + i]; }
    userRXBuffer.head += 8;
    userRXBuffer.head %= USER_RX_BUFFER_SIZE;
}

/* ========================================
   Command handlers
   ======================================== */

ProtocolStatus_t handle_start_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame) {
    ProtocolStatus_t status;

    /* START command can be sent in two frames:
     * Frame 1 (seq=0): data[0-3] = firmware size (little-endian 32-bit)
     * Frame 2 (seq=1, optional): data[0-3] = CRC32 (little-endian 32-bit)
     */

    if (frame->seq == 0 && frame->data_len >= 4) {
        /* First START frame: firmware size */
        controller->firmware_size = ((uint32_t) frame->data[3] << 24) | ((uint32_t) frame->data[2] << 16)
            | ((uint32_t) frame->data[1] << 8) | ((uint32_t) frame->data[0]);

        controller->firmware_crc = 0; /* Will be set if second START frame received */
        controller->start_size_received = true;

        /* Check if firmware size is valid */
        if (controller->firmware_size == 0 || controller->firmware_size > APPLICATION_FLASH_SIZE) {
            controller->start_size_received = false;
            protocol_send_nak(0xFFFF);
            // CDC_Transmit_FS((uint8_t *) "Invalid firmware size\r\n", 23);
            return STATUS_INVALID;
        }

        /* Send ACK for first START frame */
        protocol_send_ack(0);

        /* Wait for optional second START frame with CRC32 */
        /* If no second frame received, CRC32 will remain 0 (verification skipped) */
        return STATUS_OK;
    } else if (frame->seq == 1 && controller->start_size_received && frame->data_len >= 4) {
        /* Second START frame: CRC32 */
        controller->firmware_crc = ((uint32_t) frame->data[3] << 24) | ((uint32_t) frame->data[2] << 16)
            | ((uint32_t) frame->data[1] << 8) | ((uint32_t) frame->data[0]);

        controller->start_size_received = false;
        /* Start download and prepare empty space for firmware */
        status = prepareFlash(controller);
        if (status == STATUS_OK) {
            controller->protocol_state = PROTOCOL_STATE_RECEIVING;
            controller->expected_seq = 0;
            controller->received_bytes = 0;
            controller->retry_count = 0;
            userRXBuffer.head = 0;
            userRXBuffer.count = 0;
            /* Send ACK for second START frame */
            protocol_send_ack(1);
        } else {
            protocol_send_nak(1);
            // CDC_Transmit_FS((uint8_t *) "Failed to prepare flash\r\n", 24);
            controller->protocol_state = PROTOCOL_STATE_ERROR;
        }

        return status;
    } else if (controller->start_size_received && frame->seq != 1) {
        /* Size received but waiting for CRC32 frame (seq=1), but got different seq */
        /* Start download without CRC32 */
        controller->start_size_received = false;
        status = prepareFlash(controller);
        if (status == STATUS_OK) {
            controller->protocol_state = PROTOCOL_STATE_RECEIVING;
            controller->expected_seq = 0;
            controller->received_bytes = 0;
            userRXBuffer.head = 0;
            userRXBuffer.count = 0;
            controller->retry_count = 0;

            /* Send ACK */
            protocol_send_ack(frame->seq);
        } else {
            protocol_send_nak(frame->seq);
            // CDC_Transmit_FS((uint8_t *) "Failed to prepare flash\r\n", 24);
            controller->protocol_state = PROTOCOL_STATE_ERROR;
        }

        return status;
    } else {
        /* Invalid START frame */
        controller->start_size_received = false;
        protocol_send_nak(0xFFFF);
        // CDC_Transmit_FS((uint8_t *) "Invalid START frame\r\n", 22);
        return STATUS_INVALID;
    }
}

ProtocolStatus_t handle_data_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame) {
    ProtocolStatus_t status;

    /* Check sequence number */
    if (frame->seq != controller->expected_seq) {
        /* Sequence mismatch, send NAK with expected sequence */
        protocol_send_nak(controller->expected_seq);
        // CDC_Transmit_FS((uint8_t *) "Invalid sequence number\r\n", 24);
        controller->retry_count++;

        if (controller->retry_count >= PROTOCOL_MAX_RETRIES) {
            controller->protocol_state = PROTOCOL_STATE_ERROR;
            return STATUS_ERROR;
        }

        return STATUS_INVALID;
    }

    /* Reset retry counter on successful frame */
    controller->retry_count = 0;

    /* Write data to Flash */
    uint32_t data;
    memcpy(&data, frame->data, sizeof(uint32_t));
    status = (ProtocolStatus_t) hw_flash_program_word(APPLICATION_FLASH_START + controller->received_bytes, data);

    if (status != STATUS_OK) {
        protocol_send_nak(controller->expected_seq);
        // CDC_Transmit_FS((uint8_t *) "Failed to program flash\r\n", 24);
        controller->protocol_state = PROTOCOL_STATE_ERROR;
        return status;
    }

    controller->received_bytes += 4;

    /* Increment expected sequence number */
    controller->expected_seq++;
    if (controller->expected_seq > PROTOCOL_MAX_SEQ) { controller->expected_seq = 0; /* Wrap around */ }

    /* Send ACK */
    protocol_send_ack(frame->seq);

    /* Check if download is complete */
    if (controller->received_bytes >= controller->firmware_size) {
        controller->protocol_state = PROTOCOL_STATE_COMPLETE;
        return STATUS_OK;
    }

    return STATUS_BUSY;
}

ProtocolStatus_t handle_end_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame) {
    /* Verify we received all data */
    if (controller->received_bytes < controller->firmware_size) {
        protocol_send_nak(frame->seq);
        // CDC_Transmit_FS((uint8_t *) "Incomplete firmware download\r\n", 30);
        controller->protocol_state = PROTOCOL_STATE_ERROR;
        return STATUS_ERROR;
    }

    /* Send ACK for END */
    protocol_send_ack(frame->seq);

    controller->protocol_state = PROTOCOL_STATE_COMPLETE;
    return STATUS_OK;
}

ProtocolStatus_t handle_info_cmd(ProtocolController_t *controller, const ProtocolFrame_t *frame) {
    uint8_t type = frame->data[0];
    switch (type) {
        case INFO_TYPE_VERSION: {
            protocol_send_version(frame->seq);
        } break;
        case INFO_TYPE_SERIES: {
            protocol_send_series(frame->seq);
        } break;
        case INFO_TYPE_SPEC: {
            protocol_send_spec(frame->seq);
        } break;
        default: {
            protocol_send_nak(frame->seq);
            return STATUS_INVALID;
        } break;
    }
    return STATUS_OK;
}
ProtocolStatus_t protocol_send_version(uint16_t seq) {
    uint8_t frame[PROTOCOL_FRAME_SIZE];

    frame[0] = CMD_INFO;
    frame[1] = (uint8_t) (seq & 0xFF);        /* SEQ_L */
    frame[2] = (uint8_t) ((seq >> 8) & 0xFF); /* SEQ_H */
    frame[3] = 2;
    frame[4] = bootloader_version[0]; /* Bootloader version major */
    frame[5] = bootloader_version[1]; /* Bootloader version minor */
    frame[6] = 0;
    frame[7] = 0;

    return (ProtocolStatus_t) hw_comm_send(frame, PROTOCOL_FRAME_SIZE, 100);
}
ProtocolStatus_t protocol_send_series(uint16_t seq) {
    uint8_t frame[PROTOCOL_FRAME_SIZE];

    frame[0] = CMD_INFO;
    frame[1] = (uint8_t) (seq & 0xFF);        /* SEQ_L */
    frame[2] = (uint8_t) ((seq >> 8) & 0xFF); /* SEQ_H */
    frame[3] = 4;
    frame[4] = (uint8_t) (bootloader_product_id >> 32) & 0xFF;
    frame[5] = (uint8_t) (bootloader_product_id >> 40) & 0xFF;
    frame[6] = (uint8_t) (bootloader_product_id >> 48) & 0xFF;
    frame[7] = (uint8_t) (bootloader_product_id >> 56) & 0xFF;

    return (ProtocolStatus_t) hw_comm_send(frame, PROTOCOL_FRAME_SIZE, 100);
}
ProtocolStatus_t protocol_send_spec(uint16_t seq) {
    uint8_t frame[PROTOCOL_FRAME_SIZE];

    frame[0] = CMD_INFO;
    frame[1] = (uint8_t) (seq & 0xFF);        /* SEQ_L */
    frame[2] = (uint8_t) ((seq >> 8) & 0xFF); /* SEQ_H */
    frame[3] = 4;
    frame[4] = (uint8_t) (bootloader_product_id >> 0) & 0xFF;
    frame[5] = (uint8_t) (bootloader_product_id >> 8) & 0xFF;
    frame[6] = (uint8_t) (bootloader_product_id >> 16) & 0xFF;
    frame[7] = (uint8_t) (bootloader_product_id >> 24) & 0xFF;

    return (ProtocolStatus_t) hw_comm_send(frame, PROTOCOL_FRAME_SIZE, 100);
}

/* ========================================
   ACK/NAK sending
   ======================================== */

ProtocolStatus_t protocol_send_ack(uint16_t seq) {
    uint8_t frame[PROTOCOL_FRAME_SIZE];

    frame[0] = CMD_ACK;
    frame[1] = (uint8_t) (seq & 0xFF);        /* SEQ_L */
    frame[2] = (uint8_t) ((seq >> 8) & 0xFF); /* SEQ_H */
    frame[3] = 0;                             /* No data */
    frame[4] = 0;
    frame[5] = 0;
    frame[6] = 0;
    frame[7] = 0;

    return (ProtocolStatus_t) hw_comm_send(frame, PROTOCOL_FRAME_SIZE, 100);
}

ProtocolStatus_t protocol_send_nak(uint16_t seq) {
    uint8_t frame[PROTOCOL_FRAME_SIZE];

    frame[0] = CMD_NAK;
    frame[1] = (uint8_t) (seq & 0xFF);        /* SEQ_L */
    frame[2] = (uint8_t) ((seq >> 8) & 0xFF); /* SEQ_H */
    frame[3] = 0;                             /* No data */
    frame[4] = 0;
    frame[5] = 0;
    frame[6] = 0;
    frame[7] = 0;

    return (ProtocolStatus_t) hw_comm_send(frame, PROTOCOL_FRAME_SIZE, 100);
}

/* ========================================
   State and info queries
   ======================================== */

ProtocolState_t protocol_get_state(ProtocolController_t *controller) { return controller->protocol_state; }

uint32_t protocol_get_firmware_size(ProtocolController_t *controller) {
    if (controller->protocol_state == PROTOCOL_STATE_IDLE) { return 0; }

    if (controller->protocol_state == PROTOCOL_STATE_ERROR) { return 0; }

    return controller->firmware_size;
}

uint32_t protocol_get_firmware_crc(ProtocolController_t *controller) {
    if (controller->protocol_state == PROTOCOL_STATE_IDLE) { return 0; }

    if (controller->protocol_state == PROTOCOL_STATE_ERROR) { return 0; }

    return controller->firmware_crc;
}

/* ========================================
   Helper functions
   ======================================== */

/* System time counter (updated by main loop) */

/**
  * @brief  Update system time (should be called periodically)
  * @param  ms: Milliseconds to add
  */
void protocol_update_time(ProtocolController_t *controller, uint32_t ms) { controller->system_time_ms += ms; }

uint32_t get_system_time_ms(ProtocolController_t *controller) { return controller->system_time_ms; }
bool verify_firmware(uint32_t expected_crc, uint32_t firmware_size) {
    /* Compute CRC32 over the downloaded firmware */
    uint32_t crc = 0xFFFFFFFFU;
    const uint32_t poly = 0xEDB88320U;

    for (uint32_t offset = 0; offset < firmware_size; offset++) {
        uint8_t byte = hw_flash_read_byte(APPLICATION_FLASH_START + offset);
        crc ^= byte;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {// 检查最低位是否为1
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
    }
    crc ^= 0xFFFFFFFF;

    if (crc != expected_crc) { return 1; }

    return 0;
}
