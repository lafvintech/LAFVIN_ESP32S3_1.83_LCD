/**
 * @file rgb_control_page.cpp
 * @brief Shared UI logic for the backlight page and the RGB LED page.
 */

#include "rgb_control_page.h"

// The backlight page and the RGB LED page share one implementation.
// The page index decides which widget set and which behavior are active.

static const uint8_t RGB_LED_PIN = 38;
static const uint8_t RGB_LED_BRIGHTNESS = 25;
static const int RGB_UI_PAGE_COUNT = 2;

// Slot 0 is the backlight page, slot 1 is the RGB LED page.
static lv_obj_t* g_hint_labels[RGB_UI_PAGE_COUNT] = {nullptr, nullptr};
static lv_obj_t* g_state_labels[RGB_UI_PAGE_COUNT] = {nullptr, nullptr};
static lv_obj_t* g_value_labels[RGB_UI_PAGE_COUNT] = {nullptr, nullptr};
static lv_obj_t* g_color_previews[RGB_UI_PAGE_COUNT] = {nullptr, nullptr};

static bool g_backlight_control_active = false;
static uint8_t g_backlight_levels[] = {25, 50, 75, 100};
static int g_backlight_index = 2;

static bool g_rgb_control_active = false;
static uint32_t g_rgb_hex_color = 0x3366FF;

static int rgb_ui_slot_for_page(int page_index) {
    if (page_index == PAGE_RGB_CONTROL) {
        return 0;
    }
    if (page_index == PAGE_RGB_LED) {
        return 1;
    }
    return -1;
}

static void rgb_led_apply_color(uint32_t hex_color) {
    const uint8_t red = static_cast<uint8_t>((hex_color >> 16) & 0xFF);
    const uint8_t green = static_cast<uint8_t>((hex_color >> 8) & 0xFF);
    const uint8_t blue = static_cast<uint8_t>(hex_color & 0xFF);

    const uint8_t red_scaled = static_cast<uint8_t>((red * RGB_LED_BRIGHTNESS) / 255);
    const uint8_t green_scaled = static_cast<uint8_t>((green * RGB_LED_BRIGHTNESS) / 255);
    const uint8_t blue_scaled = static_cast<uint8_t>((blue * RGB_LED_BRIGHTNESS) / 255);

    neopixelWrite(RGB_LED_PIN, red_scaled, green_scaled, blue_scaled);
}

static void rgb_led_turn_off(void) {
    neopixelWrite(RGB_LED_PIN, 0, 0, 0);
}

static bool rgb_control_page_get_widgets(int page_index,
                                         lv_obj_t** hint_label,
                                         lv_obj_t** state_label,
                                         lv_obj_t** value_label,
                                         lv_obj_t** color_preview) {
    const int slot = rgb_ui_slot_for_page(page_index);
    if (slot < 0) {
        return false;
    }

    if (hint_label != nullptr) {
        *hint_label = g_hint_labels[slot];
    }
    if (state_label != nullptr) {
        *state_label = g_state_labels[slot];
    }
    if (value_label != nullptr) {
        *value_label = g_value_labels[slot];
    }
    if (color_preview != nullptr) {
        *color_preview = g_color_previews[slot];
    }
    return true;
}

static void rgb_control_page_refresh_backlight(void) {
    lv_obj_t* hint_label = nullptr;
    lv_obj_t* state_label = nullptr;
    lv_obj_t* value_label = nullptr;
    lv_obj_t* color_preview = nullptr;

    if (!rgb_control_page_get_widgets(PAGE_RGB_CONTROL, &hint_label, &state_label,
                                      &value_label, &color_preview)) {
        return;
    }
    if (hint_label == nullptr || state_label == nullptr ||
        value_label == nullptr || color_preview == nullptr) {
        return;
    }

    lv_obj_add_flag(color_preview, LV_OBJ_FLAG_HIDDEN);

    if (!g_backlight_control_active) {
        lv_label_set_text(hint_label, "Double click to start");
        lv_label_set_text(state_label, "Status: standby");
        lv_label_set_text(value_label, "");
        return;
    }

    lv_label_set_text(hint_label, "Single click to switch\nTriple click to return");
    lv_label_set_text(state_label, "Status: active");

    char value_buf[24];
    snprintf(value_buf, sizeof(value_buf), "%u%%", g_backlight_levels[g_backlight_index]);
    lv_label_set_text(value_label, value_buf);
}

static void rgb_control_page_refresh_rgb(void) {
    lv_obj_t* hint_label = nullptr;
    lv_obj_t* state_label = nullptr;
    lv_obj_t* value_label = nullptr;
    lv_obj_t* color_preview = nullptr;

    if (!rgb_control_page_get_widgets(PAGE_RGB_LED, &hint_label, &state_label,
                                      &value_label, &color_preview)) {
        return;
    }
    if (hint_label == nullptr || state_label == nullptr ||
        value_label == nullptr || color_preview == nullptr) {
        return;
    }

    if (!g_rgb_control_active) {
        lv_obj_add_flag(color_preview, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(hint_label, "Double click to start");
        lv_label_set_text(state_label, "Status: off");
        lv_label_set_text(value_label, "");
        return;
    }

    lv_label_set_text(hint_label, "Single click random color\nTriple click to return");
    lv_label_set_text(state_label, "Status: on");

    char value_buf[24];
    snprintf(value_buf, sizeof(value_buf), "#%06lX", static_cast<unsigned long>(g_rgb_hex_color));
    lv_label_set_text(value_label, value_buf);

    lv_obj_clear_flag(color_preview, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(color_preview, lv_color_hex(g_rgb_hex_color), 0);
    lv_obj_set_style_bg_opa(color_preview, LV_OPA_COVER, 0);
}

static void rgb_control_page_refresh(int page_index) {
    if (page_index == PAGE_RGB_LED) {
        rgb_control_page_refresh_rgb();
        return;
    }

    rgb_control_page_refresh_backlight();
}

void rgb_control_page_setup_mode(lv_obj_t* parent, int page_index) {
    if (parent == nullptr) {
        UI_LOGW("rgb_control_page_setup_mode: parent is null");
        return;
    }

    const int slot = rgb_ui_slot_for_page(page_index);
    if (slot < 0) {
        UI_LOGW("rgb_control_page_setup_mode: invalid page index %d", page_index);
        return;
    }

    // Create one independent widget set for each page so both pages can render correctly.
    lv_obj_set_style_bg_color(parent, UI_BG_COLOR, 0);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    g_hint_labels[slot] = lv_label_create(parent);
    lv_label_set_text(g_hint_labels[slot], "");
    lv_obj_set_style_text_color(g_hint_labels[slot], UI_TEXT_COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(g_hint_labels[slot], &ui_font_text, 0);
    lv_obj_set_style_text_align(g_hint_labels[slot], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_hint_labels[slot], LV_ALIGN_TOP_MID, 0, 44);

    g_color_previews[slot] = lv_obj_create(parent);
    lv_obj_set_size(g_color_previews[slot], ui_screen_width - 64, 88);
    lv_obj_align(g_color_previews[slot], LV_ALIGN_CENTER, 0, -12);
    lv_obj_set_style_radius(g_color_previews[slot], 18, 0);
    lv_obj_set_style_border_width(g_color_previews[slot], 2, 0);
    lv_obj_set_style_border_color(g_color_previews[slot], UI_BORDER_COLOR, 0);
    lv_obj_set_style_border_opa(g_color_previews[slot], LV_OPA_40, 0);
    lv_obj_set_style_pad_all(g_color_previews[slot], 0, 0);
    lv_obj_set_scrollbar_mode(g_color_previews[slot], LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(g_color_previews[slot], LV_DIR_NONE);
    lv_obj_clear_flag(g_color_previews[slot], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_color_previews[slot], LV_OBJ_FLAG_HIDDEN);

    g_state_labels[slot] = lv_label_create(parent);
    lv_label_set_text(g_state_labels[slot], "");
    lv_obj_set_style_text_color(g_state_labels[slot], UI_TEXT_COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(g_state_labels[slot], &ui_font_text, 0);
    lv_obj_set_style_text_align(g_state_labels[slot], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_state_labels[slot], LV_ALIGN_CENTER, 0, 60);

    g_value_labels[slot] = lv_label_create(parent);
    lv_label_set_text(g_value_labels[slot], "");
    lv_obj_set_style_text_color(g_value_labels[slot], UI_TEXT_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(g_value_labels[slot], &ui_font_text, 0);
    lv_obj_set_style_text_align(g_value_labels[slot], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_value_labels[slot], LV_ALIGN_BOTTOM_MID, 0, -12);

    if (page_index == PAGE_RGB_CONTROL) {
        rgb_led_turn_off();
    }

    rgb_control_page_refresh(page_index);
}

void rgb_control_page_setup(lv_obj_t* parent) {
    rgb_control_page_setup_mode(parent, PAGE_RGB_CONTROL);
    UI_LOGI("RGB control page setup complete");
}

void rgb_control_page_on_enter(int page_index) {
    if (page_index == PAGE_RGB_LED) {
        // Entering the RGB page always starts from a known "off" state.
        g_rgb_control_active = false;
        rgb_led_turn_off();
    } else {
        g_backlight_control_active = false;
    }

    rgb_control_page_refresh(page_index);
}

void rgb_control_page_handle_single_click(int page_index) {
    if (page_index == PAGE_RGB_LED) {
        if (!g_rgb_control_active) {
            return;
        }

        g_rgb_hex_color = static_cast<uint32_t>(random(0x1000000UL));
        rgb_led_apply_color(g_rgb_hex_color);
        rgb_control_page_refresh(page_index);
        return;
    }

    if (!g_backlight_control_active) {
        return;
    }

    g_backlight_index = (g_backlight_index + 1) %
                        (sizeof(g_backlight_levels) / sizeof(g_backlight_levels[0]));
    rgb_control_page_refresh(page_index);
}

void rgb_control_page_handle_double_click(int page_index) {
    if (page_index == PAGE_RGB_LED) {
        g_rgb_control_active = true;
        g_rgb_hex_color = static_cast<uint32_t>(random(0x1000000UL));
        rgb_led_apply_color(g_rgb_hex_color);
        rgb_control_page_refresh(page_index);
        return;
    }

    g_backlight_control_active = true;
    rgb_control_page_refresh(page_index);
}

void rgb_control_page_turn_off_rgb_led(void) {
    g_rgb_control_active = false;
    rgb_led_turn_off();
    rgb_control_page_refresh(PAGE_RGB_LED);
}

uint8_t rgb_control_page_get_backlight(void) {
    return g_backlight_levels[g_backlight_index];
}
