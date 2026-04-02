#include "app.h"

static void bsp_init(void)
{
    setup_default_uart();

#if ND_CFG_TICKLESS
    nd_hw_hrtimer_init();
#else
    nd_hw_tick_init();
#endif
}

int main(void)
{
    bsp_init();

    printf("Hello Nerd RTOS!\r\n");

    nd_scheduler_init();

    nd_app_init();

    nd_scheduler_start();

    return 0;
}
