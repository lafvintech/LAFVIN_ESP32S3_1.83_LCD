/**
 * @file menu.h
 * @brief Menu Page Header
 */

#ifndef MENU_H
#define MENU_H

#include "ui_common.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*menu_click_cb_t)(int page_index);

void menu_setup(lv_obj_t* parent, menu_click_cb_t click_cb);
void menu_set_selected(int index);
void menu_select_next(void);
void menu_select_prev(void);
int menu_get_selected(void);
void menu_activate_selected(void);

#ifdef __cplusplus
}
#endif

#endif // MENU_H
