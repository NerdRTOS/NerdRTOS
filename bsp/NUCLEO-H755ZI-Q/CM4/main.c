#include "stm32h7xx.h"
#include "stm32h7xx_ll_hsem.h"
#include "nerd.h"
#include "app.h"
#include "usart.h"

extern char __heap_start[];
extern char __heap_end[];

static void bsp_wait_for_boot(void);
static void bsp_init(void);

void _init(void)
{
}

int main(void)
{
    bsp_wait_for_boot();

    nd_hw_irq_disable();
    bsp_init();

    nd_scheduler_init();
    nd_app_init();

    nd_hw_tick_init();
    nd_scheduler_start();

    return 0;
}

static void bsp_wait_for_boot(void)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_HSEMEN;
    (void)RCC->AHB4ENR;

    LL_HSEM_EnableIT_C2IER(HSEM, 1UL);
    __SEV();
    __WFE();

    PWR->CR1 &= ~PWR_CR1_LPDS;
    PWR->CPU2CR &= ~PWR_CPU2CR_PDDS_D2;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __DSB();
    __ISB();
    __WFE();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    LL_HSEM_DisableIT_C2IER(HSEM, 1UL);
    LL_HSEM_ClearFlag_C2ICR(HSEM, 1UL);
}

static void bsp_init(void)
{
    SystemCoreClockUpdate();
    if (SystemCoreClock != ND_CPU_CLOCK_HZ) {
        while (1) {
            __NOP();
        }
    }

    NVIC_SetPriorityGrouping(3);

    usart_init(115200);
    usart_puts("Hello Nerd RTOS! H755 Cortex-M4, HSI 64 MHz\r\n");

    nd_system_heap_init(__heap_start,
        (nd_uint32_t)(__heap_end - __heap_start));
}
