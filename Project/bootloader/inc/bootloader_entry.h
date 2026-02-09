/**
  **************************************************************************
  * @file     bootloader_entry.h
  * @brief    Helper header for application to enter bootloader
  *           Applications can include this header to request bootloader entry
  **************************************************************************
  */

#ifndef __BOOTLOADER_ENTRY_H
#define __BOOTLOADER_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "core_cm4.h"  /* Include CMSIS core header for SCB and SysTick */
#include "bootloader_config.h"  /* Include centralized configuration */

/* ========================================
   Bootloader entry function
   ======================================== */

/**
  * @brief  Request bootloader entry on next reset
  *         This function sets a magic number in RAM that the bootloader
  *         checks on startup. After calling this function, the system
  *         will reset and enter bootloader mode.
  */
static inline void bootloader_request_entry(void)
{
    /* Bootloader control structure location (last 4 bytes of RAM) */
    volatile uint32_t *bootloader_control = (volatile uint32_t *)BOOTLOADER_CONTROL_ADDRESS;
    
    /* Magic number to request bootloader entry */
    *bootloader_control = BOOTLOADER_MAGIC_ENTER;
    
    /* Reset system to enter bootloader */
    __disable_irq();
    
    /* Wait for completion of memory access */
    __DSB();
    
    /* System reset */
    SCB->AIRCR = (uint32_t)(0x05FA0000UL | (uint32_t)(SCB->AIRCR & 0x0000FF00UL) | 0x04UL);
    
    /* Wait for reset */
    __DSB();
    while(1);
}

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_ENTRY_H */