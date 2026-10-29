#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ТЗ 2.3: Кнопка замыкает GP15 на землю. Светодиод — встроенный.
#define PLATFORM_LED_PIN 25
#define PLATFORM_BUTTON_PIN 13

void platform_init(void)
{
    stdio_init_all();

    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);

    gpio_init(PLATFORM_BUTTON_PIN);
    gpio_set_dir(PLATFORM_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

bool platform_button_read(void)
{
    // Нажатая кнопка замыкает вывод на землю и даёт ноль
    return gpio_get(PLATFORM_BUTTON_PIN) == 0;
}