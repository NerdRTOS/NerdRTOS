cmake_minimum_required(VERSION 3.13)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(nerd_a9_bsp_retired NONE)
    message(FATAL_ERROR
        "The standalone QEMU A9 BSP entry is retired. "
        "Configure from the repository root with -DBOARD=qemu_a9/cortex_a9."
    )
endif()

if(NOT NERD_TARGET)
    set(NERD_TARGET nerd)
endif()

include(${PROJECT_ROOT}/cmake/nerd_arch.cmake)
include(${PROJECT_ROOT}/cmake/nerd_sources.cmake)
include(${PROJECT_ROOT}/cmake/nerd_build_metadata.cmake)

if(NOT NERD_ARCH_SOURCES OR NOT NERD_SOC_SOURCES OR NOT NERD_BOARD_SOURCES OR NOT NERD_SAMPLE_SOURCES)
    message(FATAL_ERROR
        "QEMU A9 backend requires arch/soc/board/sample layers. "
        "Configure from the repository root with -DBOARD=qemu_a9/cortex_a9."
    )
endif()

if(NOT NERD_STARTUP_BACKEND)
    set(NERD_STARTUP_BACKEND legacy_bsp)
endif()
if(NOT NERD_STARTUP_OWNER)
    set(NERD_STARTUP_OWNER board)
endif()
if(NOT NERD_ENTRY_SOURCE)
    set(NERD_ENTRY_SOURCE ${CMAKE_CURRENT_LIST_DIR}/main.c)
endif()
if(NOT NERD_LINKER_SCRIPT)
    set(NERD_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/link.ld)
endif()

set(NERD_BSP_SOURCES
    ${NERD_ENTRY_SOURCE}
)

add_executable(${NERD_TARGET}
    ${NERD_BSP_SOURCES}
    ${NERD_SAMPLE_SOURCES}
    ${NERD_BOARD_SOURCES}
    ${NERD_SOC_SOURCES}
    ${NERD_ARCH_SOURCES}
    ${NERD_KERNEL_SOURCES}
    ${NERD_LIB_SOURCES}
    ${NERD_COMPONENT_SOURCES}
)

target_compile_options(${NERD_TARGET} PRIVATE
    ${NERD_ARCH_EXPECTED_COMPILE_OPTIONS}
    -g
    -O0
    -nostdlib
    -ffreestanding
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-but-set-variable
)

target_include_directories(${NERD_TARGET} PRIVATE
    ${AUTOCONF_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${NERD_SAMPLE_INCLUDE_DIRS}
    ${NERD_BOARD_INCLUDE_DIRS}
    ${NERD_SOC_INCLUDE_DIRS}
    ${NERD_ARCH_INCLUDE_DIRS}
    ${NERD_COMMON_INCLUDE_DIRS}
)

target_link_options(${NERD_TARGET} PRIVATE
    -T ${NERD_LINKER_SCRIPT}
    -nostdlib
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/nerd.map
    -Wl,--cref
)

target_link_libraries(${NERD_TARGET} PRIVATE gcc)

if(CMAKE_OBJCOPY)
    add_custom_command(TARGET ${NERD_TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${NERD_TARGET}> $<TARGET_FILE:${NERD_TARGET}>.bin
        COMMENT "Generating nerd.bin"
    )
endif()

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E echo "=== QEMU running. Exit: Ctrl-A then X ==="
    COMMAND qemu-system-arm -M vexpress-a9 -nographic -kernel $<TARGET_FILE:${NERD_TARGET}>
    DEPENDS ${NERD_TARGET}
    USES_TERMINAL
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -E echo "=== QEMU paused. In another terminal: arm-none-eabi-gdb ${CMAKE_CURRENT_BINARY_DIR}/nerd -ex 'target remote :1234' ==="
    COMMAND qemu-system-arm -M vexpress-a9 -nographic -kernel $<TARGET_FILE:${NERD_TARGET}> -s -S
    DEPENDS ${NERD_TARGET}
    USES_TERMINAL
)

nerd_write_build_metadata(${PROJECT_BINARY_DIR}/build_layers.txt)
