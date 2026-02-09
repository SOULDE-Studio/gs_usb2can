/**
  **************************************************************************
  * @file     app_param.h
  * @brief    Application parameter storage interface
  *           Provides interface for reading and writing parameters to flash
  *           Parameter storage area is located at the last 1KB of flash
  **************************************************************************
  */

#ifndef __APP_PARAM_H
#define __APP_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "bootloader_config.h"

/* ========================================
   Parameter storage status codes
   ======================================== */
typedef enum
{
    APP_PARAM_OK = 0,
    APP_PARAM_ERROR = 1,
    APP_PARAM_INVALID_PARAM = 2,
    APP_PARAM_SIZE_TOO_LARGE = 3,
    APP_PARAM_NOT_INITIALIZED = 4,
    APP_PARAM_CRC_ERROR = 5
} app_param_status_t;

/* ========================================
   Parameter storage structure
   ======================================== */

/**
  * @brief  Parameter storage header structure
  *         This structure is placed at the beginning of parameter storage area
  */
typedef struct
{
    uint32_t magic;         /* Magic number to identify valid parameter data */
    uint32_t version;       /* Parameter version number */
    uint32_t crc32;         /* CRC32 checksum of parameter data */
    uint32_t length;        /* Length of parameter data (excluding header) */
    uint32_t reserved[4];   /* Reserved for future use */
} app_param_header_t;

/* Magic number for parameter storage */
#define APP_PARAM_MAGIC                   (0x50415241U)  /* "PARA" in ASCII */

/* ========================================
   Parameter storage function prototypes
   ======================================== */

/**
  * @brief  Initialize parameter storage
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_init(void);

/**
  * @brief  Deinitialize parameter storage
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_deinit(void);

/**
  * @brief  Read parameters from flash
  * @param  data: Pointer to buffer to store read data
  * @param  max_length: Maximum length of buffer
  * @param  actual_length: Pointer to actual length of read data (can be NULL)
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_read(uint8_t *data, uint32_t max_length, uint32_t *actual_length);

/**
  * @brief  Write parameters to flash
  * @param  data: Pointer to data to write
  * @param  length: Length of data to write
  * @retval app_param_status_t: APP_PARAM_OK on success
  * @note   This function will erase the parameter sector if necessary
  */
app_param_status_t app_param_write(const uint8_t *data, uint32_t length);

/**
  * @brief  Erase parameter storage area
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_erase(void);

/**
  * @brief  Check if parameter storage is valid
  * @retval bool: true if valid, false otherwise
  */
bool app_param_is_valid(void);

/**
  * @brief  Get parameter storage version
  * @retval uint32_t: Version number, 0 if not valid
  */
uint32_t app_param_get_version(void);

/**
  * @brief  Get maximum parameter data length (excluding header)
  * @retval uint32_t: Maximum data length in bytes
  */
uint32_t app_param_get_max_length(void);

/**
  * @brief  Read parameter as structure (helper function)
  * @param  param_struct: Pointer to structure to fill
  * @param  struct_size: Size of structure
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_read_struct(void *param_struct, uint32_t struct_size);

/**
  * @brief  Write parameter as structure (helper function)
  * @param  param_struct: Pointer to structure to write
  * @param  struct_size: Size of structure
  * @retval app_param_status_t: APP_PARAM_OK on success
  */
app_param_status_t app_param_write_struct(const void *param_struct, uint32_t struct_size);

#ifdef __cplusplus
}
#endif

#endif /* __APP_PARAM_H */