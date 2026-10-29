#include "pi-task.h"

#include "systime/systime.h"

// Порция членов ряда за вызов обработчика.
// Член ряда стоит около 585 тактов, при 125 МГц это 4,7 мкс;
// 100 членов — около 0,47 мс, что укладывается в 0,5 мс.
#define PI_TERMS_PER_CALL 100

static pi_state_t pi_state;

static uint32_t pi_n;
static uint32_t pi_k;
static double   pi_sum;
static double   pi_sign;
static double   pi_result;
static uint64_t pi_start_ms;
static uint32_t pi_time_ms;

void pi_task_init(void)
{
    pi_state = PI_STATE_NOT_STARTED;
    pi_n = 0;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    pi_start_ms = 0;
    pi_time_ms = 0;
}

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_STATE_RUNNING)
        return false;

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    pi_start_ms = systime_ms();
    pi_time_ms = 0;
    pi_state = PI_STATE_RUNNING;
    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
        return;

    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    if (end < pi_k || end > pi_n)
        end = pi_n;

    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        pi_result = 4.0 * pi_sum;
        pi_time_ms = (uint32_t)(systime_ms() - pi_start_ms);
        pi_state = PI_STATE_DONE;
    }
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

bool pi_task_get_result(double *pi, uint32_t *time_ms)
{
    if (pi_state != PI_STATE_DONE)
        return false;

    *pi = pi_result;
    *time_ms = pi_time_ms;
    return true;
}