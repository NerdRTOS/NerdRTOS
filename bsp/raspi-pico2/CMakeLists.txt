cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(nerd C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if(PICO_PLATFORM STREQUAL "rp2350-riscv")
    set(NERD_ARCH_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/riscv/rv32/context.S
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/riscv/rv32/port.c
    )
else()
    set(NERD_ARCH_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/m33/context.S
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/m33/port.c
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/m33/hardfault.c
        ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/m33/systick.c
    )
endif()

add_executable(nerd
    main.c
    nd_hrtimer.c
    nd_led.c
    shell_port.c
    app.c
    ${NERD_ARCH_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_timer.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_klibc.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_thread.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_sem.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_queue.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_event.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_mempool.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_memheap.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_mem.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_cpuload.c
    ${CMAKE_CURRENT_LIST_DIR}/../../lib/rbtree.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/src/nd_shell.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/commands/nd_cmd_system.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/src/ntest.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/case/test_sem.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/case/test_memheap.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/case/test_mempool.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/case/test_thread_lifecycle.c
)

target_compile_options(nerd PRIVATE
    -Wall
    -Wextra
    -Wunused
    -Wshadow
    -Werror=return-type
    -Werror=vla
    -Werror=uninitialized
    -Werror=format-security
    -Wdouble-promotion
    -Wno-missing-field-initializers
    -Wno-unused-but-set-variable
    $<$<COMPILE_LANGUAGE:C>:
        -Werror=implicit-function-declaration
        -Werror=implicit-int
        -Werror=strict-prototypes
        -Wno-pointer-sign
    >
)

target_include_directories(nerd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../include/
    ${CMAKE_CURRENT_LIST_DIR}/../../include/lib
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/include
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/include
)

target_include_directories(nerd SYSTEM PRIVATE
    ${PICO_SDK_PATH}/src/rp2_common
)

pico_set_printf_implementation(${PROJECT_NAME} pico)

pico_add_extra_outputs(nerd)

pico_set_linker_script(nerd ${CMAKE_CURRENT_SOURCE_DIR}/nerd.ld)

target_link_options(nerd PRIVATE -Os
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--cref
    -Wl,--print-memory-usage
)

target_link_libraries(nerd pico_stdlib)

