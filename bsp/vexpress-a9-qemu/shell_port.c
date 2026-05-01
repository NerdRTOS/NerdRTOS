#include "nerd.h"
#include "nd_shell.h"
#include "nd_klibc.h"
#include "gic.h"

#define UART0_BASE      0x10009000
#define UART0_DR        (*(volatile nd_uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR        (*(volatile nd_uint32_t *)(UART0_BASE + 0x18))
#define UART0_CR        (*(volatile nd_uint32_t *)(UART0_BASE + 0x30))
#define UART0_IMSC      (*(volatile nd_uint32_t *)(UART0_BASE + 0x38))
#define UART0_ICR       (*(volatile nd_uint32_t *)(UART0_BASE + 0x44))

#define UART_FR_TXFF    (1U << 5)
#define UART_FR_RXFE    (1U << 4)
#define UART_IMSC_RXIM  (1U << 4)
#define UART_ICR_RXIC   (1U << 4)

#define IRQ_UART0       37

#define BUF_SIZE        64

typedef struct {
    char       buf[BUF_SIZE];
    nd_uint8_t head;
    nd_uint8_t tail;
} ring_buf_t;

static ring_buf_t rx_buf;
static nd_sem_t   rx_sem;
static nd_mutex_t put_mutex;

static void uart_rx_isr(void *arg)
{
    UART0_ICR = UART_ICR_RXIC;

    while (!(UART0_FR & UART_FR_RXFE)) {
        char c = (char)(UART0_DR & 0xFF);

        if ((nd_uint8_t)(rx_buf.head - rx_buf.tail) >= BUF_SIZE) {
            continue;
        }

        rx_buf.buf[rx_buf.head & (BUF_SIZE - 1)] = c;
        rx_buf.head++;

        nd_sem_release(&rx_sem);
    }
}

void shell_init(void)
{
    nd_sem_init(&rx_sem, 0);
    nd_mutex_init(&put_mutex);

    gic_register_handler(IRQ_UART0, uart_rx_isr, ND_NULL);
    gic_irq_enable(IRQ_UART0);

    UART0_IMSC |= UART_IMSC_RXIM;
}

int shell_putc(char c)
{
    while (UART0_FR & UART_FR_TXFF) {
        nd_thread_yield();
    }

    UART0_DR = c;

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
