// src/wifi_mgr.h
#pragma once

#include <WiFi.h>
#include "config.h"

// ── WiFi 連線 ─────────────────────────────────
bool connectWiFi() {
    Serial.printf("連接 WiFi：%s ...", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempt = 0;
    int maxAttempts = WIFI_TIMEOUT_SEC * 2;   // 每 500ms 一次
    while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✅ WiFi 連線成功！IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        return true;
    }

    Serial.println("\n⚠️ WiFi 連線失敗，繼續離線運作");
    return false;
}

// ── NTP 時間同步 ───────────────────────────────
void syncTime() {
    if (WiFi.status() != WL_CONNECTED) return;
    configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    Serial.print("同步 NTP 時間...");
    struct tm t;
    int retry = 0;
    while (!getLocalTime(&t) && retry < 10) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    if (getLocalTime(&t)) {
        char buf[32];
        strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &t);
        Serial.printf("\n✅ 時間同步成功：%s\n", buf);
    } else {
        Serial.println("\n⚠️ NTP 同步失敗");
    }
}

// ── 取得格式化時間字串 ─────────────────────────
String getCurrentTime(const char* fmt = "%H:%M") {
    struct tm t;
    if (!getLocalTime(&t)) return "--:--";
    char buf[32];
    strftime(buf, sizeof(buf), fmt, &t);
    return String(buf);
}

String getCurrentDateTime() {
    return getCurrentTime("%Y/%m/%d %H:%M:%S");
}

// ── 自動重連（在 loop 中定期呼叫）─────────────
void maintainWiFi() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 30000) return;
    lastCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi 斷線，嘗試重連...");
        WiFi.reconnect();
        delay(5000);
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("✅ WiFi 重連成功：%s\n",
                          WiFi.localIP().toString().c_str());
        }
    }
}