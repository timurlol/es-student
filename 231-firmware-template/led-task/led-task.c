#include "led-task.h"

#include "pico/stdlib.h"

// Состояние задачи: внутреннее связывание, только для этого файла
static led_state_t led_state;
static uint32_t period_ms;
static uint64_t half_period_us;
static uint64_t last_toggle_us;

// Проверка периода: следующее срабатывание отсчитывается от прошлого,
// при отставании больше периода — отсчёт начинается заново
static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();

    if (now_us - *last_us < period_us) {
        return false;
    }

    *last_us += period_us;

    if (now_us - *last_us >= period_us) {
        *last_us = now_us;
    }

    return true;
}

static void led_set(bool on)
{
    gpio_put(PICO_DEFAULT_LED_PIN, on);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;

    // Действие при входе: выполняется один раз, в момент перехода
    switch (led_state) {
    case LED_STATE_OFF:
        led_set(false);
        break;
    case LED_STATE_ON:
        led_set(true);
        break;
    case LED_STATE_BLINK:
        // Первое переключение — на ближайшей итерации
        last_toggle_us = 0;
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

bool led_task_set_period_ms(uint32_t new_period_ms)
{
    // С нулевым полупериодом светодиод переключался бы на каждой итерации
    if (new_period_ms == 0) {
        return false;
    }

    period_ms = new_period_ms;
    half_period_us = (uint64_t)period_ms * 1000 / 2;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return period_ms;
}

void led_task_init(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    period_ms = 1000;
    half_period_us = (uint64_t)period_ms * 1000 / 2;

    led_task_set_state(LED_STATE_BLINK);
}

void led_task_handle(void)
{
    switch (led_state) {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        // Светодиод установлен при входе в состояние, делать нечего
        break;
    case LED_STATE_BLINK:
        if (period_elapsed(&last_toggle_us, half_period_us)) {
            led_set(!gpio_get(PICO_DEFAULT_LED_PIN));
        }
        break;
    }
}

void led_task_next_state(void)
{
    switch (led_state) {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;
    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;
    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}