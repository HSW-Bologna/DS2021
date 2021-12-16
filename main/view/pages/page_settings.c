#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "model/parmac.h"
#include "gel/pagemanager/page_manager.h"
#include "view/widgets/custom_tabview.h"
#include "view/theme/style.h"
#include "view/intl/intl.h"
#include "view/common.h"


enum {
    BACK_BTN_ID,
    PARAMETER_BTN_ID,
    TEST_BTN_ID,
    PARAMETER_CANCEL_BTN_ID,
    PARAMETER_DEFAULT_BTN_ID,
    PARAMETER_CONFIRM_BTN_ID,
    PARAMETER_DROPDOWN_ID,
    PARAMETER_ROLLER1_ID,
    PARAMETER_ROLLER2_ID,
    PARAMETER_TA_ID,
    PARAMETER_KB_ID,
    PARAMETER_SWITCH_ID,
};


struct page_data {
    struct {
        lv_obj_t *tab;
        lv_obj_t *list;
        lv_obj_t *editor;

        uint64_t           par_buffer;
        parameter_handle_t par;
        size_t             num;
    } parmac;
};


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return (struct page_data *)malloc(sizeof(struct page_data));
}


static void update_param_list(struct page_data *data, model_t *pmodel) {
    size_t maxp = parmac_get_total_parameters(pmodel);
    while (lv_obj_get_child_cnt(data->parmac.list)) {
        lv_obj_del(lv_obj_get_child(data->parmac.list, 0));
    }

    for (size_t i = 0; i < maxp; i++) {
        lv_obj_t *btn = lv_list_add_btn(data->parmac.list, NULL, NULL);
        view_register_object_default_callback_with_number(btn, PARAMETER_BTN_ID, i);

        lv_obj_t *l1 = lv_label_create(btn);
        lv_label_set_text_fmt(l1, "%2zu ", i + 1);
        lv_obj_set_width(l1, 20);
        lv_obj_t   *l2   = lv_label_create(btn);
        const char *text = parmac_get_description(pmodel, i);
        lv_label_set_text(l2, text);
        lv_label_set_long_mode(l2, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(l2, 260);
        lv_obj_align(l1, LV_ALIGN_LEFT_MID, 8, -20);
        lv_obj_align_to(l2, l1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

        char      string[128] = {0};
        lv_obj_t *l3          = lv_label_create(btn);
        lv_label_set_text(l3, parmac_to_string(pmodel, string, sizeof(string), i));
        lv_obj_align(l3, LV_ALIGN_RIGHT_MID, -20, 8);

        // pardata.extra = l3;
        // lv_obj_set_user_data(btn, pardata);
    }
}


static void create_tab_param(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    lv_obj_t *list = lv_list_create(tab);
    lv_obj_set_size(list, LV_PCT(65), LV_PCT(95));
    lv_obj_align(list, LV_ALIGN_LEFT_MID, 10, 0);
    data->parmac.list = list;

    data->parmac.editor = NULL;

    lv_obj_set_style_pad_all(tab, 0, LV_STATE_DEFAULT);

    data->parmac.tab = tab;
}


static void create_tab_test(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    lv_obj_t *btn = lv_btn_create(tab);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_DIAGNOSI));
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    view_register_object_default_callback(btn, TEST_BTN_ID);
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;

    lv_obj_t *tab = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t *user_tab = lv_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_CONFIGURAZIONE));

    lv_obj_t *param_tab = lv_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_PARAMETRI));
    create_tab_param(param_tab, pmodel, data);

    lv_obj_t *test_tab = lv_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_DIAGNOSI));
    create_tab_test(test_tab, pmodel, data);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_add_style(btn, &style_icon_button, LV_STATE_DEFAULT);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(btn, BACK_BTN_ID);

    update_param_list(data, pmodel);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case TEST_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE;
                        msg.vmsg.page = (void *)&page_test;
                        break;

                    case BACK_BTN_ID: {
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;
                    }

                    case PARAMETER_DEFAULT_BTN_ID: {
                        parameter_reset_to_defaults(&data->parmac.par, 1);
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                        }
                        data->parmac.editor = view_common_build_param_editor(
                            data->parmac.tab, pmodel, &data->parmac.par, data->parmac.num, PARAMETER_DROPDOWN_ID,
                            PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID, PARAMETER_ROLLER1_ID,
                            PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID, PARAMETER_CONFIRM_BTN_ID,
                            PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }

                    case PARAMETER_CANCEL_BTN_ID:
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                            data->parmac.editor = NULL;
                        }
                        break;

                    case PARAMETER_CONFIRM_BTN_ID:
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                            data->parmac.editor = NULL;
                        }
                        model_parameter_set_value(parmac_get_handle(pmodel, event.data.number),
                                                  parameter_to_long(&data->parmac.par));
                        update_param_list(data, pmodel);
                        break;

                    case PARAMETER_BTN_ID: {
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                        }
                        parameter_clone(&data->parmac.par, parmac_get_handle(pmodel, event.data.number),
                                        (void *)&data->parmac.par_buffer);
                        data->parmac.num = event.data.number;

                        data->parmac.editor = view_common_build_param_editor(
                            data->parmac.tab, pmodel, &data->parmac.par, data->parmac.num, PARAMETER_DROPDOWN_ID,
                            PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID, PARAMETER_ROLLER1_ID,
                            PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID, PARAMETER_CONFIRM_BTN_ID,
                            PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }
                }
            } else if (event.event == LV_EVENT_VALUE_CHANGED) {
                switch (event.data.id) {
                    case PARAMETER_TA_ID:
                        model_parameter_set_value(&data->parmac.par, atoi(event.string_value));
                        break;

                    case PARAMETER_ROLLER1_ID:
                        model_parameter_set_value(
                            &data->parmac.par,
                            (uint16_t)((parameter_to_long(&data->parmac.par) % 60) + event.value * 60));
                        break;

                    case PARAMETER_ROLLER2_ID: {
                        uint16_t value = (uint16_t)parameter_to_long(&data->parmac.par);
                        model_parameter_set_value(&data->parmac.par, (uint16_t)(value - (value % 60) + event.value));
                        break;
                    }

                    case PARAMETER_DROPDOWN_ID:
                    case PARAMETER_SWITCH_ID:
                        model_parameter_set_value(&data->parmac.par, (uint16_t)event.value);
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



const pman_page_t page_settings = {
    .create        = create_page,
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};