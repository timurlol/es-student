#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include <stdio.h>

#define BUTTON_DEBOUNCE_US 20000

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

typedef struct {
    button_state_t state;
    uint64_t change_us;
    uint32_t press_count;
    button_task_callback_t on_press_cb;
} button_instance_t;

// Массив кнопок, размер берем из platform
static button_instance_t buttons[BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press)
{
    if (button >= BUTTON_COUNT) return;
    
    // Обращаемся к конкретному индексу в массиве кнопок!
    buttons[button].state = BUTTON_STATE_RELEASED;
    buttons[button].change_us = 0;
    buttons[button].press_count = 0;
    buttons[button].on_press_cb = on_press;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

    // Проходимся по всем кнопкам платы
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        bool pressed = platform_button_read((platform_button_t)i);

        switch (buttons[i].state)
        {
        case BUTTON_STATE_RELEASED:
            if (pressed)
            {
                buttons[i].change_us = now_us;
                buttons[i].state = BUTTON_STATE_PRESSING;
            }
            break;

        case BUTTON_STATE_PRESSING:
            if (!pressed)
            {
                buttons[i].state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - buttons[i].change_us >= BUTTON_DEBOUNCE_US)
            {
                buttons[i].state = BUTTON_STATE_PRESSED;
                buttons[i].press_count++;
                
                // Вызываем действие, привязанное к этой конкретной кнопке
                if (buttons[i].on_press_cb != NULL) {
                    buttons[i].on_press_cb();
                }
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!pressed)
            {
                buttons[i].change_us = now_us;
                buttons[i].state = BUTTON_STATE_RELEASING;
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (pressed)
            {
                buttons[i].state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - buttons[i].change_us >= BUTTON_DEBOUNCE_US)
            {
                buttons[i].state = BUTTON_STATE_RELEASED;
            }
            break;
        }
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    if (button >= BUTTON_COUNT) return false;
    return buttons[button].state == BUTTON_STATE_PRESSED 
        || buttons[button].state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if (button >= BUTTON_COUNT) return 0;
    return buttons[button].press_count;
}