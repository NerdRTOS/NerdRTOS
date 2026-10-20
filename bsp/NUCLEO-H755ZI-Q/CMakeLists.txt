cmake_minimum_required(VERSION 3.13)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

project(NerdRTOS_NUCLEO_H755ZI_Q LANGUAGES C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(STM32_DRIVERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Drivers" CACHE PATH
    "STM32CubeH7 v1.13.0 Drivers directory")
set(DEVICE_PATH "${STM32_DRIVERS_DIR}/CMSIS/Device/ST/STM32H7xx")
set(STARTUP "${DEVICE_PATH}/Source/Templates/gcc/startup_stm32h755xx.s")
set(SYSTEM_SOURCE
    "${DEVICE_PATH}/Source/Templates/system_stm32h7xx_dualcore_boot_cm4_cm7.c")
foreach(required IN ITEMS "${STARTUP}" "${SYSTEM_SOURCE}"
    "${STM32_DRIVERS_DIR}/CMSIS/Include/core_cm4.h"
    "${STM32_DRIVERS_DIR}/STM32H7xx_HAL_Driver/Inc/stm32h7xx_ll_hsem.h")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "Missing ${required}. Set STM32_DRIVERS_DIR; see README.md.")
    endif()
endforeach()

get_filename_component(NERD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(GLOB KERNEL_SOURCES "${NERD_ROOT}/kernel/*.c")
file(GLOB_RECURSE COMPONENT_SOURCES "${NERD_ROOT}/components/*.c")
file(GLOB LIB_SOURCES "${NERD_ROOT}/lib/*.c")
file(GLOB ARCH_SOURCES "${NERD_ROOT}/arch/arm/m4/*.c"
    "${NERD_ROOT}/arch/arm/m4/*.S")

file(READ "${STARTUP}" STARTUP_TEXT)
string(REPLACE ".cpu cortex-m7" ".cpu cortex-m4" M4_STARTUP "${STARTUP_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/startup_cm4.s" "${M4_STARTUP}")

add_executable(Firmware_CM4 CM4/main.c CM4/app.c
    Drivers/BSP/usart.c
    "${CMAKE_CURRENT_BINARY_DIR}/startup_cm4.s" "${SYSTEM_SOURCE}"
    ${ARCH_SOURCES} ${KERNEL_SOURCES} ${COMPONENT_SOURCES} ${LIB_SOURCES})
add_executable(Boot_CM7 CM7/main.c "${STARTUP}" "${SYSTEM_SOURCE}")

foreach(core IN ITEMS CM4 CM7)
    if(core STREQUAL "CM4")
        set(target Firmware_CM4)
        set(cpu_flags -mcpu=cortex-m4 -mfpu=fpv4-sp-d16)
    else()
        set(target Boot_CM7)
        set(cpu_flags -mcpu=cortex-m7 -mfpu=fpv5-d16)
        target_compile_definitions(${target} PRIVATE USE_PWR_DIRECT_SMPS_SUPPLY)
    endif()
    target_compile_definitions(${target} PRIVATE STM32H755xx CORE_${core}
        USER_VECT_TAB_ADDRESS)
    target_include_directories(${target} PRIVATE
        "${STM32_DRIVERS_DIR}/CMSIS/Include" "${DEVICE_PATH}/Include"
        "${STM32_DRIVERS_DIR}/STM32H7xx_HAL_Driver/Inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/${core}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP"
        "${NERD_ROOT}/include" "${NERD_ROOT}/include/lib"
        "${NERD_ROOT}/components/shell/include"
        "${NERD_ROOT}/components/ntest/include")
    target_compile_options(${target} PRIVATE ${cpu_flags} -mthumb -mfloat-abi=hard
        -Wall -Wextra -Wshadow -ffunction-sections -fdata-sections
        $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Release>:-O2>)
    set(linker_script "${CMAKE_CURRENT_SOURCE_DIR}/${core}/stm32h755zi.ld")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${linker_script}")
    target_link_options(${target} PRIVATE ${cpu_flags} -mthumb -mfloat-abi=hard
        "-T${linker_script}" -nostartfiles --specs=nosys.specs
        -Wl,--gc-sections "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
    set_target_properties(${target} PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${target}>
            "${CMAKE_CURRENT_BINARY_DIR}/${target}.hex"
        COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${target}>
            "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")
endforeach()
