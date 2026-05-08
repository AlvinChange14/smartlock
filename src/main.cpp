// src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "face_recognition.h"
#include "face_database.h"
#include "face_manager.h"
#include "oled_ui.h"
#include "battery.h"
#include "keypad.h"
#include "fingerprint.h"
#include "audio.h"
#include "relay.h"
#include "wifi_mgr.h"
#include "telegram_bot.h"
#include "weather.h"

// ===== 全域物件 =====
FaceRecognitionSystem faceSystem;
FaceDatabase faceDB;
FaceManager faceMgr(faceSystem, faceDB, *new OledUI());  // Will be properly initialized
OledUI ui;
BatteryMonitor battery;

// PCF8574 現在使用純 Wire.h 驅動，不需要物件
// keypadPCF 和 statusPCF 的 I2C 地址從 config.h 讀取

HardwareSerial fpSerial(1);
Adafruit_Fingerprint finger(&fpSerial);

// ===== 執行時狀態 =====
String currentPassword = DEFAULT_PASSWORD;
int failCount = 0;
SystemState currentState = STATE_IDLE;
String inputBuffer = "";
unsigned long unlockTimestamp = 0;
String lastUnlockName = "";

// 天氣資訊快取
WeatherInfo weatherCache;
unsigned long lastWeatherUpdate = 0;

// 前向宣告
void handleIdle();
void handleUnlocked();
void handleAlarm();
void handleFaceMgmt();
void handleKeyInput(char key);
void handleFaceCheck();
void successUnlock(const String& name, camera_fb_t* photoFb);
void failedAttempt();
void setLED(bool green, bool red);

// Telegram 指令處理的前向宣告
void processTelegramCommand(const String& text, const String& fromId);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("╔══════════════════════════════╗");
    Serial.println("║  智慧門鎖 V2 啟動中...       ║");
    Serial.println("╚══════════════════════════════╝");

    // 1. I2C 初始化（OLED + PCF8574 共用）
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    delay(500);  // 等待 I2C 匯流排穩定

    // ▼▼▼ I2C 裝置掃描程式碼 ▼▼▼
    Serial.println("\n");
    Serial.println("═══════════════════════════════");
    Serial.println("🔍 開始掃描 I2C 裝置...");
    Serial.println("═══════════════════════════════");
    delay(200);
    
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("✅ 找到 I2C 裝置！地址：0x%02X", addr);
            if (addr == 0x20) Serial.print("  ← PCF8574 #1（鍵盤）");
            else if (addr == 0x21) Serial.print("  ← PCF8574 #2（LED）");
            else if (addr == 0x3C) Serial.print("  ← SSD1306 OLED");
            Serial.println();
            found++;
            delay(100);  // 每個裝置之間延遲，讓輸出更清晰
        }
    }
    
    Serial.println("═══════════════════════════════");
    if (found == 0) {
        Serial.println("❌ 找不到任何 I2C 裝置！請檢查 SDA/SCL 接線或上拉電阻！");
    } else {
        Serial.printf("✅ 共找到 %d 個 I2C 裝置\n", found);
        if (found >= 2) {
            Serial.println("✅ 至少找到 PCF8574 和 OLED，系統可以正常運作！");
        }
    }
    Serial.println("═══════════════════════════════");
    Serial.println("\n");
    delay(3000); // 暫停 3 秒，讓你有時間看清楚 Serial Monitor 的輸出
    // ▲▲▲ I2C 掃描結束 ▲▲▲

    ui.begin();
    ui.showMessage("Starting...", "Init hardware");

    // 2. 硬體初始化
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // 確保鎖緊

    // ▼▼▼ PCF8574 #1（鍵盤）初始化 ▼▼▼
    // 使用純 Wire.h 驅動，直接初始化
    Serial.println("🔧 初始化 PCF8574 #1（鍵盤）...");
    pcf8574_init(PCF_KEYPAD_ADDR);
    Serial.println("✅ PCF8574 #1 初始化完成");
    // ▲▲▲ PCF8574 #1 初始化結束 ▲▲▲
    
    // ▼▼▼ PCF8574 #2（LED）初始化 ▼▼▼
    Serial.println("🔧 初始化 PCF8574 #2（LED）...");
    pcf8574_init(PCF_STATUS_ADDR);
    Serial.println("✅ PCF8574 #2 初始化完成");
    // ▲▲▲ PCF8574 #2 初始化結束 ▲▲▲
    battery.begin();

    // 3. 鏡頭 + 人臉辨識
    ui.showMessage("Starting...", "Init camera");
    if (!faceSystem.initCamera()) {
        ui.showMessage("ERROR", "Camera init failed");
        while (1) delay(1000);
    }

    // 4. SPIFFS + 人臉資料庫
    ui.showMessage("Starting...", "Load face DB");
    faceDB.begin();
    // 從 SPIFFS 還原人臉到辨識器
    faceMgr.restoreDatabase();

    // 5. 指紋模組
    ui.showMessage("Starting...", "Init fingerprint");
    fpSerial.begin(AS608_BAUD, SERIAL_8N1, AS608_RX_PIN, AS608_TX_PIN);
    if (!finger.verifyPassword()) {
        Serial.println("⚠️ AS608 未找到，指紋功能停用");
    }

    // 6. 音頻
    initAudio();

    // 7. WiFi
    ui.showMessage("Starting...", "Connect WiFi");
    bool wifiOK = connectWiFi();
    if (wifiOK) {
        syncTime();
        weatherCache = getWeather();
        lastWeatherUpdate = millis();
    }

    // 8. Telegram
    if (wifiOK) {
        sendTelegramMessage(
            "🔐 智慧門鎖 V2 已啟動\n"
            "IP: " + WiFi.localIP().toString() + "\n"
            "已登錄人臉: " + String(faceSystem.getCount()) + " 筆\n"
            "電量: " + battery.toDisplayString()
        );
    }

    // 9. 啟動音效
    playSound(SOUND_STARTUP);
    setLED(true, false);  // 綠燈常亮

    Serial.println("✅ 系統啟動完成！");
    currentState = STATE_IDLE;
}

void loop() {
    // ─── 週期性任務 ───
    // 每 30 秒更新天氣
    if (millis() - lastWeatherUpdate > 30000) {
        lastWeatherUpdate = millis();
        weatherCache = getWeather();
    }

    // 每 3 秒檢查 Telegram
    static unsigned long lastTgCheck = 0;
    if (millis() - lastTgCheck > 3000) {
        lastTgCheck = millis();
        handleTelegramCommands();
    }

    // 電池監控
    static unsigned long lastBattCheck = 0;
    if (millis() - lastBattCheck > 30000) {
        lastBattCheck = millis();
        auto batt = battery.getStatus();
        if (batt.lowBattery) {
            ui.showLowBattery(batt.percentage);
            delay(2000);
        }
    }

    // ─── 狀態機 ───
    switch (currentState) {
        case STATE_IDLE:
            handleIdle();
            break;
        case STATE_UNLOCKED:
            handleUnlocked();
            break;
        case STATE_ALARM:
            handleAlarm();
            break;
        case STATE_FACE_MGMT:
            handleFaceMgmt();
            break;
        default:
            break;
    }
}

// ===== 待機狀態 =====
void handleIdle() {
    // OLED 待機畫面（每秒更新時間）
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 1000) {
        lastDisplayUpdate = millis();
        auto batt = battery.getStatus();
        struct tm t;
        getLocalTime(&t);
        char timeStr[6];
        strftime(timeStr, 6, "%H:%M", &t);
        ui.showIdle(String(timeStr), weatherCache.temp,
                    weatherCache.rainToday,
                    batt.percentage, batt.charging);
    }

    // 掃描鍵盤
    char key = scanKeypad(PCF_KEYPAD_ADDR);
    if (key) {
        handleKeyInput(key);
    }

    // 輪詢指紋
    if (finger.getImage() == FINGERPRINT_OK) {
        currentState = STATE_VERIFY_FP;
        // 簡化的指紋驗證
        int fpResult = verifyFingerprint();
        if (fpResult >= 0) {
            successUnlock("Fingerprint", nullptr);
        } else {
            failedAttempt();
        }
        currentState = STATE_IDLE;
    }

    // 人臉辨識（每 300ms）
    static unsigned long lastFaceCheck = 0;
    if (millis() - lastFaceCheck > 300) {
        lastFaceCheck = millis();
        handleFaceCheck();
    }
}

// ===== 人臉辨識 =====
void handleFaceCheck() {
    camera_fb_t* fb = faceSystem.capture();
    if (!fb) return;

    // 檢查是否正在進行人臉登錄（Telegram 觸發）
    // 若在登錄狀態，優先處理登錄，不執行一般辨識
    if (pendingEnroll) {
        checkPendingEnroll(faceSystem, faceDB, fb);
        faceSystem.returnFrame(fb);
        return;
    }

    ui.showVerifying("Face Recognition");

    auto result = faceSystem.process(fb);

    if (result.recognized) {
        // 已知人員 → 解鎖
        successUnlock(result.name, fb);
    } else if (result.face_detected) {
        // 陌生人偵測
        if (STRANGER_ALERT_EN) {
            Serial.println("⚠️ 偵測到陌生人！");
            struct tm t;
            getLocalTime(&t);
            char timeStr[32];
            strftime(timeStr, 32, "%Y/%m/%d %H:%M:%S", &t);
            String caption = "⚠️ 陌生人警報！\n時間: " + String(timeStr);
            sendTelegramPhoto(fb, caption);
        }
    }

    faceSystem.returnFrame(fb);
}

// ===== 解鎖成功 =====
void successUnlock(const String& name, camera_fb_t* photoFb) {
    Serial.printf("🔓 解鎖！[%s]\n", name.c_str());
    failCount = 0;
    lastUnlockName = name;
    currentState = STATE_UNLOCKED;
    unlockTimestamp = millis();

    // 開鎖
    digitalWrite(RELAY_PIN, LOW);
    playSound(SOUND_UNLOCK);
    setLED(true, false);

    // 取得天氣提示
    String weatherMsg = getWeatherMessage(weatherCache);

    // OLED 顯示
    ui.showUnlocked(name, weatherCache.description + " " +
                    String(weatherCache.temp, 0) + "C");

    // Telegram 通知
    sendTelegramMessage("✅ 解鎖：" + name +
                        "\n" + weatherMsg.substring(0, 100));
}

// ===== 自動鎖門 =====
void handleUnlocked() {
    // 倒數顯示
    int remaining = (UNLOCK_DURATION_MS - (millis() - unlockTimestamp)) / 1000;
    if (remaining < 0) remaining = 0;

    // 每秒更新顯示
    static int lastRemaining = -1;
    if (remaining != lastRemaining) {
        lastRemaining = remaining;
        String msg = "Auto-lock in " + String(remaining) + "s";
        ui.showUnlocked(lastUnlockName, msg);
    }

    if (millis() - unlockTimestamp > UNLOCK_DURATION_MS) {
        digitalWrite(RELAY_PIN, HIGH);
        setLED(false, false);
        currentState = STATE_IDLE;
        Serial.println("🔒 已自動鎖門");
    }
}

// ===== 密碼/鍵盤 =====
void handleKeyInput(char key) {
    playSound(SOUND_BEEP);

    // A 鍵（長按 2 秒進入管理模式，需先輸入管理密碼）
    if (key == 'A') {
        if (inputBuffer == ADMIN_PASSWORD) {
            inputBuffer = "";
            currentState = STATE_FACE_MGMT;
            return;
        }
    }

    if (key == '#') {
        // 確認密碼
        if (inputBuffer == currentPassword) {
            successUnlock("Password", nullptr);
        } else {
            failedAttempt();
        }
        inputBuffer = "";
        ui.showPasswordInput(0);
    } else if (key == '*') {
        inputBuffer = "";
        ui.showPasswordInput(0);
    } else if (isDigit(key)) {
        inputBuffer += key;
        ui.showPasswordInput(inputBuffer.length());
        Serial.printf("已輸入 %d 位\n", inputBuffer.length());
    }
}

// ===== 失敗處理 =====
void failedAttempt() {
    failCount++;
    playSound(SOUND_DENY);
    setLED(false, true);
    delay(500);
    setLED(false, false);
    ui.showDenied(failCount, MAX_FAIL_ATTEMPTS);
    delay(1500);

    if (failCount >= MAX_FAIL_ATTEMPTS) {
        currentState = STATE_ALARM;
        sendTelegramMessage("🚨 警報！連續失敗 " +
                            String(MAX_FAIL_ATTEMPTS) + " 次");
    }
}

// ===== 警報 =====
void handleAlarm() {
    ui.showAlarm();
    setLED(false, (millis() / 400) % 2);  // 紅燈閃爍
    playSound(SOUND_ALARM);
    delay(200);

    char key = scanKeypad(PCF_KEYPAD_ADDR);
    if (key == '#') {
        if (inputBuffer == currentPassword) {
            failCount = 0;
            currentState = STATE_IDLE;
            setLED(false, false);
            inputBuffer = "";
            sendTelegramMessage("ℹ️ 警報已解除（密碼）");
        } else {
            inputBuffer = "";
        }
    } else if (key == '*') {
        inputBuffer = "";
    } else if (key && isDigit(key)) {
        inputBuffer += key;
    }
}

// ===== 人臉管理選單 =====
void handleFaceMgmt() {
    ui.showFaceMenu(faceSystem.getCount());

    char key = scanKeypad(PCF_KEYPAD_ADDR);
    if (!key) return;

    if (key == '1') {
        // 新增人臉
        ui.showMessage("Add Face", "Enter name via TG");
        sendTelegramMessage(
            "請輸入要登錄的人臉名稱（例如：/face_enroll 王小明）\n"
            "輸入後請站到鏡頭前 10 秒"
        );

    } else if (key == '2') {
        // 刪除人臉（顯示清單）
        auto list = faceSystem.getList();
        ui.showList("Delete Face", list);
        delay(3000);

    } else if (key == '3') {
        // 列出所有人臉
        auto list = faceSystem.getList();
        ui.showList("Enrolled Faces", list);
        delay(4000);

    } else if (key == '4') {
        // 清除所有（二次確認）
        ui.showMessage("Confirm?", "Press # to confirm");
        delay(1000);
        char confirm = 0;
        unsigned long t = millis();
        while (millis() - t < 5000) {
            confirm = scanKeypad(PCF_KEYPAD_ADDR);
            if (confirm) break;
        }
        if (confirm == '#') {
            faceSystem.deleteAll();
            faceDB.removeAll();
            ui.showMessage("Done", "All faces deleted");
            delay(2000);
        }

    } else if (key == '*') {
        currentState = STATE_IDLE;
    }
}

// ===== LED 控制 =====
void setLED(bool green, bool red) {
    // 使用純 Wire.h 控制 PCF8574 #2（LED）
    // 讀取目前狀態，修改 LED 腳位，再寫回
    uint8_t status = pcf8574_read(PCF_STATUS_ADDR);
    
    // 清除 LED 腳位（設為 0 = LOW = 點亮）
    status &= ~(1 << LED_GREEN_P);
    status &= ~(1 << LED_RED_P);
    
    // 設定 LED 狀態（0 = LOW = 點亮，1 = HIGH = 熄滅）
    if (!green) status |= (1 << LED_GREEN_P);  // green=false → HIGH → 熄滅
    if (!red)   status |= (1 << LED_RED_P);    // red=false → HIGH → 熄滅
    
    pcf8574_write(PCF_STATUS_ADDR, status);
}

