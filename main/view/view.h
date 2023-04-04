#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED


#include "model/model.h"
#include "model/model_updater.h"
#include "gel/pagemanager/page_manager.h"
#include "lv_page_manager.h"


void view_init(model_updater_t updater, lv_pman_user_msg_cb_t controller_cb,
               void (*flush_cb)(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p),
               void (*read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data));

void        view_change_page(model_t *model, const lv_pman_page_t *page);
lv_timer_t *view_register_periodic_timer(size_t period, int code);
void        view_register_object_default_callback(lv_obj_t *obj, int id);
void        view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number);

void view_event(view_event_t event);
void view_register_keyboard_plus_minus_callback(lv_obj_t *kb, int id);


extern const lv_pman_page_t page_splash, page_main, page_settings;
extern const pman_page_t    page_test, page_program, page_step, page_network, page_log, page_tech_settings, page_drive,
    page_stats;

#endif
