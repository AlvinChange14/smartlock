// src/config.h
#pragma once

// =============================================
//  WiFi
// =============================================
#define WIFI_SSID          "YOUR_WIFI_SSID"
#define WIFI_PASS          "YOUR_WIFI_PASSWORD"
#define WIFI_TIMEOUT_SEC   20          // 連線逾時秒數

// =============================================
//  Telegram Bot
// =============================================
#define BOT_TOKEN          "YOUR_BOT_TOKEN"   // 從 @BotFather 取得
#define CHAT_ID            "YOUR_CHAT_ID"     // 純數字，從 getUpdates 取得
#define BOT_POLL_MS        3000             // 輪詢間隔（毫秒），勿低於 1000

// =============================================
//  天氣 API (OpenWeatherMap)
// =============================================
#define OWM_API_KEY        "YOUR_OPENWEATHER_API_KEY"
#define OWM_CITY           "Taipei"
#define OWM_COUNTRY        "TW"
#define OWM_LANG           "zh_tw"
#define WEATHER_UPDATE_MS  (10 * 60 * 1000UL)  // 每 10 分鐘更新

// =============================================
//  NTP 時間同步
// =============================================
#define NTP_SERVER1        "pool.ntp.org"
#define NTP_SERVER2        "time.asia.apple.com"
#define TZ_OFFSET_SEC      (8 * 3600)    // UTC+8 台灣
#define DST_OFFSET_SEC     0

// =============================================
//  密碼設定
// =============================================
#define DEFAULT_PASSWORD   "1234"        // ⚠️ 上線前務必修改！
#define ADMIN_PASSWORD     "9999"        // 進入管理模式的密碼（與一般密碼不同）
#define MAX_PASSWORD_LEN   8
#define MAX_FAIL_ATTEMPTS  5             // 連續失敗幾次觸發警報

// =============================================
//  硬體腳位
// =============================================
// I2C Bus（OLED + PCF8574 共用）
#define I2C_SDA_PIN        5    // D4
#define I2C_SCL_PIN        6    // D5

// AS608 指紋（UART1）
#define AS608_TX_PIN       43   // D6 → AS608 RX
#define AS608_RX_PIN       44   // D7 ← AS608 TX
#define AS608_BAUD         57600

// I2S 音頻（MAX98357）
#define I2S_BCLK_PIN       2    // D1
#define I2S_LRCLK_PIN      3    // D2
#define I2S_DATA_PIN       4    // D3

// 繼電器
#define RELAY_PIN          1    // D0  HIGH=鎖門 / LOW=開門

// 電池監控
#define BATT_ADC_PIN       8    // D9  分壓器輸出（量到電池電壓的 0.5 倍）
#define BATT_CHRG_PIN      7    // D8  TP4056 CHRG，LOW=充電中

// =============================================
//  PCF8574 I2C 地址
// =============================================
#define PCF_KEYPAD_ADDR    0x20   // A0=A1=A2=GND
#define PCF_STATUS_ADDR    0x21   // A0=VCC, A1=A2=GND

// PCF8574 #2 （狀態指示）腳位
#define LED_GREEN_P        0      // P0 → 綠色 LED（解鎖成功）
#define LED_RED_P          1      // P1 → 紅色 LED（警報/失敗）
#define BUZZER_P           2      // P2 → 蜂鳴器（選配）

// =============================================
//  功能開關
// =============================================
#define FACE_RECOGNITION_EN   true   // 啟用本地人臉辨識
#define STRANGER_ALERT_EN     true   // 偵測陌生人時傳 Telegram 照片
#define WEATHER_NOTIFY_EN     true   // 開門時語音/顯示天氣提醒
#define FINGERPRINT_EN        true   // 啟用 AS608 指紋

// =============================================
//  行為設定
// =============================================
#define UNLOCK_DURATION_MS    5000   // 開鎖持續時間（5 秒後自動鎖門）
#define FACE_SCAN_INTERVAL_MS 300    // 人臉辨識掃描間隔
#define LOW_BATTERY_PCT       20     // 電量低於此值顯示警告
#define CRITICAL_BATTERY_PCT  10     // 電量低於此值進入省電模式

// SPIFFS 人臉資料庫
#define FACE_DB_DIR        "/faces"
#define MAX_FACE_COUNT     10        // 最多儲存幾張人臉
#define FACE_ENROLL_SAMPLES 5        // 登錄時連拍幾張

// =============================================
//  系統狀態列舉
// =============================================
enum SystemState {
    STATE_IDLE,
    STATE_VERIFY_FACE,
    STATE_VERIFY_FP,
    STATE_VERIFY_PWD,
    STATE_UNLOCKED,
    STATE_ALARM,
    STATE_FACE_MGMT,
    STATE_ENROLL_FACE
};

// =============================================
//  執行時狀態（extern 宣告，在 main.cpp 定義）
// =============================================
extern String currentPassword;       // 可透過 Telegram 動態修改
extern int    failCount;
