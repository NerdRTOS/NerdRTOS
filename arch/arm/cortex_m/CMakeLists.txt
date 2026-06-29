# Cortex-M architecture wrapper for board-target builds.
set(NERD_ARCH_LAYER cortex_m CACHE INTERNAL "Selected NerdRTOS architecture layer")

if(CONFIG_ARCH_ARM_M33)
    set(NERD_ARCH_CPU cortex-m33 CACHE INTERNAL "Selected architecture CPU")
    set(NERD_ARCH_INSTRUCTION_SET thumb CACHE INTERNAL "Selected architecture instruction set")
    set(NERD_ARCH_EXPECTED_COMPILE_OPTIONS
        -mcpu=cortex-m33
        -mthumb
        CACHE INTERNAL "Expected architecture compile options"
    )
    set(NERD_ARCH_SOURCES
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/context.S
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/port.c
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/hardfault.c
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/systick.c
        CACHE INTERNAL "Selected architecture-owned sources"
    )
    set(NERD_ARCH_INCLUDE_DIRS
        CACHE INTERNAL "Selected architecture-owned include directories"
    )
else()
    message(FATAL_ERROR "Unsupported Cortex-M architecture selection")
endif()