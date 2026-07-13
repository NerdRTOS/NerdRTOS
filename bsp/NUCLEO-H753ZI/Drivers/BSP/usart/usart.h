#ifndef __USART_H
#define __USART_H

#include <stdint.h>
#include "stm32h7xx_hal.h"

#define USART_UX             USART3

#define USART_GPIO_PORT      GPIOD
#define USART_TX_GPIO_PIN    GPIO_PIN_8
#define USART_RX_GPIO_PIN    GPIO_PIN_9
#define USART_GPIO_AF        GPIO_AF7_USART3

extern UART_HandleTypeDef g_uart3_handle;

void usart_init(uint32_t baudrate);
int  usart_putc(uint8_t c);
void usart_puts(const char *str);

#endif /* __USART_H */
