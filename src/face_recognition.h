// src/face_recognition.h
#pragma once

#include "esp_camera.h"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "face_recognition_112_v1_s16.hpp"
#include "face_recognition_tool.hpp"
#include "face_database.h"

// XIAO ESP32 S3 Sense 鏡頭腳位（固定，勿修改）
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

class FaceRecognitionSystem {
public:
    // MTCNN 兩階段偵測器
    HumanFaceDetectMSR01 detector_stage1;
    HumanFaceDetectMNP01 detector_stage2;

    // MobileFaceNet 辨識器
    FaceRecognition112V1S16 recognizer;

    // 辨識結果
    struct Result {
        bool face_detected;
        bool recognized;
        String name;
        float confidence;
    };

    FaceRecognitionSystem()
        : detector_stage1(0.3F, 0.3F, 10, 0.3F),  // min_score, nms_threshold, top_k, score_threshold
          detector_stage2(0.4F, 0.3F, 10)
    {}

    // 初始化鏡頭（RGB565 模式）
    bool initCamera() {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0  = Y2_GPIO_NUM;
        config.pin_d1  = Y3_GPIO_NUM;
        config.pin_d2  = Y4_GPIO_NUM;
        config.pin_d3  = Y5_GPIO_NUM;
        config.pin_d4  = Y6_GPIO_NUM;
        config.pin_d5  = Y7_GPIO_NUM;
        config.pin_d6  = Y8_GPIO_NUM;
        config.pin_d7  = Y9_GPIO_NUM;
        config.pin_xclk    = XCLK_GPIO_NUM;
        config.pin_pclk    = PCLK_GPIO_NUM;
        config.pin_vsync   = VSYNC_GPIO_NUM;
        config.pin_href    = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn    = PWDN_GPIO_NUM;
        config.pin_reset   = RESET_GPIO_NUM;
        config.xclk_freq_hz = 20000000;

        // ⚠️ 人臉辨識必須使用 RGB565，不能用 JPEG
        config.pixel_format = PIXFORMAT_RGB565;
        config.frame_size   = FRAMESIZE_240X240;  // 240×240，辨識最佳尺寸
        config.fb_location  = CAMERA_FB_IN_PSRAM; // 使用 PSRAM 存 frame buffer
        config.fb_count     = 1;  // 減少幀緩衝區數量以避免堆疊溢位
        config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

        if (!psramFound()) {
            Serial.println("❌ PSRAM 未找到！人臉辨識無法運作");
            return false;
        }

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("❌ 鏡頭初始化失敗: 0x%x\n", err);
            return false;
        }

        // 調整鏡頭參數以提升辨識效果
        sensor_t* s = esp_camera_sensor_get();
        s->set_hmirror(s, 1);        // 水平鏡像（門鎖鏡頭通常需要）
        s->set_vflip(s, 0);          // 垂直翻轉
        s->set_brightness(s, 1);     // 亮度 +1
        s->set_contrast(s, 1);       // 對比 +1
        s->set_saturation(s, 0);     // 飽和度
        s->set_whitebal(s, 1);       // 自動白平衡
        s->set_awb_gain(s, 1);       // AWB 增益
        s->set_exposure_ctrl(s, 1);  // 自動曝光

        Serial.println("✅ 鏡頭初始化完成（RGB565 240×240 模式）");
        return true;
    }

    // 擷取一幀影像
    camera_fb_t* capture() {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("❌ 無法擷取影像");
        }
        return fb;
    }

    void returnFrame(camera_fb_t* fb) {
        esp_camera_fb_return(fb);
    }

    // 主要辨識函式
    // 回傳：Result（包含是否偵測到人臉、是否辨識成功、名稱）
    Result process(camera_fb_t* fb) {
        Result result = {false, false, "", 0.0f};

        if (!fb || fb->format != PIXFORMAT_RGB565) return result;

        // 第一階段偵測（快速）
        auto candidates = detector_stage1.infer(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3}
        );

        if (candidates.empty()) {
            return result;  // 無人臉
        }
        result.face_detected = true;

        // 第二階段精化（準確定位關鍵點）
        auto faces = detector_stage2.infer(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3},
            candidates
        );

        if (faces.empty()) {
            return result;
        }

        // 取第一個（最大的）人臉進行辨識
        auto& face = faces.front();

        // MobileFaceNet 辨識
        auto rec_info = recognizer.recognize(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3},
            face.keypoint  // 5個關鍵點坐標
        );

        // 嚴格判斷：避免 ESP-WHO 在資料庫為空時回傳 id=0, name="empty" 的預設值
        // 只有當 name 有效且不為 "unknown" 或 "empty" 時才認為是辨識成功
        if (rec_info.name.length() > 0 
            && rec_info.name != "unknown" 
            && rec_info.name != "empty"
            && recognizer.get_enrolled_ids().size() > 0) {
            result.recognized = true;
            result.name = String(rec_info.name.c_str());
            result.confidence = 1.0f;
        }

        return result;
    }

    // 登錄新人臉（需要 5 張照片平均化，提升準確度）
    bool enroll(camera_fb_t* fb, const String& name) {
        auto candidates = detector_stage1.infer(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3}
        );
        if (candidates.empty()) {
            Serial.println("登錄失敗：未偵測到人臉");
            return false;
        }

        auto faces = detector_stage2.infer(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3},
            candidates
        );
        if (faces.empty()) {
            Serial.println("登錄失敗：人臉精化失敗");
            return false;
        }

        int id = recognizer.enroll_id(
            (uint16_t*)fb->buf,
            {(int)fb->height, (int)fb->width, 3},
            faces.front().keypoint,
            name.c_str(),
            true  // flash = true，儲存到記憶體
        );

        if (id >= 0) {
            Serial.printf("✅ 人臉登錄成功：%s (ID: %d)\n", name.c_str(), id);
            return true;
        }
        return false;
    }

    // 刪除指定名稱的人臉
    bool deleteByName(const String& name) {
        auto ids = recognizer.get_enrolled_ids();
        int index = 0;
        for (auto& face : ids) {
            if (String(face.name.c_str()) == name) {
                recognizer.delete_id(index);
                Serial.printf("✅ 已刪除人臉：%s\n", name.c_str());
                return true;
            }
            index++;
        }
        Serial.printf("❌ 找不到人臉：%s\n", name.c_str());
        return false;
    }

    // 刪除所有人臉
    void deleteAll() {
        int count = recognizer.get_enrolled_ids().size();
        for (int i = count - 1; i >= 0; i--) {
            recognizer.delete_id(i);
        }
        Serial.println("✅ 已刪除所有人臉");
    }

    // 取得已登錄人臉清單
    std::vector<String> getList() {
        std::vector<String> result;
        auto ids = recognizer.get_enrolled_ids();
        for (auto& face : ids) {
            result.push_back(String(face.name.c_str()));
        }
        return result;
    }

    int getCount() {
        return recognizer.get_enrolled_ids().size();
    }
};