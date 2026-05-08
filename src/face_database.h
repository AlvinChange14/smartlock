// src/face_database.h
#pragma once

#include <SPIFFS.h>
#include <vector>
#include <string>

#define FACE_DB_DIR   "/faces"
#define FACE_MAGIC    0x46414345  // "FACE"
#define MAX_FACES     10
#define FEATURE_SIZE  512         // MobileFaceNet 512 維特徵向量

struct FaceRecord {
    char name[32];
    int16_t feature[FEATURE_SIZE];
};

class FaceDatabase {
public:
    bool begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("❌ SPIFFS 初始化失敗");
            return false;
        }

        // 建立人臉目錄
        if (!SPIFFS.exists(FACE_DB_DIR)) {
            SPIFFS.mkdir(FACE_DB_DIR);
        }

        Serial.printf("✅ SPIFFS 已掛載，可用空間: %d KB\n",
                      SPIFFS.totalBytes() / 1024 - SPIFFS.usedBytes() / 1024);
        return true;
    }

    // 儲存人臉特徵向量
    bool save(const String& name, const int16_t* feature) {
        if (count() >= MAX_FACES) {
            Serial.println("❌ 人臉資料庫已滿（最多10筆）");
            return false;
        }

        String path = getPath(name);
        File f = SPIFFS.open(path, FILE_WRITE);
        if (!f) {
            Serial.printf("❌ 無法開啟檔案: %s\n", path.c_str());
            return false;
        }

        // 寫入 magic number
        uint32_t magic = FACE_MAGIC;
        f.write((uint8_t*)&magic, 4);

        // 寫入名稱長度和名稱
        uint8_t nameLen = min((int)name.length(), 31);
        f.write(&nameLen, 1);
        f.write((uint8_t*)name.c_str(), nameLen);

        // 寫入特徵向量（512 x int16_t = 1024 bytes）
        f.write((uint8_t*)feature, FEATURE_SIZE * sizeof(int16_t));

        f.close();
        Serial.printf("✅ 已儲存人臉：%s\n", name.c_str());
        return true;
    }

    // 載入所有人臉到辨識器
    // 回傳已載入的 FaceRecord 清單（供外部辨識器重建）
    std::vector<FaceRecord> loadAll() {
        std::vector<FaceRecord> records;

        File dir = SPIFFS.open(FACE_DB_DIR);
        if (!dir || !dir.isDirectory()) return records;

        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                FaceRecord rec;
                if (loadRecord(entry.name(), rec)) {
                    records.push_back(rec);
                }
            }
            entry = dir.openNextFile();
        }

        Serial.printf("✅ 已載入 %d 筆人臉資料\n", records.size());
        return records;
    }

    // 刪除指定人臉
    bool remove(const String& name) {
        String path = getPath(name);
        if (SPIFFS.exists(path)) {
            SPIFFS.remove(path);
            Serial.printf("✅ 已刪除人臉：%s\n", name.c_str());
            return true;
        }
        Serial.printf("❌ 找不到人臉：%s\n", name.c_str());
        return false;
    }

    // 刪除所有人臉
    void removeAll() {
        File dir = SPIFFS.open(FACE_DB_DIR);
        if (!dir || !dir.isDirectory()) return;

        std::vector<String> toDelete;
        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                toDelete.push_back(String(FACE_DB_DIR) + "/" + entry.name());
            }
            entry = dir.openNextFile();
        }

        for (auto& path : toDelete) {
            SPIFFS.remove(path);
        }
        Serial.println("✅ 已清除所有人臉資料");
    }

    // 列出所有人臉名稱
    std::vector<String> list() {
        std::vector<String> names;
        File dir = SPIFFS.open(FACE_DB_DIR);
        if (!dir || !dir.isDirectory()) return names;

        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                // 去掉 .face 副檔名
                String fname = String(entry.name());
                fname.remove(fname.lastIndexOf('.'));
                names.push_back(fname);
            }
            entry = dir.openNextFile();
        }
        return names;
    }

    // 取得人臉數量
    int count() {
        return list().size();
    }

private:
    String getPath(const String& name) {
        return String(FACE_DB_DIR) + "/" + name + ".face";
    }

    bool loadRecord(const String& filename, FaceRecord& rec) {
        File f = SPIFFS.open(String(FACE_DB_DIR) + "/" + filename, FILE_READ);
        if (!f) return false;

        // 驗證 magic number
        uint32_t magic;
        f.read((uint8_t*)&magic, 4);
        if (magic != FACE_MAGIC) {
            f.close();
            return false;
        }

        // 讀取名稱
        uint8_t nameLen;
        f.read(&nameLen, 1);
        memset(rec.name, 0, sizeof(rec.name));
        f.read((uint8_t*)rec.name, nameLen);

        // 讀取特徵向量
        f.read((uint8_t*)rec.feature, FEATURE_SIZE * sizeof(int16_t));
        f.close();
        return true;
    }
};