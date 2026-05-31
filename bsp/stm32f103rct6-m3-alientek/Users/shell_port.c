#include "usart.h"
#include "nerd.h"
#include "nd_internal.h"
#include "nd_shell.h"
#include "nd_klibc.h"

#define BUF_SIZE        128

typedef struct {
    char        buf[BUF_SIZE];
    nd_uint8_t  head;
    nd_uint8_t  tail;
} ring_buf_t;

static ring_buf_t   rx_buf;
static nd_sem_t     rx_sem;
static nd_mutex_t   put_mutex;

void shell_init(void)
{
    nd_sem_init(&rx_sem, 0);
    nd_mutex_init(&put_mutex);
}

int shell_putc(char c)
{
    return usart_putc((uint8_t)c);
}

char shell_getc(void)
{
    nd_sem_take(&rx_sem, ND_TIMEOUT_FOREVER);

    char c = rx_buf.buf[rx_buf.tail & (BUF_SIZE - 1)];
    rx_buf.tail++;

    return c;
}

void shell_puts(const char *str)
{
    if (str == ND_NULL) {
        return;
    }

    nd_mutex_lock(&put_mutex, ND_TIMEOUT_FOREVER);

    while (*str) {
        if (shell_putc(*str++) < 0) {
            break;
        }
    }

    nd_mutex_unlock(&put_mutex);
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

/*
 * HAL RX complete callback.
 * Runs inside ISR context (called from HAL_UART_IRQHandler).
 * Writes received byte to ring buffer and releases RX semaphore.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }

    nd_uint8_t next_head = (nd_uint8_t)(rx_buf.head + 1);

    if ((nd_uint8_t)(next_head - rx_buf.tail) <= BUF_SIZE) {
        rx_buf.buf[rx_buf.head & (BUF_SIZE - 1)] = (char)g_rx_buffer[0];
        rx_buf.head = next_head;
        nd_sem_release(&rx_sem);
    }

    HAL_UART_Receive_IT(&g_uart1_handle, g_rx_buffer, 1);
}

/*
 * USART1 interrupt handler.
 * Enters RTOS interrupt context, delegates to HAL, then checks for
 * pending context switch at ISR exit.
 */
void USART1_IRQHandler(void)
{
    nd_enter_interrupt();
    HAL_UART_IRQHandler(&g_uart1_handle);
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}
