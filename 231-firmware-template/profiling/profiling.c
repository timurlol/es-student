#include "profiling.h"

// Это находится в profiling.c
typedef struct {
    uint32_t time;
    uint32_t sum_time;
    bool is_running;
} stopwatch_t;

static stopwatch_t watches[PROFILING_MAX_STOPWATCHES];

profiling_result_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_MAX_STOPWATCHES)
        return;
    stopwatches[id].name = name;
    stopwatches[id].count = 0;
    stopwatches[id].mean = 0;
    stopwatches[id].max = 0;
    watches[id].sum_time = 0;
}

void profiling_start(uint32_t id)
{
    watches[id].time = time_us_32();
    watches[id].is_running = true;
}
void profiling_stop(uint32_t id)
{
    if(!watches[id].is_running)
        return;
    watches[id].time = time_us_32() - watches[id].time;
    watches[id].is_running = false;
    watches[id].sum_time = watches[id].sum_time + watches[id].time;

    stopwatches[id].count = stopwatches[id].count + 1;
    stopwatches[id].mean = watches[id].sum_time / stopwatches[id].count;
    if(stopwatches[id].max < watches[id].time)
        stopwatches[id].max = watches[id].time;
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES) {
        return false;
    }
    if (stopwatches[id].name == NULL) {
        return false;
    }
    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    result->mean = stopwatches[id].mean;
    result->max = stopwatches[id].max;
    return true;
}
void profiling_reset(void)
{
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        stopwatches[i].count = 0;
        stopwatches[i].mean = 0;
        stopwatches[i].max = 0;
 
        watches[i].time = 0;
        watches[i].sum_time = 0;
        watches[i].is_running = false;
    }
}
