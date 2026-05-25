#include "led.h"
#include "nerd.h"

static void led_clock_enable(GPIO_TypeDef* port);
static HAL_StatusTypeDef led_hardware_init(Led_t* led);

Led_t led_g[LED_NUM] = {
    [0] = {
        .port = GPIOA,
        .pin = GPIO_PIN_8,
        .active_level = LED_ACTIVE_LOW,
        .state = LED_STATE_OFF,
        .gpio = {0}
    },
    [1] = {
        .port = GPIOD,
        .pin = GPIO_PIN_2,
        .active_level = LED_ACTIVE_LOW,
        .state = LED_STATE_OFF,
        .gpio = {0}
    }
};

nd_err_t nd_led_init(void)
{
    if (led_hardware_init(led_g) == HAL_ERROR) {
        return ND_ERROR;
    }

    return ND_EOK;
}

nd_err_t nd_led_on(uint8_t index)
{
    if (index >= LED_NUM) {
        return ND_ERROR;
    }

    if (led_g[index].active_level == LED_ACTIVE_LOW) {
        HAL_GPIO_WritePin(led_g[index].port, led_g[index].pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(led_g[index].port, led_g[index].pin, GPIO_PIN_SET);
    }

    led_g[index].state = LED_STATE_ON;
    return ND_EOK;
}

nd_err_t nd_led_off(uint8_t index)
{
    if (index >= LED_NUM) {
        return ND_ERROR;
    }

    if (led_g[index].active_level == LED_ACTIVE_LOW) {
        HAL_GPIO_WritePin(led_g[index].port, led_g[index].pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(led_g[index].port, led_g[index].pin, GPIO_PIN_RESET);
    }

    led_g[index].state = LED_STATE_OFF;
    return ND_EOK;
}

nd_err_t nd_led_toggle(uint8_t index)
{
    if (index >= LED_NUM) {
        return ND_ERROR;
    }

    if (led_g[index].state == LED_STATE_ON) {
        nd_led_off(index);
    } else {
        nd_led_on(index);
    }

    return ND_EOK;
}

static void led_clock_enable(GPIO_TypeDef* port)
{
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

static HAL_StatusTypeDef led_hardware_init(Led_t* led)
{
    if ((led == NULL) || (LED_NUM <= 0)) {
        return HAL_ERROR;
    }

    for (uint8_t i = 0; i < LED_NUM; i++) {
        led_clock_enable(led[i].port);

        led[i].gpio.Pin = led[i].pin;
        led[i].gpio.Mode = GPIO_MODE_OUTPUT_PP;
        led[i].gpio.Pull = GPIO_PULLDOWN;
        led[i].gpio.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(led[i].port, &led[i].gpio);

        if (led[i].state == LED_STATE_OFF) {
            HAL_GPIO_WritePin(led[i].port, led[i].pin, GPIO_PIN_SET);
        } else if (led[i].state == LED_STATE_ON) {
            HAL_GPIO_WritePin(led[i].port, led[i].pin, GPIO_PIN_RESET);
        }
    }

    return HAL_OK;
}
