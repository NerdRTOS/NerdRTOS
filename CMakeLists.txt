cmake_minimum_required(VERSION 3.13)

set(NERD_NEEDS_PICO_SDK TRUE)
if(NOT BOARD)
    if(BUILD_CONFIG)
        if("${BUILD_CONFIG}" MATCHES "^pico2")
            message(FATAL_ERROR
                "BUILD_CONFIG=${BUILD_CONFIG} is retired for Pico2. "
                "Use -DBOARD=pico2/rp2350a/m33 or -DBOARD=pico2/rp2350a/hazard3 instead."
            )
        endif()
        message(FATAL_ERROR
            "Legacy BUILD_CONFIG=${BUILD_CONFIG} is not supported by the Zephyr-style top-level build. "
            "Use an explicit -DBOARD=<board-target>."
        )
    endif()
    message(FATAL_ERROR
        "BOARD is required. Use -DBOARD=pico2/rp2350a/m33 or "
        "-DBOARD=pico2/rp2350a/hazard3."
    )
endif()

set(NERD_BOARD_HINT ${BOARD})
if("${NERD_BOARD_HINT}" STREQUAL "qemu_a9"
   OR "${NERD_BOARD_HINT}" MATCHES "^qemu_a9/")
    set(NERD_NEEDS_PICO_SDK FALSE)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    if(NOT CMAKE_C_COMPILER)
        set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    endif()
    if(NOT CMAKE_ASM_COMPILER)
        set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
    endif()
    if(NOT CMAKE_OBJCOPY)
        set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
    endif()
elseif("${NERD_BOARD_HINT}" STREQUAL "stm32f103rct6"
   OR "${NERD_BOARD_HINT}" MATCHES "^stm32f103rct6/")
    set(NERD_NEEDS_PICO_SDK FALSE)
elseif("${NERD_BOARD_HINT}" STREQUAL "pico2"
       OR "${NERD_BOARD_HINT}" MATCHES "^pico2/")
    include(boards/raspberrypi/pico2/pre_cmake.cmake)
endif()
if(NERD_NEEDS_PICO_SDK)
    include(bsp/raspi-pico2/pico_sdk_import.cmake)
endif()
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NERD_NEEDS_PICO_SDK)
    project(nerd C CXX ASM)
else()
    project(nerd C ASM)
endif()

set(PROJECT_ROOT                ${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH   ${PROJECT_ROOT}/cmake)
if(NOT NERD_SAMPLE_DIR)
    if("${NERD_BOARD_HINT}" STREQUAL "qemu_a9"
       OR "${NERD_BOARD_HINT}" MATCHES "^qemu_a9/")
        set(NERD_SAMPLE_DIR ${PROJECT_ROOT}/samples/qemu_shell)
    else()
        set(NERD_SAMPLE_DIR ${PROJECT_ROOT}/samples/pico_shell)
    endif()
endif()
include(kconfig)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(CONFIG_BSP_RASPI_PICO2)
    if(NOT NERD_NEEDS_PICO_SDK)
        message(FATAL_ERROR "Pico BSP selected but Pico SDK import was skipped")
    endif()
    pico_sdk_init()
endif()

if(CONFIG_BOARD_TARGET_MODEL)
    if(CONFIG_ARCH_ARM_M33 OR CONFIG_ARCH_ARM_M3)
        set(NERD_ARCH_DIR ${PROJECT_ROOT}/arch/arm/cortex_m)
    elseif(CONFIG_ARCH_ARM_A9)
        set(NERD_ARCH_DIR ${PROJECT_ROOT}/arch/arm/cortex_a)
    elseif(CONFIG_ARCH_RISCV_RV32)
        set(NERD_ARCH_DIR ${PROJECT_ROOT}/arch/riscv/rv32)
    else()
        message(FATAL_ERROR "No architecture layer selected by Kconfig")
    endif()

    if(CONFIG_SOC_RP2350A_M33 OR CONFIG_SOC_RP2350A_HAZARD3 OR CONFIG_SOC_RP2350A)
        set(NERD_SOC_DIR ${PROJECT_ROOT}/soc/raspberrypi/rp2350)
    elseif(CONFIG_SOC_QEMU_CORTEX_A9)
        set(NERD_SOC_DIR ${PROJECT_ROOT}/soc/qemu/cortex_a9)
    elseif(CONFIG_SOC_STM32F103)
        set(NERD_SOC_DIR ${PROJECT_ROOT}/soc/st/stm32f1)
    else()
        message(FATAL_ERROR "No SoC layer selected by Kconfig")
    endif()

    if(CONFIG_BOARD_PICO2)
        set(NERD_BOARD_DIR ${PROJECT_ROOT}/boards/raspberrypi/pico2)
    elseif(CONFIG_BOARD_QEMU_A9)
        set(NERD_BOARD_DIR ${PROJECT_ROOT}/boards/qemu/qemu_a9)
    elseif(CONFIG_BOARD_STM32F103RCT6_M3_ALIENTEK)
        set(NERD_BOARD_DIR ${PROJECT_ROOT}/boards/st/stm32f103rct6_m3_alientek)
    else()
        message(FATAL_ERROR "No board layer selected by Kconfig")
    endif()

    add_subdirectory(${NERD_ARCH_DIR} ${PROJECT_BINARY_DIR}/arch-layer)
    add_subdirectory(${NERD_SOC_DIR} ${PROJECT_BINARY_DIR}/soc-layer)
    add_subdirectory(${NERD_BOARD_DIR} ${PROJECT_BINARY_DIR}/board-layer)
    add_subdirectory(${NERD_SAMPLE_DIR} ${PROJECT_BINARY_DIR}/sample-layer)

    if(NERD_BOARD_LEGACY_DIR)
        add_subdirectory(${NERD_BOARD_LEGACY_DIR} ${PROJECT_BINARY_DIR}/legacy-bsp)
    else()
        message(FATAL_ERROR "Selected board has no build backend yet")
    endif()
elseif(CONFIG_BSP_RASPI_PICO2)
    add_subdirectory(bsp/raspi-pico2)
else()
    message(FATAL_ERROR "No supported BSP selected")
endif()
