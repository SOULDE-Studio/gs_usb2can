/**
  **************************************************************************
  * @file     hw_interface_stm32.c
  * @brief    STM32 platform implementation of hardware interface
  *           This file implements all hardware abstraction functions
  *           for STM32F103 platform using HAL and LL libraries.
  **************************************************************************
  */

#include <string.h>

#include "bootloader_config.h"
#include "gpio.h"
#include "hw_interface.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_flash.h"
#include "stm32g0xx_ll_gpio.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"


/* ========================================
   Flash operations implementation
   ======================================== */

hw_result_t hw_flash_init(void) {
    /* Clear any pending flash flags */

    return HW_OK;
}

hw_result_t hw_flash_deinit(void) {
    /* Lock flash to prevent accidental writes */
    HAL_FLASH_Lock();
    return HW_OK;
}

hw_result_t hw_flash_unlock(void) {
    /* Unlock flash */
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK) { return HW_ERROR; }
    return HW_OK;
}

hw_result_t hw_flash_lock(void) {
    HAL_FLASH_Lock();
    return HW_OK;
}

hw_result_t hw_flash_erase_sector(uint32_t address) {
    /* Check address alignment */
    if ((address & (BOOTLOADER_FLASH_PAGE_SIZE - 1)) != 0) { return HW_INVALID_PARAM; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > APPLICATION_FLASH_END) { return HW_INVALID_PARAM; }

    /* Unlock flash */
    if (hw_flash_unlock() != HW_OK) { return HW_ERROR; }

    /* Configure erase operation */
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Page = (address - BOOTLOADER_FLASH_BASE) / BOOTLOADER_FLASH_PAGE_SIZE;
    erase_init.NbPages = 1;

    /* Erase page */
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) { return HW_ERROR; }

    return HW_OK;
}

hw_result_t hw_flash_erase_range(uint32_t start_address, uint32_t end_address) {
    uint32_t current_address;
    hw_result_t result;

    /* Check address range */
    if (start_address < PARAM_STORAGE_START || end_address > APPLICATION_FLASH_END || start_address >= end_address) {
        return HW_INVALID_PARAM;
    }

    /* Align start address to page boundary */
    current_address = (start_address / BOOTLOADER_FLASH_PAGE_SIZE) * BOOTLOADER_FLASH_PAGE_SIZE;

    /* Erase all pages in range */
    while (current_address < end_address) {
        result = hw_flash_erase_sector(current_address);
        if (result != HW_OK) { return result; }
        current_address += BOOTLOADER_FLASH_PAGE_SIZE;
    }

    return HW_OK;
}

static hw_result_t hw_flash_program_doubleword(uint32_t address, uint64_t data) {
    /* Check address alignment (64-bit) */
    if ((address & 0x7) != 0) { return HW_INVALID_PARAM; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > (APPLICATION_FLASH_END - 7U)) { return HW_INVALID_PARAM; }

    /* Unlock flash */
    if (hw_flash_unlock() != HW_OK) { return HW_ERROR; }

    /* Program doubleword (STM32G0) */
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    if (status != HAL_OK) { return HW_ERROR; }

    return HW_OK;
}

hw_result_t hw_flash_program_word(uint32_t address, uint32_t data) {
    uint32_t aligned_address = address & ~0x7U;
    uint32_t word_offset = address & 0x7U;
    uint64_t existing_data;
    uint64_t new_data;

    /* Check address alignment (32-bit) */
    if ((address & 0x3U) != 0U) { return HW_INVALID_PARAM; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > (APPLICATION_FLASH_END - 3U)) { return HW_INVALID_PARAM; }

    existing_data = *(volatile uint64_t *) aligned_address;
    if (word_offset == 0U) {
        new_data = (existing_data & 0xFFFFFFFF00000000ULL) | (uint64_t) data;
    } else {
        new_data = (existing_data & 0x00000000FFFFFFFFULL) | ((uint64_t) data << 32);
    }

    return hw_flash_program_doubleword(aligned_address, new_data);
}

hw_result_t hw_flash_program_halfword(uint32_t address, uint16_t data) {
    uint32_t aligned_address = address & ~0x7U;
    uint32_t halfword_index = (address & 0x7U) >> 1;
    uint64_t existing_data;
    uint64_t new_data;
    uint64_t mask = 0xFFFFULL << (halfword_index * 16U);

    /* Check address alignment */
    if ((address & 0x1U) != 0U) { return HW_INVALID_PARAM; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > (APPLICATION_FLASH_END - 1U)) { return HW_INVALID_PARAM; }

    existing_data = *(volatile uint64_t *) aligned_address;
    new_data = (existing_data & ~mask) | ((uint64_t) data << (halfword_index * 16U));

    return hw_flash_program_doubleword(aligned_address, new_data);
}

// STM32G0 doesn't support byte programming directly
hw_result_t hw_flash_program_byte(uint32_t address, uint8_t data) {
    uint32_t aligned_address = address & ~0x7U;
    uint32_t byte_index = address & 0x7U;
    uint64_t existing_data;
    uint64_t new_data;
    uint64_t mask = 0xFFULL << (byte_index * 8U);

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > APPLICATION_FLASH_END) { return HW_INVALID_PARAM; }

    existing_data = *(volatile uint64_t *) aligned_address;
    new_data = (existing_data & ~mask) | ((uint64_t) data << (byte_index * 8U));

    return hw_flash_program_doubleword(aligned_address, new_data);
}

uint32_t hw_flash_read_word(uint32_t address) {
    /* Check address alignment */
    if ((address & 0x3) != 0) { return 0; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > (APPLICATION_FLASH_END - 3U)) { return 0; }

    /* Read directly from flash memory */
    return *(volatile uint32_t *) address;
}

uint16_t hw_flash_read_halfword(uint32_t address) {
    /* Check address alignment */
    if ((address & 0x1) != 0) { return 0; }

    /* Check address range */
    if (address < PARAM_STORAGE_START || address > (APPLICATION_FLASH_END - 1U)) { return 0; }

    /* Read directly from flash memory */
    return *(volatile uint16_t *) address;
}

uint8_t hw_flash_read_byte(uint32_t address) {
    /* Check address range */
    if (address < PARAM_STORAGE_START || address > APPLICATION_FLASH_END) { return 0; }

    /* Read directly from flash memory */
    return *(volatile uint8_t *) address;
}

/* ========================================
   System operations implementation
   ======================================== */
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
   */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV5;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    /** Initializes the CPU, AHB and APB buses clocks
   */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

hw_result_t hw_system_init(void) {
    /* System already initialized by startup code, but we can configure here if needed */
    HAL_Init();
    SystemClock_Config();
    return HW_OK;
}

hw_result_t hw_system_deinit(void) {
    /* Deinitialize system peripherals if needed */
    HAL_DeInit();
    return HW_OK;
}

void hw_delay_ms(uint32_t ms) {
    /* Simple delay implementation using system tick */
    /* This is a basic implementation, can be improved with systick */
    HAL_Delay(ms);
}

void hw_system_reset(void) {
    /* Use STM32 reset function */
    NVIC_SystemReset();
}

void hw_jump_to_application(uint32_t address) {
    /* Function pointer type for application entry */
    typedef void (*app_entry_t)(void);
    app_entry_t app_entry;

    /* Check if address is valid (must be in flash range and word-aligned) */
    if (address < BOOTLOADER_FLASH_BASE || address > APPLICATION_FLASH_END || (address & 0x3) != 0) { return; }

    /* Disable all interrupts */
    __disable_irq();

    /* Reset SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    {
        uint32_t reg_count = (uint32_t) (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]));
        for (uint32_t i = 0; i < reg_count; i++) {
            NVIC->ICER[i] = 0xFFFFFFFFU;
            NVIC->ICPR[i] = 0xFFFFFFFFU;
        }
    }
    /* Set stack pointer from application vector table */
    __set_MSP(*(volatile uint32_t *) address);
    SCB->VTOR = address;
    __enable_irq();
    /* Get application entry point (address + 4) */
    app_entry = (app_entry_t) (*(volatile uint32_t *) (address + 4));

    /* Jump to application */
    app_entry();

    /* Should never reach here */
    while (1);
}

/* ========================================
   Communication interface implementation
   ======================================== */

hw_result_t hw_comm_init(void) {
    /* Placeholder for communication initialization */
    /* To be implemented based on specific communication interface (UART/USB/etc.) */
    MX_USB_Device_Init();
    return HW_OK;
}

hw_result_t hw_comm_deinit(void) {
    /* Placeholder for communication deinitialization */

    return HW_OK;
}

hw_result_t hw_comm_send(const uint8_t *data, uint32_t length, uint32_t timeout_ms) {
    /* Placeholder for communication send */
    /* To be implemented based on specific communication interface */
    CDC_Transmit_FS((uint8_t *) data, length);
    return HW_OK;
}

hw_result_t hw_comm_receive(uint8_t *data, uint32_t length, uint32_t *received, uint32_t timeout_ms) {
    /* Placeholder for communication receive */
    /* To be implemented based on specific communication interface */
    (void) data;
    (void) length;
    (void) received;
    (void) timeout_ms;
    return HW_OK;
}

bool hw_comm_data_available(void) {
    /* Placeholder for checking data availability */
    return false;
}

/* ========================================
   GPIO interface implementation
   ======================================== */

hw_result_t hw_gpio_init(void) {
    /* Enable GPIOC clock */
    MX_GPIO_Init();
    return HW_OK;
}
hw_result_t hw_gpio_deinit(void) {
    /* Deinitialize GPIO */
    HAL_GPIO_DeInit(BOOTLOADER_STATUS_LED_PORT, BOOTLOADER_STATUS_LED_PIN);
    return HW_OK;
}
void hw_gpio_set(bool state) {
    /* pin parameter is not used, we use the configured LED pin */
    HAL_GPIO_WritePin(BOOTLOADER_STATUS_LED_PORT, BOOTLOADER_STATUS_LED_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool hw_gpio_get() {
    /* pin parameter is not used, we use the configured LED pin */
    return HAL_GPIO_ReadPin(BOOTLOADER_STATUS_LED_PORT, BOOTLOADER_STATUS_LED_PIN) == GPIO_PIN_SET ? true : false;
}

void hw_gpio_toggle() {
    /* Toggle LED pin */
    HAL_GPIO_TogglePin(BOOTLOADER_STATUS_LED_PORT, BOOTLOADER_STATUS_LED_PIN);
}

/* ========================================
   Memory operations implementation
   ======================================== */

void hw_memcpy(void *dest, const void *src, uint32_t length) { memcpy(dest, src, length); }

void hw_memset(void *dest, uint8_t value, uint32_t length) { memset(dest, value, length); }

int hw_memcmp(const void *ptr1, const void *ptr2, uint32_t length) { return memcmp(ptr1, ptr2, length); }
