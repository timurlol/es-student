#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

// ТЗ 2.3: Кнопка замыкает GP15 на землю. Светодиод — встроенный.
#define PLATFORM_LED_PIN 25
#define PLATFORM_CAPACITOR_PIN 22
#define PLATFORM_BUTTON_PIN 13

#define ADC_CLOCK_HZ 48000000.0f

static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

// кто сейчас работает с АЦП; меняют и функции, и обработчики прерываний
typedef enum {
    ADC_FREE,
    ADC_CAPTURE,
    ADC_BLOCK,
    ADC_STREAM,
} adc_user_t;

static volatile adc_user_t adc_user;

static repeating_timer_t capture_timer;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static uint32_t capture_count;
static uint32_t capture_period_us;
static volatile uint32_t capture_index;
static volatile uint32_t capture_handler_us;

static int stream_channels[2];
// Выравнивание по 4 байта важно для DMA!
static uint8_t stream_buffers[2][PLATFORM_STREAM_BYTES] __attribute__((aligned(4)));
static volatile bool stream_ready[2];
static volatile uint32_t stream_overruns;
static uint32_t stream_samples;
static uint32_t stream_next;

// Вспомогательные функции для переключения режима АЦП
static void adc_free_running(void)
{
    adc_select_input(ADC_CHANNEL);
    // Включаем FIFO, генерируем DMA запрос на каждый отсчет
    adc_fifo_setup(true, true, 1, false, false);
    // Делитель для 44.1 кГц (48 МГц / 44100 = 1088.4)
    adc_set_clkdiv(ADC_CLOCK_HZ / PLATFORM_SAMPLE_RATE_HZ - 1);
}

static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_fifo_setup(false, false, 0, false, false);
    adc_set_clkdiv(0);
}

// обработчик прерывания DMA_IRQ_1: блок захвачен
static void on_block_done(void)
{
    dma_channel_acknowledge_irq1(block_channel);
    block_end_us = time_us_32();
    adc_one_shot();
    block_ready = true;
    adc_user = ADC_FREE;
}

// Обработчик прерывания DMA_IRQ_0
static void on_stream_buffer(void)
{
    for (int i = 0; i < 2; i++)
    {
        if (dma_channel_get_irq0_status(stream_channels[i]))
        {
            dma_channel_acknowledge_irq0(stream_channels[i]);

            // Возвращаем адрес записи в начало буфера
            dma_channel_set_write_addr(stream_channels[i], stream_buffers[i], false);

            // Если следующий буфер еще не отправлен, фиксируем переполнение
            if (stream_ready[(i + 1) % 2])
            {
                stream_overruns++;
            }
            stream_ready[i] = true;
        }
    }
}

void platform_init(void)
{
    stdio_init_all();

    gpio_init(PLATFORM_LED_PIN);
    gpio_set_dir(PLATFORM_LED_PIN, GPIO_OUT);

    gpio_init(PLATFORM_CAPACITOR_PIN);
    gpio_set_dir(PLATFORM_CAPACITOR_PIN, GPIO_OUT);

    gpio_init(PLATFORM_BUTTON_PIN);
    gpio_set_dir(PLATFORM_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PLATFORM_BUTTON_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);

    adc_set_temp_sensor_enabled(true);

    // Настройка канала DMA для блока
    block_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq1_enabled(block_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, on_block_done);
    irq_set_enabled(DMA_IRQ_1, true);

    // Инициализация каналов потока
    stream_channels[0] = dma_claim_unused_channel(true);
    stream_channels[1] = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, on_stream_buffer);
    irq_set_enabled(DMA_IRQ_0, true);
}

bool platform_stream_start(uint8_t bits)
{
    if (adc_user != ADC_FREE || bits != 16) {
        return false;
    }

    stream_samples = PLATFORM_STREAM_BYTES / 2; // 16 бит = 2 байта
    adc_free_running();

    for (int i = 0; i < 2; i++)
    {
        int next_channel = stream_channels[(i + 1) % 2];

        dma_channel_config cfg = dma_channel_get_default_config(stream_channels[i]);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        channel_config_set_chain_to(&cfg, next_channel); // Цепочка!
        
        dma_channel_configure(stream_channels[i], &cfg, stream_buffers[i], &adc_hw->fifo,
                              stream_samples, false);
        dma_channel_set_irq0_enabled(stream_channels[i], true);
        stream_ready[i] = false;
    }

    stream_overruns = 0;
    stream_next = 0;
    adc_user = ADC_STREAM;
    
    dma_channel_start(stream_channels[0]); // Запуск первого канала
    adc_run(true); // Запуск АЦП
    return true;
}

void platform_stream_stop(void)
{
    if (adc_user != ADC_STREAM) return;

    adc_run(false);

    // Обход эратты RP2040-E13: разрываем цепочку
    for (int i = 0; i < 2; i++)
    {
        dma_channel_config cfg = dma_get_channel_config(stream_channels[i]);
        channel_config_set_chain_to(&cfg, stream_channels[i]); // Сам на себя
        dma_channel_set_config(stream_channels[i], &cfg, false);
    }

    for (int i = 0; i < 2; i++)
    {
        dma_channel_set_irq0_enabled(stream_channels[i], false);
        dma_channel_abort(stream_channels[i]);
        dma_channel_acknowledge_irq0(stream_channels[i]);
    }
    
    adc_one_shot();
    adc_user = ADC_FREE;
}

const uint8_t *platform_stream_next(uint32_t *samples)
{
    if (!stream_ready[stream_next]) {
        return NULL;
    }
    *samples = stream_samples;
    return stream_buffers[stream_next];
}

void platform_stream_release(void)
{
    stream_ready[stream_next] = false;
    stream_next = (stream_next + 1) % 2; // Переключаемся на другой буфер
}

uint32_t platform_stream_overruns(void)
{
    return stream_overruns;
}

void platform_write(const void *data, uint32_t bytes)
{
    // false, false — без замены \n на \r\n
    stdio_put_string((const char *)data, (int)bytes, false, false);
}

bool platform_block_start(uint32_t count)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        return false;
    }

    adc_free_running();

    // Настройка DMA: 16-бит, чтение из одного адреса (FIFO), запись в массив
    dma_channel_config cfg = dma_channel_get_default_config(block_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC); // Ждем запроса от АЦП

    block_ready = false;
    adc_user = ADC_BLOCK;

    // Настраиваем и сразу запускаем (последний аргумент true)
    dma_channel_configure(block_channel, &cfg, block_samples, &adc_hw->fifo, count, true);
    block_start_us = time_us_32();
    adc_run(true); // Запускаем АЦП

    return true;
}

bool platform_block_done(void)
{
    return block_ready;
}

const uint16_t *platform_block_samples(void)
{
    return block_samples;
}

uint32_t platform_block_time_us(void)
{
    return block_end_us - block_start_us;
}

void platform_led_set(bool on)
{
    gpio_put(PLATFORM_LED_PIN, on);
}

void platform_charge_set(bool on)
{
    gpio_put(PLATFORM_CAPACITOR_PIN, on);
}

bool platform_button_read(void)
{
    // Нажатая кнопка замыкает вывод на землю и даёт ноль
    return gpio_get(PLATFORM_BUTTON_PIN) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    return ( 27.0 - ( (adc_read() * ADC_VREF_V / ADC_LEVELS) - 0.706) / 0.001721 );
}

static repeating_timer_t blink_timer;
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// обработчик будильника индикатора
static bool on_blink(repeating_timer_t *timer)
{
    blink_on = !blink_on;
    gpio_put(PLATFORM_LED_PIN, blink_on);
    blink_toggles++;

    // true — вызвать снова через тот же период
    return true;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10 || period_ms > 60000)
    {
        return false;
    }

    platform_blink_stop();
    blink_toggles = 0;

    // минус перед периодом — отсчитывать от начала прошлого вызова
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);
    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(PLATFORM_LED_PIN, 0);
    return blink_toggles;
}

// обработчик будильника захвата: время и отсчёт, затем время своей работы
static bool on_capture(repeating_timer_t *timer)
{
    uint32_t start_us = time_us_32();
    capture_times[capture_index] = start_us;
    capture_samples[capture_index] = adc_read();
    capture_index++;
    capture_handler_us += time_us_32() - start_us;

    if (capture_index < capture_count)
    {
        return true;
    }

    // захват окончен: АЦП свободен, будильник снимается
    adc_user = ADC_FREE;
    return false;
}

bool platform_adc_busy(void)
{
    return (adc_user != ADC_FREE);
}

bool platform_capture_start(uint32_t count, uint32_t period_us)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_CAPTURE_MAX || period_us == 0)
    {
        return false;
    }

    adc_select_input(ADC_CHANNEL);
    capture_count = count;
    capture_period_us = period_us;
    capture_index = 0;
    capture_handler_us = 0;
    adc_user = ADC_CAPTURE;

    if (!add_repeating_timer_us(-(int64_t)period_us, on_capture, NULL, &capture_timer))
    {
        adc_user = ADC_FREE;
        return false;
    }
    return true;
}

bool platform_capture_done(void)
{
    // Захват завершен, если мы его запускали (count > 0) и набрали все отсчеты
    return (capture_count > 0) && (capture_index >= capture_count);
}

void platform_capture_get(platform_capture_t *capture)
{
    capture->samples = capture_samples;
    capture->count = capture_index;
    capture->period_us = capture_period_us;

    if (capture_index < 2) {
        capture->min_interval_us = 0;
        capture->max_interval_us = 0;
        capture->handler_percent = 0.0f;
        return;
    }

    uint32_t min_int = UINT32_MAX;
    uint32_t max_int = 0;
    
    // Считаем разброс интервалов между соседними отсчетами
    for (uint32_t i = 1; i < capture_index; i++) {
        uint32_t diff = capture_times[i] - capture_times[i - 1];
        if (diff < min_int) min_int = diff;
        if (diff > max_int) max_int = diff;
    }
    
    capture->min_interval_us = min_int;
    capture->max_interval_us = max_int;

    // Доля времени ядра в процентах от общего времени захвата
    uint32_t total_time = capture_times[capture_index - 1] - capture_times[0];
    if (total_time > 0) {
        capture->handler_percent = (float)capture_handler_us * 100.0f / total_time;
    } else {
        capture->handler_percent = 0.0f;
    }
}