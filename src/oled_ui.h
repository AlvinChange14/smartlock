// src/oled_ui.h
#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDRESS 0x3C
#define OLED_RESET   -1  // 共用 ESP32 的 RST

class OledUI {
public:
    Adafruit_SSD1306 display;
    
    // 動畫計數器
    uint8_t spinnerFrame = 0;
    unsigned long lastAnimUpdate = 0;
    static const uint16_t ANIM_INTERVAL = 150;  // ms
    
    // 警報閃爍
    bool alarmVisible = true;
    unsigned long lastAlarmBlink = 0;
    
    OledUI() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET) {}
    
    bool begin() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
            Serial.println("❌ OLED 初始化失敗");
            return false;
        }
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.cp437(true);
        Serial.println("✅ OLED 初始化完成");
        return true;
    }
    
    // ===== 畫面 1：待機 =====
    void showIdle(const String& time, float temp, bool raining,
                  int batteryPct, bool charging) {
        display.clearDisplay();
        
        // 標題
        display.setTextSize(1);
        display.setFont(nullptr);
        display.setCursor(0, 0);
        display.println("  SMART LOCK  v2.0");
        
        // 分隔線
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        // 時間
        display.setTextSize(2);
        display.setCursor(20, 14);
        display.print(time);
        
        // 天氣
        display.setTextSize(1);
        display.setCursor(0, 34);
        display.printf("%.0f C  %s", temp, raining ? "Rain" : "Clear");
        
        // 分隔線
        display.drawLine(0, 44, 127, 44, SSD1306_WHITE);
        
        // 電量條
        drawBatteryBar(0, 47, batteryPct, charging);
        
        // 提示文字
        display.setTextSize(1);
        display.setCursor(0, 57);
        display.print("Face/FP/Password");
        
        display.display();
    }
    
    // ===== 畫面 2：驗證中（帶動畫）=====
    void showVerifying(const String& method) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(25, 0);
        display.print("Verifying...");
        
        // 旋轉動畫（使用 8 幀）
        const char* spinFrames[] = {"|", "/", "-", "\\", "|", "/", "-", "\\"};
        display.setTextSize(3);
        display.setCursor(55, 20);
        display.print(spinFrames[spinnerFrame]);
        
        // 更新動畫幀
        if (millis() - lastAnimUpdate > ANIM_INTERVAL) {
            spinnerFrame = (spinnerFrame + 1) % 8;
            lastAnimUpdate = millis();
        }
        
        display.setTextSize(1);
        display.setCursor(0, 52);
        display.print(method);
        
        display.display();
    }
    
    // ===== 畫面 3：解鎖成功 =====
    void showUnlocked(const String& name, const String& weather) {
        display.clearDisplay();
        
        // 打勾符號（大）
        display.setTextSize(2);
        display.setCursor(0, 5);
        display.print("OK");
        
        // 歡迎訊息
        display.setTextSize(1);
        display.setCursor(30, 8);
        display.print("Welcome!");
        display.setCursor(30, 20);
        display.print(name.substring(0, 14));
        
        // 水平線
        display.drawLine(0, 33, 127, 33, SSD1306_WHITE);
        
        // 天氣提示
        display.setCursor(0, 37);
        display.print(weather.substring(0, 21));
        
        // 計時 bar（倒數顯示還有幾秒自動鎖門）
        display.setCursor(0, 55);
        display.print("Auto-lock in 5s...");
        
        display.display();
    }
    
    // ===== 畫面 4：失敗 =====
    void showDenied(int failCount, int maxFail) {
        display.clearDisplay();
        
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.print("DENIED");
        
        display.setTextSize(1);
        display.setCursor(0, 25);
        display.printf("Attempts: %d/%d", failCount, maxFail);
        
        display.setCursor(0, 38);
        display.printf("Remaining: %d", maxFail - failCount);
        
        display.setCursor(0, 55);
        display.print("[*]Clear  [#]Enter");
        
        display.display();
    }
    
    // ===== 畫面 5：警報（閃爍）=====
    void showAlarm() {
        // 閃爍邏輯
        if (millis() - lastAlarmBlink > 400) {
            alarmVisible = !alarmVisible;
            lastAlarmBlink = millis();
        }
        
        display.clearDisplay();
        
        if (alarmVisible) {
            display.setTextSize(2);
            display.setCursor(10, 5);
            display.print("! ALARM !");
        }
        
        display.setTextSize(1);
        display.setCursor(0, 30);
        display.print("Too many failures!");
        
        display.setCursor(0, 42);
        display.print("Enter password:");
        
        display.setCursor(0, 55);
        display.print("[*]Clear  [#]Confirm");
        
        display.display();
    }
    
    // ===== 畫面 6：人臉管理選單 =====
    void showFaceMenu(int faceCount) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.printf("Face Mgmt (%d/%d)", faceCount, 10);
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        display.setCursor(0, 14);  display.print("1. Add face");
        display.setCursor(0, 24);  display.print("2. Delete face");
        display.setCursor(0, 34);  display.print("3. List all");
        display.setCursor(0, 44);  display.print("4. Delete ALL");
        display.setCursor(0, 55);  display.print("[*]Back");
        
        display.display();
    }
    
    // ===== 畫面 7：密碼輸入 =====
    void showPasswordInput(int inputLen, const String& prompt = "Enter Password") {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print(prompt.substring(0, 21));
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        // 顯示已輸入位數的點
        display.setTextSize(2);
        String dots = "";
        for (int i = 0; i < inputLen && i < 8; i++) dots += "*";
        display.setCursor(10, 20);
        display.print(dots.length() > 0 ? dots : "_");
        
        display.setTextSize(1);
        display.setCursor(0, 45);
        display.printf("Length: %d", inputLen);
        
        display.setCursor(0, 55);
        display.print("[*]Clear  [#]Confirm");
        
        display.display();
    }
    
    // ===== 畫面 8：登錄人臉進度 =====
    void showEnrollScreen(const String& name, int sample, int total) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Enrolling face...");
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        display.setCursor(0, 14);
        display.print(name.substring(0, 16));
        
        display.setCursor(0, 28);
        display.printf("Sample %d of %d", sample + 1, total);
        
        // 進度條
        int barWidth = (sample * 100 / total) * 126 / 100;
        display.drawRect(0, 40, 128, 10, SSD1306_WHITE);
        display.fillRect(0, 40, barWidth, 10, SSD1306_WHITE);
        
        display.setCursor(0, 55);
        display.print("Face the camera...");
        
        display.display();
    }
    
    // ===== 通用訊息畫面 =====
    void showMessage(const String& title, const String& message) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print(title);
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        display.setCursor(0, 20);
        display.print(message.substring(0, 21));
        
        display.display();
    }
    
    // ===== 列表顯示（人臉清單）=====
    void showList(const String& title, const std::vector<String>& items, int page = 0) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print(title);
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        int start = page * 4;
        for (int i = 0; i < 4 && start + i < (int)items.size(); i++) {
            display.setCursor(0, 14 + i * 12);
            display.printf("%d. %s", start + i + 1,
                           items[start + i].substring(0, 15).c_str());
        }
        
        if (items.size() > 4) {
            display.setCursor(0, 55);
            display.printf("Page %d/%d  [#]Next", page + 1,
                           (items.size() + 3) / 4);
        }
        
        display.display();
    }
    
    // ===== 電量低警告 =====
    void showLowBattery(int batteryPct) {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(20, 0);
        display.print("LOW BATTERY!");
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        
        display.setTextSize(2);
        display.setCursor(30, 20);
        display.printf("%d%%", batteryPct);
        
        display.setTextSize(1);
        display.setCursor(10, 45);
        display.print("Please charge soon");
        
        display.display();
    }
    
private:
    // 電量條繪製（含充電閃爍效果）
    void drawBatteryBar(int x, int y, int pct, bool charging) {
        // 電池外框
        display.drawRect(x, y, 60, 8, SSD1306_WHITE);
        display.drawRect(x + 60, y + 2, 3, 4, SSD1306_WHITE);  // 正極凸起
        
        // 電量填充
        int fillWidth = pct * 58 / 100;
        if (fillWidth > 0) {
            display.fillRect(x + 1, y + 1, fillWidth, 6, SSD1306_WHITE);
        }
        
        // 百分比文字
        display.setTextSize(1);
        display.setCursor(x + 65, y);
        if (charging) {
            display.print("CHG");
        } else {
            display.printf("%d%%", pct);
        }
    }
};