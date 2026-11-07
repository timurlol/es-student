#pragma once

#include <stdbool.h>

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);

void platform_charge_set(bool on);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(void);

// напряжение на входе АЦП, выводе GP26, в вольтах
float platform_voltage_read(void);

float platform_temp_read(void);