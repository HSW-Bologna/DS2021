#if 0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "model/parmac.h"
#include "gel/pagemanager/page_manager.h"
#include "view/widgets/custom_tabview.h"
#include "view/theme/style.h"
#include "view/intl/intl.h"
#include "view/common.h"
#include "utils/system_time.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"


#define STEP_BTNMX_UP      0
#define STEP_BTNMX_COPY    1
#define STEP_BTNMX_DOWN    2
#define STEP_BTNMX_REMOVE  3
#define STEP_BTNMX_INSERT  4
#define STEP_BTNMX_CONFIRM 5


enum {
    BACK_BTN_ID,
    NAME_TEXTAREA_ID,
    TYPE_DDLIST_ID,
    LINGUA_DDLIST_ID,
    NAME_KB_ID,
    BTNMATRIX_ID,
    STEP_BTN_ID,
    TYPE_BTN_ID,
    ADD_STEP_BTN_ID,
    BLANKET_ID,
};


struct page_data {
    int       num_prog, lingua, selected_step, future_pos;
    lv_obj_t *ltitle, *lnum, *ldurata;
    lv_obj_t *kbname;
    lv_obj_t *langlist, *steplist, *typelist;
    lv_obj_t *btnmx;
    lv_obj_t *addbtn;
    lv_obj_t *blanket;
    lv_obj_t *mbox;
};


static void create_step_type_blanket(model_t *pmodel, struct page_data *data) {
    data->blanket = view_common_create_blanket(lv_scr_act());
    view_register_object_default_callback(data->blanket, BLANKET_ID);
    lv_obj_t *list = lv_list_create(data->blanket);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    for (size_t i = 0; i < NUM_DRYER_PROGRAM_STEP_TYPES; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL, parameters_tipi_step[i][model_get_language(pmodel)]);
        view_register_object_default_callback_with_number(btn, TYPE_BTN_ID, i);
    }
}



static void update_step_list(struct page_data *data, model_t *pmodel) {
    lv_obj_t *focus = NULL;
    size_t    i     = 0;

    while (lv_obj_get_child_cnt(data->steplist)) {
        lv_obj_del(lv_obj_get_child(data->steplist, 0));
    }

    int              lingua = model_get_language(pmodel);
    dryer_program_t *p      = model_get_program(pmodel, data->num_prog);

    for (i = 0; i < p->num_steps; i++) {
        char string[64] = {0};
        snprintf(string, sizeof(string), "%zu %s", i + 1, parameters_tipi_step[p->steps[i].type][lingua]);
        lv_obj_t *btn = lv_list_add_btn(data->steplist, NULL, string);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(btn), LV_STATE_CHECKED);
        view_register_object_default_callback_with_number(btn, STEP_BTN_ID, i);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    }
    view_common_select_btn_in_list(data->steplist, data->selected_step);

    if (focus) {
        // TODO: add again
        // lv_list_focus(focus, LV_ANIM_OFF);
    }

    if (i < MAX_STEPS) {
        lv_obj_t *btn =
            lv_list_add_btn(data->steplist, LV_SYMBOL_PLUS, view_intl_get_string(pmodel, STRINGS_NUOVO_PASSO));
        view_register_object_default_callback(btn, ADD_STEP_BTN_ID);
        if (data->selected_step >= 0) {
            lv_btnmatrix_clear_btn_ctrl(data->btnmx, STEP_BTNMX_INSERT, LV_BTNMATRIX_CTRL_DISABLED);
        }
    } else {
        lv_btnmatrix_set_btn_ctrl(data->btnmx, STEP_BTNMX_INSERT, LV_BTNMATRIX_CTRL_DISABLED);
    }
}


static void *create_page(model_t *model, void *extra) {
    struct page_data *data = (struct page_data *)malloc(sizeof(struct page_data));
    if (extra) {
        data->num_prog = (int)(uintptr_t)extra;
    } else {
        data->num_prog = 0;
    }

    data->mbox          = NULL;
    data->selected_step = -1;

    return data;
}


static void open_page(model_t *pmodel, void *arg) {
    int               lingua = model_get_language(pmodel);
    struct page_data *data   = arg;

    data->lingua       = lingua;
    dryer_program_t *p = model_get_program(pmodel, data->num_prog);

    lv_obj_t *back = view_common_back_button(lv_scr_act(), BACK_BTN_ID);

    data->langlist = lv_dropdown_create(lv_scr_act());
    lv_obj_set_width(data->langlist, 160);
    // lv_ddlist_set_draw_arrow(data->langlist, 0);


    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(cont, LV_HOR_RES - lv_obj_get_width(back) - 240, 50);
    lv_obj_align_to(cont, back, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    lv_obj_add_style(cont, (lv_style_t *)&style_title, LV_STATE_DEFAULT);

    data->ltitle = lv_textarea_create(cont);
    // lv_ta_set_cursor_type(data->ltitle, lv_textarea_get_cursor_type(data->ltitle) | LV_CURSOR_HIDDEN);
    lv_textarea_set_one_line(data->ltitle, 1);
    lv_textarea_set_max_length(data->ltitle, 32);
    lv_obj_set_size(data->ltitle, LV_PCT(90), 56);
    lv_obj_align(data->ltitle, LV_ALIGN_LEFT_MID, 16, 0);

    view_register_object_default_callback(data->ltitle, NAME_TEXTAREA_ID);
    lv_textarea_set_text(data->ltitle, p->nomi[data->lingua]);

    char options[512] = {0};
    memset(options, 0, sizeof(options));
    for (int i = 0; i < NUM_LINGUE; i++) {
        strcat(options, parameters_lingue[i][data->lingua]);
        strcat(options, "\n");
    }
    options[strlen(options) - 1] = '\0';

    lv_dropdown_set_options(data->langlist, options);
    lv_dropdown_set_selected(data->langlist, data->lingua);
    lv_obj_align_to(data->langlist, cont, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    view_register_object_default_callback(data->langlist, LINGUA_DDLIST_ID);

    data->steplist = lv_list_create(lv_scr_act());

    lv_obj_t *l = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(l, "%s:", view_intl_get_string(pmodel, STRINGS_DURATA));
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, 8, 80);

    l = lv_label_create(lv_scr_act());
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, 78, 80);
    data->ldurata = l;

    l = lv_label_create(lv_scr_act());
    lv_label_set_text(l, view_intl_get_string(pmodel, STRINGS_TIPO));
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, 8, 128);

    lv_obj_t *ddlist = lv_dropdown_create(lv_scr_act());
    view_register_object_default_callback(ddlist, TYPE_DDLIST_ID);
    lv_obj_set_width(ddlist, 245);
    lv_obj_align(ddlist, LV_ALIGN_TOP_LEFT, 70, 120);

    for (size_t i = 0; i < NUM_DRYER_PROGRAM_STEP_TYPES; i++) {
        strcat(options, parameters_tipi_step[i][data->lingua]);
        strcat(options, "\n");
    }
    options[strlen(options) - 1] = '\0';
    lv_dropdown_set_options(ddlist, options);
    lv_dropdown_set_selected(ddlist, p->type);

    static const char *btnmap[] = {
        LV_SYMBOL_UP, LV_SYMBOL_COPY, "\n", LV_SYMBOL_DOWN, LV_SYMBOL_TRASH,
        "\n",         LV_SYMBOL_PLUS, "\n", LV_SYMBOL_OK,   "",
    };

    lv_obj_t *btnmatrix = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(btnmatrix, btnmap);
    lv_obj_set_size(btnmatrix, 160, 280);
    lv_obj_align(btnmatrix, LV_ALIGN_RIGHT_MID, -16, 0);
    // set_btnm_style(btnmatrix);
    view_register_object_default_callback(btnmatrix, BTNMATRIX_ID);


    lv_obj_set_size(data->steplist, 280, 360);
    // lv_list_set_scroll_propagation(data->steplist, 1);
    // lv_list_set_sb_mode(data->steplist, LV_SB_MODE_AUTO);
    // lv_list_set_single_mode(data->steplist, 1);

    lv_obj_align_to(data->steplist, btnmatrix, LV_ALIGN_OUT_LEFT_TOP, -16, 0);

    if (data->selected_step < 0) {
        lv_btnmatrix_set_btn_ctrl_all(btnmatrix, LV_BTNMATRIX_CTRL_DISABLED);
    }

    data->lnum = lv_label_create(lv_scr_act());
    // lv_obj_set_style(data->lnum, &style_settings_text_medium);
    lv_obj_align_to(data->lnum, data->steplist, LV_ALIGN_OUT_TOP_LEFT, 8, -8);
    lv_obj_add_flag(data->lnum, LV_OBJ_FLAG_HIDDEN);


    data->btnmx = btnmatrix;

    data->kbname = lv_keyboard_create(lv_scr_act());
    view_register_object_default_callback(data->kbname, NAME_KB_ID);
    lv_keyboard_set_textarea(data->kbname, data->ltitle);
    lv_obj_add_flag(data->kbname, LV_OBJ_FLAG_HIDDEN);

    update_step_list(data, pmodel);
}



static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;

                    case STEP_BTN_ID:
                        data->selected_step = event.data.number;
                        view_common_select_btn_in_list(data->steplist, data->selected_step);
                        lv_btnmatrix_clear_btn_ctrl_all(data->btnmx, LV_BTNMATRIX_CTRL_DISABLED);
                        if (model_get_program(pmodel, data->num_prog)->num_steps >= MAX_STEPS) {
                            lv_btnmatrix_set_btn_ctrl(data->btnmx, STEP_BTNMX_INSERT, LV_BTNMATRIX_CTRL_DISABLED);
                        }
                        break;

                    case NAME_TEXTAREA_ID:
                        lv_obj_clear_flag(data->kbname, LV_OBJ_FLAG_HIDDEN);
                        break;

                    case TYPE_BTN_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                            data->blanket = NULL;
                        }
                        program_insert_step(model_get_program(pmodel, data->num_prog), event.data.number,
                                            data->future_pos);
                        update_step_list(data, pmodel);
                        break;

                    case BLANKET_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                            data->blanket = NULL;
                        }
                        break;

                    case ADD_STEP_BTN_ID: {
                        create_step_type_blanket(pmodel, data);
                        data->future_pos = model_get_program(pmodel, data->num_prog)->num_steps;
                        break;
                    }
                }
            } else if (event.event == LV_EVENT_VALUE_CHANGED) {
                switch (event.data.id) {
                    case BTNMATRIX_ID: {
                        switch (event.value) {
                            case STEP_BTNMX_INSERT: {
                                create_step_type_blanket(pmodel, data);
                                data->future_pos = data->selected_step;
                                break;
                            }

                            case STEP_BTNMX_UP:
                                if (data->selected_step > 0) {
                                    program_swap_steps(model_get_program(pmodel, data->num_prog), data->selected_step,
                                                       data->selected_step - 1);
                                    model_mark_program_to_save(pmodel, data->num_prog);
                                    data->selected_step--;
                                    update_step_list(data, pmodel);
                                }
                                break;

                            case STEP_BTNMX_DOWN: {
                                dryer_program_t *p = model_get_program(pmodel, data->num_prog);
                                if (data->selected_step >= 0 && (size_t)data->selected_step + 1 < p->num_steps) {
                                    program_swap_steps(p, data->selected_step, data->selected_step + 1);
                                    data->selected_step++;
                                    model_mark_program_to_save(pmodel, data->num_prog);
                                    update_step_list(data, pmodel);
                                }
                                break;
                            }

                            case STEP_BTNMX_CONFIRM: {
                                size_t *extra  = malloc(sizeof(size_t) * 2);
                                extra[0]       = data->num_prog;
                                extra[1]       = data->selected_step;
                                msg.vmsg.code  = VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE_EXTRA;
                                msg.vmsg.extra = extra;
                                msg.vmsg.page  = (void *)&page_step;
                                break;
                            }

                            case STEP_BTNMX_REMOVE:
                                if (data->selected_step >= 0) {
                                    program_remove_step(model_get_program(pmodel, data->num_prog), data->selected_step);
                                    model_mark_program_to_save(pmodel, data->num_prog);
                                    data->selected_step = -1;
                                    lv_btnmatrix_set_btn_ctrl_all(data->btnmx, LV_BTNMATRIX_CTRL_DISABLED);
                                    update_step_list(data, pmodel);
                                }
                                break;

                            case STEP_BTNMX_COPY: {
                                size_t s_index = data->selected_step + 1;
                                if (data->selected_step >= 0) {
                                    program_copy_step(model_get_program(pmodel, data->num_prog), data->selected_step,
                                                      s_index);
                                    model_mark_program_to_save(pmodel, data->num_prog);
                                    update_step_list(data, pmodel);
                                }
                                break;
                            }
                        }
                        break;
                    }

                    case LINGUA_DDLIST_ID: {
                        data->lingua = event.value;
                        lv_textarea_set_text(data->ltitle,
                                             model_get_program(pmodel, data->num_prog)->nomi[data->lingua]);
                        break;
                    }
                }
            } else if (event.event == LV_EVENT_CANCEL) {
                switch (event.data.id) {
                    case NAME_KB_ID: {
                        lv_textarea_set_text(data->ltitle,
                                             model_get_program(pmodel, data->num_prog)->nomi[data->lingua]);
                        lv_obj_add_flag(data->kbname, LV_OBJ_FLAG_HIDDEN);
                        break;
                    }
                }
            } else if (event.event == LV_EVENT_READY) {
                switch (event.data.id) {
                    case NAME_KB_ID: {
                        strcpy(model_get_program(pmodel, data->num_prog)->nomi[data->lingua], event.string_value);
                        lv_obj_add_flag(data->kbname, LV_OBJ_FLAG_HIDDEN);
                        model_mark_program_to_save(pmodel, data->num_prog);
                        break;
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    return msg;
}


void destroy_page(void *data, void *extra) {
    (void)extra;
    free(data);
}


const pman_page_t page_program = {
    .create        = create_page,
    .close         = view_close_all,
    .destroy       = destroy_page,
    .process_event = process_page_event,
    .open          = open_page,
};
#endif