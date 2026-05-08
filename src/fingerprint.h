// src/fingerprint.h
#pragma once

#include <Adafruit_Fingerprint.h>
#include "config.h"
#include "audio.h"

extern HardwareSerial fpSerial;
extern Adafruit_Fingerprint finger;

bool fpInitialized = false;

// ── 初始化 ──────────────────────────────────────
bool initFingerprint() {
    fpSerial.begin(AS608_BAUD, SERIAL_8N1, AS608_RX_PIN, AS608_TX_PIN);
    delay(200);

    finger.begin(AS608_BAUD);

    if (finger.verifyPassword()) {
        fpInitialized = true;
        finger.getParameters();
        Serial.printf("✅ AS608 就緒 | 容量: %d 筆 | 安全等級: %d\n",
                      finger.capacity, finger.security_level);
        return true;
    }

    Serial.println("⚠️ AS608 未偵測到，指紋功能停用");
    return false;
}

// ── 驗證指紋（回傳 ID，-1 = 不匹配，-2 = 無法掃描）──
int verifyFingerprint() {
    if (!fpInitialized) return -2;

    // 取得影像
    uint8_t ret = finger.getImage();
    if (ret != FINGERPRINT_OK) return -2;  // 手指未放穩

    // 轉換特徵
    if (finger.image2Tz() != FINGERPRINT_OK) return -2;

    // 搜尋資料庫
    if (finger.fingerFastSearch() != FINGERPRINT_OK) return -1;  // 不匹配

    // 信心分數過低也視為不匹配（設定 >= 40 才接受）
    if (finger.confidence < 40) return -1;

    Serial.printf("✅ 指紋匹配 ID=%d，信心=%d\n",
                  finger.fingerID, finger.confidence);
    return finger.fingerID;
}

// ── 登錄指紋 ──────────────────────────────────────
// id: 1~127（AS608 最多 127 筆）
// 回傳 true 代表成功
bool enrollFingerprint(int id) {
    if (!fpInitialized) return false;

    Serial.printf("=== 開始登錄指紋 ID #%d ===\n", id);

    // ── 第一次掃描 ──
    Serial.println("請放上手指...");
    unsigned long t = millis();
    while (finger.getImage() != FINGERPRINT_OK) {
        if (millis() - t > 10000) {
            Serial.println("❌ 逾時，取消登錄");
            return false;
        }
        delay(50);
    }

    if (finger.image2Tz(1) != FINGERPRINT_OK) {
        Serial.println("❌ 特徵提取失敗，請重試");
        return false;
    }
    Serial.println("✅ 第一次掃描完成，請移開手指...");

    // 等待手指移開
    delay(1500);
    while (finger.getImage() != FINGERPRINT_NOFINGER) delay(50);

    // ── 第二次掃描 ──
    Serial.println("請再次放上同一根手指...");
    t = millis();
    while (finger.getImage() != FINGERPRINT_OK) {
        if (millis() - t > 10000) {
            Serial.println("❌ 逾時，取消登錄");
            return false;
        }
        delay(50);
    }

    if (finger.image2Tz(2) != FINGERPRINT_OK) {
        Serial.println("❌ 特徵提取失敗，請重試");
        return false;
    }

    // ── 建立模型並儲存 ──
    if (finger.createModel() != FINGERPRINT_OK) {
        Serial.println("❌ 兩次指紋不匹配，請重試");
        return false;
    }

    if (finger.storeModel(id) != FINGERPRINT_OK) {
        Serial.println("❌ 儲存失敗（Flash 可能已滿）");
        return false;
    }

    Serial.printf("✅ 指紋 #%d 登錄成功！\n", id);
    return true;
}

// ── 刪除指定 ID ──
bool deleteFingerprint(int id) {
    if (!fpInitialized) return false;
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
        Serial.printf("✅ 指紋 #%d 已刪除\n", id);
        return true;
    }
    Serial.printf("❌ 刪除指紋 #%d 失敗\n", id);
    return false;
}

// ── 清空所有指紋 ──
bool clearAllFingerprints() {
    if (!fpInitialized) return false;
    if (finger.emptyDatabase() == FINGERPRINT_OK) {
        Serial.println("✅ 所有指紋已清空");
        return true;
    }
    return false;
}

// ── 取得已使用的指紋數量 ──
uint16_t getFingerprintCount() {
    if (!fpInitialized) return 0;
    finger.getParameters();
    // AS608 並沒有直接提供已用數量的指令
    // 只能透過逐一讀取模板確認（略耗時）
    uint16_t count = 0;
    for (int i = 1; i <= finger.capacity; i++) {
        if (finger.loadModel(i) == FINGERPRINT_OK) count++;
    }
    return count;
}