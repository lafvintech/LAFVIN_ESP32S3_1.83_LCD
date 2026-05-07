/**
 * @file page_manager.h
 * @brief Page Manager Header File
 */

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "ui_common.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* pages[PAGE_COUNT];
    int current_page;
} page_manager_t;

bool page_manager_init(page_manager_t* pm);
void page_manager_switch_to(page_manager_t* pm, int page_index, bool anim);
int page_manager_get_current(page_manager_t* pm);
lv_obj_t* page_manager_get_page(page_manager_t* pm, int page_index);
void page_manager_next_from_watchface(page_manager_t* pm);
void page_manager_back_to_watchface(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif // PAGE_MANAGER_H
