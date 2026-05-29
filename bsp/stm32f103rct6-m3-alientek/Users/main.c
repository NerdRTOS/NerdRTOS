#include "stm32f1xx_hal.h"
#include "nerd.h"

#include "led.h"
#include "app.h"
#include "usart.h"

/*
 * Stub for newlib __libc_init_array.
 * Required when linking with newlib-nano.
 */
void _init(void)
{
}

extern char __heap_start[];
extern char __heap_end[];

static void bsp_init(void);
static nd_err_t system_clock_init(uint32_t plln);
static void uart_puts(const char *str);

int main(void)
{
    const char *hello_msg = "Hello Nerd RTOS!\r\n";

    nd_kernel_def();
    nd_kernel_lock();

    HAL_Init();
    bsp_init();

    uart_puts(hello_msg);

    nd_scheduler_init();
    nd_app_init();
    nd_scheduler_start();

    return 0;
}

static void bsp_init(void)
{
    system_clock_init(RCC_PLL_MUL9);

    usart_init(115200);

#if ND_CFG_TICKLESS
    nd_hw_hrtimer_init();
#else
    nd_hw_tick_init();
#endif

    nd_system_heap_init(__heap_start,
                        (nd_uint32_t)(__heap_end - __heap_start));

    nd_led_init();
}

static nd_err_t system_clock_init(uint32_t plln)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};

    /* DEBUG: Use HSI instead of HSE to rule out crystal issues */
    rcc_osc_init.OscillatorType     = RCC_OSCILLATORTYPE_HSI;
    rcc_osc_init.HSIState           = RCC_HSI_ON;
    rcc_osc_init.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    rcc_osc_init.PLL.PLLState       = RCC_PLL_ON;
    rcc_osc_init.PLL.PLLSource      = RCC_PLLSOURCE_HSI_DIV2;
    rcc_osc_init.PLL.PLLMUL         = plln;

    ret = HAL_RCC_OscConfig(&rcc_osc_init);
    if (ret != HAL_OK) {
        while (1) {
        }
    }

    rcc_clk_init.ClockType      = (RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_HCLK
                                   | RCC_CLOCKTYPE_PCLK1
                                   | RCC_CLOCKTYPE_PCLK2);
    rcc_clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    rcc_clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;

    ret = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_ACR_LATENCY_2);
    if (ret != HAL_OK) {
        while (1) {
        }
    }

    return ND_EOK;
}

static void uart_puts(const char *str)
{
    while (*str) {
        usart_putc((uint8_t)(*str++));
    }
}
