cmake_minimum_required(VERSION 3.13)

set(PICO_PLATFORM rp2350-riscv)
set(PICO_BOARD    pico2)

set(CMAKE_CXX_COMPILER_WORKS 1 CACHE BOOL "" FORCE)
set(CMAKE_C_COMPILER_WORKS   1 CACHE BOOL "" FORCE)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(nerd C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

option(NERD_ENABLE_TEST "Enable NerdRTOS test suites" OFF)

add_executable(nerd
    main.c
    nd_hrtimer.c
    nd_led.c
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/riscv/rv32/context.S
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/riscv/rv32/port.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_timer.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_klibc.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_thread.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_sem.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_queue.c
    ${CMAKE_CURRENT_LIST_DIR}/../../kernel/nd_event.c
    ${CMAKE_CURRENT_LIST_DIR}/../../lib/rbtree.c
    ${CMAKE_CURRENT_LIST_DIR}/../../mm/mm.c
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/riscv/rv32/trap.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/src/nd_shell.c
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/commands/nd_cmd_system.c
)



target_include_directories(nerd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../include/
    ${CMAKE_CURRENT_LIST_DIR}/../../include/lib
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/include
)

if(NERD_ENABLE_TEST)
    target_sources(nerd PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/src/ntest.c
        ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/case/test_sem.c
        ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/commands/nd_cmd_system.c
        ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/src/nd_shell.c
        ${CMAKE_CURRENT_LIST_DIR}/../../bsp/raspi-pico2/shell_port.c
        
    )
    target_include_directories(nerd PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/include
    )
endif()
target_link_libraries(nerd pico_stdlib hardware_timer hardware_irq pico_time)
pico_set_linker_script(nerd ${CMAKE_CURRENT_LIST_DIR}/nerd.ld)


pico_add_extra_outputs(nerd)
pico_enable_stdio_usb(nerd 0)



