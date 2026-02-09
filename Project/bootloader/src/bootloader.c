/**
  **************************************************************************
  * @file     bootloader.c
  * @brief    Bootloader main implementation
  *           This file implements the core bootloader logic:
  *           - Application validation
  *           - Application jump
  *           - Bootloader entry control
  **************************************************************************
  */

#include "bootloader.h"

#include "download_protocol.h"

/* ========================================
   Private function prototypes
   ======================================== */
static bool is_valid_vector_table(uint32_t address);
static uint32_t get_stack_pointer(uint32_t address);
static uint32_t get_reset_handler(uint32_t address);

/* ========================================
   Bootloader control structure (in RAM)
   ======================================== */
static volatile bootloader_control_t *bootloader_control = (volatile bootloader_control_t *) BOOTLOADER_CONTROL_ADDRESS;
ProtocolStatus_t bootloader_spin(void);
/* ========================================
   Bootloader initialization
   ======================================== */

bootloader_status_t bootloader_init(void) {
    hw_result_t result;

    /* Initialize hardware interface */
    result = hw_system_init();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    result = hw_flash_init();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    /* Initialize GPIO for status indicator */
    result = hw_gpio_init();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    /* Initialize communication interface */
    result = hw_comm_init();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    /* Initialize download module */
    protocol_init(&controller);

    return BOOTLOADER_OK;
}

bootloader_status_t bootloader_deinit(void) {
    hw_result_t result;

    /* Deinitialize download module */
    protocol_deinit(&controller);

    /* Deinitialize hardware interface */
    result = hw_flash_deinit();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    result = hw_gpio_deinit();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    result = hw_system_deinit();
    if (result != HW_OK) { return BOOTLOADER_ERROR; }

    return BOOTLOADER_OK;
}

/* ========================================
   Application validation
   ======================================== */

static bool is_valid_vector_table(uint32_t address) {
    uint32_t stack_pointer;
    uint32_t reset_handler;

    /* Check if address is in valid flash range */
    if (address < APPLICATION_FLASH_START || address >= APPLICATION_FLASH_END) { return false; }

    /* Check if address is word-aligned */
    if ((address & 0x3) != 0) { return false; }

    /* Read stack pointer from vector table */
    stack_pointer = get_stack_pointer(address);

    /* Check if stack pointer is in valid RAM range */
    if (stack_pointer < BOOTLOADER_RAM_VALID_START || stack_pointer > BOOTLOADER_RAM_VALID_END) { return false; }

    /* Read reset handler from vector table */
    reset_handler = get_reset_handler(address);

    /* Check if reset handler is in valid flash range */
    /* Reset handler should be in application flash area */
    if (reset_handler < APPLICATION_FLASH_START || reset_handler >= APPLICATION_FLASH_END) { return false; }

    /* Additional check: verify reset handler address is odd (Thumb mode) */
    /* ARM Cortex-M always runs in Thumb mode, so the LSB should be set */
    if ((reset_handler & 0x1) == 0) { return false; }

    /* Check if first word of vector table is not erased (0xFFFFFFFF) */
    if (stack_pointer == 0xFFFFFFFF || reset_handler == 0xFFFFFFFF) { return false; }

    return true;
}

static uint32_t get_stack_pointer(uint32_t address) {
    return hw_flash_read_word(address + APPLICATION_STACK_POINTER_OFFSET);
}

static uint32_t get_reset_handler(uint32_t address) {
    return hw_flash_read_word(address + APPLICATION_RESET_HANDLER_OFFSET);
}

bootloader_status_t bootloader_check_application(uint32_t app_address) {
    if (is_valid_vector_table(app_address)) {
        return BOOTLOADER_APP_VALID;
    } else {
        return BOOTLOADER_APP_INVALID;
    }
}

/* ========================================
   Application jump
   ======================================== */

void bootloader_jump_to_application(uint32_t app_address) {
    /* Deinitialize bootloader before jumping */
    bootloader_deinit();

    /* Clear bootloader entry flag so next boot will go directly to application */
    bootloader_clear_entry_flag();

    /* Jump to application */
    hw_jump_to_application(app_address);

    /* Should never reach here */
    while (1);
}

/* ========================================
   Bootloader entry control
   ======================================== */

bool bootloader_should_enter(void) {
    /* Check if magic number is set to request bootloader entry */
    if (bootloader_control->magic == BOOTLOADER_MAGIC_ENTER) { return true; }

    return false;
}

void bootloader_clear_entry_flag(void) {
    /* Clear the magic number */
    bootloader_control->magic = BOOTLOADER_MAGIC_EXIT;
}

void bootloader_request_entry(void) {
    /* Set magic number to request bootloader entry on next reset */
    bootloader_control->magic = BOOTLOADER_MAGIC_ENTER;

    /* Reset system to enter bootloader */
    hw_system_reset();
}

/* ========================================
   Main bootloader process
   ======================================== */

bootloader_status_t bootloader_process(void) {
    bootloader_status_t status;
    uint32_t app_address = APPLICATION_FLASH_START;

    /* Check if application is valid */
    status = bootloader_check_application(app_address);

    if (status == BOOTLOADER_APP_VALID) {
        /* Application is valid, check if we should enter bootloader */
        if (!bootloader_should_enter()) {
            /* Normal boot: jump directly to application */
            bootloader_jump_to_application(app_address);

            /* Should never reach here */
            return BOOTLOADER_OK;
        } else {
            /* Bootloader entry requested: stay in bootloader mode */
            /* This will be handled by main loop for firmware update */
            return BOOTLOADER_OK;
        }
    } else {
        /* Application is invalid, stay in bootloader mode */
        /* This allows firmware update to fix corrupted application */
        return BOOTLOADER_APP_INVALID;
    }
}

/**
  * @brief  Bootloader main entry point (called from main())
  * @retval bootloader_status_t: Bootloader status
  */
bootloader_status_t bootloader_main(void) {
    bootloader_status_t status;

    /* Initialize bootloader */
    status = bootloader_init();
    if (status != BOOTLOADER_OK) { return status; }

    /* Process bootloader logic */
    status = bootloader_process();

    hw_gpio_toggle();
    while (1) { bootloader_spin(); }

    return status;
}

ProtocolStatus_t bootloader_spin(void) {
    /* Process bootloader logic */
    ProtocolStatus_t status;
    status = protocol_process_frame(&controller);
    switch (status) {
        case STATUS_OK:
            /* Frame processed successfully */
            break;
        case STATUS_BUSY:
            /* Frame not processed yet */
            break;
        case STATUS_INVALID:
            /* Invalid frame received */
            break;
        case STATUS_VERIFY_FAILED:
            /* Firmware verification failed */
            break;
        case STATUS_COMPLETE:
            /* Firmware update complete */
            /* Jump to application */
            bootloader_jump_to_application(APPLICATION_FLASH_START);
            break;
        default:
            /* Unknown error */
            break;
    }
    return status;
}