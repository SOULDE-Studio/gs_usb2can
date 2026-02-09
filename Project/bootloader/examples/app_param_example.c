/**
  **************************************************************************
  * @file     app_param_example.c
  * @brief    Example usage of application parameter storage interface
  *           This file demonstrates how to use the parameter storage API
  **************************************************************************
  */

#include "app_param.h"

/* Example parameter structure */
typedef struct
{
    uint32_t device_id;
    uint32_t baudrate;
    uint8_t  mode;
    uint8_t  reserved[3];
    float    calibration_value;
} app_params_t;

/**
  * @brief  Example: Initialize and read parameters
  */
void example_read_parameters(void)
{
    app_param_status_t status;
    app_params_t params;
    
    /* Initialize parameter storage */
    status = app_param_init();
    if (status != APP_PARAM_OK)
    {
        /* Handle error */
        return;
    }
    
    /* Read parameters as structure */
    status = app_param_read_struct(&params, sizeof(app_params_t));
    if (status == APP_PARAM_OK)
    {
        /* Parameters read successfully */
        /* Use parameters... */
        (void)params.device_id;
        (void)params.baudrate;
        (void)params.mode;
        (void)params.calibration_value;
    }
    else if (status == APP_PARAM_CRC_ERROR)
    {
        /* CRC error - parameters may be corrupted */
        /* Use default values */
    }
    else
    {
        /* No valid parameters - use default values */
    }
    
    /* Deinitialize parameter storage */
    app_param_deinit();
}

/**
  * @brief  Example: Write parameters
  */
void example_write_parameters(void)
{
    app_param_status_t status;
    app_params_t params;
    
    /* Initialize parameter storage */
    status = app_param_init();
    if (status != APP_PARAM_OK)
    {
        /* Handle error */
        return;
    }
    
    /* Prepare parameters */
    params.device_id = 0x12345678;
    params.baudrate = 115200;
    params.mode = 1;
    params.reserved[0] = 0;
    params.reserved[1] = 0;
    params.reserved[2] = 0;
    params.calibration_value = 1.234f;
    
    /* Write parameters as structure */
    status = app_param_write_struct(&params, sizeof(app_params_t));
    if (status == APP_PARAM_OK)
    {
        /* Parameters written successfully */
    }
    else if (status == APP_PARAM_SIZE_TOO_LARGE)
    {
        /* Parameter structure is too large */
    }
    else
    {
        /* Write error */
    }
    
    /* Deinitialize parameter storage */
    app_param_deinit();
}

/**
  * @brief  Example: Read/write raw data
  */
void example_raw_data_operations(void)
{
    app_param_status_t status;
    uint8_t read_buffer[256];
    uint8_t write_buffer[256] = "Example parameter data";
    uint32_t actual_length;
    
    /* Initialize parameter storage */
    status = app_param_init();
    if (status != APP_PARAM_OK)
    {
        /* Handle error */
        return;
    }
    
    /* Write raw data */
    status = app_param_write(write_buffer, sizeof(write_buffer));
    if (status == APP_PARAM_OK)
    {
        /* Data written successfully */
    }
    
    /* Read raw data */
    status = app_param_read(read_buffer, sizeof(read_buffer), &actual_length);
    if (status == APP_PARAM_OK)
    {
        /* Data read successfully, actual_length contains actual data length */
    }
    
    /* Deinitialize parameter storage */
    app_param_deinit();
}

/**
  * @brief  Example: Check parameter validity and version
  */
void example_check_parameters(void)
{
    bool is_valid;
    uint32_t version;
    uint32_t max_length;
    
    /* Initialize parameter storage */
    app_param_init();
    
    /* Check if parameters are valid */
    is_valid = app_param_is_valid();
    if (is_valid)
    {
        /* Parameters are valid */
        version = app_param_get_version();
        /* Use version number... */
        (void)version;
    }
    
    /* Get maximum parameter length */
    max_length = app_param_get_max_length();
    /* Use max_length to allocate buffer... */
    (void)max_length;
    
    /* Deinitialize parameter storage */
    app_param_deinit();
}

/**
  * @brief  Example: Erase parameters
  */
void example_erase_parameters(void)
{
    app_param_status_t status;
    
    /* Initialize parameter storage */
    status = app_param_init();
    if (status != APP_PARAM_OK)
    {
        /* Handle error */
        return;
    }
    
    /* Erase parameter storage area */
    status = app_param_erase();
    if (status == APP_PARAM_OK)
    {
        /* Parameters erased successfully */
    }
    
    /* Deinitialize parameter storage */
    app_param_deinit();
}