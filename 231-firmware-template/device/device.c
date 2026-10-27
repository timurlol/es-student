#include "device.h"
#include "pico/version.h"
#include <stdio.h>
#include <stddef.h>
#include "pico/unique_id.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

void device_get_info(device_info_t *info)
{
	if (info == NULL)
		return;

    info->name = DEVICE_NAME;
    info->version = FIRMWARE_VERSION;
    info->project = DEVICE_PROJECT;
    info->repo = DEVICE_REPO;
    info->board = DEVICE_BOARD;
    info->sdk_version = PICO_SDK_VERSION_STRING;
    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));
    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;
    info->manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
}