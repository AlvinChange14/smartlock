// src/audio.h
#pragma once

#include <driver/i2s.h>
#include "config.h"
#include <math.h>

// I2S 驅動設定
#define I2S_PORT         I2S_NUM_0
#define SAMPLE_RATE      22050
#define SAMPLE_BITS      I2S_BITS_PER_SAMPLE_16BIT
#define AUDIO_AMPLITUDE  8000    // 0~32767，越大越響（注意不要破音）

// 音效定義：{ 頻率Hz, 持續ms }，結尾用 {0, 0}
struct ToneNote { int freq; int duration; };

static const ToneNote SOUND_STARTUP[]   = {{880,80},{1175,80},{1760,120},{0,0}};
static const ToneNote SOUND_UNLOCK[]    = {{784,80},{988,80},{1319,150},{0,0}};
static const ToneNote SOUND_DENY[]      = {{350,200},{250,300},{0,0}};
static const ToneNote SOUND_BEEP[]      = {{1200,60},{0,0}};
static const ToneNote SOUND_ALARM[]     = {{2000,180},{0,120},{2000,180},{0,120},{2000,180},{0,0}};
static const ToneNote SOUND_ENROLL_OK[] = {{1047,80},{1319,80},{1568,120},{0,0}};
static const ToneNote SOUND_LOWBATT[]   = {{600,150},{0,50},{600,150},{0,0}};

bool audioInitialized = false;

// ── 初始化 I2S ──────────────────────────────────
void initAudio() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = SAMPLE_BITS,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = 256,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCLK_PIN,
        .ws_io_num    = I2S_LRCLK_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("❌ I2S 安裝失敗: %d\n", err);
        return;
    }
    i2s_set_pin(I2S_PORT, &pins);
    i2s_zero_dma_buffer(I2S_PORT);

    audioInitialized = true;
    Serial.println("✅ I2S 音頻初始化完成");
}

// ── 產生並輸出單個音符（正弦波）──────────────
void playNote(int freqHz, int durationMs) {
    if (!audioInitialized || freqHz <= 0 || durationMs <= 0) {
        if (durationMs > 0) delay(durationMs);
        return;
    }

    int totalSamples = (SAMPLE_RATE * durationMs) / 1000;
    const int BUF_SIZE = 512;
    int16_t buf[BUF_SIZE * 2];   // 立體聲，左右各一份

    int written = 0;
    while (written < totalSamples) {
        int chunk = min(BUF_SIZE, totalSamples - written);
        for (int i = 0; i < chunk; i++) {
            float t = (float)(written + i) / SAMPLE_RATE;
            int16_t sample = (int16_t)(AUDIO_AMPLITUDE *
                             sinf(2.0f * M_PI * freqHz * t));
            buf[i * 2]     = sample;  // 左聲道
            buf[i * 2 + 1] = sample;  // 右聲道
        }
        size_t bytesOut;
        i2s_write(I2S_PORT, buf, chunk * 4, &bytesOut, portMAX_DELAY);
        written += chunk;
    }
}

// ── 播放音效序列 ──────────────────────────────
void playSound(const ToneNote* notes) {
    for (int i = 0; notes[i].freq != 0 || notes[i].duration != 0; i++) {
        playNote(notes[i].freq, notes[i].duration);
    }
}

// ── 非同步播放（使用 FreeRTOS task，不阻塞主迴圈）──
struct AsyncSoundArg { const ToneNote* notes; };

void asyncSoundTask(void* arg) {
    auto* a = (AsyncSoundArg*)arg;
    playSound(a->notes);
    delete a;
    vTaskDelete(NULL);
}

void playSoundAsync(const ToneNote* notes) {
    auto* arg = new AsyncSoundArg{notes};
    xTaskCreate(asyncSoundTask, "snd", 2048, arg, 1, NULL);
}