set(BOOTLOADER_VERSION_MAJOR 1)
set(BOOTLOADER_VERSION_MINOR 0)
set(BOOTLOADER_ID 0)

message("Bootloader version: ${BOOTLOADER_VERSION_MAJOR}.${BOOTLOADER_VERSION_MINOR}")
message("Product ID: " 0x${BOOTLOADER_ID}${PRODUCT_SERIES_ID}${PRODUCT_ID})

add_executable(${CMAKE_PROJECT_NAME}_bootloader)

target_include_directories(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32G0B1XX_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target)

target_sources(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usb_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_desc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_cdc_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_ctlreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_ioreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_cdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32g0xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32g0xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hw_interface_stm32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/download_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bootloader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_SOURCE_DIR}/startup_stm32g0b1xx.s
)
target_link_options(${CMAKE_PROJECT_NAME}_bootloader PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/STM32G0B1XX_FLASH.ld)

target_compile_definitions(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    BOOTLOADER_VERSION_MAJOR=${BOOTLOADER_VERSION_MAJOR}
    BOOTLOADER_VERSION_MINOR=${BOOTLOADER_VERSION_MINOR}
    BOOTLOADER_PRODUCT_ID=0x${BOOTLOADER_ID}${PRODUCT_SERIES_ID}${PRODUCT_ID})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME}_bootloader
    stm32cubemx
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES})
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}_bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
    $<TARGET_FILE:${CMAKE_PROJECT_NAME}_bootloader>
    $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}_bootloader>/$<TARGET_FILE_BASE_NAME:${CMAKE_PROJECT_NAME}_bootloader>.bin
    COMMENT "Generating ${CMAKE_PROJECT_NAME}_bootloader.bin"
)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}_bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
    $<TARGET_FILE:${CMAKE_PROJECT_NAME}_bootloader>
    $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}_bootloader>/$<TARGET_FILE_BASE_NAME:${CMAKE_PROJECT_NAME}_bootloader>.hex
)
