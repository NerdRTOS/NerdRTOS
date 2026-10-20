#ifndef ND_H755_USART_H
#define ND_H755_USART_H

#include <stdint.h>

void usart_init(uint32_t baudrate);
int usart_putc(uint8_t c);
void usart_puts(const char *str);

#endif
