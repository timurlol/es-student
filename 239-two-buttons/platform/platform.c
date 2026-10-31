#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ТЗ 2.3: Кнопка замыкает GP15 на землю. Светодиод — встроенный.
#define PLATFORM_LED_PIN 25

static const uint8_t BUTTON_PINS[BUTTON_COUNT] = {
    [BUTTON_1] = 13, 
    [BUTTON_2] = 18 
};

void platform_init(void)
{
    stdio_init_all();

    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);

    for (int i = 0; i < BUTTON_COUNT; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]); // Подтяжка к питанию
    }
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

bool platform_button_read(platform_button_t button) {
    if (button >= BUTTON_COUNT)
        return false;
    return gpio_get(BUTTON_PINS[button]) == 0;
}