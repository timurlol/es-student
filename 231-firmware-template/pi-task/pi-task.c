#include "pi-task.h"

#include "pico/stdlib.h"

// Порция членов ряда за вызов обработчика.
// Член ряда стоит около 585 тактов, при 125 МГц это 4,7 мкс;
// 100 членов — около 0,47 мс, что укладывается в 0,5 мс.
#define PI_TERMS_PER_CALL 100

// Число членов по умолчанию для команды pi_start без аргумента
#define PI_DEFAULT_TERMS 1000000

// Состояние задачи: внутреннее связывание, только для этого файла
static pi_state_t pi_state;

// Переменные расчёта
static uint32_t pi_n;      // сколько членов всего
static uint32_t pi_k;      // на каком члене остановились
static double pi_sum;
static double pi_sign;
static double pi_result;
static uint32_t pi_start_ms;
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
    // Новый расчёт не запускается, пока не закончен текущий
    if (pi_state == PI_STATE_RUNNING)
    {
        return false;
    }

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    pi_start_ms = time_us_32() / 1000;
    pi_time_ms = 0;
    pi_state = PI_STATE_RUNNING;
    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
    {
        return;
    }

    // Конец порции. Возле UINT32_MAX сумма не помещается в 32 бита
    // и становится маленьким числом — тогда концом порции станет
    // последний член ряда.
    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    if (end < pi_k || end > pi_n)
    {
        end = pi_n;
    }

    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        pi_result = 4.0 * pi_sum;
        pi_time_ms = time_us_32() / 1000 - pi_start_ms;
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
    {
        return false;
    }

    *pi = pi_result;
    *time_ms = pi_time_ms;
    return true;
}