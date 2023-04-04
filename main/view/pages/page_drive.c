#if 0
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
};


struct page_data {
    lv_obj_t *blanket;
    lv_obj_t *import_btn;
    lv_obj_t *list;
    size_t    selected_archive;
};


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
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
}


static void open_page(model_t *pmodel, void *arg) {
    struct page_data *data = arg;

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

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_thumb_drive);
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -80, 0);

    update_buttons(data, pmodel);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_DRIVE:
            if (!model_is_drive_mounted(pmodel)) {
                msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
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

            if (event.error) {
                view_common_io_error_toast(pmodel) ;
            }
            break;

        case VIEW_EVENT_CODE_TIMER:
            break;

        case VIEW_EVENT_CODE_LVGL:
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;

                    case LIST_ITEM_ID:
                        view_common_select_btn_in_list(data->list, event.data.number);
                        data->selected_archive = event.data.number;
                        break;

                    case CANCEL_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        break;

                    case IMPORT_CONFIRM_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_IMPORT_CURRENT_MACHINE;
                        strcpy(msg.cmsg.name, pmodel->system.drive_machines[data->selected_archive]);
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        data->blanket = view_common_create_blanket(lv_scr_act());
                        break;

                    case OVERWRITE_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                        }
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXPORT_CURRENT_MACHINE;
                        data->blanket = view_common_create_blanket(lv_scr_act());
                        break;

                    case EXPORT_BTN_ID:
                        if (model_is_current_archive_in_drive(pmodel)) {
                            lv_obj_t *popup = view_common_create_choice_popup(
                                lv_scr_act(), 1, view_intl_get_string(pmodel, STRINGS_CONFERMA_ESPORTA), LV_SYMBOL_OK,
                                OVERWRITE_BTN_ID, LV_SYMBOL_CLOSE, CANCEL_BTN_ID);
                            data->blanket = lv_obj_get_parent(popup);
                        } else {
                            msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXPORT_CURRENT_MACHINE;
                            data->blanket = view_common_create_blanket(lv_scr_act());
                        }
                        break;

                    case IMPORT_BTN_ID: {
                        lv_obj_t *popup =
                            create_list_choice_popup(data, view_intl_get_string(pmodel, STRINGS_SCEGLI_CONFIGURAZIONI),
                                                     pmodel->system.drive_machines, pmodel->system.num_drive_machines);
                        data->blanket = lv_obj_get_parent(popup);
                        break;
                    }
                }
            } else if (event.event == LV_EVENT_VALUE_CHANGED) {
            } else if (event.event == LV_EVENT_CANCEL) {
            } else if (event.event == LV_EVENT_READY) {
            }
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_drive = {
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
#endif