cmake_minimum_required(VERSION 3.13)

# Set BEFORE project() — required for cross-compilation
# Without these, CMake assumes native (Windows) build and fails
# the compiler test by trying to link with host system libraries.
set(CMAKE_SYSTEM_NAME            Generic)
set(CMAKE_SYSTEM_PROCESSOR       cortex-m3)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")

project(NerdRTOS_STM32F103RCT6 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Target MCU compilling options
set(MCU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

# Compilation warning options
set(WARNING_FLAGS
    -Wall
    -Wextra
    -Wshadow
    -Wundef
    -Wmaybe-uninitialized
)

# Debug/Release optimization level
set(DEBUG_OPT   -Og)
set(RELEASE_OPT -O2)

# Set outputs path
set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Outputs)
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
set(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})

add_subdirectory(Drivers)
add_subdirectory(Users)
