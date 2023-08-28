#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "view/intl/intl.h"
#include "view/common.h"
#include "model/model.h"


enum {
    BACK_BTN_ID,
};


struct page_data {
    char *logdata;
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    struct page_data *data = (struct page_data *)malloc(sizeof(struct page_data));
    data->logdata          = extra;
    return data;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *data = state;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    view_common_create_title(lv_scr_act(), view_intl_get_string(pmodel, STRINGS_VERBALE), BACK_BTN_ID);

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(90));
    lv_obj_align(page, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *label = lv_label_create(page);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(95));

    if (data->logdata != NULL) {
        lv_label_set_text(label, data->logdata);
    }

    lv_obj_t *obj = lv_label_create(page);
    lv_label_set_text(obj, "");
    lv_obj_align_to(obj, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_scroll_to_view(obj, 0);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = state;
    (void)data;
    (void)handle;

    switch (event.tag) {
        case PMAN_EVENT_TAG_LVGL:
            if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CLICKED) {
                lv_obj_t           *target  = lv_event_get_target(event.as.lvgl);
                view_object_data_t *objdata = lv_obj_get_user_data(target);

                switch (objdata->id) {
                    case BACK_BTN_ID:
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        break;
                }
            }
            break;

        default:
            break;
    }

    return msg;
}


const pman_page_t page_log = {
    .create        = create_page,
    .open          = open_page,
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
};
