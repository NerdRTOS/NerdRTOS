cmake_minimum_required(VERSION 3.13)

include(bsp/raspi-pico2/pico_sdk_import.cmake)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(nerd C CXX ASM)

set(PROJECT_ROOT                ${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH   ${PROJECT_ROOT}/cmake)
if(NOT BUILD_CONFIG)
    set(BUILD_CONFIG pico2)
endif()

include(kconfig)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if(CONFIG_BSP_RASPI_PICO2)
    add_subdirectory(bsp/raspi-pico2)
else()
    message(FATAL_ERROR "No supported BSP selected")
endif()

