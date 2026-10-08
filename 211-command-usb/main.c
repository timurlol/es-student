#include "led.h"
#include "log.h"
#include "device.h"
#include "memory.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#define LINE_SIZE 32

typedef void (*command_handler_t)(void);

struct command_t
{
    const char *name;
    command_handler_t handler;
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

char line[LINE_SIZE];
uint line_length = 0;
const uint BUTTON_PIN = 13;
const uint DEBOUNCE_MS = 20;


bool get_button_debounce(uint pin)	
{
    bool state = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    return state && gpio_get(pin);
}

void cmd_enable(void)
{
	led_set(true);
	LOG_INF("led %s\n", led_is_on() ? "on" : "off");
}

void cmd_disable(void)
{
	led_set(false);
	LOG_INF("led %s\n", led_is_on() ? "on" : "off");
}

void cmd_info(void)
{
        device_info();
}

void cmd_version(void)
{
        log_version();
}
void cmd_ping(void)
{
    printf("pong\n");
}
void cmd_mem_info(void)
{
	mem_info();		
}

const struct command_t commands[] = {
    { "enable", cmd_enable },
    { "disable", cmd_disable },
    { "info", cmd_info },
    { "version", cmd_version },
    { "ping", cmd_ping },
    { "mem_info", cmd_mem_info },
};

void handle_command(const char *command)	//реагирование на команды
{
	for (uint i = 0; i < COMMAND_COUNT; i++)
	{
        	if (strcmp(command, commands[i].name) == 0)
        	{
        	    if (commands[i].handler != NULL)
        	    {
        	        commands[i].handler();
		    }

		return;
		}
	}
	LOG_ERR("unknown command: %s\n", command);
}

void read_line(void)	//функция приема строки команды
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        line[line_length] = '\0';

        if (line_length > 0)
        {
            LOG_DBG("got %s\n", line);
            handle_command(line);
        }

        line_length = 0;
        return;
    }

    if (line_length + 1 < LINE_SIZE)	//+1 для того, чтобы оставить место под \0
    {
        line[line_length] = (char)symbol;
        line_length = line_length + 1;
        putchar(symbol);
    }
}

int main()
{
	stdio_init_all();
	
	led_init();
	gpio_init(BUTTON_PIN);
	gpio_set_dir(BUTTON_PIN, GPIO_IN);
	gpio_pull_up(BUTTON_PIN);
	
    	bool previous = false;

	while(1)
	{
	        bool current = get_button_debounce(BUTTON_PIN);

        	if (previous == true && current == false)
		{
		    led_toggle();
		    LOG_INF("led %s\n", led_is_on() ? "on" : "off");
	        }

	        previous = current;

		read_line();
	}
}
