#pragma once

#include <stdbool.h>
#include <stdint.h>

// занят ли АЦП захватом
bool platform_adc_busy(void);

// захват count отсчётов GP26 по будильнику с периодом period_us;
// false — АЦП занят или count больше PLATFORM_CAPTURE_MAX
#define PLATFORM_CAPTURE_MAX 2000
bool platform_capture_start(uint32_t count, uint32_t period_us);
bool platform_capture_done(void);

// итог захвата по будильнику: отсчёты, разброс интервалов
// и доля времени ядра внутри обработчика, в процентах
typedef struct {
    const uint16_t *samples;
    uint32_t count;
    uint32_t period_us;
    uint32_t min_interval_us;
    uint32_t max_interval_us;
    float handler_percent;
} platform_capture_t;

void platform_capture_get(platform_capture_t *capture);

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