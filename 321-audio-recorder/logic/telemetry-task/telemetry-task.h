// telemetry-task.h — задача телеметрии: с заданным периодом снимает напряжение
// и температуру и отдаёт запись функции, переданной при запуске.
// Как запись попадёт на компьютер, задача не знает.

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TELEMETRY_STATE_IDLE,
    TELEMETRY_STATE_RUN,
} telemetry_state_t;

// одна запись: время отсчёта по часам прибора и измеренные значения
typedef struct {
    uint64_t time_us;
    float voltage_v;
    float temp_c;
} telemetry_record_t;

// функция, которой задача отдаёт каждую запись
typedef void (*telemetry_task_callback_t)(const telemetry_record_t *record);

void telemetry_task_init(telemetry_task_callback_t on_record);

// вызывается на каждой итерации суперцикла, не ждёт
void telemetry_task_handle(void);

// false — период не принят
bool telemetry_task_start(uint32_t period_ms);
void telemetry_task_stop(void);

telemetry_state_t telemetry_task_get_state(void);
uint32_t telemetry_task_get_period_ms(void);