/**
 * @file watchface.cpp
 * @brief Implementation of the Dial Page
 */

#include "watchface.h"

static lv_obj_t* watch_time_label_ = nullptr;
static lv_obj_t* main_date_label_ = nullptr;
static lv_obj_t* main_steps_label_ = nullptr;

void watchface_setup(lv_obj_t* parent) {
    if (parent == nullptr) {
        UI_LOGW("watchface_setup: parent is null");
        return;
    }

    const int screen_w = ui_screen_width;
    const int screen_h = ui_screen_height;
    const int time_offset_y = -(screen_h * 22 / 100);
    const int card_y = screen_h * 12 / 100;

    lv_obj_set_style_bg_color(parent, UI_BG_COLOR, 0);

    watch_time_label_ = lv_label_create(parent);
    lv_label_set_text(watch_time_label_, "--:--");
    lv_obj_set_style_text_font(watch_time_label_, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(watch_time_label_, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(watch_time_label_, 3, 0);
    lv_obj_set_style_bg_color(watch_time_label_, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_bg_opa(watch_time_label_, LV_OPA_40, 0);
    lv_obj_set_style_radius(watch_time_label_, 14, 0);
    lv_obj_set_style_pad_hor(watch_time_label_, UI_SPACING(9), 0);
    lv_obj_set_style_pad_ver(watch_time_label_, UI_SPACING(6), 0);
    lv_obj_align(watch_time_label_, LV_ALIGN_CENTER, 0, time_offset_y);

    lv_obj_t* info_card = lv_obj_create(parent);
    lv_obj_set_size(info_card, screen_w - UI_SPACING(12), LV_SIZE_CONTENT);
    lv_obj_align(info_card, LV_ALIGN_CENTER, 0, card_y);
    lv_obj_set_style_bg_opa(info_card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_card, 0, 0);
    lv_obj_set_style_radius(info_card, 0, 0);
    lv_obj_set_style_pad_all(info_card, UI_SPACING(6), 0);
    lv_obj_set_style_pad_row(info_card, UI_SPACING(3), 0);
    lv_obj_set_flex_flow(info_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(info_card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(info_card, LV_DIR_NONE);
    lv_obj_clear_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

    main_date_label_ = lv_label_create(info_card);
    lv_label_set_text(main_date_label_, "----/--/-- ---");
    lv_obj_set_style_text_color(main_date_label_, lv_color_make(200, 220, 255), 0);
    lv_obj_set_style_text_font(main_date_label_, &ui_font_text, 0);
    lv_obj_set_style_text_align(main_date_label_, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* divider = lv_obj_create(info_card);
    lv_obj_set_size(divider, LV_PCT(85), 1);
    lv_obj_set_style_bg_color(divider, lv_color_make(100, 170, 255), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_40, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_pad_all(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    lv_obj_set_scrollbar_mode(divider, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(divider, LV_DIR_NONE);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* steps_row = lv_obj_create(info_card);
    lv_obj_set_size(steps_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(steps_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(steps_row, 0, 0);
    lv_obj_set_style_pad_all(steps_row, 0, 0);
    lv_obj_set_style_shadow_width(steps_row, 0, 0);
    lv_obj_set_flex_flow(steps_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(steps_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(steps_row, UI_SPACING(2), 0);
    lv_obj_set_scrollbar_mode(steps_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(steps_row, LV_DIR_NONE);
    lv_obj_clear_flag(steps_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* steps_icon = lv_label_create(steps_row);
    lv_label_set_text(steps_icon, UI_ICON_HEART);
    lv_obj_set_style_text_font(steps_icon, &ui_font_icon, 0);
    lv_obj_set_style_text_color(steps_icon, lv_color_make(255, 100, 130), 0);

    main_steps_label_ = lv_label_create(steps_row);
    lv_label_set_text(main_steps_label_, "0 steps");
    lv_obj_set_style_text_color(main_steps_label_, lv_color_make(130, 220, 180), 0);
    lv_obj_set_style_text_font(main_steps_label_, &ui_font_text, 0);

    UI_LOGI("Watch face page setup complete");
}

void watchface_update_time(const char* time_str) {
    if (watch_time_label_ == nullptr || time_str == nullptr) return;
    lv_label_set_text(watch_time_label_, time_str);
}

void watchface_update_date(const char* date_str) {
    if (main_date_label_ == nullptr || date_str == nullptr) return;
    lv_label_set_text(main_date_label_, date_str);
}

void watchface_update_steps(int steps) {
    if (main_steps_label_ == nullptr) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d steps", steps);
    lv_label_set_text(main_steps_label_, buf);
}
