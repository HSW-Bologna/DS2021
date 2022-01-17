#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/common.h"
#include "view/intl/intl.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"


enum {
    BACK_BTN_ID,
    ACCESS_LEVEL_DD_ID,
};


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return NULL;
}


static void open_page(model_t *pmodel, void *arg) {
    (void)arg;

    view_common_create_title(lv_scr_act(), view_intl_get_string(pmodel, STRINGS_IMPOSTAZIONI), BACK_BTN_ID);

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(lbl, "%s:", view_intl_get_string(pmodel, STRINGS_LIVELLO_DI_ACCESSO));
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 100);

    lv_obj_t *dd          = lv_dropdown_create(lv_scr_act());
    char      string[256] = {0};
    for (size_t i = 0; i < NUM_ACCESS_LEVELS; i++) {
        strcat(string, parameters_livello_accesso[i][model_get_language(pmodel)]);
        if (i < NUM_ACCESS_LEVELS - 1) {
            strcat(string, "\n");
        }
    }
    lv_dropdown_set_options(dd, string);
    lv_dropdown_set_selected(dd, model_get_access_level_code(pmodel));
    view_register_object_default_callback(dd, ACCESS_LEVEL_DD_ID);
    lv_obj_align_to(dd, lbl, LV_ALIGN_OUT_RIGHT_MID, 0, 8);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t msg = VIEW_NULL_MESSAGE;

    switch (event.code) {
        case VIEW_EVENT_CODE_TIMER:
            break;

        case VIEW_EVENT_CODE_LVGL:
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;
                }
            } else if (event.event == LV_EVENT_VALUE_CHANGED) {
                switch (event.data.id) {
                    case ACCESS_LEVEL_DD_ID:
                        model_mark_parmac_to_save(pmodel);
                        model_set_access_level_code(pmodel, event.value);
                        break;
                }
            }
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_tech_settings = {
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};