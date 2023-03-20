#include <stdio.h>
#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "model/parciclo.h"
#include "view/common.h"
#include "view/intl/intl.h"


enum {
    BACK_BTN_ID,
    PARAMETER_BTN_ID,
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
    lv_obj_t *list;
    lv_obj_t *editor;
    lv_obj_t *textarea;

    size_t             num;
    uint64_t           par_buffer;
    parameter_handle_t par;

    size_t num_prog;
    size_t num_step;
};


static void update_param_list(struct page_data *data, model_t *pmodel) {
    size_t maxp = parciclo_get_total_parameters(pmodel);
    while (lv_obj_get_child_cnt(data->list)) {
        lv_obj_del(lv_obj_get_child(data->list, 0));
    }

    for (size_t i = 0; i < maxp; i++) {
        lv_obj_t *btn = lv_list_add_btn(data->list, NULL, NULL);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(btn), LV_STATE_CHECKED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        view_register_object_default_callback_with_number(btn, PARAMETER_BTN_ID, i);

        lv_obj_t *l1 = lv_label_create(btn);
        lv_label_set_text_fmt(l1, "%2zu ", i + 1);
        lv_obj_set_width(l1, 32);
        lv_obj_t   *l2   = lv_label_create(btn);
        const char *text = parciclo_get_description(pmodel, i);
        lv_label_set_text(l2, text);
        lv_label_set_long_mode(l2, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(l2, 320);
        lv_obj_align(l1, LV_ALIGN_LEFT_MID, 8, -20);
        lv_obj_align_to(l2, l1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

        char      string[128] = {0};
        lv_obj_t *l3          = lv_label_create(btn);
        lv_label_set_text(l3, parciclo_to_string(pmodel, string, sizeof(string), i));
        lv_obj_align(l3, LV_ALIGN_RIGHT_MID, -20, 8);
    }
}


static void *create_page(model_t *pmodel, void *extra) {
    struct page_data *data = malloc(sizeof(struct page_data));
    size_t           *pars = extra;
    data->num_prog         = pars[0];
    data->num_step         = pars[1];
    parciclo_init(pmodel, data->num_prog, data->num_step);
    return data;
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;

    dryer_program_t   *p      = model_get_program(pmodel, data->num_prog);
    parameters_step_t *s      = &p->steps[data->num_step];
    int                lingua = model_get_language(pmodel);

    char string[512] = {0};
    snprintf(string, sizeof(string), "%s - %zu/%u - %s", p->nomi[lingua], data->num_step + 1, p->num_steps,
             view_intl_step_description(pmodel, s->type));
    view_common_create_title(lv_scr_act(), string, BACK_BTN_ID);

    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, LV_PCT(65), LV_PCT(80));
    lv_obj_align(list, LV_ALIGN_LEFT_MID, 10, 0);
    data->list = list;

    data->editor = NULL;

    update_param_list(data, pmodel);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_TIMER:
            break;

        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case PARAMETER_KB_ID:
                        parameter_operator(&data->par, event.data.number);
                        if (data->textarea != NULL) {
                            char string[64] = {0};
                            snprintf(string, sizeof(string), "%li", parameter_to_long(&data->par));
                            lv_textarea_set_text(data->textarea, string);
                        }
                        break;

                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;

                    case PARAMETER_BTN_ID: {
                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                        }
                        view_common_select_btn_in_list(data->list, event.data.number);
                        parameter_clone(&data->par, parciclo_get_handle(pmodel, event.data.number),
                                        (void *)&data->par_buffer);
                        data->num = event.data.number;

                        data->editor = view_common_build_param_editor(
                            lv_scr_act(), &data->textarea, pmodel, &data->par, data->num, PARAMETER_DROPDOWN_ID,
                            PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID, PARAMETER_ROLLER1_ID,
                            PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID, PARAMETER_CONFIRM_BTN_ID,
                            PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }

                    case PARAMETER_DEFAULT_BTN_ID: {
                        parameter_reset_to_defaults(&data->par, 1);
                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                        }
                        data->editor = view_common_build_param_editor(
                            lv_scr_act(), &data->textarea, pmodel, &data->par, data->num, PARAMETER_DROPDOWN_ID,
                            PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID, PARAMETER_ROLLER1_ID,
                            PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID, PARAMETER_CONFIRM_BTN_ID,
                            PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }

                    case PARAMETER_CANCEL_BTN_ID:
                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                            data->editor = NULL;
                        }
                        break;

                    case PARAMETER_CONFIRM_BTN_ID:
                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                            data->editor = NULL;
                        }
                        model_parameter_set_value(parciclo_get_handle(pmodel, event.data.number),
                                                  parameter_to_long(&data->par));
                        update_param_list(data, pmodel);
                        model_mark_program_to_save(pmodel, data->num_prog);
                        break;
                }
            } else if (event.event == LV_EVENT_VALUE_CHANGED) {
                switch (event.data.id) {
                    case PARAMETER_TA_ID:
                        model_parameter_set_value(&data->par, atoi(event.string_value));
                        break;

                    case PARAMETER_ROLLER1_ID:
                        model_parameter_set_value(&data->par,
                                                  (uint16_t)((parameter_to_long(&data->par) % 60) + event.value * 60));
                        break;

                    case PARAMETER_ROLLER2_ID: {
                        uint16_t value = (uint16_t)parameter_to_long(&data->par);
                        model_parameter_set_value(&data->par, (uint16_t)(value - (value % 60) + event.value));
                        break;
                    }

                    case PARAMETER_DROPDOWN_ID:
                    case PARAMETER_SWITCH_ID:
                        model_parameter_set_value(&data->par, (uint16_t)event.value);
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



const pman_page_t page_step = {
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};