#if 0
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


static void *create_page(model_t *model, void *extra) {
    struct page_data *data = (struct page_data *)malloc(sizeof(struct page_data));
    data->logdata          = extra;
    return data;
}


static void open_page(model_t *pmodel, void *arg) {
    struct page_data *data = arg;
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


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;
    (void)data;

    switch (event.code) {
        case VIEW_EVENT_CODE_LVGL:
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
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
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
};
#endif