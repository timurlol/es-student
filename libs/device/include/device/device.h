#pragma once

#include <stdint.h>

// 8 байт идентификатора платы → 16 hex-цифр + нулевой символ
#define BOARD_ID_STR_SIZE 17

typedef struct {
    const char *board;
    char serial[BOARD_ID_STR_SIZE];
    uint32_t manufacturer;
    uint32_t part;
    uint32_t revision;
    const char *sdk_version;
} device_info_t;

void device_get_info(device_info_t *info);