#include "block-task/block-task.h"
#include "platform/platform.h"
#include <stddef.h>

static block_task_callback_t pending_cb = NULL;
static uint32_t expected_count = 0;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (!platform_block_start(count)) {
        return false;
    }
    
    pending_cb = on_block;
    expected_count = count;
    return true;
}

void block_task_handle(void)
{
    // Если коллбэка нет, значит ничего не запущено — выходим
    if (pending_cb == NULL) {
        return;
    }

    if (platform_block_done()) {
        // Забываем коллбэк ДО вызова, так как он может запустить следующий захват
        block_task_callback_t cb = pending_cb;
        pending_cb = NULL;

        cb(platform_block_samples(), expected_count, platform_block_time_us());
    }
}