#ifndef CUSTOM_TABVIEW_H_INCLUDED
#define CUSTOM_TABVIEW_H_INCLUDED


#include "lvgl.h"


lv_obj_t *custom_tabview_create(lv_obj_t *parent, lv_dir_t tab_pos, lv_coord_t tab_size);
lv_obj_t *custom_tabview_add_tab(lv_obj_t *obj, const char *name);
lv_obj_t *custom_tabview_get_content(lv_obj_t *tv);
void      custom_tabview_set_act(lv_obj_t *obj, uint32_t id, lv_anim_enable_t anim_en);
lv_obj_t *custom_tabview_get_tab_btns(lv_obj_t *tv);

extern const lv_obj_class_t custom_tabview_class;


#endif