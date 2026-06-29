include_guard(GLOBAL)

if(NERD_ARCH_SOURCES)
    return()
endif()

if(CONFIG_ARCH_RISCV_RV32)
    set(NERD_ARCH_SOURCES
        ${PROJECT_ROOT}/arch/riscv/rv32/context.S
        ${PROJECT_ROOT}/arch/riscv/rv32/port.c
        ${PROJECT_ROOT}/arch/riscv/rv32/trap.S
        ${PROJECT_ROOT}/arch/riscv/rv32/trap.c
    )

    set(NERD_ARCH_INCLUDE_DIRS
        ${PROJECT_ROOT}/arch/riscv/rv32
    )
elseif(CONFIG_ARCH_ARM_M33)
    set(NERD_ARCH_SOURCES
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/context.S
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/port.c
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/hardfault.c
        ${PROJECT_ROOT}/arch/arm/cortex_m/m33/systick.c
    )

    set(NERD_ARCH_INCLUDE_DIRS)
else()
    message(FATAL_ERROR "No supported architecture selected")
endif()
