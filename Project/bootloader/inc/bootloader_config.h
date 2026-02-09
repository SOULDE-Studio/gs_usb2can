/**
  **************************************************************************
  * @file     bootloader_config.h
  * @brief    Bootloader configuration definitions
  *           Centralized address and configuration definitions
  *           Used by both bootloader core and application entry helper
  **************************************************************************
  */

#ifndef __BOOTLOADER_CONFIG_H
#define __BOOTLOADER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================
   Flash memory layout (128KB total)
   ======================================== */
#include "stdint.h"
/* Flash base address */
#define BOOTLOADER_FLASH_BASE (0x08000000U)

/* Flash total size */
#define BOOTLOADER_FLASH_TOTAL_SIZE (128U * 1024U) /* 128KB */

/* Flash page size (from HAL) */
#define BOOTLOADER_FLASH_PAGE_SIZE (FLASH_PAGE_SIZE)

/* Parameter storage area size (last 1KB of flash) */
#define PARAM_STORAGE_SIZE (1U * 1024U) /* 1KB */

/* Bootloader flash size (first 19KB) */
#define BOOTLOADER_FLASH_SIZE (19U * 1024U) /* 19KB */

/* Application flash size (middle, excluding parameter area) - calculated from total size */
#define APPLICATION_FLASH_SIZE (BOOTLOADER_FLASH_TOTAL_SIZE - BOOTLOADER_FLASH_SIZE - PARAM_STORAGE_SIZE)

/* Bootloader flash area (first 19KB) */
#define BOOTLOADER_FLASH_START (BOOTLOADER_FLASH_BASE)
#define BOOTLOADER_FLASH_END (BOOTLOADER_FLASH_START + BOOTLOADER_FLASH_SIZE - 1U)

/* Parameter storage area (last 1KB of configured flash) */
#define PARAM_STORAGE_START (BOOTLOADER_FLASH_END + 1U)
#define PARAM_STORAGE_END (PARAM_STORAGE_START + PARAM_STORAGE_SIZE - 1U)
/* Application flash area (last region before flash end, after parameter area) */
#define APPLICATION_FLASH_START (PARAM_STORAGE_END + 1U)
#define APPLICATION_FLASH_END (APPLICATION_FLASH_START + APPLICATION_FLASH_SIZE - 1U)

/* ========================================
   GPIO status LED configuration
   ======================================== */
/* User should modify these according to hardware design */
/* For STM32G0, LED is on PC13 */
#define BOOTLOADER_STATUS_LED_PORT GPIOC
#define BOOTLOADER_STATUS_LED_PIN GPIO_PIN_13

/* ========================================
   RAM memory layout (16KB total)
   ======================================== */

/* RAM base address */
#define BOOTLOADER_RAM_BASE (0x20000000U)

/* RAM total size */
#define BOOTLOADER_RAM_TOTAL_SIZE (20U * 1024U) /* 20KB */

/* RAM end address */
#define BOOTLOADER_RAM_END (BOOTLOADER_RAM_BASE + BOOTLOADER_RAM_TOTAL_SIZE)

/* RAM valid range for stack pointer validation */
#define BOOTLOADER_RAM_VALID_START (BOOTLOADER_RAM_BASE)
#define BOOTLOADER_RAM_VALID_END (BOOTLOADER_RAM_END)

/* Bootloader control area size (4 bytes) */
#define BOOTLOADER_CONTROL_SIZE (4U)

/* Bootloader control area (last 4 bytes of RAM) */
#define BOOTLOADER_CONTROL_ADDRESS (BOOTLOADER_RAM_END - BOOTLOADER_CONTROL_SIZE)

/* ========================================
   Application vector table offsets
   ======================================== */

#define APPLICATION_VECTOR_TABLE_OFFSET (0x00000000U)
#define APPLICATION_STACK_POINTER_OFFSET (0x00000000U) /* Stack pointer at offset 0 */
#define APPLICATION_RESET_HANDLER_OFFSET (0x00000004U) /* Reset handler at offset 4 */

/* ========================================
   Magic numbers for bootloader control
   ======================================== */

#define BOOTLOADER_MAGIC_ENTER (0xDEADBEEFU)   /* Magic number to request bootloader entry */
#define BOOTLOADER_MAGIC_EXIT (0xCAFEBABEU)    /* Magic number to exit bootloader mode */
#define BOOTLOADER_MAGIC_INVALID (0xFFFFFFFFU) /* Invalid/erased magic number */

/* ========================================
   Bootloader timeout configuration
   ======================================== */

#define BOOTLOADER_STARTUP_TIMEOUT_MS (5000U) /* Bootloader startup timeout (5 seconds) */

#define SET_ADDRESS(addr) __attribute__((section(".ARM.__at_" #addr)))

#define BOOTLOADER_VERSION_OFFSET (0x000193e0U)
#define BOOTLOADER_VERSION_ADDR (BOOTLOADER_FLASH_BASE + BOOTLOADER_VERSION_OFFSET)

#define BOOTLOADER_PRODUCT_ID_OFFSET (0x000193c0U)
#define BOOTLOADER_PRODUCT_ID_ADDR (BOOTLOADER_FLASH_BASE + BOOTLOADER_PRODUCT_ID_OFFSET)

#define FIRMWARE_VERSION_ADDR (BOOTLOADER_FLASH_BASE + BOOTLOADER_VERSION_OFFSET)
#define FIRMWARE_PRODUCT_ID_ADDR (BOOTLOADER_FLASH_BASE + BOOTLOADER_PRODUCT_ID_OFFSET)

static const uint8_t bootloader_version[2] SET_ADDRESS(BOOTLOADER_VERSION_ADDR) = {BOOTLOADER_VERSION_MAJOR,
                                                                               BOOTLOADER_VERSION_MINOR};
static const uint64_t bootloader_product_id SET_ADDRESS(BOOTLOADER_PRODUCT_ID_ADDR) = BOOTLOADER_PRODUCT_ID;

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_CONFIG_H */
