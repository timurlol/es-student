#include "api.h"
#include "device.h"
#include "profiling.h"
#include "button-task.h"
#include "pi-task.h"
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
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", info.name);
    printf("version: %s\n", info.version);
    printf("project: %s\n", info.project);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", 
           (unsigned)info.manufacturer, 
           (unsigned)info.part, 
           (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void clbk_pi_start(const command_t *command)
{
    uint32_t terms = 1000000; // значение по умолчанию

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            printf("error: usage pi_start [<terms>]\n");
            return;
        }
        if (terms == 0)
        {
            printf("error: terms must be greater than 0\n");
            return;
        }
    }
    else if (command->argc > 1)
    {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void clbk_pi(const command_t *command)
{
    double pi;
    uint32_t time_ms;

    if (pi_task_get_result(&pi, &time_ms))
    {
        printf("pi: %.9f in %u ms\n", pi, (unsigned)time_ms);
        return;
    }

    switch (pi_task_get_state())
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not started\n");
        break;
    case PI_STATE_RUNNING:
        printf("pi: running\n");
        break;
    default:
        break;
    }
}

static void clbk_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void clbk_profiling(const command_t *command)
{
    profiling_result_t result;

    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (profiling_get(i, &result)) { 
            printf("%-10s count %-10u mean %.2f us  max %u us\n", 
               result.name, 
               (unsigned)result.count, 
               result.mean, 
               (unsigned)result.max);
        }
    }
}

static void clbk_profiling_reset(const command_t *command)
{
    profiling_reset();
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

static void clbk_button(const command_t *command)
{
    printf("button: %s, presses %u\n",
           button_task_is_pressed() ? "pressed" : "released",
           (unsigned)button_task_get_press_count());
}

// --- Таблица команд ---

const api_command_t api_commands[] = {
    { "info",            "Print device info",           clbk_info },
    { "pi_start",        "Start pi calculation [<terms>]", clbk_pi_start },
    { "pi",              "Print pi result or state",       clbk_pi },
    { "uptime",          "Print uptime in ms",          clbk_uptime },
    { "led_enable",      "Turn led on",                 clbk_led_enable },
    { "led_disable",     "Turn led off",                clbk_led_disable },
    { "led_blink",       "Blink led",                   clbk_led_blink },
    { "led_period",      "Set blink period in ms",      clbk_led_period },
    { "button",          "Print button state and press count", clbk_button },
    { "profiling_reset", "Reset all stopwatches", clbk_profiling_reset },
    { "profiling",       "Prints the results of all initialized stopwatches since the last reset in ms", clbk_profiling },
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);