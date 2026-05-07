/**
 * @file weather_page.h
 * @brief 天气页面头文件
 */

#ifndef WEATHER_PAGE_H
#define WEATHER_PAGE_H

#include "ui_common.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a Weather Page
 * @param parent Parent container
 */
void weather_page_setup(lv_obj_t* parent);

/**
 * @brief Update Weather Data Display
 * @param data Pointer to the weather data structure
 * @note If data is NULL or data->is_valid is false, display "Loading..."
 */
void weather_page_update(const WeatherData* data);

/**
 * @brief Update Weather Display with Demo Data (for testing)
 */
void weather_page_update_demo(void);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_PAGE_H
