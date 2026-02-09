/**
  **************************************************************************
  * @file     hw_interface.h
  * @brief    Hardware abstraction interface for bootloader
  *           This interface provides platform-independent functions that
  *           bootloader core logic can use, ensuring high portability.
  **************************************************************************
  */

#ifndef __HW_INTERFACE_H
#define __HW_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>


/* ========================================
   Return code definitions
   ======================================== */
typedef enum { HW_OK = 0, HW_ERROR = 1, HW_TIMEOUT = 2, HW_BUSY = 3, HW_INVALID_PARAM = 4 } hw_result_t;

/* ========================================
   Flash operations interface
   ======================================== */

/**
  * @brief  Initialize flash controller
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_init(void);

/**
  * @brief  Deinitialize flash controller
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_deinit(void);

/**
  * @brief  Unlock flash for write operations
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_unlock(void);

/**
  * @brief  Lock flash to prevent write operations
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_lock(void);

/**
  * @brief  Erase a flash sector
  * @param  address: Sector address (must be aligned to sector boundary)
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_erase_sector(uint32_t address);

/**
  * @brief  Erase multiple flash sectors
  * @param  start_address: Start address
  * @param  end_address: End address (inclusive)
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_erase_range(uint32_t start_address, uint32_t end_address);

/**
  * @brief  Program a word (32-bit) to flash
  * @param  address: Flash address (must be word-aligned)
  * @param  data: Data to program
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_program_word(uint32_t address, uint32_t data);

/**
  * @brief  Program a halfword (16-bit) to flash
  * @param  address: Flash address (must be halfword-aligned)
  * @param  data: Data to program
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_program_halfword(uint32_t address, uint16_t data);

/**
  * @brief  Program a byte to flash
  * @param  address: Flash address
  * @param  data: Data to program
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_flash_program_byte(uint32_t address, uint8_t data);

/**
  * @brief  Read a word from flash
  * @param  address: Flash address (must be word-aligned)
  * @retval uint32_t: Read value
  */
uint32_t hw_flash_read_word(uint32_t address);

/**
  * @brief  Read a halfword from flash
  * @param  address: Flash address (must be halfword-aligned)
  * @retval uint16_t: Read value
  */
uint16_t hw_flash_read_halfword(uint32_t address);

/**
  * @brief  Read a byte from flash
  * @param  address: Flash address
  * @retval uint8_t: Read value
  */
uint8_t hw_flash_read_byte(uint32_t address);

/**
  * @brief  Get flash sector size
  * @retval uint32_t: Sector size in bytes
  */
uint32_t hw_flash_get_sector_size(void);

/**
  * @brief  Get flash page size (minimum program unit)
  * @retval uint32_t: Page size in bytes
  */
uint32_t hw_flash_get_page_size(void);

/* ========================================
   System operations interface
   ======================================== */

/**
  * @brief  Initialize system clock
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_system_init(void);

/**
  * @brief  Deinitialize system
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_system_deinit(void);

/**
  * @brief  Delay in milliseconds
  * @param  ms: Delay time in milliseconds
  */
void hw_delay_ms(uint32_t ms);

/**
  * @brief  Reset the system
  */
void hw_system_reset(void);

/**
  * @brief  Jump to application at specified address
  * @param  address: Application entry address
  * @note   This function will not return if successful
  */
void hw_jump_to_application(uint32_t address);

/* ========================================
   Communication interface (for future use)
   ======================================== */

/**
  * @brief  Initialize communication interface (UART/USB/etc.)
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_comm_init(void);

/**
  * @brief  Deinitialize communication interface
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_comm_deinit(void);

/**
  * @brief  Send data via communication interface
  * @param  data: Pointer to data buffer
  * @param  length: Data length in bytes
  * @param  timeout_ms: Timeout in milliseconds (0 = no timeout)
  * @retval hw_result_t: HW_OK on success, HW_ERROR or HW_TIMEOUT on failure
  */
hw_result_t hw_comm_send(const uint8_t *data, uint32_t length, uint32_t timeout_ms);

/**
  * @brief  Receive data via communication interface
  * @param  data: Pointer to receive buffer
  * @param  length: Maximum data length in bytes
  * @param  received: Pointer to actual received length
  * @param  timeout_ms: Timeout in milliseconds (0 = no timeout)
  * @retval hw_result_t: HW_OK on success, HW_ERROR or HW_TIMEOUT on failure
  */
hw_result_t hw_comm_receive(uint8_t *data, uint32_t length, uint32_t *received, uint32_t timeout_ms);

/**
  * @brief  Check if data is available to receive
  * @retval bool: true if data is available, false otherwise
  */
bool hw_comm_data_available(void);

/* ========================================
   GPIO interface (required for status indicator)
   ======================================== */

/**
  * @brief  Initialize GPIO
  * @retval hw_result_t: HW_OK on success, HW_ERROR on failure
  */
hw_result_t hw_gpio_init(void);

hw_result_t hw_gpio_deinit(void);
/**
  * @brief  Set GPIO pin state
  * @param  pin: GPIO pin identifier (platform-specific)
  * @param  state: true for high, false for low
  */
void hw_gpio_set(bool state);

/**
  * @brief  Get GPIO pin state
  * @param  pin: GPIO pin identifier (platform-specific)
  * @retval bool: true if high, false if low
  */
bool hw_gpio_get();

/**
  * @brief  Toggle GPIO pin state
  * @param  pin: GPIO pin identifier (platform-specific)
  */
void hw_gpio_toggle();

/* ========================================
   Memory operations
   ======================================== */

/**
  * @brief  Copy data from source to destination
  * @param  dest: Destination address
  * @param  src: Source address
  * @param  length: Number of bytes to copy
  */
void hw_memcpy(void *dest, const void *src, uint32_t length);

/**
  * @brief  Set memory to a specific value
  * @param  dest: Destination address
  * @param  value: Value to set
  * @param  length: Number of bytes to set
  */
void hw_memset(void *dest, uint8_t value, uint32_t length);

/**
  * @brief  Compare two memory regions
  * @param  ptr1: First memory region
  * @param  ptr2: Second memory region
  * @param  length: Number of bytes to compare
  * @retval int: 0 if equal, non-zero if different
  */
int hw_memcmp(const void *ptr1, const void *ptr2, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __HW_INTERFACE_H */