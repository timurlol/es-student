#include "log.h"
#include "device.h"
#include "profiling.h"
#include <stdio.h>
#include <string.h>
#include "stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

int main()
{
	stdio_init_all();

	profiling_init();
	led_task_init();
	button_task_init();
	pi_task_init();
	stdio_text_protocol_init();

	while(1)
	{
		profiling_iteration();

		led_task_handle();
		button_task_handle();
		pi_task_handle();

		const command_t *command = stdio_text_protocol_handle();
		if (command != NULL)
			api_handle(command);
	}
}