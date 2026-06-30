# Cortex-A architecture wrapper for board-target builds.
set(NERD_ARCH_LAYER cortex_a CACHE INTERNAL "Selected NerdRTOS architecture layer")

if(CONFIG_ARCH_ARM_A9)
    set(NERD_ARCH_CPU cortex-a9 CACHE INTERNAL "Selected architecture CPU")
    set(NERD_ARCH_INSTRUCTION_SET arm CACHE INTERNAL "Selected architecture instruction set")
    set(NERD_ARCH_EXPECTED_COMPILE_OPTIONS
        -mcpu=cortex-a9
        -marm
        CACHE INTERNAL "Expected architecture compile options"
    )
    set(NERD_ARCH_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/a9/startup.S
        ${CMAKE_CURRENT_LIST_DIR}/a9/context.S
        ${CMAKE_CURRENT_LIST_DIR}/a9/port.c
        ${CMAKE_CURRENT_LIST_DIR}/a9/timer.c
        CACHE INTERNAL "Selected architecture-owned sources"
    )
    set(NERD_ARCH_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/a9
        CACHE INTERNAL "Selected architecture-owned include directories"
    )
else()
    message(FATAL_ERROR "Unsupported Cortex-A architecture selection")
endif()
