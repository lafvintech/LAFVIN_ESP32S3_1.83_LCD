/**
 * @file ui_common.h
 * @brief Extracted UI common definition files
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <Arduino.h>
#include <lvgl.h>
#include <stdint.h>

// Shared UI constants and small data types used by multiple pages.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UI_LOGI
  #define UI_LOGI(fmt, ...) Serial.printf("[UI] " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef UI_LOGW
  #define UI_LOGW(fmt, ...) Serial.printf("[UI][WARN] " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef UI_LOGE
  #define UI_LOGE(fmt, ...) Serial.printf("[UI][ERR] " fmt "\n", ##__VA_ARGS__)
#endif

enum PageIndex {
    PAGE_WATCHFACE   = 0,
    PAGE_MENU        = 1,
    PAGE_WEATHER     = 2,
    PAGE_RGB_CONTROL = 3,
    PAGE_RGB_LED     = 4,
    PAGE_COUNT
};

#define UI_BG_COLOR              lv_color_hex(0x000000)
#define UI_TEXT_COLOR            lv_color_hex(0xFFFFFF)
#define UI_TEXT_COLOR_SECONDARY  lv_color_make(160, 160, 160)
#define UI_TEXT_COLOR_ACCENT     lv_color_make(100, 180, 255)
#define UI_CHAT_BG_COLOR         lv_color_hex(0x1F1F1F)
#define UI_BORDER_COLOR          lv_color_hex(0xFFFFFF)
#define UI_LOW_BATTERY_COLOR     lv_color_hex(0xFF0000)

#define UI_WEATHER_TEMP_COLOR    lv_color_make(100, 200, 255)
#define UI_WEATHER_HUMID_COLOR   lv_color_make(245, 158, 11)
#define UI_WEATHER_GRID_COLOR    lv_color_make(100, 100, 100)

#define UI_SPACING(scale)   ((scale) * 2)

extern const lv_font_t ui_font_text;
extern const lv_font_t ui_font_icon;
extern const lv_font_t ui_font_large_icon;

#define UI_ICON_HEART        "HR"
#define UI_ICON_CLOUD        "CLD"
#define UI_ICON_TEMPERATURE  "TMP"

typedef struct WeatherData {
    char city[32];
    char text[32];
    int temperature;
    int humidity;
    int pressure;
    char wind_direction[16];
    int wind_scale;
    char last_update[32];
    bool is_valid;
} WeatherData;

extern int ui_screen_width;
extern int ui_screen_height;

#ifdef __cplusplus
}
#endif

#endif // UI_COMMON_H
