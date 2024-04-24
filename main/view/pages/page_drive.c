#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/common.h"
#include "view/intl/intl.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"


LV_IMG_DECLARE(img_thumb_drive);


enum {
    BACK_BTN_ID,
    EXPORT_BTN_ID,
    IMPORT_BTN_ID,
    OVERWRITE_BTN_ID,
    IMPORT_CONFIRM_BTN_ID,
    CANCEL_BTN_ID,
    LIST_ITEM_ID,
    BTN_FIRMWARE_UPDATE_ID,
    FIRMWARE_CONFIRM_BTN_ID,
};


struct page_data {
    lv_obj_t *blanket;
    lv_obj_t *import_btn;
    lv_obj_t *btn_update;
    lv_obj_t *list;
    size_t    selected_archive;

    view_controller_message_t cmsg;
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    return malloc(sizeof(struct page_data));
}


static lv_obj_t *create_list_choice_popup(struct page_data *data, const char *text, name_t *options, size_t num) {
    lv_obj_t *popup = view_common_create_popup(lv_scr_act(), 1);

    lv_obj_t *btn = lv_btn_create(popup);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_OK);
    lv_obj_set_width(btn, 120);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    view_register_object_default_callback(btn, IMPORT_CONFIRM_BTN_ID);

    btn = lv_btn_create(popup);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_width(btn, 120);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    view_register_object_default_callback(btn, CANCEL_BTN_ID);

    lbl = lv_label_create(popup);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(90));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);
    lv_label_set_text(lbl, text);

    lv_obj_t *list = lv_list_create(popup);
    for (size_t i = 0; i < num; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL, options[i]);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(btn), LV_STATE_CHECKED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        view_register_object_default_callback_with_number(btn, LIST_ITEM_ID, i);
    }
    lv_obj_set_size(list, LV_PCT(80), LV_PCT(50));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    data->list = list;

    return popup;
}


static void update_buttons(struct page_data *data, model_t *pmodel) {
    if (pmodel->system.num_drive_machines > 0) {
        lv_obj_clear_state(data->import_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(data->import_btn, LV_STATE_DISABLED);
    }

    if (pmodel->system.firmware_update_ready > 0) {
        lv_obj_clear_state(data->btn_update, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(data->btn_update, LV_STATE_DISABLED);
    }
}


static void open_page(pman_handle_t handle, void *arg) {
    struct page_data *data = arg;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    view_common_create_title(lv_scr_act(), view_intl_get_string(pmodel, STRINGS_GESTIONE_CHIAVETTA_ESTERNA),
                             BACK_BTN_ID);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_ESPORTA_MACCHINA_SU_CHIAVETTA));
    lv_obj_set_size(btn, 420, 100);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 4, 100);
    view_register_object_default_callback(btn, EXPORT_BTN_ID);
    lv_obj_t *prev = btn;

    btn = lv_btn_create(lv_scr_act());
    lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_IMPORTA_MACCHINA_DA_CHIAVETTA));
    lv_obj_set_size(btn, 420, 100);
    lv_obj_align_to(btn, prev, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    view_register_object_default_callback(btn, IMPORT_BTN_ID);
    data->import_btn = btn;
    prev             = btn;

    btn = lv_btn_create(lv_scr_act());
    lbl = lv_label_create(btn);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_AGGIORNA_FIRMWARE));
    lv_obj_set_size(btn, 420, 100);
    lv_obj_align_to(btn, prev, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    view_register_object_default_callback(btn, BTN_FIRMWARE_UPDATE_ID);
    data->btn_update = btn;

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_thumb_drive);
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -40, 0);

    update_buttons(data, pmodel);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *arg, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = arg;

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *user_event = event.as.user;
            switch (user_event->code) {
                case VIEW_EVENT_CODE_DRIVE:
                    if (!model_is_drive_mounted(pmodel)) {
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                    } else {
                        update_buttons(data, pmodel);
                    }
                    break;

                case VIEW_EVENT_CODE_IO_DONE:
                    if (data->blanket != NULL) {
                        lv_obj_del(data->blanket);
                    }
                    data->blanket = NULL;
                    update_buttons(data, pmodel);

                    if (user_event->error) {
                        view_common_io_error_toast(pmodel);
                    }
                    break;

                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target  = lv_event_get_current_target(event.as.lvgl);
            view_object_data_t *objdata = lv_obj_get_user_data(target);

            if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CLICKED) {
                switch (objdata->id) {
                    case BACK_BTN_ID:
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        break;

                    case LIST_ITEM_ID:
                        view_common_select_btn_in_list(data->list, objdata->number);
                        data->selected_archive = objdata->number;
                        break;

                    case CANCEL_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        break;

                    case IMPORT_CONFIRM_BTN_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_IMPORT_CURRENT_MACHINE;
                        strcpy(data->cmsg.name, pmodel->system.drive_machines[data->selected_archive]);
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        data->blanket = view_common_create_blanket(lv_scr_act());
                        break;

                    case OVERWRITE_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXPORT_CURRENT_MACHINE;
                        data->blanket   = view_common_create_blanket(lv_scr_act());
                        break;

                    case EXPORT_BTN_ID:
                        if (model_is_current_archive_in_drive(pmodel)) {
                            lv_obj_t *popup = view_common_create_choice_popup(
                                lv_scr_act(), 1, view_intl_get_string(pmodel, STRINGS_CONFERMA_ESPORTA), LV_SYMBOL_OK,
                                OVERWRITE_BTN_ID, LV_SYMBOL_CLOSE, CANCEL_BTN_ID);
                            data->blanket = lv_obj_get_parent(popup);
                        } else {
                            data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXPORT_CURRENT_MACHINE;
                            data->blanket   = view_common_create_blanket(lv_scr_act());
                        }
                        break;

                    case IMPORT_BTN_ID: {
                        lv_obj_t *popup =
                            create_list_choice_popup(data, view_intl_get_string(pmodel, STRINGS_SCEGLI_CONFIGURAZIONI),
                                                     pmodel->system.drive_machines, pmodel->system.num_drive_machines);
                        data->blanket = lv_obj_get_parent(popup);
                        break;
                    }

                    case BTN_FIRMWARE_UPDATE_ID: {
                        lv_obj_t *popup = view_common_create_choice_popup(
                            lv_scr_act(), 1, view_intl_get_string(pmodel, STRINGS_AGGIORNARE_FIRMWARE), LV_SYMBOL_OK,
                            FIRMWARE_CONFIRM_BTN_ID, LV_SYMBOL_CLOSE, CANCEL_BTN_ID);
                        data->blanket = lv_obj_get_parent(popup);
                        break;
                    }

                    case FIRMWARE_CONFIRM_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_FIRMWARE_UPDATE;
                        data->blanket   = view_common_create_blanket(lv_scr_act());
                        break;
                }
            }
            break;
        }

        default:
            break;
    }

    return msg;
}



const pman_page_t page_drive = {
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
