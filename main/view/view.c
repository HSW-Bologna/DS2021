#include <stdlib.h>
#include "gel/collections/queue.h"
#include "gel/pagemanager/page_manager.h"
#include "config/app_conf.h"
#include "model/model.h"
#include "view.h"
#include "theme/style.h"
#include "theme/theme.h"
#include "widgets/custom_tabview.h"


static void event_callback(lv_event_t *event);
static void plus_minus_keyboard_cb(lv_event_t *event);


static lv_indev_t *indev;

static pman_t page_manager = {0};
pman_handle_t pman_handle  = &page_manager;


void view_init(model_updater_t updater, pman_user_msg_cb_t controller_cb,
               void (*flush_cb)(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p),
               void (*read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data)) {
    lv_init();

    /*A static or global variable to store the buffers*/
    static lv_disp_draw_buf_t disp_buf;

    /*Static or global buffer(s). The second buffer is optional*/
    static lv_color_t buf_1[DISPLAY_HORIZONTAL_RESOLUTION * 200];
    static lv_color_t buf_2[DISPLAY_HORIZONTAL_RESOLUTION * 200];

    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, DISPLAY_HORIZONTAL_RESOLUTION * 200);

    static lv_disp_drv_t disp_drv;                     /*A variable to hold the drivers. Must be static or global.*/
    lv_disp_drv_init(&disp_drv);                       /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf;                     /*Set an initialized buffer*/
    disp_drv.flush_cb = flush_cb;                      /*Set a flush callback to draw to the display*/
    disp_drv.hor_res  = DISPLAY_HORIZONTAL_RESOLUTION; /*Set the horizontal resolution in pixels*/
    disp_drv.ver_res  = DISPLAY_VERTICAL_RESOLUTION;   /*Set the vertical resolution in pixels*/

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
    theme_init(disp);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = read_cb;
    /*Register the driver in LVGL and save the created input device object*/
    indev = lv_indev_drv_register(&indev_drv);

    pman_init(&page_manager, updater, indev, controller_cb);
}


void view_change_page_extra(model_t *model, const pman_page_t *page, void *extra) {
    pman_change_page_extra(&page_manager, *page, extra);
    // event_queue_init(&q);     // Butta tutti gli eventi precedenti quando cambi la pagina
    // view_event((view_event_t){.code = VIEW_EVENT_CODE_OPEN});
    // lv_indev_reset(indev, NULL);
    // return pman_change_page_extra(&pman, model, *page, extra);
}


void view_change_page(model_t *model, const pman_page_t *page) {
    return view_change_page_extra(model, page, NULL);
}



void view_event(view_event_t event) {
    // event_queue_enqueue(&q, &event);
    pman_event(&page_manager, PMAN_USER_EVENT(&event));
}


void view_register_object_default_callback(lv_obj_t *obj, int id) {
    view_register_object_default_callback_with_number(obj, id, 0);
}


void view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number) {
    view_object_data_t *data = malloc(sizeof(view_object_data_t));
    data->id                 = id;
    data->number             = number;
    lv_obj_set_user_data(obj, data);

    pman_unregister_obj_event(&page_manager, obj);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_CLICKED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_VALUE_CHANGED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_RELEASED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_PRESSED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_PRESSING);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_LONG_PRESSED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_LONG_PRESSED_REPEAT);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_CANCEL);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_READY);
    pman_set_obj_self_destruct(obj);
}



void view_register_keyboard_plus_minus_callback(lv_obj_t *kb, int id) {
    view_object_data_t *data = malloc(sizeof(view_object_data_t));
    data->id                 = id;
    data->number             = 0;
    lv_obj_set_user_data(kb, data);
    lv_obj_remove_event_cb(kb, event_callback);
    lv_obj_remove_event_cb(kb, lv_keyboard_def_event_cb);
    pman_set_obj_self_destruct(kb);
    lv_obj_add_event_cb(kb, plus_minus_keyboard_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(kb, plus_minus_keyboard_cb, LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(kb, plus_minus_keyboard_cb, LV_EVENT_READY, NULL);
}


static void plus_minus_keyboard_cb(lv_event_t *event) {
    lv_obj_t           *obj     = lv_event_get_target(event);
    view_object_data_t *data    = lv_obj_get_user_data(lv_event_get_current_target(event));
    lv_obj_t           *ta      = lv_keyboard_get_textarea(obj);

    /* TODO:
    pman_event_t        myevent = {.code = VIEW_EVENT_CODE_LVGL, .event = lv_event_get_code(event), .data = *data};
    if (ta != NULL) {
        myevent.string_value = lv_textarea_get_text(ta);
    }

    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        myevent.event    = LV_EVENT_CLICKED;
        unsigned int btn = lv_btnmatrix_get_selected_btn(obj);
        if (btn == 7 || btn == 11) {
            if (btn == 7) {
                myevent.data.number = +1;
            } else {
                myevent.data.number = -1;
            }
        } else {
            lv_keyboard_def_event_cb(event);
        }
    }
    view_event(myevent);
    */
}


static void event_callback(lv_event_t *event) {
    view_object_data_t *data       = lv_obj_get_user_data(lv_event_get_current_target(event));
    view_event_t        pman_event = {.code = VIEW_EVENT_CODE_LVGL, .data = *data, .event = lv_event_get_code(event)};

    lv_obj_t *obj = lv_event_get_current_target(event);
    if (lv_obj_check_type(obj, &lv_btn_class)) {
        pman_event.value = lv_obj_has_state(obj, LV_STATE_CHECKED);
    } else if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        pman_event.value = lv_dropdown_get_selected(obj);
        lv_obj_t *list   = lv_dropdown_get_list(obj);
        if (list != NULL) {
            lv_obj_set_width(list, lv_obj_get_width(obj));
        }
    } else if (lv_obj_check_type(obj, &lv_switch_class)) {
        pman_event.value = lv_obj_has_state(obj, LV_STATE_CHECKED);
    } else if (lv_obj_check_type(obj, &lv_roller_class)) {
        pman_event.value = lv_roller_get_selected(obj);
    } else if (lv_obj_check_type(obj, &lv_textarea_class)) {
        pman_event.string_value = lv_textarea_get_text(obj);
    } else if (lv_obj_check_type(obj, &lv_slider_class)) {
        pman_event.value = lv_slider_get_value(obj);
    } else if (lv_obj_check_type(obj, &lv_keyboard_class)) {
        lv_obj_t *ta = lv_keyboard_get_textarea(obj);
        if (ta != NULL) {
            pman_event.string_value = lv_textarea_get_text(ta);
        }
    } else if (lv_obj_check_type(obj, &lv_btnmatrix_class) && lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        pman_event.value = lv_btnmatrix_get_selected_btn(obj);
    } else if ((lv_obj_check_type(obj, &lv_tabview_class) || lv_obj_check_type(obj, &custom_tabview_class)) &&
               lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        pman_event.value = lv_tabview_get_tab_act(obj);
    }

    view_event(pman_event);
}
