#pragma once

#include <stdint.h>
#include "pico/unique_id.h"

#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.2.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/timurlol/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

#define BOARD_ID_STR_SIZE (PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1)

typedef struct {
    const char *name;           // DEVICE_NAME
    const char *version;        // FIRMWARE_VERSION
    const char *project;        // DEVICE_PROJECT
    const char *repo;           // DEVICE_REPO
    const char *board;          // DEVICE_BOARD
    char serial[BOARD_ID_STR_SIZE]; // board_id (массив на 17 символов)
    uint32_t manufacturer;      // manufacturer
    uint32_t part;              // part
    uint32_t revision;          // revision
    const char *sdk_version;    // PICO_SDK_VERSION_STRING
} device_info_t;

void device_get_info(device_info_t *info);