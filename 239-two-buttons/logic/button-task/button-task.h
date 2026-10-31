#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform/platform.h"

// функция, которую задача вызывает на каждое нажатие
typedef void (*button_task_callback_t)(void);

void button_task_init(platform_button_t button, button_task_callback_t on_press);

// Обработка всех кнопок (вызывается в суперцикле)
void button_task_handle(void);

// Запрос состояния по номеру кнопки
bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);