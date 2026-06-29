include_guard(GLOBAL)

set(NERD_KERNEL_SOURCES
    ${PROJECT_ROOT}/kernel/nd_scheduler.c
    ${PROJECT_ROOT}/kernel/nd_timer.c
    ${PROJECT_ROOT}/kernel/nd_klibc.c
    ${PROJECT_ROOT}/kernel/nd_thread.c
    ${PROJECT_ROOT}/kernel/nd_sem.c
    ${PROJECT_ROOT}/kernel/nd_mutex.c
    ${PROJECT_ROOT}/kernel/nd_queue.c
    ${PROJECT_ROOT}/kernel/nd_event.c
    ${PROJECT_ROOT}/kernel/nd_mempool.c
    ${PROJECT_ROOT}/kernel/nd_memheap.c
    ${PROJECT_ROOT}/kernel/nd_mem.c
    ${PROJECT_ROOT}/kernel/nd_cpuload.c
)

set(NERD_LIB_SOURCES
    ${PROJECT_ROOT}/lib/rbtree.c
)

set(NERD_COMPONENT_SOURCES
    ${PROJECT_ROOT}/components/shell/src/nd_shell.c
    ${PROJECT_ROOT}/components/shell/commands/nd_cmd_system.c
    ${PROJECT_ROOT}/components/ntest/src/ntest.c
    ${PROJECT_ROOT}/components/ntest/case/test_sem.c
    ${PROJECT_ROOT}/components/ntest/case/test_memheap.c
    ${PROJECT_ROOT}/components/ntest/case/test_mempool.c
    ${PROJECT_ROOT}/components/ntest/case/test_thread_lifecycle.c
)

set(NERD_COMMON_INCLUDE_DIRS
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/include/lib
    ${PROJECT_ROOT}/components/shell/include
    ${PROJECT_ROOT}/components/ntest/include
)
