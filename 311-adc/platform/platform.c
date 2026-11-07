#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

// ТЗ 2.3: Кнопка замыкает GP15 на землю. Светодиод — встроенный.
#define PLATFORM_LED_PIN 25
#define PLATFORM_CAPACITOR_PIN 22
#define PLATFORM_BUTTON_PIN 13

void platform_init(void)
{
    stdio_init_all();

    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);

    gpio_init(PLATFORM_CAPACITOR_PIN);
    gpio_set_dir(PLATFORM_CAPACITOR_PIN, GPIO_OUT);

    gpio_init(PLATFORM_BUTTON_PIN);
    gpio_set_dir(PLATFORM_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);

    adc_set_temp_sensor_enabled(true);
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

void platform_charge_set(bool on)
{
    gpio_put(PLATFORM_CAPACITOR_PIN, on);
}

bool platform_button_read(void)
{
    // Нажатая кнопка замыкает вывод на землю и даёт ноль
    return gpio_get(PLATFORM_BUTTON_PIN) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    return ( 27.0 - ( (adc_read() * ADC_VREF_V / ADC_LEVELS) - 0.706) / 0.001721 );
}