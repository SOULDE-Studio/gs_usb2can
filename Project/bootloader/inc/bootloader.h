/**
  **************************************************************************
  * @file     bootloader.h
  * @brief    Bootloader main header file
  *           Provides bootloader configuration and interface
  **************************************************************************
  */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_config.h" /* Include centralized configuration */
#include "hw_interface.h"


/* ========================================
   Bootloader status codes
   ======================================== */
typedef enum {
    BOOTLOADER_OK = 0,
    BOOTLOADER_ERROR = 1,
    BOOTLOADER_APP_INVALID = 2,
    BOOTLOADER_APP_VALID = 3,
    BOOTLOADER_TIMEOUT = 4
} bootloader_status_t;

/* ========================================
   Bootloader control structure
   ======================================== */
typedef struct {
    uint32_t magic;       /* Magic number to request bootloader entry */
    uint32_t reserved[3]; /* Reserved for future use */
} bootloader_control_t;

/* ========================================
   Bootloader function prototypes
   ======================================== */

/**
  * @brief  Initialize bootloader
  * @retval bootloader_status_t: BOOTLOADER_OK on success
  */
bootloader_status_t bootloader_init(void);

/**
  * @brief  Deinitialize bootloader
  * @retval bootloader_status_t: BOOTLOADER_OK on success
  */
bootloader_status_t bootloader_deinit(void);

/**
  * @brief  Check if application is valid
  * @param  app_address: Application start address
  * @retval bootloader_status_t: BOOTLOADER_APP_VALID if valid, BOOTLOADER_APP_INVALID otherwise
  */
bootloader_status_t bootloader_check_application(uint32_t app_address);

/**
  * @brief  Jump to application
  * @param  app_address: Application start address
  * @note   This function will not return if successful
  */
void bootloader_jump_to_application(uint32_t app_address);

/**
  * @brief  Check if bootloader should be entered
  * @retval bool: true if bootloader should be entered, false otherwise
  */
bool bootloader_should_enter(void);

/**
  * @brief  Clear bootloader entry request flag
  */
void bootloader_clear_entry_flag(void);

/**
  * @brief  Set bootloader entry request flag (called by application)
  */
void bootloader_request_entry(void);

/**
  * @brief  Main bootloader process
  *         This function handles bootloader logic:
  *         1. Check if application is valid
  *         2. If valid and no bootloader entry request, jump to application
  *         3. Otherwise, enter bootloader mode for firmware update
  * @retval bootloader_status_t: Bootloader status
  */
bootloader_status_t bootloader_process(void);
bootloader_status_t bootloader_main(void);
#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_H */