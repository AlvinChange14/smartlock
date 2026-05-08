// src/relay.h
#pragma once

#include <Arduino.h>
#include "config.h"

// 繼電器邏輯說明：
//   RELAY_PIN = HIGH → 繼電器不動作 → 電磁鎖通電 → 門鎖緊  (正常)
//   RELAY_PIN = LOW  → 繼電器動作   → 電磁鎖斷電 → 門打開  (解鎖)
// 使用 NC（常閉）接線，斷電後門自動鎖緊（安全設計）

bool relayUnlocked = false;
unsigned long relayOpenTime = 0;

void initRelay() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);   // 確保啟動時門是鎖緊的
    Serial.println("✅ 繼電器初始化（門已鎖緊）");
}

// 開鎖（持續 durationMs 毫秒後自動鎖門）
// durationMs = 0 表示永久打開直到呼叫 lockDoor()
void unlockDoor(unsigned long durationMs = UNLOCK_DURATION_MS) {
    digitalWrite(RELAY_PIN, LOW);
    relayUnlocked  = true;
    relayOpenTime  = millis();
    Serial.printf("🔓 開鎖 %lu ms\n", durationMs);
}

// 手動鎖門
void lockDoor() {
    digitalWrite(RELAY_PIN, HIGH);
    relayUnlocked = false;
    Serial.println("🔒 已鎖門");
}

// 自動鎖門計時器（在 loop 中每次呼叫）
void updateRelay() {
    if (relayUnlocked && UNLOCK_DURATION_MS > 0) {
        if (millis() - relayOpenTime >= UNLOCK_DURATION_MS) {
            lockDoor();
        }
    }
}

bool isDoorUnlocked() {
    return relayUnlocked;
}

// 取得剩餘開鎖秒數（用於 OLED 倒數顯示）
int getUnlockRemainingSeconds() {
    if (!relayUnlocked) return 0;
    long remaining = (long)UNLOCK_DURATION_MS -
                     (long)(millis() - relayOpenTime);
    return (int)(remaining / 1000) + 1;
}