#include "usart.h"

UART_HandleTypeDef g_uart1_handle;
uint8_t g_rx_buffer[RX_BUFFER_SIZE];

void usart_init(uint32_t baudrate)
{
    g_uart1_handle.Instance        = USART_UX;
    g_uart1_handle.Init.BaudRate   = baudrate;
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits   = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity     = UART_PARITY_NONE;
    g_uart1_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&g_uart1_handle);

    HAL_UART_Receive_IT(&g_uart1_handle, g_rx_buffer, RX_BUFFER_SIZE);
}

int usart_putc(uint8_t c)
{
    HAL_StatusTypeDef ret;

    ret = HAL_UART_Transmit(&g_uart1_handle, &c, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK) {
        return -1;
    }

    return 0;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        /* RX = PA10: alternate function input with pull-up */
        gpio_init_struct.Pin   = USART_RX_GPIO_PIN;
        gpio_init_struct.Mode  = GPIO_MODE_AF_INPUT;
        gpio_init_struct.Pull  = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);

        /* TX = PA9: alternate function push-pull */
        gpio_init_struct.Pin   = USART_TX_GPIO_PIN;
        gpio_init_struct.Mode  = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull  = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

        HAL_NVIC_EnableIRQ(USART_UX_IRQn);
        HAL_NVIC_SetPriority(USART_UX_IRQn, 15, 0);
    }
}
