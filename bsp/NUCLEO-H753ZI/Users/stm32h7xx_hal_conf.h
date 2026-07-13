#ifndef STM32H7xx_HAL_CONF_H
#define STM32H7xx_HAL_CONF_H

/* Enabled HAL modules - minimal set for UART bring-up */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* Oscillator values */
#define HSE_VALUE              (8000000UL)  /* ST-LINK MCO on NUCLEO-H753ZI */
#define HSE_STARTUP_TIMEOUT    (100UL)
#define CSI_VALUE              (4000000UL)
#define HSI_VALUE              (64000000UL)
#define LSI_VALUE              (32000UL)
#define LSE_VALUE              (32768UL)
#define LSE_STARTUP_TIMEOUT    (5000UL)
#define EXTERNAL_CLOCK_VALUE   (12288000UL)

/* System configuration */
#define VDD_VALUE                        (3300UL)
#define TICK_INT_PRIORITY                (0x0FUL)
#define USE_RTOS                         0
#define USE_FLASH_ECC                    0U
#define USE_HAL_UART_REGISTER_CALLBACKS  0U

/* Module headers */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32h7xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32h7xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32h7xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32h7xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32h7xx_hal_flash.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32h7xx_hal_pwr.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32h7xx_hal_uart.h"
#endif

#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#endif /* STM32H7xx_HAL_CONF_H */
