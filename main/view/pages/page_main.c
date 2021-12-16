#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "view/common.h"
#include "gel/pagemanager/page_manager.h"
#include "view/widgets/custom_tabview.h"
#include "view/intl/intl.h"


LV_IMG_DECLARE(img_gears);
LV_IMG_DECLARE(img_gears_disabled);
LV_IMG_DECLARE(img_gears_press);


enum {
    SETTINGS_BTN_ID,
    RETRY_COMM_BTN_ID,
    DISABLE_COMM_BTN_ID,
    START_BTN_ID,
    STOP_BTN_ID,
};


struct page_data {
    lv_obj_t *blanket;

    lv_obj_t *lbl_state;
};


static void update_state(model_t *pmodel, struct page_data *data) {
    switch (model_get_machine_state(pmodel)) {
        case MACHINE_STATE_STOPPED:
            lv_label_set_text(data->lbl_state, "stop");
            break;
        case MACHINE_STATE_RUNNING:
            lv_label_set_text(data->lbl_state, "run");
            break;
    }
}


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return (struct page_data *)malloc(sizeof(struct page_data));
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;
    data->blanket          = NULL;

    lv_obj_t *btn = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &img_gears, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, NULL, &img_gears_press, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_DISABLED, NULL, &img_gears_disabled, NULL);
    lv_obj_set_width(btn, img_gears.header.w);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    view_register_object_default_callback(btn, SETTINGS_BTN_ID);

    btn           = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "start");
    view_register_object_default_callback(btn, START_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_CENTER, -200, 0);

    btn = lv_btn_create(lv_scr_act());
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "stop");
    view_register_object_default_callback(btn, STOP_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_CENTER, 00, 0);

    lbl             = lv_label_create(lv_scr_act());
    data->lbl_state = lbl;

    update_state(pmodel, data);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case SETTINGS_BTN_ID: {
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE;
                        msg.vmsg.page = (void *)&page_settings;
                        break;
                    }

                    case RETRY_COMM_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_RESTART_COMMUNICATION;
                        break;

                    case DISABLE_COMM_BTN_ID:
                        model_set_machine_communication(pmodel, 0);
                        lv_obj_del(data->blanket);
                        data->blanket = NULL;
                        break;

                    case START_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_START_MACHINE;
                        break;

                    case STOP_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_STOP_MACHINE;
                        break;
                }
            }

            break;
        }

        case VIEW_EVENT_CODE_STATE_CHANGED:
            update_state(pmodel, data);
            break;

        case VIEW_EVENT_CODE_OPEN:
        case VIEW_EVENT_CODE_ALARM:
            if (!model_is_machine_communication_ok(pmodel) && data->blanket == NULL) {
                lv_obj_t *popup = view_common_create_choice_popup(
                    lv_scr_act(), 1, view_intl_get_string(pmodel, STRINGS_ERRORE_DI_COMUNICAZIONE),
                    view_intl_get_string(pmodel, STRINGS_RIPROVA), RETRY_COMM_BTN_ID,
                    view_intl_get_string(pmodel, STRINGS_DISABILITA), DISABLE_COMM_BTN_ID);
                data->blanket = lv_obj_get_parent(popup);
            } else if (data->blanket != NULL) {
                lv_obj_del(data->blanket);
                data->blanket = NULL;
            }
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_main = {
    .create        = create_page,
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};