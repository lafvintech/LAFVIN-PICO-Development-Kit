# FreeRTOS-Kernel import for Pico W / Pico WH.
#
# This keeps the original project FreeRTOS checkout for the RP2040-based boards.

if (NOT DEFINED FREERTOS_KERNEL_PATH)
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/components/FreeRTOS")
endif()

if (DEFINED ENV{FREERTOS_KERNEL_PATH} AND (NOT FREERTOS_KERNEL_PATH))
    set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
    message("Using FREERTOS_KERNEL_PATH from environment ('${FREERTOS_KERNEL_PATH}')")
endif()

set(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" CACHE PATH "Path to the FreeRTOS Kernel")
get_filename_component(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS "${FREERTOS_KERNEL_PATH}")
    message(FATAL_ERROR "Directory '${FREERTOS_KERNEL_PATH}' not found")
endif()

set(FREERTOS_KERNEL_RP2040_RELATIVE_PATH "portable/ThirdParty/GCC/RP2040")

if (NOT EXISTS "${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_RP2040_RELATIVE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "FreeRTOS-Kernel at '${FREERTOS_KERNEL_PATH}' does not contain the Pico RP2040 port: "
        "${FREERTOS_KERNEL_RP2040_RELATIVE_PATH}")
endif()

message("Using legacy FreeRTOS-Kernel for Pico W: ${FREERTOS_KERNEL_PATH}")

set(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" CACHE PATH "Path to the FreeRTOS Kernel" FORCE)

add_subdirectory(
    "${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_RP2040_RELATIVE_PATH}"
    FREERTOS_KERNEL
)
