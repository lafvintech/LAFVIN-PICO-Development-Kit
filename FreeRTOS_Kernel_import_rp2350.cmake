# FreeRTOS-Kernel import for Pico 2 W / RP2350.
#
# Pico/Pico W keep using the original bundled FreeRTOS submodule. Pico 2 W uses
# a newer Raspberry Pi FreeRTOS-Kernel checkout so the Pico-specific SMP port can
# match RP2350 SDK headers and interrupt naming.

if (NOT DEFINED FREERTOS_KERNEL_RP2350_PATH)
    set(FREERTOS_KERNEL_RP2350_PATH "${CMAKE_CURRENT_LIST_DIR}/components/FreeRTOS-RaspberryPi")
endif()

if (DEFINED ENV{FREERTOS_KERNEL_RP2350_PATH})
    set(FREERTOS_KERNEL_RP2350_PATH $ENV{FREERTOS_KERNEL_RP2350_PATH})
    message("Using FREERTOS_KERNEL_RP2350_PATH from environment ('${FREERTOS_KERNEL_RP2350_PATH}')")
endif()

set(FREERTOS_KERNEL_RP2350_PATH "${FREERTOS_KERNEL_RP2350_PATH}" CACHE PATH "Path to Raspberry Pi FreeRTOS-Kernel for RP2350")
get_filename_component(FREERTOS_KERNEL_RP2350_PATH "${FREERTOS_KERNEL_RP2350_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS "${FREERTOS_KERNEL_RP2350_PATH}")
    message(FATAL_ERROR
        "Pico 2 W requires a newer Raspberry Pi FreeRTOS-Kernel checkout.\n"
        "Clone https://github.com/raspberrypi/FreeRTOS-Kernel.git to:\n"
        "  ${CMAKE_CURRENT_LIST_DIR}/components/FreeRTOS-RaspberryPi\n"
        "or configure with:\n"
        "  -DFREERTOS_KERNEL_RP2350_PATH=/path/to/raspberrypi/FreeRTOS-Kernel")
endif()

set(FREERTOS_KERNEL_RP2350_PORT_PATH "portable/ThirdParty/GCC/RP2040")

if (NOT EXISTS "${FREERTOS_KERNEL_RP2350_PATH}/${FREERTOS_KERNEL_RP2350_PORT_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "Raspberry Pi FreeRTOS-Kernel at '${FREERTOS_KERNEL_RP2350_PATH}' does not contain "
        "the Pico SMP port: ${FREERTOS_KERNEL_RP2350_PORT_PATH}")
endif()

message("Using Raspberry Pi FreeRTOS-Kernel for Pico 2 W: ${FREERTOS_KERNEL_RP2350_PATH}")

# The Raspberry Pi Pico port CMake expects FREERTOS_KERNEL_PATH to name the
# kernel root. This branch is only included for pico2_w, so it does not affect
# the legacy Pico/Pico W import path.
set(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_RP2350_PATH}" CACHE PATH "Path to FreeRTOS Kernel" FORCE)

add_subdirectory(
    "${FREERTOS_KERNEL_RP2350_PATH}/${FREERTOS_KERNEL_RP2350_PORT_PATH}"
    FREERTOS_KERNEL_RP2350
)
