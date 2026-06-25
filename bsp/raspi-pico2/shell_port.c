#include "nd_def.h"
#include "pico/stdlib.h"
#include "nerd.h"
#include "nd_internal.h"
#include "nd_shell.h"
#include "nd_klibc.h"

#define SHELL_UART_HW   uart0_hw
#define BUF_SIZE        64

typedef struct {
    char        buf[BUF_SIZE];
    nd_uint8_t  head;
    nd_uint8_t  tail;
} ring_buf_t;

static ring_buf_t   rx_buf;
static nd_sem_t     rx_sem;
static nd_mutex_t   put_mutex;

static void uart_rx_isr(void)
{
    nd_enter_interrupt();

    while (!(SHELL_UART_HW->fr & UART_UARTFR_RXFE_BITS)) {
        char c = (char)(SHELL_UART_HW->dr & 0xFF);

        if ((nd_uint8_t)(rx_buf.head - rx_buf.tail) >= BUF_SIZE) {
            continue;
        }

        rx_buf.buf[rx_buf.head & (BUF_SIZE - 1)] = c;
        rx_buf.head++;

        nd_sem_release(&rx_sem);
    }

    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}

void shell_init(void)
{
    nd_sem_init(&rx_sem, 0);
    nd_mutex_init(&put_mutex);

    irq_set_exclusive_handler(UART0_IRQ, uart_rx_isr);
    irq_set_enabled(UART0_IRQ, true);

    SHELL_UART_HW->lcr_h &= ~UART_UARTLCR_H_FEN_BITS;
    SHELL_UART_HW->imsc |= UART_UARTIMSC_RXIM_BITS;
}

nd_int32_t shell_putc(char c)
{
    while ((SHELL_UART_HW->fr & UART_UARTFR_TXFF_BITS)) {
        nd_thread_yield();
    }

    SHELL_UART_HW->dr = c;

    return 0;
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
    if (str == ND_NULL) return;

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

