#ifndef __LED_H
#define __LED_H

#include "stdint.h"
#include "stm32f1xx_hal.h"
#include "nerd.h"

#define LED_NUM 2
#define LED0    0
#define LED1    1

typedef enum {
    LED_ACTIVE_LOW = 0,
    LED_ACTIVE_HIGH
} LedActiveLevel_t;

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON
} LedState_t;

typedef struct {
    GPIO_TypeDef        *port;
    uint32_t            pin;
    LedActiveLevel_t    active_level;
    LedState_t          state;
    GPIO_InitTypeDef    gpio;
} Led_t;

nd_err_t nd_led_init(void);
nd_err_t nd_led_on(uint8_t index);
nd_err_t nd_led_off(uint8_t index);
nd_err_t nd_led_toggle(uint8_t index);

#endif /* __LED_H */
