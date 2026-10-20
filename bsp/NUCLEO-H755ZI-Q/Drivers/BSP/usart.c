#include "stm32h7xx.h"
#include "nerd.h"
#include "nd_shell.h"
#include "nd_klibc.h"
#include "nd_internal.h"
#include "usart.h"
#include <stdarg.h>

#define RX_BUFFER_SIZE 128U

static char rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static nd_sem_t rx_sem;

void usart_init(uint32_t baudrate)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    (void)RCC->AHB4ENR;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    (void)RCC->APB1LENR;
    RCC->D2CCIP2R &= ~RCC_D2CCIP2R_USART28SEL;
    GPIOD->MODER = (GPIOD->MODER & ~((3UL << 16) | (3UL << 18))) |
        (2UL << 16) | (2UL << 18);
    GPIOD->OTYPER &= ~((1UL << 8) | (1UL << 9));
    GPIOD->OSPEEDR |= (3UL << 16) | (3UL << 18);
    GPIOD->PUPDR = (GPIOD->PUPDR & ~((3UL << 16) | (3UL << 18))) |
        (1UL << 16) | (1UL << 18);
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~0xffUL) | 0x77UL;
    RCC->APB1LRSTR |= RCC_APB1LRSTR_USART3RST;
    RCC->APB1LRSTR &= ~RCC_APB1LRSTR_USART3RST;
    USART3->BRR = (SystemCoreClock + baudrate / 2U) / baudrate;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

int usart_putc(uint8_t c)
{
    while (!(USART3->ISR & USART_ISR_TXE_TXFNF)) {
    }
    USART3->TDR = c;
    return 0;
}

void usart_puts(const char *str)
{
    while (*str) {
        usart_putc((uint8_t)*str++);
    }
}

nd_int32_t shell_putc(char c)
{
    return usart_putc((uint8_t)c);
}

char shell_getc(void)
{
    nd_sem_take(&rx_sem, ND_TIMEOUT_FOREVER);
    char c = rx_buffer[rx_tail & (RX_BUFFER_SIZE - 1U)];
    rx_tail++;
    return c;
}

void shell_init(void)
{
    nd_sem_init(&rx_sem, 0);
    USART3->ICR = USART_ICR_ORECF;
    NVIC_SetPriority(USART3_IRQn, 5);
    NVIC_ClearPendingIRQ(USART3_IRQn);
    USART3->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
    NVIC_EnableIRQ(USART3_IRQn);
}

void USART3_IRQHandler(void)
{
    nd_enter_interrupt();
    while (USART3->ISR & USART_ISR_RXNE_RXFNE) {
        char c = (char)USART3->RDR;
        uint8_t next_head = (uint8_t)(rx_head + 1U);
        if ((uint8_t)(next_head - rx_tail) <= RX_BUFFER_SIZE) {
            rx_buffer[rx_head & (RX_BUFFER_SIZE - 1U)] = c;
            rx_head = next_head;
            nd_sem_release(&rx_sem);
        }
    }
    USART3->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}

void shell_puts(const char *str)
{
    if (str != ND_NULL) {
        usart_puts(str);
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
