#pragma once

#include <stdbool.h>

typedef enum {
    BUTTON_1 = 0,
    BUTTON_2,     
    BUTTON_COUNT
} platform_button_t;

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(platform_button_t button);