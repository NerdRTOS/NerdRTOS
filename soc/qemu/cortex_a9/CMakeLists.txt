set(NERD_SOC_LEGACY_BSP_DIR
    ${PROJECT_ROOT}/bsp/vexpress-a9-qemu
    CACHE INTERNAL "Legacy QEMU A9 BSP directory"
)
set(NERD_SOC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gic.c
    CACHE INTERNAL "Selected SoC-owned sources"
)
set(NERD_SOC_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "Selected SoC-owned include directories"
)