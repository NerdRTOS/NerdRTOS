cmake_minimum_required(VERSION 3.13)

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER        arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER      arm-none-eabi-gcc)
set(CMAKE_OBJCOPY           arm-none-eabi-objcopy)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(nerd C ASM)
set(CMAKE_C_STANDARD 11)

add_executable(nerd
    main.c
    app.c
    shell_port.c
    gic.c
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/a9/startup.S
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/a9/context.S
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/a9/port.c
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/a9/timer.c
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
    -mcpu=cortex-a9
    -marm
    -g -O0
    -nostdlib
    -ffreestanding
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-but-set-variable
)

target_include_directories(nerd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../include
    ${CMAKE_CURRENT_LIST_DIR}/../../include/lib
    ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/a9
    ${CMAKE_CURRENT_LIST_DIR}/../../components/shell/include
    ${CMAKE_CURRENT_LIST_DIR}/../../components/ntest/include
)

target_link_options(nerd PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld
    -nostdlib
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/nerd.map
    -Wl,--cref
)

target_link_libraries(nerd PRIVATE gcc)

add_custom_command(TARGET nerd POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:nerd> $<TARGET_FILE:nerd>.bin
    COMMENT "Generating nerd.bin"
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E echo "=== QEMU running. Exit: Ctrl-A then X ==="
    COMMAND qemu-system-arm -M vexpress-a9 -nographic -kernel $<TARGET_FILE:nerd>

    USES_TERMINAL
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -E echo "=== QEMU paused. In another terminal: arm-none-eabi-gdb build/nerd -ex 'target remote :1234' ==="
    COMMAND qemu-system-arm -M vexpress-a9 -nographic -kernel $<TARGET_FILE:nerd> -s -S
    DEPENDS nerd
    USES_TERMINAL
)
