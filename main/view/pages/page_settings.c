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
#include "config/app_conf.h"
#include "log.h"


#define PROG_BTNMX_UP      0
#define PROG_BTNMX_COPY    1
#define PROG_BTNMX_DOWN    2
#define PROG_BTNMX_REMOVE  3
#define PROG_BTNMX_CONFIRM 4


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
    SAVE_ALL_IO_ID,
    READ_LOG_IO_ID,
    PROGRAM_BTN_ID,
    PROGRAM_BTNMATRIX_ID,
    ADD_PROG_BTN_ID,
    KEYBOARD_ID,
    NETWORK_BTN_ID,
    DRIVE_BTN_ID,
    SETTINGS_BTN_ID,
    LOG_BTN_ID,
    TABVIEW_ID,
    PASSWORD_ID,
    PASSWORD_KEYBOARD_ID,
    STATS_BTN_ID,
    BLANKET_ID,
};



struct page_data {
    struct {
        lv_obj_t *password;
        int       tosave;
        lv_obj_t *drive_btn;
    } user;
    struct {
        lv_obj_t *tab;
        lv_obj_t *list;
        lv_obj_t *editor;
        lv_obj_t *textarea;

        uint64_t           par_buffer;
        parameter_handle_t par;
        size_t             num;
    } parmac;

    struct {
        lv_obj_t *list;
        int       selected_prog;
        lv_obj_t *btnmx;
        lv_obj_t *keyboard_blanket;
        int       index_to_save;
    } prog;

    struct {
        lv_obj_t *test_btn;
    } diagnose;

    lv_obj_t *blanket;
    lv_obj_t *password_blanket;
    size_t    waiting_io;
    size_t    selected_tab;

    view_controller_message_t cmsg;
};


static void table_draw_event(lv_event_t *e) {
    lv_obj_t               *obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    /*If the cells are drawn...*/
    if (dsc->part == LV_PART_ITEMS) {
        uint32_t row = dsc->id / lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);

        /*Make the texts in the first cell center aligned*/
        if (row == 1) {
            dsc->rect_dsc->bg_color      = lv_color_darken(lv_palette_main(LV_PALETTE_RED), LV_OPA_30);
            dsc->rect_dsc->bg_grad_color = lv_palette_main(LV_PALETTE_RED);
            dsc->rect_dsc->bg_opa        = LV_OPA_COVER;
        } else if (row == 2) {
            dsc->rect_dsc->bg_color      = lv_color_darken(lv_palette_main(LV_PALETTE_GREEN), LV_OPA_30);
            dsc->rect_dsc->bg_grad_color = lv_palette_main(LV_PALETTE_GREEN);
            dsc->rect_dsc->bg_opa        = LV_OPA_COVER;
        } else if (row == 3) {
            dsc->rect_dsc->bg_color      = lv_color_darken(lv_palette_main(LV_PALETTE_BLUE), LV_OPA_30);
            dsc->rect_dsc->bg_grad_color = lv_palette_main(LV_PALETTE_BLUE);
            dsc->rect_dsc->bg_opa        = LV_OPA_COVER;
        }
    }
}


static size_t num_of_io_operations_to_wait(model_t *pmodel, struct page_data *data) {
    size_t res = 0;
    if (data->user.tosave) {
        res++;
    }
    if (data->prog.index_to_save) {
        res++;
    }
    for (size_t i = 0; i < MAX_PROGRAMS; i++) {
        if ((model_get_programs_to_save(pmodel) & (1 << i)) > 0) {
            res++;
        }
    }
    if (model_get_parmac_to_save(pmodel)) {
        res++;
    }
    return res;
}


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    struct page_data *pdata = (struct page_data *)malloc(sizeof(struct page_data));
    memset(pdata, 0, sizeof(struct page_data));

    return pdata;
}


static lv_obj_t *create_program_name_kb(lv_obj_t *root, model_t *pmodel) {
    lv_obj_t *blanket = view_common_create_blanket(root);
    lv_obj_t *kb      = lv_keyboard_create(blanket);
    lv_obj_set_size(kb, LV_HOR_RES, 240);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *ta = lv_textarea_create(blanket);
    lv_obj_set_size(ta, 520, 56);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_placeholder_text(ta, view_intl_get_string(pmodel, STRINGS_NOME_PROGRAMMA));
    lv_textarea_set_max_length(ta, 32);
    // lv_textarea_set_one_line(ta, 1);
    lv_keyboard_set_textarea(kb, ta);
    view_register_object_default_callback(kb, KEYBOARD_ID);

    return blanket;
}


static void update_test_btn(model_t *pmodel, struct page_data *data) {
    if (model_is_machine_communication_active(pmodel)) {
        lv_obj_clear_state(data->diagnose.test_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(data->diagnose.test_btn, LV_STATE_DISABLED);
    }
}


static void update_drive_btn(struct page_data *data, model_t *pmodel) {
    if (model_is_drive_mounted(pmodel)) {
        lv_obj_clear_state(data->user.drive_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(data->user.drive_btn, LV_STATE_DISABLED);
    }
}


static void update_prog_list(struct page_data *data, model_t *pmodel) {
    lv_obj_t *focus = NULL;
    size_t    i     = 0;

    while (lv_obj_get_child_cnt(data->prog.list)) {
        lv_obj_del(lv_obj_get_child(data->prog.list, 0));
    }

    for (i = 0; i < model_get_num_programs(pmodel); i++) {
        char string[64] = {0};
        snprintf(string, sizeof(string), "%zu %s", i + 1, model_get_program_name(pmodel, i));
        lv_obj_t *btn = lv_list_add_btn(data->prog.list, NULL, string);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(btn), LV_STATE_CHECKED);
        view_register_object_default_callback_with_number(btn, PROGRAM_BTN_ID, i);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    }
    view_common_select_btn_in_list(data->prog.list, data->prog.selected_prog);

    if (focus) {
        // TODO: add again
        // lv_list_focus(focus, LV_ANIM_OFF);
    }

    if (i < model_get_max_programs(pmodel)) {
        lv_obj_t *btn =
            lv_list_add_btn(data->prog.list, LV_SYMBOL_PLUS, view_intl_get_string(pmodel, STRINGS_NUOVO_PROGRAMMA));
        view_register_object_default_callback(btn, ADD_PROG_BTN_ID);
    }
}


static void update_password(struct page_data *data, model_t *pmodel) {
    if (strlen(model_get_password(pmodel)) > 0) {
        lv_label_set_text_fmt(data->user.password, "%s: %s",
                              view_intl_get_string(pmodel, STRINGS_CODICE_ACCESSO_UTENTE), model_get_password(pmodel));
    } else {
        lv_label_set_text_fmt(data->user.password, "%s: %s",
                              view_intl_get_string(pmodel, STRINGS_CODICE_ACCESSO_UTENTE),
                              view_intl_get_string(pmodel, STRINGS_NESSUNO));
    }
}


static void update_param_list(struct page_data *data, model_t *pmodel) {
    size_t maxp     = parmac_get_total_parameters(pmodel);
    int    rescroll = lv_obj_get_scroll_y(data->parmac.list);

    while (lv_obj_get_child_cnt(data->parmac.list)) {
        lv_obj_del(lv_obj_get_child(data->parmac.list, 0));
    }

    for (size_t i = 0; i < maxp; i++) {
        lv_obj_t *btn = lv_list_add_btn(data->parmac.list, NULL, NULL);
        lv_obj_set_style_pad_hor(btn, 2, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(btn), LV_STATE_CHECKED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        view_register_object_default_callback_with_number(btn, PARAMETER_BTN_ID, i);

        lv_obj_t *l1 = lv_label_create(btn);
        lv_label_set_text_fmt(l1, "%2zu ", i + 1);
        lv_label_set_long_mode(l1, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(l1, 30);
        lv_obj_t   *l2   = lv_label_create(btn);
        const char *text = parmac_get_description(pmodel, i);
        lv_label_set_text(l2, text);
        lv_label_set_long_mode(l2, LV_LABEL_LONG_SCROLL);
        lv_obj_set_width(l2, 350);
        lv_obj_set_style_anim_speed(l2, 12, LV_STATE_DEFAULT);
        lv_obj_align(l1, LV_ALIGN_LEFT_MID, 8, -20);
        lv_obj_align_to(l2, l1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

        char      string[128] = {0};
        lv_obj_t *l3          = lv_label_create(btn);
        lv_label_set_long_mode(l3, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(l3, 100);
        lv_label_set_text(l3, parmac_to_string(pmodel, string, sizeof(string), i));
        lv_obj_align(l3, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    lv_obj_scroll_to_y(data->parmac.list, rescroll, 0);
}


static void create_tab_programs(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    static const char *btnmap[] = {LV_SYMBOL_UP,    LV_SYMBOL_COPY, "\n",         LV_SYMBOL_DOWN,
                                   LV_SYMBOL_TRASH, "\n",           LV_SYMBOL_OK, ""};

    lv_obj_t *list           = lv_list_create(tab);
    data->prog.selected_prog = -1;

    lv_obj_t *lbl = lv_label_create(tab);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_width(lbl, 520);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    // data->prog.lwarn = lbl;

    lv_obj_set_size(list, LV_PCT(55), LV_PCT(95));
    lv_obj_align(list, LV_ALIGN_LEFT_MID, 16, 0);

    lv_obj_t *btnmatrix = lv_btnmatrix_create(tab);
    lv_btnmatrix_set_map(btnmatrix, btnmap);
    lv_obj_set_size(btnmatrix, LV_PCT(35), 320);
    lv_obj_align(btnmatrix, LV_ALIGN_RIGHT_MID, -16, 0);
    view_register_object_default_callback(btnmatrix, PROGRAM_BTNMATRIX_ID);

    lv_btnmatrix_set_btn_ctrl_all(btnmatrix, LV_BTNMATRIX_CTRL_DISABLED);

    data->prog.list  = list;
    data->prog.btnmx = btnmatrix;
}


static void create_tab_user(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    lv_obj_t *btn = view_common_icon_button(tab, LV_SYMBOL_SETTINGS, SETTINGS_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *prev_btn = btn;

    btn = view_common_icon_button(tab, LV_SYMBOL_WIFI, NETWORK_BTN_ID);
    lv_obj_align_to(btn, prev_btn, LV_ALIGN_OUT_TOP_MID, 0, -10);
    prev_btn = btn;

    btn = view_common_icon_button(tab, LV_SYMBOL_DRIVE, DRIVE_BTN_ID);
    lv_obj_align_to(btn, prev_btn, LV_ALIGN_OUT_TOP_MID, 0, -10);
    data->user.drive_btn = btn;

    btn           = lv_btn_create(tab);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_width(btn, 540);
    view_register_object_default_callback(btn, PASSWORD_ID);
    data->user.password = lbl;
    update_password(data, pmodel);
}


static void create_tab_param(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    lv_obj_t *list = lv_list_create(tab);
    lv_obj_set_size(list, LV_PCT(65), LV_PCT(90));
    lv_obj_set_style_pad_hor(list, 0, LV_STATE_DEFAULT);
    lv_obj_align(list, LV_ALIGN_LEFT_MID, 10, 0);
    data->parmac.list = list;

    data->parmac.editor = NULL;

    lv_obj_set_style_pad_all(tab, 0, LV_STATE_DEFAULT);

    data->parmac.tab = tab;
}


static void create_tab_test(lv_obj_t *tab, model_t *pmodel, struct page_data *data) {
    lv_obj_t *btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 240, 60);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_TEST_MACCHINA));
    view_register_object_default_callback(btn, TEST_BTN_ID);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -80);
    data->diagnose.test_btn = btn;

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 240, 60);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_STATISTICHE));
    view_register_object_default_callback(btn, STATS_BTN_ID);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 240, 60);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_EVENTI));
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 280, -10);

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 240, 60);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_VERBALE));
    view_register_object_default_callback(btn, LOG_BTN_ID);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 280, -80);

    lv_obj_t *table = lv_table_create(tab);
    lv_obj_add_event_cb(table, table_draw_event, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_table_set_cell_value(table, 0, 1, view_intl_get_string(pmodel, STRINGS_VERSIONE));
    lv_table_set_cell_value(table, 0, 2, view_intl_get_string(pmodel, STRINGS_DATA));
    lv_table_set_cell_value(table, 1, 0, view_intl_get_string(pmodel, STRINGS_INTERFACCIA));
    lv_table_set_cell_value(table, 2, 0, view_intl_get_string(pmodel, STRINGS_MACCHINA));
    lv_table_set_cell_value(table, 3, 0, view_intl_get_string(pmodel, STRINGS_SISTEMA_OPERATIVO));

    lv_table_set_cell_value(table, 1, 1, SOFTWARE_VERSION);
    lv_table_set_cell_value(table, 1, 2, SOFTWARE_BUILD_DATE);
    lv_table_set_cell_value(table, 2, 1, pmodel->machine.version);
    lv_table_set_cell_value(table, 2, 2, pmodel->machine.date);

    lv_table_set_col_width(table, 0, 240);
    lv_table_set_col_width(table, 1, 210);
    lv_table_set_col_width(table, 2, 210);

    lv_obj_align(table, LV_ALIGN_CENTER, 0, -80);
}


static void open_page(pman_handle_t handle, void *state) {
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = (model_t *)model_updater_get(updater);
    struct page_data *data    = state;

    data->prog.keyboard_blanket = NULL;

    lv_obj_t *tab = custom_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t *user_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_CONFIGURAZIONE));
    create_tab_user(user_tab, pmodel, data);

    lv_obj_t *param_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_PARAMETRI));
    create_tab_param(param_tab, pmodel, data);

    lv_obj_t *program_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_PROGRAMMI));
    create_tab_programs(program_tab, pmodel, data);

    lv_obj_t *test_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_DIAGNOSI));
    create_tab_test(test_tab, pmodel, data);

    view_register_object_default_callback(tab, TABVIEW_ID);
    custom_tabview_set_act(tab, data->selected_tab, 0);

    view_common_back_button(lv_scr_act(), BACK_BTN_ID);

    update_param_list(data, pmodel);
    update_prog_list(data, pmodel);
    update_drive_btn(data, pmodel);
    update_test_btn(pmodel, data);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t        msg     = PMAN_MSG_NULL;
    struct page_data *data    = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = (model_t *)model_updater_get(updater);

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *user_event = event.as.user;
            switch (user_event->code) {
                case VIEW_EVENT_CODE_DRIVE:
                    update_drive_btn(data, pmodel);
                    break;

                case VIEW_EVENT_CODE_ALARM:
                    update_test_btn(pmodel, data);
                    break;

                case VIEW_EVENT_CODE_OPEN:
                case VIEW_EVENT_CODE_STATE_CHANGED:
                    if (model_is_in_test(pmodel)) {
                        msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_CHANGE_PAGE;
                        msg.stack_msg.as.destination.page = (void *)&page_test;
                    }
                    break;

                case VIEW_EVENT_CODE_IO_DONE:
                    if (user_event->io_op == SAVE_ALL_IO_ID) {
                        if (user_event->error) {
                            view_common_io_error_toast(pmodel);
                            msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        } else if (--data->waiting_io == 0) {
                            msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        }
                    } else if (user_event->io_op == READ_LOG_IO_ID) {
                        if (user_event->error) {
                            view_common_io_error_toast(pmodel);
                        }
                        msg.stack_msg.tag                  = PMAN_STACK_MSG_TAG_CHANGE_PAGE_EXTRA;
                        msg.stack_msg.as.destination.extra = user_event->io_data;
                        msg.stack_msg.as.destination.page  = (void *)&page_log;
                    }
                    break;

                case VIEW_EVENT_CODE_LVGL: {
                    switch (user_event->data.id) {
                        case PARAMETER_KB_ID:
                            parameter_operator(&data->parmac.par, user_event->data.number);
                            if (data->parmac.textarea != NULL) {
                                char string[64] = {0};
                                snprintf(string, sizeof(string), "%li", parameter_to_long(&data->parmac.par));
                                lv_textarea_set_text(data->parmac.textarea, string);
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
                    case BLANKET_ID:
                        if (data->password_blanket) {
                            lv_obj_del(data->password_blanket);
                            data->password_blanket = NULL;
                        }
                        break;

                    case STATS_BTN_ID:
                        msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_CHANGE_PAGE;
                        msg.stack_msg.as.destination.page = (void *)&page_stats;
                        break;

                    case SETTINGS_BTN_ID:
                        msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_CHANGE_PAGE;
                        msg.stack_msg.as.destination.page = (void *)&page_tech_settings;
                        break;

                    case PASSWORD_ID:
                        if (data->password_blanket != NULL) {
                            lv_obj_del(data->password_blanket);
                        }
                        data->password_blanket =
                            view_common_create_password_popup(lv_scr_act(), PASSWORD_KEYBOARD_ID, 0);
                        view_register_object_default_callback(data->password_blanket, BLANKET_ID);
                        break;

                    case NETWORK_BTN_ID:
                        msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_CHANGE_PAGE;
                        msg.stack_msg.as.destination.page = (void *)&page_network;
                        break;

                    case LOG_BTN_ID:
                        data->cmsg.code       = VIEW_CONTROLLER_MESSAGE_CODE_READ_LOG_FILE;
                        data->cmsg.disk_op_id = READ_LOG_IO_ID;
                        data->blanket         = view_common_create_blanket(lv_scr_act());
                        break;

                    case DRIVE_BTN_ID:
                        msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_CHANGE_PAGE;
                        msg.stack_msg.as.destination.page = (void *)&page_drive;
                        break;

                    case PARAMETER_KB_ID:
                        break;

                    case PROGRAM_BTN_ID:
                        data->prog.selected_prog = objdata->number;
                        view_common_select_btn_in_list(data->prog.list, data->prog.selected_prog);
                        lv_btnmatrix_clear_btn_ctrl_all(data->prog.btnmx, LV_BTNMATRIX_CTRL_DISABLED);
                        break;

                    case TEST_BTN_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_ENTER_TEST;
                        break;

                    case ADD_PROG_BTN_ID: {
                        lv_btnmatrix_set_btn_ctrl_all(data->prog.btnmx, LV_BTNMATRIX_CTRL_DISABLED);
                        data->prog.keyboard_blanket = create_program_name_kb(lv_scr_act(), pmodel);
                        data->prog.selected_prog    = -1;
                        break;
                    }

                    case BACK_BTN_ID: {
                        size_t tosave = num_of_io_operations_to_wait(pmodel, data);
                        if (tosave > 0) {
                            data->waiting_io = tosave;

                            data->cmsg.code           = VIEW_CONTROLLER_MESSAGE_CODE_SAVE_ALL;
                            data->cmsg.parmac         = model_get_parmac_to_save(pmodel);
                            data->cmsg.index          = data->prog.index_to_save;
                            data->cmsg.password       = data->user.tosave;
                            data->cmsg.programs       = model_get_programs_to_save(pmodel);
                            data->cmsg.save_all_io_op = SAVE_ALL_IO_ID;
                            model_clear_marks_to_save(pmodel);
                            data->blanket = view_common_create_blanket(lv_scr_act());
                        } else {
                            msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        }
                        break;
                    }

                    case PARAMETER_DEFAULT_BTN_ID: {
                        parameter_reset_to_defaults(&data->parmac.par, 1);
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                        }
                        data->parmac.editor = view_common_build_param_editor(
                            data->parmac.tab, &data->parmac.textarea, pmodel, &data->parmac.par, data->parmac.num,
                            PARAMETER_DROPDOWN_ID, PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID,
                            PARAMETER_ROLLER1_ID, PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID,
                            PARAMETER_CONFIRM_BTN_ID, PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }

                    case PARAMETER_CANCEL_BTN_ID:
                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                            data->parmac.editor = NULL;
                        }
                        break;

                    case PARAMETER_CONFIRM_BTN_ID:
                        model_parameter_set_value(parmac_get_handle(pmodel, objdata->number),
                                                  parameter_to_long(&data->parmac.par));
                        update_param_list(data, pmodel);
                        model_mark_parmac_to_save(pmodel);

                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                            data->parmac.editor = NULL;
                        }
                        break;

                    case PARAMETER_BTN_ID: {
                        view_common_select_btn_in_list(data->parmac.list, objdata->number);
                        parameter_clone(&data->parmac.par, parmac_get_handle(pmodel, objdata->number),
                                        (void *)&data->parmac.par_buffer);
                        data->parmac.num = objdata->number;

                        if (data->parmac.editor != NULL) {
                            lv_obj_del(data->parmac.editor);
                        }

                        data->parmac.editor = view_common_build_param_editor(
                            data->parmac.tab, &data->parmac.textarea, pmodel, &data->parmac.par, data->parmac.num,
                            PARAMETER_DROPDOWN_ID, PARAMETER_SWITCH_ID, PARAMETER_TA_ID, PARAMETER_KB_ID,
                            PARAMETER_ROLLER1_ID, PARAMETER_ROLLER2_ID, PARAMETER_CANCEL_BTN_ID,
                            PARAMETER_CONFIRM_BTN_ID, PARAMETER_DEFAULT_BTN_ID);
                        break;
                    }
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_VALUE_CHANGED) {
                switch (objdata->id) {
                    case PROGRAM_BTNMATRIX_ID: {
                        switch (lv_btnmatrix_get_selected_btn(target)) {
                            case PROG_BTNMX_UP:
                                if (data->prog.selected_prog > 0) {
                                    model_swap_programs(pmodel, data->prog.selected_prog, data->prog.selected_prog - 1);
                                    data->prog.index_to_save = 1;
                                    data->prog.selected_prog--;
                                    update_prog_list(data, pmodel);
                                }
                                break;

                            case PROG_BTNMX_DOWN:
                                if (data->prog.selected_prog >= 0 &&
                                    (size_t)data->prog.selected_prog + 1 < model_get_num_programs(pmodel)) {
                                    model_swap_programs(pmodel, data->prog.selected_prog, data->prog.selected_prog + 1);
                                    data->prog.index_to_save = 1;
                                    data->prog.selected_prog++;
                                    update_prog_list(data, pmodel);
                                }
                                break;

                            case PROG_BTNMX_CONFIRM:
                                msg.stack_msg.tag                  = PMAN_STACK_MSG_TAG_CHANGE_PAGE_EXTRA;
                                msg.stack_msg.as.destination.extra = (void *)(uintptr_t)data->prog.selected_prog;
                                msg.stack_msg.as.destination.page  = (void *)&page_program;
                                break;

                            case PROG_BTNMX_REMOVE:
                                if (data->prog.selected_prog >= 0) {
                                    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_REMOVE_PROGRAM;
                                    strncpy(data->cmsg.name,
                                            model_get_program(pmodel, data->prog.selected_prog)->filename,
                                            sizeof(data->cmsg.name));
                                    data->cmsg.name[sizeof(data->cmsg.name) - 1] = '\0';

                                    model_remove_program(pmodel, data->prog.selected_prog);

                                    data->prog.index_to_save = 1;
                                    data->prog.selected_prog = -1;
                                    lv_btnmatrix_set_btn_ctrl_all(data->prog.btnmx, LV_BTNMATRIX_CTRL_DISABLED);
                                    update_prog_list(data, pmodel);
                                }
                                break;

                            case PROG_BTNMX_COPY: {
                                size_t p_index = data->prog.selected_prog + 1;
                                if (data->prog.selected_prog >= 0) {
                                    dryer_program_t *p = model_create_new_program_from(pmodel, data->prog.selected_prog,
                                                                                       p_index, get_millis());

                                    if (p) {
                                        data->prog.index_to_save = 1;
                                        model_mark_program_to_save(pmodel, p_index);
                                    }

                                    update_prog_list(data, pmodel);
                                }
                                break;
                            }
                        }
                        break;
                    }

                    case TABVIEW_ID:
                        data->selected_tab = lv_tabview_get_tab_act(target);
                        break;

                    case PARAMETER_TA_ID:
                        model_parameter_set_value(&data->parmac.par, atoi(lv_textarea_get_text(target)));
                        break;

                    case PARAMETER_ROLLER1_ID:
                        model_parameter_set_value(&data->parmac.par,
                                                  (uint16_t)((parameter_to_long(&data->parmac.par) % 60) +
                                                             lv_roller_get_selected(target) * 60));
                        break;

                    case PARAMETER_ROLLER2_ID: {
                        uint16_t value = (uint16_t)parameter_to_long(&data->parmac.par);
                        model_parameter_set_value(&data->parmac.par,
                                                  (uint16_t)(value - (value % 60) + lv_roller_get_selected(target)));
                        break;
                    }

                    case PARAMETER_DROPDOWN_ID:
                        model_parameter_set_value(&data->parmac.par, (uint16_t)lv_dropdown_get_selected(target));
                        break;

                    case PARAMETER_SWITCH_ID:
                        model_parameter_set_value(&data->parmac.par,
                                                  (uint16_t)lv_obj_has_state(target, LV_STATE_CHECKED));
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_READY) {
                switch (objdata->id) {
                    case KEYBOARD_ID: {
                        lv_obj_t *ta = lv_keyboard_get_textarea(target);

                        size_t           p_index = 0;
                        dryer_program_t *p       = model_create_new_program(
                            pmodel, lv_textarea_get_text(ta), model_get_language(pmodel), get_millis(), &p_index);
                        if (p) {
                            data->prog.index_to_save = 1;
                            model_mark_program_to_save(pmodel, p_index);
                        }

                        if (data->prog.keyboard_blanket != NULL) {
                            lv_obj_del(data->prog.keyboard_blanket);
                            data->prog.keyboard_blanket = NULL;
                        }
                        update_prog_list(data, pmodel);
                        break;
                    }

                    case PASSWORD_KEYBOARD_ID: {
                        lv_obj_t *ta = lv_keyboard_get_textarea(target);

                        data->user.tosave = 1;
                        model_set_password(pmodel, lv_textarea_get_text(ta));
                        update_password(data, pmodel);
                        if (data->password_blanket != NULL) {
                            lv_obj_del(data->password_blanket);
                            data->password_blanket = NULL;
                        }
                        break;
                    }
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CANCEL) {
                switch (objdata->id) {
                    case KEYBOARD_ID: {
                        if (data->prog.keyboard_blanket != NULL) {
                            lv_obj_del(data->prog.keyboard_blanket);
                            data->prog.keyboard_blanket = NULL;
                        }
                        break;
                    }

                    case PASSWORD_KEYBOARD_ID:
                        if (data->password_blanket != NULL) {
                            lv_obj_del(data->password_blanket);
                            data->password_blanket = NULL;
                        }
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
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};
