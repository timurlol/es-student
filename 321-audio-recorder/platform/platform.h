#pragma once

#include <stdbool.h>
#include <stdint.h>
// частота отсчётов АЦП в блоке и потоке
#define PLATFORM_SAMPLE_RATE_HZ 44100

// блок count отсчётов GP26 через DMA, о конце сообщает прерывание DMA;
// false — АЦП занят или count больше PLATFORM_BLOCK_MAX
#define PLATFORM_BLOCK_MAX 8192
bool platform_block_start(uint32_t count);
bool platform_block_done(void);
const uint16_t *platform_block_samples(void);

// время захвата блока, от запуска АЦП до последнего отсчёта
uint32_t platform_block_time_us(void);

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

#define PLATFORM_STREAM_BYTES 1024

// поток: два буфера по PLATFORM_STREAM_BYTES байт по очереди; false — АЦП занят
bool platform_stream_start(uint8_t bits);
void platform_stream_stop(void);

// следующий заполненный буфер по порядку или NULL, если он ещё не готов;
// samples — число отсчётов в буфере
const uint8_t *platform_stream_next(uint32_t *samples);

// буфер отправлен, его можно заполнять снова
void platform_stream_release(void);

// сколько раз DMA застал буфер неотправленным
uint32_t platform_stream_overruns(void);

// отправляет байты в порт как есть, без замены \n на \r\n
void platform_write(const void *data, uint32_t bytes);