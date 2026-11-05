#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include "stdlib.h"

static telemetry_state_t state;
static uint64_t period_us;
static uint32_t period_ms;
static uint64_t last_record_us;
static telemetry_task_callback_t on_record_cb;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    state = TELEMETRY_STATE_IDLE;
    on_record_cb = on_record;
}

bool telemetry_task_start(uint32_t new_period_ms)
{
    // Проверка минимального периода (ТЗ: не меньше 10 мс)
    if (new_period_ms < 10) {
        return false;
    }

    period_ms = new_period_ms;
    period_us = (uint64_t)period_ms * 1000;
    
    // Чтобы первая запись случилась сразу при запуске
    last_record_us = 0; 
    state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void)
{
    state = TELEMETRY_STATE_IDLE;
}

void telemetry_task_handle(void)
{
    if (state != TELEMETRY_STATE_RUN) {
        return;
    }

    if (systime_period_elapsed(&last_record_us, period_us)) {
        telemetry_record_t record;
        
        record.time_us = systime_us();
        record.voltage_v = platform_voltage_read();
        record.temp_c = platform_temp_read();      

        if (on_record_cb != NULL) {
            on_record_cb(&record);
        }
    }
}

telemetry_state_t telemetry_task_get_state(void)
{
    return state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return period_ms;
}