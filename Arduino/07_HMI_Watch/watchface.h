/**
 * @file watchface.h
 * @brief 表盘页面头文件
 */

#ifndef WATCHFACE_H
#define WATCHFACE_H

#include "ui_common.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void watchface_setup(lv_obj_t* parent);
void watchface_update_time(const char* time_str);
void watchface_update_date(const char* date_str);
void watchface_update_steps(int steps);

#ifdef __cplusplus
}
#endif

#endif // WATCHFACE_H
