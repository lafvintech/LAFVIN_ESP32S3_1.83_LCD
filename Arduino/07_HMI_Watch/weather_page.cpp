/**
 * @file weather_page.cpp
 * @brief Weather Page Implementation
 */

#include "weather_page.h"

#include "weather_client.h"

static lv_obj_t* weather_city_label_ = nullptr;
static lv_obj_t* weather_temp_label_ = nullptr;
static lv_obj_t* weather_humidity_label_ = nullptr;
static lv_obj_t* weather_condition_label_ = nullptr;
static lv_obj_t* weather_pressure_label_ = nullptr;

void weather_page_setup(lv_obj_t* parent) {
    if (parent == nullptr) {
        UI_LOGW("weather_page_setup: parent is null");
        return;
    }

    lv_obj_set_style_bg_color(parent, UI_BG_COLOR, 0);

    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Weather");
    lv_obj_set_style_text_color(title, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(title, &ui_font_text, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_SPACING(6));

    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, ui_screen_width - UI_SPACING(10), ui_screen_height - UI_SPACING(24));
    lv_obj_align(card, LV_ALIGN_BOTTOM_MID, 0, -UI_SPACING(4));
    lv_obj_set_style_bg_color(card, UI_CHAT_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, UI_BORDER_COLOR, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, UI_SPACING(6), 0);
    lv_obj_set_style_pad_row(card, UI_SPACING(4), 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(card, LV_DIR_NONE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    weather_city_label_ = lv_label_create(card);
    lv_label_set_text(weather_city_label_, "--");
    lv_obj_set_style_text_color(weather_city_label_, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(weather_city_label_, &ui_font_text, 0);

    weather_temp_label_ = lv_label_create(card);
    lv_label_set_text(weather_temp_label_, "--°C");
    lv_obj_set_style_text_color(weather_temp_label_, UI_WEATHER_TEMP_COLOR, 0);
    lv_obj_set_style_text_font(weather_temp_label_, &lv_font_montserrat_40, 0);

    weather_condition_label_ = lv_label_create(card);
    lv_label_set_text(weather_condition_label_, "Loading...");
    lv_obj_set_style_text_color(weather_condition_label_, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(weather_condition_label_, &ui_font_text, 0);

    weather_humidity_label_ = lv_label_create(card);
    lv_label_set_text(weather_humidity_label_, "Humidity --%");
    lv_obj_set_style_text_color(weather_humidity_label_, UI_WEATHER_HUMID_COLOR, 0);
    lv_obj_set_style_text_font(weather_humidity_label_, &ui_font_text, 0);

    weather_pressure_label_ = lv_label_create(card);
    lv_label_set_text(weather_pressure_label_, "Pressure ----hPa");
    lv_obj_set_style_text_color(weather_pressure_label_, UI_WEATHER_HUMID_COLOR, 0);
    lv_obj_set_style_text_font(weather_pressure_label_, &ui_font_text, 0);

    weather_page_update_demo();
    UI_LOGI("Weather page setup complete");
}

void weather_page_update(const WeatherData* data) {
    if (data == nullptr || !data->is_valid) {
        if (weather_condition_label_ != nullptr) {
            lv_label_set_text(weather_condition_label_, "Load failed");
        }
        return;
    }

    if (weather_city_label_ != nullptr) {
        lv_label_set_text(weather_city_label_, data->city);
    }

    if (weather_temp_label_ != nullptr) {
        char temp_buf[16];
        snprintf(temp_buf, sizeof(temp_buf), "%d°C", data->temperature);
        lv_label_set_text(weather_temp_label_, temp_buf);
    }

    if (weather_condition_label_ != nullptr) {
        lv_label_set_text(weather_condition_label_, data->text);
    }

    if (weather_humidity_label_ != nullptr) {
        char humidity_buf[24];
        snprintf(humidity_buf, sizeof(humidity_buf), "Humidity %d%%", data->humidity);
        lv_label_set_text(weather_humidity_label_, humidity_buf);
    }

    if (weather_pressure_label_ != nullptr) {
        char pressure_buf[32];
        if (data->pressure > 0) {
            snprintf(pressure_buf, sizeof(pressure_buf), "Pressure %dhPa", data->pressure);
        } else {
            snprintf(pressure_buf, sizeof(pressure_buf), "Pressure --");
        }
        lv_label_set_text(weather_pressure_label_, pressure_buf);
    }
}

void weather_page_update_demo(void) {
    WeatherData demo = {};
    weather_client_generate_demo(&demo);
    weather_page_update(&demo);
}
