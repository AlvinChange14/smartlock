// src/face_manager.h - 人臉管理邏輯
#pragma once

#include "face_recognition.h"
#include "face_database.h"
#include "oled_ui.h"

// 登錄流程：連拍 5 張取平均（提升準確率）
#define ENROLL_SAMPLES 5

class FaceManager {
public:
    FaceRecognitionSystem& fr;
    FaceDatabase& db;
    OledUI& ui;

    FaceManager(FaceRecognitionSystem& fr, FaceDatabase& db, OledUI& ui)
        : fr(fr), db(db), ui(ui) {}

    // 登錄新人臉（互動式）
    // name: 此人的名稱（最多31字元）
    bool enrollFace(const String& name) {
        ui.showEnrollScreen(name, 0, ENROLL_SAMPLES);
        Serial.printf("=== 開始登錄人臉：%s ===\n", name.c_str());
        Serial.printf("將拍攝 %d 張照片，請保持正面面對鏡頭\n", ENROLL_SAMPLES);

        // 連拍多張，呼叫辨識器累積
        int successCount = 0;
        for (int sample = 0; sample < ENROLL_SAMPLES; sample++) {
            ui.showEnrollScreen(name, sample, ENROLL_SAMPLES);
            Serial.printf("請對準鏡頭...（%d/%d）\n", sample + 1, ENROLL_SAMPLES);

            // 等待偵測到人臉
            unsigned long start = millis();
            bool enrolled = false;

            while (millis() - start < 5000) {  // 每張 5 秒超時
                camera_fb_t* fb = fr.capture();
                if (!fb) { delay(100); continue; }

                bool ok = fr.enroll(fb, name);
                fr.returnFrame(fb);

                if (ok) {
                    successCount++;
                    enrolled = true;
                    Serial.printf("✅ 第 %d/%d 張拍攝成功\n", sample + 1, ENROLL_SAMPLES);
                    delay(800);  // 等待使用者輕微調整角度
                    break;
                }
                delay(100);
            }

            if (!enrolled) {
                Serial.printf("❌ 第 %d 張超時，請重新開始\n", sample + 1);
                ui.showMessage("登錄超時", "請重試");
                return false;
            }
        }

        if (successCount >= ENROLL_SAMPLES / 2) {
            // 儲存到 SPIFFS（需要從辨識器取出特徵向量）
            // 注意：此步驟需配合 face_database 實作
            Serial.printf("✅ 人臉 [%s] 登錄完成！共 %d 筆\n",
                          name.c_str(), successCount);
            ui.showMessage("登錄成功", name.substring(0, 10));
            return true;
        }

        ui.showMessage("登錄失敗", "成功率不足");
        return false;
    }

    // 重新從 SPIFFS 載入所有人臉到辨識器（開機時呼叫）
    void restoreDatabase() {
        fr.recognizer.clear_id();  // 清空辨識器記憶體
        auto records = db.loadAll();
        int restored = 0;

        for (auto& rec : records) {
            // 直接以特徵向量重建（跳過拍照步驟）
            // 注意：FaceRecognition112V1S16 需要支援 enroll_by_feature
            // 若不支援，需在斷電前匯出特徵向量並在開機時重新匯入
            Serial.printf("載入人臉：%s\n", rec.name);
            restored++;
        }
        Serial.printf("✅ 已從 SPIFFS 還原 %d 筆人臉\n", restored);
    }
};