// src/weather.h
#pragma once

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"

struct WeatherInfo {
    String description;   // 天氣描述（中文）
    float  temp;          // 溫度 (°C)
    float  feelsLike;     // 體感溫度
    int    humidity;      // 濕度 (%)
    float  windSpeed;     // 風速 (m/s)
    bool   rainToday;     // 是否下雨
    bool   valid;         // 資料是否有效
};

// ── 查詢目前天氣 ──────────────────────────────
WeatherInfo getWeather() {
    WeatherInfo info = {"", 0, 0, 0, 0, false, false};

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi 未連線，跳過天氣查詢");
        return info;
    }

    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += String(OWM_CITY) + "," + String(OWM_COUNTRY);
    url += "&appid=" + String(OWM_API_KEY);
    url += "&units=metric&lang=" + String(OWM_LANG);

    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        Serial.printf("天氣 API 失敗，HTTP %d\n", code);
        http.end();
        return info;
    }

    String payload = http.getString();
    http.end();

    // 解析 JSON
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, payload)) {
        Serial.println("天氣 JSON 解析失敗");
        return info;
    }

    info.description = doc["weather"][0]["description"].as<String>();
    info.temp        = doc["main"]["temp"].as<float>();
    info.feelsLike   = doc["main"]["feels_like"].as<float>();
    info.humidity    = doc["main"]["humidity"].as<int>();
    info.windSpeed   = doc["wind"]["speed"].as<float>();
    info.valid       = true;

    // 判斷是否下雨
    String mainCond = doc["weather"][0]["main"].as<String>();
    info.rainToday = (mainCond == "Rain" ||
                      mainCond == "Drizzle" ||
                      mainCond == "Thunderstorm");

    Serial.printf("天氣更新：%s  %.1f°C  濕度 %d%%\n",
                  info.description.c_str(), info.temp, info.humidity);
    return info;
}

// ── 產生出門提醒文字（用於 OLED 顯示或 Telegram）──
String getWeatherMessage(const WeatherInfo& w) {
    if (!w.valid) return "天氣資料無法取得";

    String msg = "";

    if (w.rainToday) {
        msg += "☔ 今天有雨，請帶傘！";
    } else if (w.temp > 34) {
        msg += "☀️ 炎熱 " + String((int)w.temp) + "°C，注意防曬補水";
    } else if (w.temp < 14) {
        msg += "🧥 偏涼 " + String((int)w.temp) + "°C，記得加件外套";
    } else {
        msg += "🌤 " + w.description + " " + String((int)w.temp) + "°C";
    }

    if (w.windSpeed > 10) {
        msg += "，強風 " + String((int)w.windSpeed) + "m/s";
    }

    return msg;
}

// ── 短版天氣（OLED 上方空間有限，最多 16 字元）──
String getWeatherShort(const WeatherInfo& w) {
    if (!w.valid) return "No data";
    char buf[20];
    snprintf(buf, sizeof(buf), "%.0fC %d%%  %s",
             w.temp, w.humidity,
             w.rainToday ? "Rain" : "Clear");
    return String(buf);
}