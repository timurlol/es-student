#pragma once

#include <stdbool.h>
#include <stdint.h>

// Состояние задачи: не запускался, считает, готово
typedef enum {
    PI_STATE_NOT_STARTED,
    PI_STATE_RUNNING,
    PI_STATE_DONE,
} pi_state_t;

void pi_task_init(void);
void pi_task_handle(void);

// false — расчёт уже идёт
bool pi_task_start(uint32_t terms);

pi_state_t pi_task_get_state(void);

// Готов ли результат: если да, записать значение и время расчёта
bool pi_task_get_result(double *pi, uint32_t *time_ms);