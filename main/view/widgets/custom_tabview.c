#include "lvgl.h"


static void constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void btns_value_changed_event_cb(lv_event_t *e);
static void cont_scroll_end_event_cb(lv_event_t *e);


static lv_dir_t   tabpos_create;
static lv_coord_t tabsize_create;


lv_obj_t *view_custom_tabview_create(lv_obj_t *parent, lv_dir_t tab_pos, lv_coord_t tab_size) {
    lv_obj_class_t custom_tabview_class = lv_tabview_class;
    custom_tabview_class.constructor_cb = constructor;

    tabpos_create  = tab_pos;
    tabsize_create = tab_size;

    lv_obj_t *obj = lv_obj_class_create_obj(&custom_tabview_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}



static void constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_tabview_t *tabview = (lv_tabview_t *)obj;

    tabview->tab_pos = tabpos_create;

    switch (tabview->tab_pos) {
        case LV_DIR_TOP:
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
            break;
        case LV_DIR_BOTTOM:
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN_REVERSE);
            break;
        case LV_DIR_LEFT:
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
            break;
        case LV_DIR_RIGHT:
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW_REVERSE);
            break;
    }

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));

    lv_obj_t *btnm;
    lv_obj_t *cont;

    btnm = lv_btnmatrix_create(obj);
    cont = lv_obj_create(obj);

    lv_btnmatrix_set_one_checked(btnm, true);
    tabview->map    = lv_mem_alloc(sizeof(const char *));
    tabview->map[0] = "";
    lv_btnmatrix_set_map(btnm, (const char **)tabview->map);
    lv_obj_add_event_cb(btnm, btns_value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(btnm, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_add_event_cb(cont, cont_scroll_end_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    switch (tabview->tab_pos) {
        case LV_DIR_TOP:
        case LV_DIR_BOTTOM:
            lv_obj_set_size(btnm, LV_PCT(100), tabsize_create);
            lv_obj_set_width(cont, LV_PCT(100));
            lv_obj_set_flex_grow(cont, 1);
            break;
        case LV_DIR_LEFT:
        case LV_DIR_RIGHT:
            lv_obj_set_size(btnm, tabsize_create, LV_PCT(100));
            lv_obj_set_height(cont, LV_PCT(100));
            lv_obj_set_flex_grow(cont, 1);
            break;
    }

    lv_group_t *g = lv_group_get_default();
    if (g)
        lv_group_add_obj(g, btnm);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_snap_x(cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
}


static void btns_value_changed_event_cb(lv_event_t *e) {
    lv_obj_t *btns = lv_event_get_target(e);

    lv_obj_t *tv = lv_obj_get_parent(btns);
    uint32_t  id = lv_btnmatrix_get_selected_btn(btns);
    lv_tabview_set_act(tv, id, LV_ANIM_ON);
}

static void cont_scroll_end_event_cb(lv_event_t *e) {
    lv_obj_t       *cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t *tv = lv_obj_get_parent(cont);
    if (code == LV_EVENT_LAYOUT_CHANGED) {
        lv_tabview_set_act(tv, lv_tabview_get_tab_act(tv), LV_ANIM_OFF);
    } else if (code == LV_EVENT_SCROLL_END) {
        lv_point_t p;
        lv_obj_get_scroll_end(cont, &p);

        lv_coord_t w = lv_obj_get_content_width(cont);
        lv_coord_t t;

        if (lv_obj_get_style_base_dir(tv, LV_PART_MAIN) == LV_BASE_DIR_RTL)
            t = -(p.x - w / 2) / w;
        else
            t = (p.x + w / 2) / w;

        if (t < 0)
            t = 0;
        bool new_tab = false;
        if (t != lv_tabview_get_tab_act(tv))
            new_tab = true;
        lv_tabview_set_act(tv, t, LV_ANIM_ON);

        if (new_tab)
            lv_event_send(tv, LV_EVENT_VALUE_CHANGED, NULL);
    }
}