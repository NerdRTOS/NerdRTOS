cmake_minimum_required(VERSION 3.13)

set(CMAKE_SYSTEM_NAME             Generic)
set(CMAKE_SYSTEM_PROCESSOR        cortex-m7)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")

project(NerdRTOS_NUCLEO_H753ZI LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MCU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
)

set(WARNING_FLAGS
    -Wall
    -Wextra
    -Wshadow
    # -Wundef omitted: nd_def.h evaluates ND_CFG_DEBUG without including nd_config.h
    -Wmaybe-uninitialized
)

set(DEBUG_OPT   -Og)
set(RELEASE_OPT -O2)

set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Outputs)
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
set(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})

add_subdirectory(Drivers)
add_subdirectory(Users)
