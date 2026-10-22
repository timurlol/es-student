#include "stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdbool.h>

static char buffer[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint buffer_caret = 0;
static bool truncated = false;

// Создаём структуру в памяти и указатель на неё
static command_t cmd_storage;
static command_t* cmd = &cmd_storage;

void stdio_text_protocol_init(void)
{
    buffer_caret = 0;
    truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        buffer[buffer_caret] = '\0';

        if (buffer_caret > 0)
        {
            cmd->name = &buffer[0];
            cmd->truncated = truncated; // Переносим флаг из глобальной переменной
            uint args_count = 0;

            for(int i=0; i < buffer_caret; i++)
            {
                if(buffer[i] == ' ')
                {
                    buffer[i] = '\0';
                    
                    if(args_count >= STDIO_TEXT_PROTOCOL_MAX_ARGS)
                    {
                        cmd->truncated = true; // Ставим флаг, если аргументов слишком много
                        break;
                    }
                    
                    cmd->argv[args_count] = &buffer[i+1];
                    args_count++;
                }    
            }
            
            cmd->argc = args_count;
            
            stdio_text_protocol_init(); // Сбрасываем состояние для следующей команды
            return cmd;
        }

        stdio_text_protocol_init();
        return NULL;
    }

    if (symbol == '\b' || symbol == 127)
    {
        if(buffer_caret > 0)
        {
            buffer_caret = buffer_caret - 1;
            
            // ANSI-последовательности:
            // \033[D — сдвинуть курсор на 1 позицию влево
            // \033[K — стереть всё от курсора до конца строки
            printf("\033[D\033[K");
        }
        return NULL;
    }

    if (buffer_caret + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE) //+1 для того, чтобы оставить место под \0
    {
        buffer[buffer_caret] = (char)symbol;
        buffer_caret = buffer_caret + 1;
        putchar(symbol);
        return NULL;
    }
    else
    {
        truncated = true;
        return NULL;
    }
}