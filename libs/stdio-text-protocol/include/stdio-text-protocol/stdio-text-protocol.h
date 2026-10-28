#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STDIO_TEXT_PROTOCOL_LINE_SIZE 64
#define STDIO_TEXT_PROTOCOL_MAX_ARGS 4

// команда, принятая из одной строки
typedef struct
{
    const char *name;
    uint32_t argc;
    const char *argv[STDIO_TEXT_PROTOCOL_MAX_ARGS];

    // строка длиннее буфера или аргументов больше, чем помещается в argv:
    // команда принята не целиком, выполнять её нельзя
    bool truncated;
} command_t;

void stdio_text_protocol_init(void);

// Читает один символ, не ожидая его. Когда строка закончена, возвращает
// команду, иначе NULL. Команда действительна до следующего вызова.
const command_t *stdio_text_protocol_handle(void);