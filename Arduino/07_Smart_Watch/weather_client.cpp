/**
 * @file weather_client.cpp
 * @brief Weather provider adapters for the watch demo.
 */

#include "weather_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

static const char *OPENWEATHER_URL_TEMPLATE =
    "%s/data/2.5/weather?q=%s&appid=%s&units=%s&lang=%s";
static const char *XINZHI_URL_TEMPLATE =
    "%s/v3/weather/now.json?key=%s&location=%s&language=%s&unit=%s";

static bool http_get_json(const char *url, String *payload_out);

static bool key_is_configured(const char *key, const char *placeholder) {
    if (key == nullptr || key[0] == '\0') {
        return false;
    }
    return strcmp(key, placeholder) != 0;
}

static const char *wind_deg_to_text(int degrees) {
    static const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int normalized = degrees % 360;
    if (normalized < 0) {
        normalized += 360;
    }
    const int index = ((normalized + 22) % 360) / 45;
    return directions[index];
}

static int wind_speed_to_scale(float speed_mps) {
    if (speed_mps < 0.3f) return 0;
    if (speed_mps < 1.6f) return 1;
    if (speed_mps < 3.4f) return 2;
    if (speed_mps < 5.5f) return 3;
    if (speed_mps < 8.0f) return 4;
    if (speed_mps < 10.8f) return 5;
    if (speed_mps < 13.9f) return 6;
    if (speed_mps < 17.2f) return 7;
    if (speed_mps < 20.8f) return 8;
    if (speed_mps < 24.5f) return 9;
    if (speed_mps < 28.5f) return 10;
    if (speed_mps < 32.7f) return 11;
    return 12;
}

static bool http_get_json(const char *url, String *payload_out) {
    if (url == nullptr || payload_out == nullptr) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        UI_LOGW("WiFi not connected");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(WEATHER_TIMEOUT_MS);
    http.begin(client, url);
    http.addHeader("User-Agent", "ESP32-WeatherClient/1.0");

    const int http_code = http.GET();
    if (http_code != HTTP_CODE_OK) {
        UI_LOGW("HTTP GET failed, code: %d", http_code);
        http.end();
        return false;
    }

    *payload_out = http.getString();
    http.end();
    UI_LOGI("Weather response: %d bytes", payload_out->length());
    return true;
}

bool weather_client_fetch(WeatherData *data) {
    if (data == nullptr) {
        return false;
    }

    if (weather_client_fetch_openweathermap(data)) {
        return true;
    }

    UI_LOGW("OpenWeatherMap fetch failed, trying Seniverse fallback");
    return weather_client_fetch_xinzhi(data);
}

bool weather_client_fetch_openweathermap(WeatherData *data) {
    if (data == nullptr) {
        return false;
    }

    if (!key_is_configured(WEATHER_API_KEY, "your_openweathermap_api_key_here")) {
        UI_LOGW("OpenWeatherMap API key is not configured");
        return false;
    }

    char url[512];
    snprintf(url, sizeof(url), OPENWEATHER_URL_TEMPLATE,
             WEATHER_API_HOST, WEATHER_CITY, WEATHER_API_KEY, WEATHER_UNITS, WEATHER_LANGUAGE);
    UI_LOGI("Fetching OpenWeatherMap: %s", url);

    String payload;
    if (!http_get_json(url, &payload)) {
        return false;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        UI_LOGW("OpenWeatherMap JSON parse failed: %s", error.c_str());
        return false;
    }

    const char *city_name = doc["name"] | WEATHER_CITY_DISPLAY;
    const char *description = doc["weather"][0]["description"] | doc["weather"][0]["main"] | "--";
    const float temperature = doc["main"]["temp"] | 0.0f;
    const int humidity = doc["main"]["humidity"] | 0;
    const int pressure = doc["main"]["pressure"] | 0;
    const float wind_speed = doc["wind"]["speed"] | 0.0f;
    const int wind_deg = doc["wind"]["deg"] | 0;
    const long updated_at = doc["dt"] | 0L;

    strncpy(data->city, city_name, sizeof(data->city) - 1);
    data->city[sizeof(data->city) - 1] = '\0';

    strncpy(data->text, description, sizeof(data->text) - 1);
    data->text[sizeof(data->text) - 1] = '\0';

    data->temperature = static_cast<int>(temperature);
    data->humidity = humidity;
    data->pressure = pressure;

    strncpy(data->wind_direction, wind_deg_to_text(wind_deg), sizeof(data->wind_direction) - 1);
    data->wind_direction[sizeof(data->wind_direction) - 1] = '\0';
    data->wind_scale = wind_speed_to_scale(wind_speed);

    snprintf(data->last_update, sizeof(data->last_update), "%ld", updated_at);
    data->is_valid = true;

    UI_LOGI("OpenWeatherMap parsed: %s %dC %s, humidity %d%%",
            data->city, data->temperature, data->text, data->humidity);
    return true;
}

bool weather_client_fetch_xinzhi(WeatherData *data) {
    if (data == nullptr) {
        return false;
    }

    if (!key_is_configured(XINZHI_API_KEY, "your_xinzhi_api_key_here")) {
        UI_LOGW("Seniverse fallback key is not configured");
        return false;
    }

    char url[512];
    snprintf(url, sizeof(url), XINZHI_URL_TEMPLATE,
             XINZHI_API_HOST, XINZHI_API_KEY, WEATHER_CITY, XINZHI_LANGUAGE, XINZHI_UNIT);
    UI_LOGI("Fetching Seniverse fallback: %s", url);

    String payload;
    if (!http_get_json(url, &payload)) {
        return false;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        UI_LOGW("Seniverse JSON parse failed: %s", error.c_str());
        return false;
    }

    JsonArray results = doc["results"];
    if (results.isNull() || results.size() == 0) {
        UI_LOGW("No results in Seniverse response");
        return false;
    }

    JsonObject first_result = results[0];
    JsonObject location = first_result["location"];
    JsonObject now = first_result["now"];

    const char *city_name = location["name"] | WEATHER_CITY_DISPLAY;
    const char *text = now["text"] | "--";
    const char *temperature = now["temperature"] | "0";
    const char *humidity = now["humidity"] | "0";
    const char *pressure = now["pressure"] | "0";
    const char *wind_direction = now["wind_direction"] | "--";
    const char *wind_scale = now["wind_scale"] | "0";
    const char *last_update = first_result["last_update"] | "";

    strncpy(data->city, city_name, sizeof(data->city) - 1);
    data->city[sizeof(data->city) - 1] = '\0';

    strncpy(data->text, text, sizeof(data->text) - 1);
    data->text[sizeof(data->text) - 1] = '\0';

    data->temperature = atoi(temperature);
    data->humidity = atoi(humidity);
    data->pressure = atoi(pressure);

    strncpy(data->wind_direction, wind_direction, sizeof(data->wind_direction) - 1);
    data->wind_direction[sizeof(data->wind_direction) - 1] = '\0';
    data->wind_scale = atoi(wind_scale);

    strncpy(data->last_update, last_update, sizeof(data->last_update) - 1);
    data->last_update[sizeof(data->last_update) - 1] = '\0';
    data->is_valid = true;

    UI_LOGI("Seniverse parsed: %s %dC %s, humidity %d%%",
            data->city, data->temperature, data->text, data->humidity);
    return true;
}

void weather_client_generate_demo(WeatherData *data) {
    if (data == nullptr) {
        return;
    }

    strncpy(data->city, WEATHER_CITY_DISPLAY, sizeof(data->city) - 1);
    data->city[sizeof(data->city) - 1] = '\0';
    strncpy(data->text, "Sunny", sizeof(data->text) - 1);
    data->text[sizeof(data->text) - 1] = '\0';
    data->temperature = 26;
    data->humidity = 65;
    data->pressure = 1012;
    strncpy(data->wind_direction, "SE", sizeof(data->wind_direction) - 1);
    data->wind_direction[sizeof(data->wind_direction) - 1] = '\0';
    data->wind_scale = 2;
    strncpy(data->last_update, "2026-05-06T14:30:00+08:00", sizeof(data->last_update) - 1);
    data->last_update[sizeof(data->last_update) - 1] = '\0';
    data->is_valid = true;
}
