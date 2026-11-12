#include "stream-task/stream-task.h"
#include "platform/platform.h"
#include <stddef.h>

static stream_state_t state = STREAM_STATE_IDLE;
static stream_task_callback_t on_frame_cb = NULL;
static uint8_t stream_bits = 16;

static uint16_t frame_number = 0;
static uint32_t frames_sent = 0;
static uint32_t total_samples = 0;

void stream_task_init(stream_task_callback_t on_frame)
{
    state = STREAM_STATE_IDLE;
    on_frame_cb = on_frame;
}

bool stream_task_start(uint8_t bits)
{
    if (state == STREAM_STATE_RUN) return false;
    if (!platform_stream_start(bits)) return false;

    stream_bits = bits;
    frame_number = 0;
    frames_sent = 0;
    total_samples = 0;
    state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (state != STREAM_STATE_RUN) return;
    platform_stream_stop();
    state = STREAM_STATE_IDLE;
}

void stream_task_handle(void)
{
    if (state != STREAM_STATE_RUN) return;

    uint32_t samples = 0;
    const uint8_t *buffer = platform_stream_next(&samples);
    
    if (buffer != NULL && on_frame_cb != NULL)
    {
        stream_header_t header;
        header.sync_word = STREAM_SYNC_WORD;
        header.number = frame_number++;
        header.count = samples;

        // Считаем байты: если 16 бит, то samples * 2
        uint32_t bytes = samples * (stream_bits / 8);

        on_frame_cb(&header, buffer, bytes);

        // Сообщаем platform, что буфер свободен
        platform_stream_release();

        frames_sent++;
        total_samples += samples;
    }
}

stream_state_t stream_task_get_state(void) { return state; }
uint8_t stream_task_get_bits(void) { return stream_bits; }
uint32_t stream_task_get_frames(void) { return frames_sent; }
uint32_t stream_task_get_samples(void) { return total_samples; }
uint32_t stream_task_get_overruns(void) { return platform_stream_overruns(); }