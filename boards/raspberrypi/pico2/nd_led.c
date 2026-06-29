#include "nd_led.h"
#include "pico/stdlib.h"

#define LED_PIN 25

void nd_led_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_set_drive_strength(LED_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_slew_rate(LED_PIN, GPIO_SLEW_RATE_SLOW);
}

void nd_led_on(void)
{
    gpio_put(LED_PIN, 1);
}

void nd_led_off(void)
{
    gpio_put(LED_PIN, 0);
}

void nd_led_toggle(void)
{
    gpio_xor_mask(1u << LED_PIN);
}

int nd_led_get_state(void)
{
    return gpio_get(LED_PIN) ? 1 : 0;
}
