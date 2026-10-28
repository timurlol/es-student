#include "button-task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "systime/systime.h"
#include "led-task/led-task.h"

// Кнопка замыкает GP13 на землю, вывод подтянут к питанию
#define BUTTON_PIN 13

// Время дребезга: новый уровень принимается, только если продержался столько
#define BUTTON_DEBOUNCE_US 20000

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_state_t button_state;
static uint64_t change_us;
static uint32_t press_count;

static bool button_level_pressed(void)
{
    return gpio_get(BUTTON_PIN) == 0;
}

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = BUTTON_STATE_RELEASED;
    change_us = 0;
    press_count = 0;
}

void button_task_handle(void)
{
    bool pressed = button_level_pressed();
    uint64_t now_us = systime_us();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        if (pressed)
        {
            change_us = now_us;
            button_state = BUTTON_STATE_PRESSING;
        }
        break;

    case BUTTON_STATE_PRESSING:
        if (!pressed)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_PRESSED;
            press_count++;
            led_task_next_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        if (!pressed)
        {
            change_us = now_us;
            button_state = BUTTON_STATE_RELEASING;
        }
        break;

    case BUTTON_STATE_RELEASING:
        if (pressed)
        {
            button_state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}

bool button_task_is_pressed(void)
{
    return button_state == BUTTON_STATE_PRESSED
        || button_state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}