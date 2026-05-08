# 🔐 智慧門鎖 V2

一個基於 ESP32-S3 的多功能智慧門鎖系統，整合人臉辨識、指紋辨識、密碼輸入、OLED 顯示、電池備用電源和 Telegram 遠端控制功能。

## 📋 功能特色

- **本地人臉辨識** - 使用 MTCNN + MobileFaceNet（esp-face 框架），不需網路即可辨識
- **人臉資料庫管理** - 使用 SPIFFS 持久化儲存，支援新增/刪除，重啟後保留
- **指紋辨識** - 支援 AS608 指紋模組，最多可儲存 127 枚指紋
- **密碼開鎖** - 4×4 矩陣鍵盤輸入密碼
- **OLED 顯示** - 128×64 SSD1306 螢幕，顯示時間、天氣、電量等資訊
- **電池備用電源** - 支援 18650 鋰電池，斷電後繼續運作
- **Telegram Bot** - 遠端控制、狀態查詢、人臉管理
- **天氣提醒** - 開鎖時顯示天氣資訊和出門建議
- **I2S 音頻** - 使用 MAX98357 播放提示音

## 🛠️ 硬體需求

### 主要控制器
- **Seeed XIAO ESP32 S3 Sense**（內建 OV2640 鏡頭）

### 周邊模組
| 模組 | 型號 | 數量 | 備註 |
|------|------|------|------|
| OLED 顯示器 | SSD1306 128×64 I2C | 1 | |
| 指紋模組 | AS608 | 1 | UART 介面 |
| 鍵盤 | 4×4 矩陣鍵盤 | 1 | |
| I2C 擴充板 | PCF8574 | 2 | 一個用於鍵盤，一個用於 LED |
| 音頻擴大機 | MAX98357 | 1 | I2S 介面 |
| 繼電器模組 | 5V 繼電器 | 1 | 控制電磁鎖 |
| 鋰電池 | 18650 3.7V | 1~2 | 備用電源 |
| 充電模組 | TP4056 Type-C | 1 | 鋰電池充電 |
| UPS 模組 | IP5306 | 1 | 不斷電系統 |

### 腳位配置

| 功能 | XIAO ESP32 S3 腳位 | GPIO |
|------|-------------------|------|
| I2C SDA | D4 | GPIO5 |
| I2C SCL | D5 | GPIO6 |
| AS608 TX→ | D6 | GPIO43 |
| AS608 RX← | D7 | GPIO44 |
| I2S BCLK | D1 | GPIO2 |
| I2S LRC | D2 | GPIO3 |
| I2S DIN | D3 | GPIO4 |
| 繼電器 | D0 | GPIO1 |
| 電池 ADC | D9 | GPIO8 |
| 充電狀態 | D8 | GPIO7 |

## 📁 專案結構

```
SmartLock_V2/
├── platformio.ini           # PlatformIO 設定檔
├── partitions_smartlock.csv # Flash 分割表
├── README.md                # 本檔案
├── src/
│   ├── main.cpp             # 主程式（狀態機 + 初始化）
│   ├── config.h             # 所有設定（WiFi、密碼、腳位）
│   ├── face_recognition.h   # 人臉辨識核心（MTCNN + MobileFaceNet）
│   ├── face_database.h      # 人臉資料庫（SPIFFS 持久化）
│   ├── face_manager.h       # 人臉管理邏輯
│   ├── oled_ui.h            # OLED 多頁面 UI
│   ├── battery.h            # 電池監控
│   ├── keypad.h             # 鍵盤掃描（PCF8574）
│   ├── fingerprint.h        # AS608 指紋模組
│   ├── audio.h              # I2S 音頻播放
│   ├── relay.h              # 繼電器控制
│   ├── wifi_mgr.h           # WiFi 管理
│   ├── telegram_bot.h       # Telegram Bot（含指令處理）
│   └── weather.h            # OpenWeatherMap 天氣 API
└── data/
    └── faces/               # SPIFFS 人臉資料庫目錄
```

## 🚀 快速開始

### 1. 環境設定

1. 安裝 [VS Code](https://code.visualstudio.com/)
2. 安裝 [PlatformIO IDE 擴充功能](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
3. 克隆或下載此專案

### 2. 設定配置

修改 `src/config.h` 中的以下參數：

```cpp
// WiFi 設定
#define WIFI_SSID          "你的WiFi名稱"
#define WIFI_PASS          "你的WiFi密碼"

// Telegram Bot 設定
#define BOT_TOKEN          "你的BotToken"    // 從 @BotFather 取得
#define CHAT_ID            "你的ChatID"      // 純數字

// 天氣 API 設定
#define OWM_API_KEY        "你的OWM_APIKey"  // 從 OpenWeatherMap 取得
#define OWM_CITY           "Taipei"

// 密碼設定（務必修改！）
#define DEFAULT_PASSWORD   "1234"            // 一般開鎖密碼
#define ADMIN_PASSWORD     "9999"            // 管理員密碼
```

### 3. 編譯與燒錄

1. 用 USB-C 連接 XIAO ESP32 S3 到電腦
2. 在 VS Code 中開啟專案
3. PlatformIO 會自動下載依賴庫（首次需數分鐘）
4. 點擊底部工具列的 **Upload** 按鈕（或按 `Ctrl+Alt+U`）
5. 上傳完成後，開啟 Serial Monitor（`Ctrl+Alt+S`，鮑率 115200）查看輸出

### 4. 上傳 SPIFFS 檔案系統

首次燒錄後，建議上傳 SPIFFS 以初始化人臉資料庫目錄：

1. 在 PlatformIO 選單中選擇 **Project Tasks**
2. 展開 **Platform** → **Upload Filesystem Image**

## 📱 Telegram Bot 指令

| 指令 | 功能 |
|------|------|
| `/unlock` | 遠端開鎖 5 秒 |
| `/status` | 查看系統狀態（IP、電量、人臉數等）|
| `/weather` | 查詢目前天氣 |
| `/battery` | 查看電池狀態 |
| `/alarm_off` | 遠端解除警報 |
| `/face_list` | 列出所有登錄人臉 |
| `/face_enroll [名稱]` | 登錄新人臉 |
| `/face_delete [名稱]` | 刪除指定人臉 |
| `/face_deleteall` | 清除所有人臉 |
| `/set_password [新密碼]` | 修改開鎖密碼 |
| `/help` | 顯示所有指令說明 |

## 🎯 使用方式

### 日常開鎖

1. **人臉辨識** - 站到鏡頭前，系統自動辨識（約 0.5~1 秒）
2. **指紋開鎖** - 按壓指紋感測器
3. **密碼開鎖** - 輸入密碼後按 `#` 確認
4. **遠端開鎖** - 使用 Telegram `/unlock` 指令

### 管理操作

1. 輸入管理員密碼後按 `A` 進入管理選單
2. 按 `1` 新增人臉（透過 Telegram 輸入名稱）
3. 按 `2` 刪除人臉
4. 按 `3` 列出所有人臉
5. 按 `4` 清除所有人臉（需二次確認）
6. 按 `*` 返回

### 人臉登錄流程

1. 使用 Telegram 傳送 `/face_enroll 王小明`
2. Bot 回覆準備就緒後，站到鏡頭前
3. 系統自動拍攝 5 張照片（約 10 秒）
4. 完成後 Bot 會通知登錄成功

## ⚡ 電源管理

### 電池電量顯示

| 電池電壓 | 電量% | 狀態 |
|---------|-------|------|
| 4.2V | 100% | 滿電 |
| 4.0V | 75% | 良好 |
| 3.7V | 50% | 中等 |
| 3.5V | 25% | 偏低 |
| 3.3V | 10% | 需充電 |

### 低電量警告

- 電量 < 20%：OLED 顯示警告，Telegram 推送通知
- 電量 < 10%：進入省電模式（降低 CPU 頻率，停用 WiFi）

## 🔧 調校與最佳化

### 人臉辨識準確率

在 `src/face_recognition.h` 中可調整偵測器靈敏度：

```cpp
HumanFaceDetectMSR01 detector_stage1(
    0.3F,   // score_threshold（越小越靈敏，假陽性也越多）
    0.3F,   // nms_threshold
    10,     // top_k
    0.3F    // min_score
);
```

**建議**：
- 光線充足：`score_threshold = 0.4~0.5`
- 光線不足：`score_threshold = 0.2~0.3`

### 鏡頭參數調整

在 `src/face_recognition.h` 的 `initCamera()` 中可調整：

```cpp
s->set_brightness(s, 1);   // 亮度（-2~2）
s->set_contrast(s, 1);     // 對比（-2~2）
s->set_hmirror(s, 1);      // 水平鏡像
```

## ❓ 常見問題

### Q: 編譯時出現 "human_face_detect_msr01.hpp not found"
**A**: 執行 `PlatformIO: Clean` 後重新編譯，確保 platformio.ini 中的 include 路徑正確。

### Q: OLED 顯示花屏或白屏
**A**: 檢查 I2C 接線，確認 OLED 地址是 0x3C（部分模組為 0x3D）。

### Q: 人臉辨識準確率低
**A**: 
1. 增加登錄時的照片數量（修改 `FACE_ENROLL_SAMPLES`）
2. 登錄時拍攝不同角度（輕微左右轉頭）
3. 確保登錄和使用時光線條件相似
4. 考慮加裝 940nm 紅外補光燈

### Q: 電池 ADC 讀值不穩定
**A**: 在 ADC 腳位和 GND 之間加 100nF 電容去耦，或增加取樣次數。

## 📄 授權

本專案供學習和研究使用。

## 🙏 致謝

- [esp-face](https://github.com/espressif/esp-face) - 本地人臉辨識框架
- [Universal-Arduino-Telegram-Bot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot) - Telegram Bot 庫
- [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306) - OLED 顯示庫
- [PCF8574 library](https://github.com/xreef/PCF8574_library) - I2C 擴充庫

---

**版本**: V2.0  
**更新日期**: 2026/05/03  
**開發環境**: PlatformIO + Arduino Framework  
**目標硬體**: Seeed XIAO ESP32 S3 Sense