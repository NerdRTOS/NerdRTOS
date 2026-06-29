# Step 5 wrapper. Board-owned Pico 2 sources live here while the legacy BSP
# remains the backend for startup, app glue, and Pico SDK integration.
set(NERD_BOARD_LEGACY_DIR ${PROJECT_ROOT}/bsp/raspi-pico2 CACHE INTERNAL "Legacy Pico 2 BSP directory")
set(NERD_BOARD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/nd_led.c
    ${CMAKE_CURRENT_LIST_DIR}/shell_port.c
    CACHE INTERNAL "Selected board-owned sources"
)
set(NERD_BOARD_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "Selected board-owned include directories"
)
set(NERD_LINKER_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/nerd.ld
    CACHE INTERNAL "Selected linker script"
)
set(NERD_STARTUP_BACKEND
    legacy_bsp
    CACHE INTERNAL "Startup backend selected by the board"
)
set(NERD_STARTUP_OWNER
    board
    CACHE INTERNAL "Target layer that owns startup entry code"
)
set(NERD_ENTRY_SOURCE
    ${PROJECT_ROOT}/bsp/raspi-pico2/main.c
    CACHE INTERNAL "Current startup entry source"
)
set(NERD_TOOLCHAIN_BACKEND
    pico_sdk
    CACHE INTERNAL "Toolchain and platform flag backend selected by the board"
)
set(NERD_PICO_SDK_SYSTEM_INCLUDE_DIRS
    ${PICO_SDK_PATH}/src/rp2_common
    CACHE INTERNAL "Pico SDK system include directories selected by the board"
)
set(NERD_PICO_PRINTF_IMPLEMENTATION
    pico
    CACHE INTERNAL "Pico SDK printf implementation selected by the board"
)
set(NERD_PICO_EXTRA_OUTPUTS
    ON
    CACHE INTERNAL "Whether the board requests Pico SDK extra output files"
)
set(NERD_PICO_LIBRARIES
    pico_stdlib
    CACHE INTERNAL "Pico SDK libraries selected by the board"
)