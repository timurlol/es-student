#pragma once

#include "pico/types.h"
#include "pico/stdlib.h"

#define PROFILING_MAX_STOPWATCHES 5

#include <stdint.h>

typedef struct {
    const char *name;   // Имя секундомера (например, "loop", "command")
    uint32_t count;     // Число замеров
    float mean;      // Среднее время в микросекундах
    uint32_t max;       // Максимальное время в микросекундах
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);

void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);