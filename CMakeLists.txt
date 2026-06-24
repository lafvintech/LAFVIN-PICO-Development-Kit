cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w CACHE STRING "Board type")
set_property(CACHE PICO_BOARD PROPERTY STRINGS pico_w pico2_w)

if (NOT PICO_BOARD STREQUAL "pico_w" AND NOT PICO_BOARD STREQUAL "pico2_w")
    message(FATAL_ERROR "Unsupported PICO_BOARD '${PICO_BOARD}'. This project now supports only pico_w and pico2_w.")
endif()

option(BUTTON_ACTIVE_LOW "Use falling-edge active-low GPIO buttons" OFF)
option(PICO2W_LCD_SMOKE_TEST "Fill the LCD with solid colors before starting FreeRTOS" OFF)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config/freertos
)

include(pico_sdk_import.cmake)

if (PICO_BOARD STREQUAL "pico2_w")
    if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 or later is required for Pico 2 W. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()
else()
    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 or later is required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()
    include(cmake/FreeRTOS_PicoW.cmake)
endif()

project(hello_world C CXX ASM)

pico_sdk_init()

if (PICO_BOARD STREQUAL "pico2_w")
    include(cmake/FreeRTOS_Pico2W.cmake)
endif()

add_compile_definitions(LV_LVGL_H_INCLUDE_SIMPLE=1)
add_subdirectory(components/lvgl)

file(GLOB_RECURSE LVGL_DEMO_SOURCES CONFIGURE_DEPENDS
    components/lvgl/demos/*.c
)

set(APP_SOURCES
    app/main.c
    app/assets/sea.c
)

set(DRIVER_SOURCES
    drivers/display/st7796.c
    drivers/touch/gt911.c
)

set(LVGL_PORT_SOURCES
    ports/lvgl/lv_port_disp.c
    ports/lvgl/lv_port_indev.c
)

add_executable(hello_world
    ${APP_SOURCES}
    ${DRIVER_SOURCES}
    ${LVGL_PORT_SOURCES}
    ${LVGL_DEMO_SOURCES}
)

pico_generate_pio_header(hello_world
    ${CMAKE_CURRENT_LIST_DIR}/drivers/rgb/ws2812.pio
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)

pico_set_program_name(hello_world "hello_world")
pico_set_program_version(hello_world "0.1")

pico_enable_stdio_uart(hello_world 1)
pico_enable_stdio_usb(hello_world 0)

target_include_directories(hello_world PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/app
    ${CMAKE_CURRENT_LIST_DIR}/config/freertos
    ${CMAKE_CURRENT_LIST_DIR}/drivers/display
    ${CMAKE_CURRENT_LIST_DIR}/drivers/touch
    ${CMAKE_CURRENT_LIST_DIR}/ports/lvgl
    ${CMAKE_CURRENT_LIST_DIR}/components/lvgl
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(hello_world
    pico_stdlib
    hardware_spi
    hardware_adc
    hardware_i2c
    hardware_pio
    hardware_dma
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    pico_multicore
    lvgl
    lvgl_demos
)

target_compile_definitions(hello_world PRIVATE
    mainRUN_FREE_RTOS_ON_CORE=0
    PICO_STACK_SIZE=0x1000
    PICO_STDIO_STACK_BUFFER_SIZE=64
)

if (PICO_BOARD STREQUAL "pico2_w")
    target_compile_options(hello_world PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-include>
        $<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_LIST_DIR}/config/freertos/freertos_rp2350_irq_compat.h>
    )
endif()

if (BUTTON_ACTIVE_LOW)
    target_compile_definitions(hello_world PRIVATE BUTTON_ACTIVE_LOW=1)
endif()

if (PICO2W_LCD_SMOKE_TEST)
    target_compile_definitions(hello_world PRIVATE PICO2W_LCD_SMOKE_TEST=1)
endif()

pico_add_extra_outputs(hello_world)
