#include "api/api-commands.h"
#include "firmware.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define TELEMETRY_DEFAULT_PERIOD_MS 100
#define CHARGE_STATE_ON true
#define CHARGE_STATE_OFF false

static bool parse_u32(const char *text, uint32_t *value)
{
    if (!isdigit((unsigned char)text[0]))
        return false;

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
        return false;

    *value = (uint32_t)number;
    return true;
}

static void clbk_info(const command_t *command)
{
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void clbk_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;

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
    case PI_STATE_NOT_STARTED: printf("pi: not started\n"); break;
    case PI_STATE_RUNNING:     printf("pi: running\n");     break;
    default: break;
    }
}

static void clbk_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", (unsigned long long)systime_ms());
}

static void clbk_profiling(const command_t *command)
{
#if PROFILING_ENABLED
    profiling_result_t result;
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (profiling_get(i, &result))
        {
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   result.name, (unsigned)result.count, result.mean, (unsigned)result.max);
        }
    }
#else
    (void)command;
    printf("profiling is disabled in this build\n");
#endif
}

static void clbk_profiling_reset(const command_t *command)
{
#if PROFILING_ENABLED
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
#else
    (void)command;
    printf("profiling is disabled in this build\n");
#endif
}

static void print_led_state(void)
{
    const char *state_name;

    switch (led_task_get_state()) {
    case LED_STATE_OFF:  state_name = "off";   break;
    case LED_STATE_ON:   state_name = "on";    break;
    case LED_STATE_BLINK:
    default:             state_name = "blink"; break;
    }

    printf("led: %s, period %u ms\n", state_name, (unsigned)led_task_get_period_ms());
}

static void clbk_charge_enable(const command_t *command)
{
    platform_charge_set(CHARGE_STATE_ON);
    printf("charge: on\n");
}
static void clbk_charge_disable(const command_t *command)
{
    platform_charge_set(CHARGE_STATE_OFF);
    printf("charge: off\n");
}

static void clbk_led_enable(const command_t *command)  { led_task_set_state(LED_STATE_ON);    print_led_state(); }
static void clbk_led_disable(const command_t *command) { led_task_set_state(LED_STATE_OFF);   print_led_state(); }
static void clbk_led_blink(const command_t *command)   { led_task_set_state(LED_STATE_BLINK); print_led_state(); }

static void clbk_led_period(const command_t *command)
{
    uint32_t value_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &value_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

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

// напряжение на GP26 у platform, ответ voltage: <напряжение> V
static void clbk_get_voltage(const command_t *command)
{
    printf("voltage: %.3f V\n", platform_voltage_read() );
}

// температура у platform, ответ temp: <температура> C
static void clbk_get_temp(const command_t *command)
{
    printf("temp: %.1f C\n", platform_temp_read() );
}

// Печать записи, которую задача телеметрии передает через коллбэк
void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", 
           (unsigned long long)record->time_us, 
           record->voltage_v, 
           record->temp_c);
}

static void clbk_tm_start(const command_t *command)
{
    uint32_t period_ms = TELEMETRY_DEFAULT_PERIOD_MS;

    if (command->argc == 1) {
        if (!parse_u32(command->argv[0], &period_ms)) {
            printf("error: usage tm_start [<period_ms>]\n");
            return;
        }
    } else if (command->argc > 1) {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    // Задача сама отклонит период < 10 мс
    if (!telemetry_task_start(period_ms)) {
        printf("error: period_ms must be at least 10\n");
        return;
    }

    printf("telemetry: started, period %u ms\n", (unsigned)period_ms);
}

static void clbk_tm_stop(const command_t *command)
{
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

// --- Таблица команд ---
const api_command_t api_commands[] = {
    { "help",            "list of commands",            NULL },
    { "info",            "Print device info",           clbk_info },
    { "pi_start",        "Start pi calculation [<terms>]", clbk_pi_start },
    { "pi",              "Print pi result or state",    clbk_pi },
    { "uptime",          "Print uptime in ms",          clbk_uptime },
    { "led_enable",      "Turn led on",                 clbk_led_enable },
    { "led_disable",     "Turn led off",                clbk_led_disable },
    { "led_blink",       "Blink led",                   clbk_led_blink },
    { "led_period",      "Set blink period in ms",      clbk_led_period },
    { "button",          "Print button state and press count", clbk_button },
    { "profiling_reset", "Reset all stopwatches",       clbk_profiling_reset },
    { "profiling",       "Prints the results of all initialized stopwatches since the last reset in ms", clbk_profiling },
    { "get_voltage",     "voltage on GP26, ADC input 0", clbk_get_voltage },
    { "get_temp",        "chip temperature, degrees Celsius", clbk_get_temp },
    { "tm_start",        "Start telemetry [<period_ms>]", clbk_tm_start },
    { "tm_stop",         "Stop telemetry",              clbk_tm_stop },
    { "charge_on",       "Turn on the chrage",        clbk_charge_enable },
    { "charge_off",      "Turn off the chrage",      clbk_charge_disable },
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);