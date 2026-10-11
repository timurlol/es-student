#include "memory.h"
#include "command.h"
#include "device.h"
#include <stdio.h>
#include "command.h"
#include <stdlib.h>
#include <stdint.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"

#define ROM_SIZE 16384 
#define SRAM_SIZE 270336

uint32_t data_variable = 100;
uint32_t bss_variable;

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;



static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void fw_info(void)
{
	data_variable++;
	bss_variable++;

	int main(void);	//адресса функций со сброшенным признаком Thumb
	uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);	
	uint16_t *fw_info_code = (uint16_t *)((uintptr_t)fw_info & ~1u);	
	
	uint32_t stack_variable = 1946;	//отображения стека и кучи
	uint32_t *heap_variable = malloc(sizeof(uint32_t));
	if (heap_variable != NULL)
	{
		*heap_variable = 1951;
	}
	
	printf("object          address     value\n");
	
	printf("%-15s 0x%08x  0x%04x\n","main",&main, *main_code);
	printf("%-15s 0x%08x  0x%04x\n","fw_info",&fw_info, *fw_info_code);
	
	printf("%-15s 0x%08x\n","commands",&commands[0]);
	
	for(int i=0;i<command_count;i++)
	{
		printf("%-15s 0x%08x\n",commands[i].name, (commands[i].handler) );
	}	
	
	printf("%-15s 0x%08x  %s\n","DEVICE_PROJECT",&DEVICE_PROJECT, DEVICE_PROJECT );
	printf("%-15s 0x%08x  %s\n","DEVICE_BOARD",&DEVICE_BOARD, DEVICE_BOARD );
	
	printf("%-15s 0x%08x  %d\n","data_variable",&data_variable, data_variable  );
	printf("%-15s 0x%08x  %d\n","bss_variable",&bss_variable, bss_variable  );
	printf("%-15s 0x%08x  %d\n","stack_variable", &stack_variable, stack_variable );
	printf("%-15s 0x%08x  %d\n","heap_variable",heap_variable,*heap_variable  );

	free(heap_variable);
}

void mem_info(void)
{
	printf("area       start      end        size\n");
	row("flash", (uintptr_t)XIP_BASE, (uintptr_t)(XIP_BASE + PICO_FLASH_SIZE_BYTES) );
	row("sram", (uintptr_t)SRAM_BASE, (uintptr_t)(SRAM_BASE + SRAM_SIZE) );
	row("rom", (uintptr_t)ROM_BASE, (uintptr_t)(ROM_BASE + ROM_SIZE) );
	
	row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);	
	row("free", (uintptr_t)&__flash_binary_end, (uintptr_t)(XIP_BASE + PICO_FLASH_SIZE_BYTES) );
	row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
	row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

	row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__);
	row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
	row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
	row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
	row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\ntotal\n");

    printf("  flash image  %zu = boot2 %zu + text %zu + data %zu\n",
           (size_t)((uintptr_t)&__flash_binary_end - (uintptr_t)&__flash_binary_start),
           (size_t)((uintptr_t)&__boot2_end__      - (uintptr_t)&__boot2_start__),
           (size_t)((uintptr_t)&__etext            - (uintptr_t)&__boot2_end__),
           (size_t)((uintptr_t)&__data_end__       - (uintptr_t)&__data_start__));

    printf("  flash free   %zu of %zu\n",
           (size_t)((uintptr_t)(XIP_BASE + PICO_FLASH_SIZE_BYTES) - (uintptr_t)&__flash_binary_end),
           (size_t)(uintptr_t)PICO_FLASH_SIZE_BYTES);

    printf("  ram used     %zu = data %zu + bss %zu\n",
           (size_t)((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__)
         + (size_t)((uintptr_t)&__bss_end__  - (uintptr_t)&__bss_start__),
           (size_t)((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__),
           (size_t)((uintptr_t)&__bss_end__  - (uintptr_t)&__bss_start__));

    printf("  ram free     %zu for heap and %zu for stack\n",
           (size_t)((uintptr_t)&__HeapLimit    - (uintptr_t)&__bss_end__),
           (size_t)((uintptr_t)&__StackTop     - (uintptr_t)&__StackBottom));	

}

