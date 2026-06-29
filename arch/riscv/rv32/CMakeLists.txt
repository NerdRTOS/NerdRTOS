# RV32 architecture wrapper for board-target builds.
set(NERD_ARCH_LAYER rv32 CACHE INTERNAL "Selected NerdRTOS architecture layer")
set(NERD_ARCH_CPU hazard3 CACHE INTERNAL "Selected architecture CPU")
set(NERD_ARCH_INSTRUCTION_SET rv32 CACHE INTERNAL "Selected architecture instruction set")
set(NERD_ARCH_EXPECTED_COMPILE_OPTIONS
    CACHE INTERNAL "Expected architecture compile options"
)
set(NERD_ARCH_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/context.S
    ${CMAKE_CURRENT_LIST_DIR}/port.c
    ${CMAKE_CURRENT_LIST_DIR}/trap.S
    ${CMAKE_CURRENT_LIST_DIR}/trap.c
    CACHE INTERNAL "Selected architecture-owned sources"
)
set(NERD_ARCH_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE INTERNAL "Selected architecture-owned include directories"
)