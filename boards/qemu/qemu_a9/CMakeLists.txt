# QEMU Versatile Express A9 board wrapper.
set(NERD_BOARD_LEGACY_DIR
    ${PROJECT_ROOT}/bsp/vexpress-a9-qemu
    CACHE INTERNAL "Legacy QEMU A9 BSP directory"
)

set(NERD_BOARD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/shell_port.c
    CACHE INTERNAL "Selected board-owned sources"
)

set(NERD_BOARD_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "Selected board-owned include directories"
)

set(NERD_LINKER_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/link.ld
    CACHE INTERNAL "Selected board linker script"
)

set(NERD_STARTUP_BACKEND legacy_bsp CACHE INTERNAL "Selected startup backend")
set(NERD_STARTUP_OWNER board CACHE INTERNAL "Selected startup owner")
set(NERD_ENTRY_SOURCE
    ${NERD_BOARD_LEGACY_DIR}/main.c
    CACHE INTERNAL "Selected board entry source"
)
