/**
 * @file watchface.cpp
 * @brief Watch face page implementation.
 */

#include "watchface.h"

LV_FONT_DECLARE(watch_semibold_176);
LV_FONT_DECLARE(watch_semibold_112);

static lv_obj_t* hour_label_ = nullptr;
static lv_obj_t* minute_label_ = nullptr;
static lv_obj_t* second_label_ = nullptr;

static void disable_scroll(lv_obj_t* obj) {
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t* create_time_label(lv_obj_t* parent,
                                   int x,
                                   int y,
                                   int w,
                                   int h,
                                   const lv_font_t* font,
                                   lv_color_t color) {
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, "00");

    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_obj_set_style_shadow_width(label, 0, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);

    disable_scroll(label);
    return label;
}

static void set_two_digit_label(lv_obj_t* label, int value) {
    if (label == nullptr) {
        return;
    }

    if (value < 0) {
        value = 0;
    }

    char buf[3];
    snprintf(buf, sizeof(buf), "%02d", value % 100);
    lv_label_set_text(label, buf);
}

void watchface_setup(lv_obj_t* parent) {
    if (parent == nullptr) {
        UI_LOGW("watchface_setup: parent is null");
        return;
    }

    const int screen_w = ui_screen_width;
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    disable_scroll(parent);

    const int hour_w = 240;
    const int hour_h = 132;
    const int minute_w = 152;
    const int minute_h = 86;
    const int second_w = 92;
    const int second_h = 52;
    const int hour_x = (screen_w - hour_w) / 2;
    const int minute_x = screen_w - minute_w - UI_SPACING(6);

    hour_label_ = create_time_label(parent, hour_x, 30, hour_w, hour_h, &watch_semibold_176,
                                    lv_color_make(255, 211, 0));
    minute_label_ = create_time_label(parent, minute_x, 170, minute_w, minute_h, &watch_semibold_112,
                                      lv_color_make(188, 239, 92));
    second_label_ = create_time_label(parent, UI_SPACING(2), 166, second_w, second_h, &lv_font_montserrat_48,
                                      lv_color_make(103, 224, 0));
    UI_LOGI("Watch face page setup complete");
}

void watchface_update_time(const char* time_str) {
    if (time_str == nullptr) return;

    int hour = 0;
    int minute = 0;
    int second = (millis() / 1000) % 60;
    const int parsed = sscanf(time_str, "%d:%d:%d", &hour, &minute, &second);
    if (parsed < 2) {
        return;
    }

    set_two_digit_label(hour_label_, hour);
    set_two_digit_label(minute_label_, minute % 60);
    set_two_digit_label(second_label_, second % 60);
}

void watchface_update_date(const char* date_str) {
    LV_UNUSED(date_str);
}
