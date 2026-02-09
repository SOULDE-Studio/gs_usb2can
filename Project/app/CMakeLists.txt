add_executable(${CMAKE_PROJECT_NAME}_app)
target_include_directories(${CMAKE_PROJECT_NAME}_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/usb
    ${CMAKE_CURRENT_SOURCE_DIR}/gs_usb
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)
target_sources(${CMAKE_PROJECT_NAME}_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb/usb_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gs_usb/gs_usb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/usb_desc.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/usb_ep.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/usb_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/fdcan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usb_drd_fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32g0xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32g0xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/startup_stm32g0b1xx.s
)
target_link_libraries(${CMAKE_PROJECT_NAME}_app
    stm32cubemx
    STM32_Drivers
    # Add user defined libraries
)
target_link_options(${CMAKE_PROJECT_NAME}_app PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/STM32G0B1XX_FLASH.ld)
