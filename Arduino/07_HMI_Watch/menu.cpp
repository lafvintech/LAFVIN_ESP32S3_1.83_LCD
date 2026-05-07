/**
 * @file menu.cpp
 * @brief Menu Page Implementation
 */

#include "menu.h"
#include "menu_icons.h"

struct MenuItem {
    const char* name;
    const lv_img_dsc_t* icon;
    int page_index;
};

static const MenuItem menu_items[] = {
    {"Watch", &menu_icon_clock, PAGE_WATCHFACE},
    {"Weather", &menu_icon_cloud_sun, PAGE_WEATHER},
    {"Backlight", &menu_icon_brightness, PAGE_RGB_CONTROL},
    {"RGB", &menu_icon_lightbulb, PAGE_RGB_LED},
};

static lv_obj_t* menu_item_objs_[PAGE_COUNT] = {nullptr};
static menu_click_cb_t g_click_cb = nullptr;
static int g_selected_index = 0;

static int menu_item_count() {
    return sizeof(menu_items) / sizeof(menu_items[0]);
}

static void menu_refresh_selection() {
    const int count = menu_item_count();
    for (int i = 0; i < count; ++i) {
        if (menu_item_objs_[i] == nullptr) {
            continue;
        }

        const bool selected = (i == g_selected_index);
        lv_obj_set_style_bg_opa(menu_item_objs_[i], selected ? LV_OPA_20 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(menu_item_objs_[i], selected ? UI_TEXT_COLOR_ACCENT : UI_BG_COLOR, 0);
        lv_obj_set_style_border_width(menu_item_objs_[i], selected ? 1 : 0, 0);
        lv_obj_set_style_border_color(menu_item_objs_[i], UI_TEXT_COLOR_ACCENT, 0);
        lv_obj_set_style_radius(menu_item_objs_[i], 14, 0);
    }
}

static void on_menu_item_clicked(lv_event_t* e) {
    const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
    menu_set_selected(static_cast<int>(index));
    menu_activate_selected();
}

void menu_setup(lv_obj_t* parent, menu_click_cb_t click_cb) {
    if (parent == nullptr) {
        UI_LOGW("menu_setup: parent is null");
        return;
    }

    g_click_cb = click_cb;
    g_selected_index = 0;

    lv_obj_set_style_bg_color(parent, UI_BG_COLOR, 0);

    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Menu");
    lv_obj_set_style_text_color(title, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(title, &ui_font_text, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_SPACING(6));

    lv_obj_t* grid_container = lv_obj_create(parent);
    lv_obj_set_size(grid_container, ui_screen_width - UI_SPACING(8), ui_screen_height - UI_SPACING(26));
    lv_obj_align(grid_container, LV_ALIGN_BOTTOM_MID, 0, -UI_SPACING(4));
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);
    lv_obj_set_style_pad_all(grid_container, UI_SPACING(4), 0);
    lv_obj_set_style_pad_row(grid_container, UI_SPACING(6), 0);
    lv_obj_set_style_pad_column(grid_container, UI_SPACING(6), 0);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(grid_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(grid_container, LV_DIR_NONE);
    lv_obj_clear_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);

    const int count = menu_item_count();
    const lv_coord_t item_w = (ui_screen_width - UI_SPACING(22)) / 2;
    const lv_coord_t item_h = (ui_screen_height - UI_SPACING(52)) / 2;

    for (int i = 0; i < count; ++i) {
        const MenuItem& item = menu_items[i];
        lv_obj_t* icon_container = lv_obj_create(grid_container);
        menu_item_objs_[i] = icon_container;
        lv_obj_set_size(icon_container, item_w, item_h);
        lv_obj_set_style_bg_opa(icon_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(icon_container, 0, 0);
        lv_obj_set_style_pad_all(icon_container, UI_SPACING(3), 0);
        lv_obj_set_style_shadow_width(icon_container, 0, 0);
        lv_obj_set_flex_flow(icon_container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(icon_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(icon_container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(icon_container, LV_DIR_NONE);
        lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(icon_container, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(icon_container, on_menu_item_clicked, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));

        lv_obj_t* icon_image = lv_img_create(icon_container);
        lv_img_set_src(icon_image, item.icon);
        lv_obj_set_style_pad_bottom(icon_image, UI_SPACING(1), 0);

        lv_obj_t* name_label = lv_label_create(icon_container);
        lv_label_set_text(name_label, item.name);
        lv_obj_set_style_text_color(name_label, UI_TEXT_COLOR, 0);
        lv_obj_set_style_text_font(name_label, &ui_font_text, 0);
    }

    menu_refresh_selection();
    UI_LOGI("Menu page setup complete (%d items)", count);
}

void menu_set_selected(int index) {
    const int count = menu_item_count();
    if (count <= 0) {
        return;
    }

    if (index < 0) {
        index = 0;
    } else if (index >= count) {
        index = count - 1;
    }

    g_selected_index = index;
    menu_refresh_selection();
}

void menu_select_next(void) {
    const int count = menu_item_count();
    if (count <= 0) {
        return;
    }
    menu_set_selected((g_selected_index + 1) % count);
}

void menu_select_prev(void) {
    const int count = menu_item_count();
    if (count <= 0) {
        return;
    }
    menu_set_selected((g_selected_index + count - 1) % count);
}

int menu_get_selected(void) {
    return g_selected_index;
}

void menu_activate_selected(void) {
    if (g_click_cb == nullptr) {
        return;
    }
    g_click_cb(menu_items[g_selected_index].page_index);
}
