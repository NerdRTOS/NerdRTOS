#include "usart.h"
#include "nerd.h"
#include "nd_klibc.h"

UART_HandleTypeDef g_uart3_handle;

void usart_init(uint32_t baudrate)
{
    g_uart3_handle.Instance        = USART_UX;
    g_uart3_handle.Init.BaudRate   = baudrate;
    g_uart3_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart3_handle.Init.StopBits   = UART_STOPBITS_1;
    g_uart3_handle.Init.Parity     = UART_PARITY_NONE;
    g_uart3_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    g_uart3_handle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&g_uart3_handle);
}

int usart_putc(uint8_t c)
{
    /* Direct TDR write: HAL_UART_Transmit manipulates CR1,
       breaking concurrent RX (see F1 BSP usart.c) */
    while (!(USART_UX->ISR & USART_ISR_TXE_TXFNF)) {
    }

    USART_UX->TDR = c;

    return 0;
}

void usart_puts(const char *str)
{
    while (*str) {
        usart_putc((uint8_t)(*str++));
    }
}

/* Shell component port (polling; moves to shell_port.c with IRQ RX later) */
nd_int32_t shell_putc(char c)
{
    return usart_putc((uint8_t)c);
}

char shell_getc(void)
{
    while (!(USART_UX->ISR & USART_ISR_RXNE_RXFNE)) {
        nd_thread_delay(1);
    }

    return (char)(USART_UX->RDR & 0xFF);
}

void shell_init(void)
{
}

void shell_puts(const char *str)
{
    if (str == ND_NULL) {
        return;
    }

    while (*str) {
        if (shell_putc(*str++) < 0) {
            break;
        }
    }
}

void shell_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    nd_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    shell_puts(buf);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init = {0};

    if (huart->Instance == USART_UX) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_USART3_CLK_ENABLE();

        /* PD8 = TX, PD9 = RX - routed to ST-LINK VCP */
        gpio_init.Pin       = USART_TX_GPIO_PIN | USART_RX_GPIO_PIN;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_NOPULL;
        gpio_init.Speed     = GPIO_SPEED_FREQ_LOW;
        gpio_init.Alternate = USART_GPIO_AF;
        HAL_GPIO_Init(USART_GPIO_PORT, &gpio_init);
    }
}
