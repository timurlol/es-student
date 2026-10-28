#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PROFILING_MAX_STOPWATCHES 5

// по умолчанию профилирование включено
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

typedef struct {
    const char *name;   // Имя секундомера
    uint32_t count;     // Число замеров
    float mean;         // Среднее время в микросекундах
    uint32_t max;       // Максимальное время в микросекундах
} profiling_result_t;

// Ограничение: сумма замеров одного секундомера хранится в uint32_t,
// переполнится через ~71 минуту непрерывной работы.
void profiling_stopwatch_init(uint32_t id, const char *name);

#if PROFILING_ENABLED
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
#else
// профилирование выключено: вызовы ничего не делают
#define profiling_start(id) ((void)0)
#define profiling_stop(id)  ((void)0)
#endif

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);