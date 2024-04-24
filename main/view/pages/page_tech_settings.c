#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/common.h"
#include "view/intl/intl.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"


enum {
    BACK_BTN_ID,
    ACCESS_LEVEL_DD_ID,
    NAME_TA_ID,
    KEYBOARD_ID,
    COM_ENABLE_ID,
};


struct page_data {
    lv_obj_t *kb;
    lv_obj_t *com_btn;
    lv_obj_t *com_led;

    view_controller_message_t cmsg;
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    return malloc(sizeof(struct page_data));
}


static void update_communication(model_t *pmodel, struct page_data *data) {
    if (model_is_machine_communication_active(pmodel)) {
        lv_obj_add_state(data->com_btn, LV_STATE_CHECKED);
        lv_led_set_brightness(data->com_led, LV_LED_BRIGHT_MAX);
    } else {
        lv_obj_clear_state(data->com_btn, LV_STATE_CHECKED);
        lv_led_set_brightness(data->com_led, LV_LED_BRIGHT_MIN);
    }
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *data = state;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

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
    lv_obj_align_to(dd, lbl, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_MODELLO_MACCHINA_IN_USO));
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, LV_HOR_RES / 2 + 4, 100);

    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
    lv_obj_align_to(ta, lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_textarea_set_placeholder_text(ta, view_intl_get_string(pmodel, STRINGS_MODELLO_MACCHINA));
    lv_textarea_set_text(ta, pmodel->configuration.parmac.nome);
    lv_textarea_set_max_length(ta, 32);
    lv_textarea_set_one_line(ta, 1);
    lv_obj_set_size(ta, LV_PCT(48), 56);
    view_register_object_default_callback(ta, NAME_TA_ID);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 280, 100);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    view_register_object_default_callback(btn, COM_ENABLE_ID);
    lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(80));
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_COMUNICAZIONE_MACCHINA));
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *led = lv_led_create(btn);
    lv_obj_align(led, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 4, 200);
    data->com_btn = btn;
    data->com_led = led;

    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    view_register_object_default_callback(kb, KEYBOARD_ID);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    data->kb = kb;

    update_communication(pmodel, data);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = state;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    switch (event.tag) {
        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target  = lv_event_get_current_target(event.as.lvgl);
            view_object_data_t *objdata = lv_obj_get_user_data(target);

            if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CLICKED) {
                switch (objdata->id) {
                    case BACK_BTN_ID:
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        break;

                    case NAME_TA_ID:
                        lv_obj_clear_flag(data->kb, LV_OBJ_FLAG_HIDDEN);
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_VALUE_CHANGED) {
                switch (objdata->id) {
                    case ACCESS_LEVEL_DD_ID:
                        model_mark_parmac_to_save(pmodel);
                        model_set_access_level_code(pmodel, lv_dropdown_get_selected(target));
                        break;

                    case COM_ENABLE_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_TOGGLE_COMMUNICATION;
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CANCEL) {
                switch (objdata->id) {
                    case KEYBOARD_ID:
                        lv_obj_add_flag(data->kb, LV_OBJ_FLAG_HIDDEN);
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_READY) {
                switch (objdata->id) {
                    case KEYBOARD_ID: {
                        lv_obj_t *ta = lv_keyboard_get_textarea(target);
                        lv_obj_add_flag(data->kb, LV_OBJ_FLAG_HIDDEN);
                        strcpy(pmodel->configuration.parmac.nome, lv_textarea_get_text(ta));
                        model_mark_parmac_to_save(pmodel);
                        break;
                    }
                }
            }
            break;
        }

        case PMAN_EVENT_TAG_USER:
            update_communication(pmodel, data);
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_tech_settings = {
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
