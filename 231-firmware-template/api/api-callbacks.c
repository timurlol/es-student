#include "api.h"
#include "device.h"
#include "profiling.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdint.h>
#include "api-commands.h"

#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include "led-task/led-task.h"

#define CALC_PI_TERMS 1000000
volatile double pi_result;
static volatile bool is_pi_calculating = false;

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

double calc_pi(uint terms)
{
    double sum = 0.0;
    double sign = 1.0;

    for (uint k = 0; k < terms; k++)
    {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }

    return sum * 4.0;
}

// --- Обработчики команд (callbacks) ---

static void clbk_info(const command_t *command)
{
    device_info();
}

static void clbk_calc_pi(const command_t *command)
{
    uint64_t start_us = time_us_64();
    uint32_t value;
    if(is_pi_calculating)
    {
        printf("error: pi is busy\n");
        return;
    }
    if(command->argc == 0)
    {
        is_pi_calculating = true;
        pi_result = calc_pi(CALC_PI_TERMS);
    }
    else if(command->argc == 1)
    {
        if(parse_u32(command->argv[0],&value))
        {
            if(value == 0)
            {
                printf("error: terms must be greater than 0\n");
                return;
            }
            is_pi_calculating = true;
            pi_result = calc_pi(value);
        }
        else
        {
            printf("error: usage calc_pi [<terms>]\n");
            return;
        }
    }
    else
    {
        printf("error: usage calc_pi [<terms>]\n");
        return;
    }

    uint64_t spent_us = time_us_64() - start_us;
    is_pi_calculating = false;
    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
}

static void clbk_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void clbk_main_time_exec(const command_t *command)
{
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(),
           (unsigned)profiling_max_us());
}

static void clbk_main_time_reset(const command_t *command)
{
    profiling_reset_max();
    printf("max reset\n");
}

// Общий ответ: состояние и период светодиода
static void print_led_state(void)
{
    const char *state_name;

    switch (led_task_get_state()) {
    case LED_STATE_OFF:
        state_name = "off";
        break;
    case LED_STATE_ON:
        state_name = "on";
        break;
    case LED_STATE_BLINK:
    default:
        state_name = "blink";
        break;
    }

    printf("led: %s, period %u ms\n",
           state_name,
           (unsigned)led_task_get_period_ms());
}

static void clbk_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void clbk_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void clbk_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void clbk_led_period(const command_t *command)
{
    uint32_t value_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &value_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    // годится ли такой период, решает задача светодиода
    if (!led_task_set_period_ms(value_ms))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    print_led_state();
}

// --- Таблица команд ---

const api_command_t api_commands[] = {
    { "info",            "Print device info",           clbk_info },
    { "calc_pi",         "Calculate Pi (Leibniz)",      clbk_calc_pi },
    { "uptime",          "Print uptime in ms",          clbk_uptime },
    { "main_time_exec",  "Print loop iteration time",   clbk_main_time_exec },
    { "main_time_reset", "Reset max iteration time",    clbk_main_time_reset },
    { "led_enable",      "Turn led on",                 clbk_led_enable },
    { "led_disable",     "Turn led off",                clbk_led_disable },
    { "led_blink",       "Blink led",                   clbk_led_blink },
    { "led_period",      "Set blink period in ms",      clbk_led_period },
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);