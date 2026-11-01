#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include <stddef.h>

#define BUTTON_DEBOUNCE_US 20000
#define BUTTON_LONG_PRESS_US 1000000 // 1 секунда

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

typedef struct {
    button_state_t state;
    uint64_t change_us;        // Время последнего изменения уровня (для дребезга)
    uint64_t press_start_us;   // Время подтвержденного нажатия (для долгого нажатия)
    uint32_t press_count;
    button_task_callback_t on_press_cb;
    button_task_callback_t on_long_press_cb;
    bool long_press_reported;  // Сообщили ли мы уже о долгом нажатии?
} button_instance_t;

static button_instance_t buttons[BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press, button_task_callback_t on_long_press)
{
    if (button >= BUTTON_COUNT) return;
    
    buttons[button].state = BUTTON_STATE_RELEASED;
    buttons[button].change_us = 0;
    buttons[button].press_start_us = 0;
    buttons[button].press_count = 0;
    buttons[button].on_press_cb = on_press;
    buttons[button].on_long_press_cb = on_long_press;
    buttons[button].long_press_reported = false;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

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
                // Нажатие подтвердилось (дребезг прошел)
                buttons[i].state = BUTTON_STATE_PRESSED;
                buttons[i].press_start_us = now_us;
                buttons[i].long_press_reported = false;
                buttons[i].press_count++;
                
                // Если долгого действия нет — сообщаем о нажатии сразу
                if (buttons[i].on_long_press_cb == NULL && buttons[i].on_press_cb != NULL) {
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
            else if (now_us - buttons[i].press_start_us >= BUTTON_LONG_PRESS_US && !buttons[i].long_press_reported)
            {
                // Кнопку держат дольше секунды
                buttons[i].long_press_reported = true;
                if (buttons[i].on_long_press_cb != NULL) {
                    buttons[i].on_long_press_cb();
                }
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (pressed)
            {
                buttons[i].state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - buttons[i].change_us >= BUTTON_DEBOUNCE_US)
            {
                // Отпускание подтвердилось
                buttons[i].state = BUTTON_STATE_RELEASED;
                
                // Если есть долгое действие, и о долгом не сообщали — это было КОРОТКОЕ нажатие
                if (buttons[i].on_long_press_cb != NULL && !buttons[i].long_press_reported && buttons[i].on_press_cb != NULL) {
                    buttons[i].on_press_cb();
                }
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