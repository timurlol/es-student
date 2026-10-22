#include "button-task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

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

// Нажатая кнопка замыкает вывод на землю и даёт ноль
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
    uint64_t now_us = time_us_64();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        // Уровень сменился — запомнить время и проверить, продержится ли он
        if (pressed)
        {
            change_us = now_us;
            button_state = BUTTON_STATE_PRESSING;
        }
        break;

    case BUTTON_STATE_PRESSING:
        // Уровень вернулся раньше порога — это был дребезг или помеха
        if (!pressed)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        // Уровень продержался порог — нажатие состоялось
        else if (now_us - change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_PRESSED;
            press_count++;
            led_task_next_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        // Уровень сменился — запомнить время и проверить отпускание
        if (!pressed)
        {
            change_us = now_us;
            button_state = BUTTON_STATE_RELEASING;
        }
        break;

    case BUTTON_STATE_RELEASING:
        // Уровень вернулся раньше порога — кнопка всё ещё нажата
        if (pressed)
        {
            button_state = BUTTON_STATE_PRESSED;
        }
        // Уровень продержался порог — отпускание состоялось
        else if (now_us - change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}

// Кнопка нажата и в «нажата», и в «отпускание?»: пока отпускание
// не подтвердилось, кнопку считают нажатой
bool button_task_is_pressed(void)
{
    return button_state == BUTTON_STATE_PRESSED
        || button_state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}