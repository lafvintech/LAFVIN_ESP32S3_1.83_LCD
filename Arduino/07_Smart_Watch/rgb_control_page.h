/**
 * @file rgb_control_page.h
 * @brief RGB Control 页面头文件
 */

#ifndef RGB_CONTROL_PAGE_H
#define RGB_CONTROL_PAGE_H

#include "ui_common.h"
#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void rgb_control_page_setup(lv_obj_t* parent);
void rgb_control_page_setup_mode(lv_obj_t* parent, int page_index);
void rgb_control_page_on_enter(int page_index);
void rgb_control_page_handle_single_click(int page_index);
void rgb_control_page_handle_double_click(int page_index);
void rgb_control_page_turn_off_rgb_led(void);
uint8_t rgb_control_page_get_backlight(void);

#ifdef __cplusplus
}
#endif

#endif // RGB_CONTROL_PAGE_H
