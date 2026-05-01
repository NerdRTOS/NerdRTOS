#include "app.h"

extern char __heap_start[];
extern char __heap_end[];

static void bsp_init(void)
{
    setup_default_uart();

#if ND_CFG_TICKLESS
    nd_hw_hrtimer_init();
#else
    nd_hw_tick_init();
#endif

    nd_system_heap_init(__heap_start, (nd_uint32_t)(__heap_end - __heap_start));
}

int main(void)
{
    nd_kernel_def();
    nd_kernel_lock();

    bsp_init();

    printf("Hello Nerd RTOS!\r\n");

    nd_scheduler_init();

    nd_app_init();

    nd_scheduler_start();

    return 0;
}
