# Step 5 wrapper. RP2350-owned sources live here while the legacy Pico BSP
# remains the backend for startup, linker script, and Pico SDK integration.
set(NERD_SOC_LEGACY_BSP_DIR ${PROJECT_ROOT}/bsp/raspi-pico2 CACHE INTERNAL "Legacy RP2350 BSP directory")
set(NERD_SOC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/nd_hrtimer.c
    CACHE INTERNAL "Selected SoC-owned sources"
)
set(NERD_SOC_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "Selected SoC-owned include directories"
)