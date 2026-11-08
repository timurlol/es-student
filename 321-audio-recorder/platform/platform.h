#pragma once

#include <stdbool.h>
#include <stdint.h>

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);

void platform_charge_set(bool on);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(void);

// напряжение на входе АЦП, выводе GP26, в вольтах
float platform_voltage_read(void);

float platform_temp_read(void);

// индикатор: светодиод мигает по будильнику, независимо от суперцикла;
// false — период меньше 10 мс или больше 60 000 мс
bool platform_blink_start(uint32_t period_ms);

// гасит светодиод и возвращает число переключений с запуска
uint32_t platform_blink_stop(void);