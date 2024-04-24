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

    view_controller_message_t cmsg;
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


static void *create_page(pman_handle_t handle, void *extra) {
    struct page_data *data = malloc(sizeof(struct page_data));

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    size_t *pars   = extra;
    data->num_prog = pars[0];
    data->num_step = pars[1];
    parciclo_init(pmodel, data->num_prog, data->num_step);
    return data;
}


static void open_page(pman_handle_t handle, void *arg) {
    struct page_data *data = arg;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    dryer_program_t   *p      = model_get_program(pmodel, data->num_prog);
    parameters_step_t *s      = &p->steps[data->num_step];
    int                lingua = model_get_language(pmodel);

    char string[512] = {0};
    snprintf(string, sizeof(string), "%s - %zu/%u - %s", p->nomi[lingua], data->num_step + 1, p->num_steps,
             view_intl_step_description(pmodel, s->type));
    view_common_create_title(lv_scr_act(), string, BACK_BTN_ID);

    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, LV_PCT(65), 400);
    lv_obj_align(list, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    data->list = list;

    data->editor = NULL;

    update_param_list(data, pmodel);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *arg, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = arg;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *user_event = event.as.user;
            switch (user_event->code) {
                case VIEW_EVENT_CODE_LVGL: {
                    switch (user_event->data.id) {
                        case PARAMETER_KB_ID:
                            parameter_operator(&data->par, user_event->data.number);
                            if (data->textarea != NULL) {
                                char string[64] = {0};
                                snprintf(string, sizeof(string), "%li", parameter_to_long(&data->par));
                                lv_textarea_set_text(data->textarea, string);
                            }
                            break;
                    }
                    break;
                }

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
                    case PARAMETER_KB_ID:
                        break;

                    case BACK_BTN_ID:
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        break;

                    case PARAMETER_BTN_ID: {
                        view_common_select_btn_in_list(data->list, objdata->number);
                        parameter_clone(&data->par, parciclo_get_handle(pmodel, objdata->number),
                                        (void *)&data->par_buffer);
                        data->num = objdata->number;

                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                        }

                        data->editor = view_common_build_param_editor(
                            lv_scr_act(), &data->textarea, pmodel, &data->par, data->num, PARAMETER_DROPDOWN_ID,
                            PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID, PARAMETER_ROLLER1_ID,
                            PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID, PARAMETER_CONFIRM_BTN_ID,
                            PARAMETER_DEFAULT_BTN_ID);
                        lv_obj_set_height(data->editor, 400);
                        lv_obj_align(data->editor, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
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
                        lv_obj_set_height(data->editor, 400);
                        lv_obj_align(data->editor, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
                        break;
                    }

                    case PARAMETER_CANCEL_BTN_ID:
                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                            data->editor = NULL;
                        }
                        break;

                    case PARAMETER_CONFIRM_BTN_ID:
                        model_parameter_set_value(parciclo_get_handle(pmodel, objdata->number),
                                                  parameter_to_long(&data->par));

                        if (data->editor != NULL) {
                            lv_obj_del(data->editor);
                            data->editor = NULL;
                        }

                        update_param_list(data, pmodel);
                        model_mark_program_to_save(pmodel, data->num_prog);
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_VALUE_CHANGED) {
                switch (objdata->id) {
                    case PARAMETER_TA_ID:
                        model_parameter_set_value(&data->par, atoi(lv_textarea_get_text(target)));
                        break;

                    case PARAMETER_ROLLER1_ID:
                        model_parameter_set_value(&data->par, (uint16_t)((parameter_to_long(&data->par) % 60) +
                                                                         lv_roller_get_selected(target) * 60));
                        break;

                    case PARAMETER_ROLLER2_ID: {
                        uint16_t value = (uint16_t)parameter_to_long(&data->par);
                        model_parameter_set_value(&data->par,
                                                  (uint16_t)(value - (value % 60) + lv_roller_get_selected(target)));
                        break;
                    }

                    case PARAMETER_DROPDOWN_ID:
                        model_parameter_set_value(&data->par, (uint16_t)lv_dropdown_get_selected(target));
                        break;

                    case PARAMETER_SWITCH_ID:
                        model_parameter_set_value(&data->par, (uint16_t)lv_obj_has_state(target, LV_STATE_CHECKED));
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
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
