#include "stdio-text-protocol/stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdbool.h>

static char buffer[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint buffer_caret = 0;
static bool truncated = false;

static command_t cmd_storage;
static command_t *cmd = &cmd_storage;

void stdio_text_protocol_init(void)
{
    buffer_caret = 0;
    truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
        return NULL;

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        buffer[buffer_caret] = '\0';

        if (buffer_caret > 0)
        {
            int i = 0;
            // 1. Пропускаем пробелы в начале кадра
            while (i < buffer_caret && buffer[i] == ' ') {
                i++;
            }

            // Если кадр состоял только из пробелов — игнорируем
            if (i >= buffer_caret) {
                stdio_text_protocol_init();
                return NULL;
            }

            cmd->name = &buffer[i];
            cmd->truncated = truncated;
            uint32_t args_count = 0;

            // 2. Ищем слова и заменяем пробелы на '\0'
            while (i < buffer_caret) {
                // Доходим до конца текущего слова
                while (i < buffer_caret && buffer[i] != ' ') {
                    i++;
                }
                
                if (i < buffer_caret) {
                    buffer[i] = '\0'; // Отрезаем слово
                    i++;
                    
                    // Пропускаем лишние пробелы между словами
                    while (i < buffer_caret && buffer[i] == ' ') {
                        i++;
                    }
                    
                    // Если после пробелов есть еще текст — это новый аргумент
                    if (i < buffer_caret) {
                        if (args_count >= STDIO_TEXT_PROTOCOL_MAX_ARGS) {
                            cmd->truncated = true;
                            break;
                        }
                        cmd->argv[args_count] = &buffer[i];
                        args_count++;
                    }
                }
            }
            
            cmd->argc = args_count;
            
            stdio_text_protocol_init();
            return cmd;
        }

        stdio_text_protocol_init();
        return NULL;
    }

    if (symbol == '\b' || symbol == 127)
    {
        if (buffer_caret > 0)
        {
            buffer_caret = buffer_caret - 1;
            putchar('\b');
            putchar(' ');
            putchar('\b');
        }
        return NULL;
    }

    if (buffer_caret + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
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