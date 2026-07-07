/**
 * @file page_manager.cpp
 * @brief Page Manager Implementation
 */

#include "page_manager.h"

#include "menu.h"
#include "rgb_control_page.h"
#include "watchface.h"
#include "weather_page.h"

// Pages are placed side by side inside one wide container.
// Switching pages only moves the container on the X axis.

static page_manager_t* g_pm = nullptr;

static void menu_click_handler(int page_index) {
    if (g_pm == nullptr) {
        return;
    }
    page_manager_switch_to(g_pm, page_index, true);
}

bool page_manager_init(page_manager_t* pm) {
    if (pm == nullptr) {
        UI_LOGW("page_manager_init: pm is null");
        return false;
    }

    g_pm = pm;
    pm->current_page = PAGE_WATCHFACE;

    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(screen, LV_DIR_NONE);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // A single horizontal container keeps page switching simple and lightweight.
    pm->container = lv_obj_create(screen);
    lv_obj_set_size(pm->container, ui_screen_width * PAGE_COUNT, ui_screen_height);
    lv_obj_set_pos(pm->container, 0, 0);
    lv_obj_set_style_bg_color(pm->container, UI_BG_COLOR, 0);
    lv_obj_set_style_border_width(pm->container, 0, 0);
    lv_obj_set_style_pad_all(pm->container, 0, 0);
    lv_obj_set_style_radius(pm->container, 0, 0);
    lv_obj_set_scrollbar_mode(pm->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(pm->container, LV_DIR_NONE);
    lv_obj_clear_flag(pm->container, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < PAGE_COUNT; i++) {
        pm->pages[i] = lv_obj_create(pm->container);
        lv_obj_set_size(pm->pages[i], ui_screen_width, ui_screen_height);
        lv_obj_set_pos(pm->pages[i], i * ui_screen_width, 0);
        lv_obj_set_style_bg_color(pm->pages[i], UI_BG_COLOR, 0);
        lv_obj_set_style_border_width(pm->pages[i], 0, 0);
        lv_obj_set_style_pad_all(pm->pages[i], 0, 0);
        lv_obj_set_style_radius(pm->pages[i], 0, 0);
        lv_obj_set_scrollbar_mode(pm->pages[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(pm->pages[i], LV_DIR_NONE);
        lv_obj_clear_flag(pm->pages[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    watchface_setup(pm->pages[PAGE_WATCHFACE]);
    menu_setup(pm->pages[PAGE_MENU], menu_click_handler);
    weather_page_setup(pm->pages[PAGE_WEATHER]);
    rgb_control_page_setup(pm->pages[PAGE_RGB_CONTROL]);
    rgb_control_page_setup_mode(pm->pages[PAGE_RGB_LED], PAGE_RGB_LED);

    page_manager_switch_to(pm, PAGE_WATCHFACE, false);
    UI_LOGI("Page manager init complete, default page: WATCHFACE");
    return true;
}

void page_manager_switch_to(page_manager_t* pm, int page_index, bool anim) {
    if (pm == nullptr || pm->container == nullptr) {
        return;
    }
    if (page_index < 0 || page_index >= PAGE_COUNT) {
        UI_LOGW("Invalid page index: %d", page_index);
        return;
    }

    const int previous_page = pm->current_page;
    pm->current_page = page_index;
    lv_obj_set_x(pm->container, -(page_index * ui_screen_width));

    if (page_index == PAGE_MENU && previous_page == PAGE_WATCHFACE) {
        menu_set_selected(0);
    }

    if (page_index == PAGE_RGB_CONTROL || page_index == PAGE_RGB_LED) {
        rgb_control_page_on_enter(page_index);
    }

    LV_UNUSED(anim);
    UI_LOGI("Switched to page %d", page_index);
}

int page_manager_get_current(page_manager_t* pm) {
    if (pm == nullptr) {
        return PAGE_WATCHFACE;
    }
    return pm->current_page;
}

lv_obj_t* page_manager_get_page(page_manager_t* pm, int page_index) {
    if (pm == nullptr || page_index < 0 || page_index >= PAGE_COUNT) {
        return nullptr;
    }
    return pm->pages[page_index];
}

void page_manager_next_from_watchface(page_manager_t* pm) {
    if (pm == nullptr) {
        return;
    }
    page_manager_switch_to(pm, PAGE_MENU, true);
}

void page_manager_back_to_watchface(page_manager_t* pm) {
    if (pm == nullptr) {
        return;
    }
    page_manager_switch_to(pm, PAGE_WATCHFACE, true);
}
