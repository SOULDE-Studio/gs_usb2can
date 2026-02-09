/**
  **************************************************************************
  * @file     app_param.c
  * @brief    Application parameter storage implementation
  *           Implements parameter read/write operations using hardware interface
  **************************************************************************
  */

#include "app_param.h"
#include "hw_interface.h"
#include <string.h>
#include <stdlib.h>

/* ========================================
   Private defines and macros
   ======================================== */

/* Parameter storage header size */
#define APP_PARAM_HEADER_SIZE             (sizeof(app_param_header_t))

/* Calculate CRC32 (simple implementation, can be replaced with hardware CRC) */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    uint32_t polynomial = 0xEDB88320U;
    
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ polynomial;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/* ========================================
   Private function prototypes
   ======================================== */
static bool is_address_in_param_area(uint32_t address);
static app_param_status_t read_header(app_param_header_t *header);
static app_param_status_t verify_header(const app_param_header_t *header, uint32_t data_length);

/* ========================================
   Parameter storage implementation
   ======================================== */

static bool is_address_in_param_area(uint32_t address)
{
    return (address >= PARAM_STORAGE_START && address <= PARAM_STORAGE_END);
}

static app_param_status_t read_header(app_param_header_t *header)
{
    if (header == NULL)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    /* Read header from flash */
    uint32_t address = PARAM_STORAGE_START;
    hw_memcpy(header, (const void *)address, APP_PARAM_HEADER_SIZE);
    
    return APP_PARAM_OK;
}

static app_param_status_t verify_header(const app_param_header_t *header, uint32_t data_length)
{
    if (header == NULL)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    /* Check magic number */
    if (header->magic != APP_PARAM_MAGIC)
    {
        return APP_PARAM_ERROR;
    }
    
    /* Check length */
    if (header->length > app_param_get_max_length() || header->length != data_length)
    {
        return APP_PARAM_SIZE_TOO_LARGE;
    }
    
    return APP_PARAM_OK;
}

app_param_status_t app_param_init(void)
{
    /* Initialize hardware flash interface */
    hw_result_t result = hw_flash_init();
    if (result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    return APP_PARAM_OK;
}

app_param_status_t app_param_deinit(void)
{
    /* Lock flash */
    hw_result_t result = hw_flash_lock();
    if (result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    return APP_PARAM_OK;
}

app_param_status_t app_param_read(uint8_t *data, uint32_t max_length, uint32_t *actual_length)
{
    app_param_header_t header;
    app_param_status_t status;
    uint32_t address;
    
    if (data == NULL)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    /* Read header */
    status = read_header(&header);
    if (status != APP_PARAM_OK)
    {
        if (actual_length != NULL)
        {
            *actual_length = 0;
        }
        return status;
    }
    
    /* Verify header */
    status = verify_header(&header, header.length);
    if (status != APP_PARAM_OK)
    {
        if (actual_length != NULL)
        {
            *actual_length = 0;
        }
        return status;
    }
    
    /* Check if buffer is large enough */
    if (max_length < header.length)
    {
        if (actual_length != NULL)
        {
            *actual_length = 0;
        }
        return APP_PARAM_SIZE_TOO_LARGE;
    }
    
    /* Read parameter data */
    address = PARAM_STORAGE_START + APP_PARAM_HEADER_SIZE;
    
    /* Read data word by word for efficiency */
    uint32_t words_to_read = (header.length + 3) / 4;  /* Round up to word boundary */
    uint32_t *dest = (uint32_t *)data;
    const uint32_t *src = (const uint32_t *)address;
    
    for (uint32_t i = 0; i < words_to_read; i++)
    {
        dest[i] = hw_flash_read_word((uint32_t)(src + i));
    }
    
    /* Verify CRC32 */
    uint32_t calculated_crc = calculate_crc32(data, header.length);
    if (calculated_crc != header.crc32)
    {
        if (actual_length != NULL)
        {
            *actual_length = 0;
        }
        return APP_PARAM_CRC_ERROR;
    }
    
    if (actual_length != NULL)
    {
        *actual_length = header.length;
    }
    
    return APP_PARAM_OK;
}

app_param_status_t app_param_write(const uint8_t *data, uint32_t length)
{
    app_param_header_t header;
    app_param_status_t status;
    uint32_t address;
    hw_result_t hw_result;
    
    if (data == NULL || length == 0)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    /* Check if data fits in parameter area */
    if (length > app_param_get_max_length())
    {
        return APP_PARAM_SIZE_TOO_LARGE;
    }
    
    /* Erase parameter sector first */
    status = app_param_erase();
    if (status != APP_PARAM_OK)
    {
        return status;
    }
    
    /* Calculate CRC32 of data */
    uint32_t crc32 = calculate_crc32(data, length);
    
    /* Prepare header */
    header.magic = APP_PARAM_MAGIC;
    /* Get current version and increment, or start from 1 if invalid */
    uint32_t current_version = app_param_get_version();
    header.version = (current_version > 0) ? (current_version + 1) : 1;
    header.crc32 = crc32;
    header.length = length;
    hw_memset(header.reserved, 0, sizeof(header.reserved));
    
    /* Write header */
    address = PARAM_STORAGE_START;
    hw_result = hw_flash_unlock();
    if (hw_result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    /* Write header word by word */
    const uint32_t *header_words = (const uint32_t *)&header;
    for (uint32_t i = 0; i < (APP_PARAM_HEADER_SIZE / 4); i++)
    {
        hw_result = hw_flash_program_word(address + (i * 4), header_words[i]);
        if (hw_result != HW_OK)
        {
            hw_flash_lock();
            return APP_PARAM_ERROR;
        }
    }
    
    /* Write parameter data */
    address = PARAM_STORAGE_START + APP_PARAM_HEADER_SIZE;
    
    /* Write data word by word */
    uint32_t words_to_write = (length + 3) / 4;  /* Round up to word boundary */
    
    for (uint32_t i = 0; i < words_to_write; i++)
    {
        uint32_t word_data = 0xFFFFFFFFU;  /* Initialize with erased value */
        uint32_t offset = i * 4;
        
        if (offset < length)
        {
            /* Copy actual data bytes */
            uint32_t bytes_to_copy = (length - offset) > 4 ? 4 : (length - offset);
            hw_memcpy(&word_data, &data[offset], bytes_to_copy);
            /* Remaining bytes in word remain 0xFF (erased value) */
        }
        
        hw_result = hw_flash_program_word(address + offset, word_data);
        if (hw_result != HW_OK)
        {
            hw_flash_lock();
            return APP_PARAM_ERROR;
        }
    }
    
    /* Lock flash */
    hw_result = hw_flash_lock();
    if (hw_result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    return APP_PARAM_OK;
}

app_param_status_t app_param_erase(void)
{
    hw_result_t result;
    uint32_t sector_size;
    
    /* Get flash sector size */
    sector_size = hw_flash_get_sector_size();
    
    /* Align address to sector boundary */
    uint32_t sector_address = (PARAM_STORAGE_START / sector_size) * sector_size;
    
    /* Check if sector address is in valid flash range */
    if (!is_address_in_param_area(sector_address) && 
        sector_address < PARAM_STORAGE_START)
    {
        /* If sector starts before parameter area, we need to use parameter area start aligned to sector */
        sector_address = (PARAM_STORAGE_START / sector_size) * sector_size;
    }
    
    /* Unlock flash */
    result = hw_flash_unlock();
    if (result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    /* Erase sector */
    result = hw_flash_erase_sector(sector_address);
    if (result != HW_OK)
    {
        hw_flash_lock();
        return APP_PARAM_ERROR;
    }
    
    /* Lock flash */
    result = hw_flash_lock();
    if (result != HW_OK)
    {
        return APP_PARAM_ERROR;
    }
    
    return APP_PARAM_OK;
}

bool app_param_is_valid(void)
{
    app_param_header_t header;
    
    /* Read header */
    if (read_header(&header) != APP_PARAM_OK)
    {
        return false;
    }
    
    /* Verify header magic and length */
    if (header.magic != APP_PARAM_MAGIC)
    {
        return false;
    }
    
    if (header.length == 0 || header.length > app_param_get_max_length())
    {
        return false;
    }
    
    /* Basic validation passed */
    /* Note: Full CRC verification should be done in app_param_read() */
    return true;
}

uint32_t app_param_get_version(void)
{
    app_param_header_t header;
    
    if (read_header(&header) != APP_PARAM_OK)
    {
        return 0;
    }
    
    if (verify_header(&header, header.length) != APP_PARAM_OK)
    {
        return 0;
    }
    
    return header.version;
}

uint32_t app_param_get_max_length(void)
{
    /* Maximum data length is parameter area size minus header size */
    return PARAM_STORAGE_SIZE - APP_PARAM_HEADER_SIZE;
}

app_param_status_t app_param_read_struct(void *param_struct, uint32_t struct_size)
{
    uint32_t actual_length;
    
    if (param_struct == NULL || struct_size == 0)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    return app_param_read((uint8_t *)param_struct, struct_size, &actual_length);
}

app_param_status_t app_param_write_struct(const void *param_struct, uint32_t struct_size)
{
    if (param_struct == NULL || struct_size == 0)
    {
        return APP_PARAM_INVALID_PARAM;
    }
    
    if (struct_size > app_param_get_max_length())
    {
        return APP_PARAM_SIZE_TOO_LARGE;
    }
    
    return app_param_write((const uint8_t *)param_struct, struct_size);
}