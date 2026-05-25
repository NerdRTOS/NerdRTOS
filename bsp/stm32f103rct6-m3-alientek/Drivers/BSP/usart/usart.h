#ifndef __USART_H
#define __USART_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define USART_UX             USART1
#define USART_UX_IRQn        USART1_IRQn

#define USART_TX_GPIO_PORT   GPIOA
#define USART_TX_GPIO_PIN    GPIO_PIN_9

#define USART_RX_GPIO_PORT   GPIOA
#define USART_RX_GPIO_PIN    GPIO_PIN_10

#define RX_BUFFER_SIZE       1

extern UART_HandleTypeDef g_uart1_handle;
extern uint8_t g_rx_buffer[RX_BUFFER_SIZE];

void usart_init(uint32_t baudrate);
int  usart_putc(uint8_t c);

#endif /* __USART_H */
