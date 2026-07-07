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
 * @brief 创建天气页面
 * @param parent 父容器
 */
void weather_page_setup(lv_obj_t* parent);

/**
 * @brief 更新天气数据显示
 * @param data 天气数据结构指针
 * @note 如果 data 为 NULL 或 data->is_valid 为 false，则显示"加载中..."
 */
void weather_page_update(const WeatherData* data);

/**
 * @brief 使用模拟数据更新天气显示（用于测试）
 */
void weather_page_update_demo(void);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_PAGE_H
