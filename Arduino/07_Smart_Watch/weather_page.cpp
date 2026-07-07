/**
 * @file weather_page.cpp
 * @brief 天气页面实现
 */

#include "weather_page.h"
#include "weather_client.h"

static lv_obj_t* weather_city_label_ = nullptr;
static lv_obj_t* weather_temp_label_ = nullptr;
static lv_obj_t* weather_humidity_label_ = nullptr;
static lv_obj_t* weather_condition_label_ = nullptr;
static lv_obj_t* weather_pressure_label_ = nullptr;

/* 页面颜色 */
#define WEATHER_PAGE_BG_COLOR          lv_color_hex(0xF4F7FB)
#define WEATHER_CARD_BG_COLOR          lv_color_hex(0xFFFFFF)
#define WEATHER_HERO_BG_COLOR          lv_color_hex(0xEAF2FF)
#define WEATHER_METRIC_BG_COLOR        lv_color_hex(0xF8FAFC)

#define WEATHER_BORDER_COLOR           lv_color_hex(0xE2E8F0)
#define WEATHER_TEXT_COLOR             lv_color_hex(0x0F172A)
#define WEATHER_MUTED_TEXT_COLOR       lv_color_hex(0x64748B)

#define WEATHER_PRIMARY_COLOR          lv_color_hex(0x2563EB)
#define WEATHER_BADGE_BG_COLOR         lv_color_hex(0xDBEAFE)

#define WEATHER_HUMIDITY_COLOR         lv_color_hex(0x0891B2)
#define WEATHER_PRESSURE_COLOR         lv_color_hex(0x7C3AED)

/**
 * @brief 设置透明容器的通用样式
 */
static void weather_set_clean_container(lv_obj_t* obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * @brief 创建底部指标卡片
 */
static void weather_create_metric_card(
    lv_obj_t* parent,
    const char* title_text,
    lv_color_t accent_color,
    lv_obj_t** value_label)
{
    lv_obj_t* metric_card = lv_obj_create(parent);

    /*
     * 宽度设为0，通过flex grow平均分配空间。
     * 两张卡片会自动保持相同宽度。
     */
    lv_obj_set_size(metric_card, 0, LV_PCT(100));
    lv_obj_set_flex_grow(metric_card, 1);

    lv_obj_set_style_bg_color(
        metric_card,
        WEATHER_METRIC_BG_COLOR,
        0
    );
    lv_obj_set_style_bg_opa(metric_card, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(metric_card, 1, 0);
    lv_obj_set_style_border_color(
        metric_card,
        WEATHER_BORDER_COLOR,
        0
    );
    lv_obj_set_style_border_opa(metric_card, LV_OPA_COVER, 0);

    lv_obj_set_style_radius(metric_card, 12, 0);
    lv_obj_set_style_pad_all(metric_card, 7, 0);
    lv_obj_set_style_pad_row(metric_card, 3, 0);

    lv_obj_set_flex_flow(metric_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        metric_card,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_set_scrollbar_mode(
        metric_card,
        LV_SCROLLBAR_MODE_OFF
    );
    lv_obj_clear_flag(
        metric_card,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /* 顶部短色条 */
    lv_obj_t* accent_line = lv_obj_create(metric_card);
    lv_obj_set_size(accent_line, 24, 3);
    lv_obj_set_style_bg_color(
        accent_line,
        accent_color,
        0
    );
    lv_obj_set_style_bg_opa(
        accent_line,
        LV_OPA_COVER,
        0
    );
    lv_obj_set_style_border_width(accent_line, 0, 0);
    lv_obj_set_style_radius(accent_line, 99, 0);
    lv_obj_set_style_pad_all(accent_line, 0, 0);
    lv_obj_clear_flag(
        accent_line,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /* 指标名称 */
    lv_obj_t* title_label = lv_label_create(metric_card);
    lv_label_set_text(title_label, title_text);
    lv_obj_set_style_text_color(
        title_label,
        WEATHER_MUTED_TEXT_COLOR,
        0
    );
    lv_obj_set_style_text_font(
        title_label,
        &ui_font_text,
        0
    );

    /* 指标值 */
    *value_label = lv_label_create(metric_card);
    lv_label_set_text(*value_label, "--");

    lv_obj_set_style_text_color(
        *value_label,
        accent_color,
        0
    );
    lv_obj_set_style_text_font(
        *value_label,
        &ui_font_text,
        0
    );
}

void weather_page_setup(lv_obj_t* parent)
{
    if (parent == nullptr) {
        UI_LOGW("weather_page_setup: parent is null");
        return;
    }

    lv_obj_set_style_bg_color(
        parent,
        WEATHER_PAGE_BG_COLOR,
        0
    );
    lv_obj_set_style_bg_opa(
        parent,
        LV_OPA_COVER,
        0
    );

    /*
     * 页面标题
     * 使用左对齐比居中更有仪表盘、系统界面的感觉。
     */
    lv_obj_t* page_title = lv_label_create(parent);
    lv_label_set_text(page_title, "Weather");

    lv_obj_set_style_text_color(
        page_title,
        WEATHER_TEXT_COLOR,
        0
    );
    lv_obj_set_style_text_font(
        page_title,
        &ui_font_text,
        0
    );

    lv_obj_align(
        page_title,
        LV_ALIGN_TOP_LEFT,
        12,
        7
    );

    /*
     * 外层主卡片
     */
    lv_obj_t* main_card = lv_obj_create(parent);

    lv_obj_set_size(
        main_card,
        ui_screen_width - 16,
        ui_screen_height - 42
    );

    lv_obj_align(
        main_card,
        LV_ALIGN_BOTTOM_MID,
        0,
        -6
    );

    lv_obj_set_style_bg_color(
        main_card,
        WEATHER_CARD_BG_COLOR,
        0
    );
    lv_obj_set_style_bg_opa(
        main_card,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(main_card, 1, 0);
    lv_obj_set_style_border_color(
        main_card,
        WEATHER_BORDER_COLOR,
        0
    );
    lv_obj_set_style_border_opa(
        main_card,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_radius(main_card, 18, 0);
    lv_obj_set_style_pad_all(main_card, 9, 0);
    lv_obj_set_style_pad_row(main_card, 8, 0);

    lv_obj_set_flex_flow(
        main_card,
        LV_FLEX_FLOW_COLUMN
    );

    lv_obj_set_flex_align(
        main_card,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_set_scrollbar_mode(
        main_card,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_set_scroll_dir(
        main_card,
        LV_DIR_NONE
    );

    lv_obj_clear_flag(
        main_card,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /*
     * 主天气区域
     */
    lv_obj_t* hero_card = lv_obj_create(main_card);

    lv_obj_set_size(
        hero_card,
        LV_PCT(100),
        0
    );
    lv_obj_set_flex_grow(hero_card, 1);

    lv_obj_set_style_bg_color(
        hero_card,
        WEATHER_HERO_BG_COLOR,
        0
    );
    lv_obj_set_style_bg_opa(
        hero_card,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(hero_card, 0, 0);
    lv_obj_set_style_radius(hero_card, 14, 0);

    lv_obj_set_style_pad_all(hero_card, 8, 0);
    lv_obj_set_style_pad_row(hero_card, 3, 0);

    lv_obj_set_flex_flow(
        hero_card,
        LV_FLEX_FLOW_COLUMN
    );

    lv_obj_set_flex_align(
        hero_card,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_set_scrollbar_mode(
        hero_card,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_clear_flag(
        hero_card,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /*
     * 城市名称
     */
    weather_city_label_ = lv_label_create(hero_card);
    lv_label_set_text(weather_city_label_, "--");

    lv_obj_set_width(
        weather_city_label_,
        LV_PCT(90)
    );

    lv_label_set_long_mode(
        weather_city_label_,
        LV_LABEL_LONG_DOT
    );

    lv_obj_set_style_text_align(
        weather_city_label_,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_style_text_color(
        weather_city_label_,
        WEATHER_TEXT_COLOR,
        0
    );

    lv_obj_set_style_text_font(
        weather_city_label_,
        &ui_font_text,
        0
    );

    /*
     * 核心温度
     */
    weather_temp_label_ = lv_label_create(hero_card);
    lv_label_set_text(weather_temp_label_, "--°C");

    lv_obj_set_style_text_color(
        weather_temp_label_,
        WEATHER_PRIMARY_COLOR,
        0
    );

    lv_obj_set_style_text_font(
        weather_temp_label_,
        &lv_font_montserrat_40,
        0
    );

    /*
     * 天气状态胶囊标签
     */
    lv_obj_t* condition_badge = lv_obj_create(hero_card);

    lv_obj_set_size(
        condition_badge,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT
    );

    lv_obj_set_style_bg_color(
        condition_badge,
        WEATHER_BADGE_BG_COLOR,
        0
    );

    lv_obj_set_style_bg_opa(
        condition_badge,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        condition_badge,
        0,
        0
    );

    lv_obj_set_style_radius(
        condition_badge,
        99,
        0
    );

    lv_obj_set_style_pad_hor(
        condition_badge,
        10,
        0
    );

    lv_obj_set_style_pad_ver(
        condition_badge,
        4,
        0
    );

    lv_obj_set_scrollbar_mode(
        condition_badge,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_clear_flag(
        condition_badge,
        LV_OBJ_FLAG_SCROLLABLE
    );

    weather_condition_label_ =
        lv_label_create(condition_badge);

    lv_label_set_text(
        weather_condition_label_,
        "Loading..."
    );

    lv_obj_set_style_text_color(
        weather_condition_label_,
        WEATHER_PRIMARY_COLOR,
        0
    );

    lv_obj_set_style_text_font(
        weather_condition_label_,
        &ui_font_text,
        0
    );

    /*
     * 底部湿度、气压区域
     */
    lv_obj_t* metrics_row = lv_obj_create(main_card);

    lv_obj_set_size(
        metrics_row,
        LV_PCT(100),
        68
    );

    weather_set_clean_container(metrics_row);

    lv_obj_set_style_pad_column(
        metrics_row,
        8,
        0
    );

    lv_obj_set_flex_flow(
        metrics_row,
        LV_FLEX_FLOW_ROW
    );

    lv_obj_set_flex_align(
        metrics_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    weather_create_metric_card(
        metrics_row,
        "Humidity",
        WEATHER_HUMIDITY_COLOR,
        &weather_humidity_label_
    );

    weather_create_metric_card(
        metrics_row,
        "Pressure",
        WEATHER_PRESSURE_COLOR,
        &weather_pressure_label_
    );

    weather_page_update_demo();

    UI_LOGI("Weather page setup complete");
}

void weather_page_update(const WeatherData* data)
{
    if (data == nullptr || !data->is_valid) {
        if (weather_city_label_ != nullptr) {
            lv_label_set_text(
                weather_city_label_,
                "--"
            );
        }

        if (weather_temp_label_ != nullptr) {
            lv_label_set_text(
                weather_temp_label_,
                "--°C"
            );
        }

        if (weather_condition_label_ != nullptr) {
            lv_label_set_text(
                weather_condition_label_,
                "Unavailable"
            );
        }

        if (weather_humidity_label_ != nullptr) {
            lv_label_set_text(
                weather_humidity_label_,
                "--%"
            );
        }

        if (weather_pressure_label_ != nullptr) {
            lv_label_set_text(
                weather_pressure_label_,
                "-- hPa"
            );
        }

        return;
    }

    if (weather_city_label_ != nullptr) {
        lv_label_set_text(
            weather_city_label_,
            data->city
        );
    }

    if (weather_temp_label_ != nullptr) {
        char temp_buf[16];

        snprintf(
            temp_buf,
            sizeof(temp_buf),
            "%d°C",
            data->temperature
        );

        lv_label_set_text(
            weather_temp_label_,
            temp_buf
        );
    }

    if (weather_condition_label_ != nullptr) {
        lv_label_set_text(
            weather_condition_label_,
            data->text
        );
    }

    /*
     * 指标卡片已经显示了“Humidity”，
     * 所以数值标签只显示数值，不再重复标题。
     */
    if (weather_humidity_label_ != nullptr) {
        char humidity_buf[16];

        snprintf(
            humidity_buf,
            sizeof(humidity_buf),
            "%d%%",
            data->humidity
        );

        lv_label_set_text(
            weather_humidity_label_,
            humidity_buf
        );
    }

    if (weather_pressure_label_ != nullptr) {
        char pressure_buf[20];

        if (data->pressure > 0) {
            snprintf(
                pressure_buf,
                sizeof(pressure_buf),
                "%d hPa",
                data->pressure
            );
        } else {
            snprintf(
                pressure_buf,
                sizeof(pressure_buf),
                "-- hPa"
            );
        }

        lv_label_set_text(
            weather_pressure_label_,
            pressure_buf
        );
    }
}

void weather_page_update_demo(void)
{
    WeatherData demo = {};

    weather_client_generate_demo(&demo);
    weather_page_update(&demo);
}