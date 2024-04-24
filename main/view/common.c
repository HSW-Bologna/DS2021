#include <stdio.h>
#include "view.h"
#include "common.h"
#include "model/parmac.h"
#include "intl/intl.h"
#include "theme/style.h"


static void delete_obj_timer(lv_timer_t *timer);


lv_obj_t *view_common_back_button(lv_obj_t *parent, int id) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    // lv_obj_add_style(btn, (lv_style_t *)&style_icon_button, LV_STATE_DEFAULT);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(btn, id);

    return btn;
}


lv_obj_t *view_common_create_blanket(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_style_radius(root, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(root, LV_OPA_40, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(root, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(root, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    return root;
}


lv_obj_t *view_common_create_popup(lv_obj_t *parent, int covering) {
    const int sizex = 480;
    const int sizey = 360;

    lv_obj_t *root;
    if (covering) {
        root = view_common_create_blanket(parent);
    } else {
        root = parent;
    }

    lv_obj_t *popup = lv_obj_create(root);
    lv_obj_set_size(popup, sizex, sizey);
    lv_obj_align(popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_scrollbar_mode(popup, LV_SCROLLBAR_MODE_OFF);

    return popup;
}


lv_obj_t *view_common_create_info_popup(lv_obj_t *parent, int covering, const lv_img_dsc_t *src, const char *text,
                                        const char *confirm_text, int confirm_id) {
    lv_obj_t *popup = view_common_create_popup(parent, covering);

    lv_obj_t *btn = lv_btn_create(popup);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, confirm_text);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    if (lv_obj_get_width(btn) < 120) {
        lv_obj_set_width(btn, 120);
    }
    view_register_object_default_callback(btn, confirm_id);

    if (src != NULL) {
        lv_obj_t *img = lv_img_create(popup);
        lv_img_set_src(img, src);
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);
    }

    lbl = lv_label_create(popup);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(90));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 8);
    lv_label_set_text(lbl, text);

    return popup;
}


lv_obj_t *view_common_create_choice_popup(lv_obj_t *parent, int covering, const char *text, const char *confirm_text,
                                          int confirm_id, const char *refuse_text, int refuse_id) {
    lv_obj_t *popup = view_common_create_popup(parent, covering);

    lv_obj_t *btn = lv_btn_create(popup);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, confirm_text);
    if (lv_obj_get_width(btn) < 120) {
        lv_obj_set_width(btn, 120);
    }
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    view_register_object_default_callback(btn, confirm_id);

    btn = lv_btn_create(popup);
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, refuse_text);
    if (lv_obj_get_width(btn) < 120) {
        lv_obj_set_width(btn, 120);
    }
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    view_register_object_default_callback(btn, refuse_id);

    lbl = lv_label_create(popup);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(90));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -8);
    lv_label_set_text(lbl, text);

    return popup;
}


lv_obj_t *view_common_create_title(lv_obj_t *parent, const char *title, int back_id) {
    lv_obj_t *back = view_common_back_button(parent, back_id);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(cont, LV_HOR_RES - lv_obj_get_width(back) - 80, 50);
    lv_obj_align_to(cont, back, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    lv_obj_add_style(cont, (lv_style_t *)&style_title, LV_STATE_DEFAULT);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, title);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 16, 0);

    return back;
}


lv_obj_t *view_common_build_param_editor(lv_obj_t *root, lv_obj_t **textarea, model_t *pmodel, parameter_handle_t *par,
                                         size_t num, int dd_id, int sw_id, int ta_id, int kb_id, int roller1_id,
                                         int roller2_id, int cancel_id, int confirm_id, int default_id) {
    char      options[512] = {0};
    lv_obj_t *cont         = lv_obj_create(root);
    int       len;

    lv_obj_set_size(cont, LV_PCT(32), LV_PCT(95));
    lv_obj_align(cont, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_pad_all(cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *lbl_description = lv_label_create(cont);
    lv_obj_set_style_text_align(lbl_description, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_text(lbl_description, model_parameter_get_description(pmodel, par));
    lv_obj_align(lbl_description, LV_ALIGN_TOP_LEFT, 8, 8);

    pudata_t data = parameter_get_user_data(par);
    int      type = data.t;

    lv_obj_t *btn = lv_btn_create(cont);
    view_register_object_default_callback_with_number(btn, confirm_id, num);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, LV_SYMBOL_OK);
    lv_obj_set_size(btn, 80, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    btn = lv_btn_create(cont);
    view_register_object_default_callback(btn, cancel_id);
    l = lv_label_create(btn);
    lv_label_set_text(l, LV_SYMBOL_CLOSE);
    lv_obj_set_size(btn, 80, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);

    btn = lv_btn_create(cont);
    view_register_object_default_callback_with_number(btn, default_id, num);
    l = lv_label_create(btn);
    lv_label_set_text(l, view_intl_get_string(pmodel, STRINGS_DEFAULT));
    lv_obj_set_size(btn, 120, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -54);

    *textarea = NULL;

    switch (type) {
        case PTYPE_DROPDOWN: {
            unsigned long max = parameter_get_total_values(par);
            for (unsigned int i = 0; i < max; i++) {
                len = strlen(options);
                if (len > 0) {
                    options[len] = '\n';
                    len++;
                }

                strcpy(&options[len], model_parameter_value_to_string(pmodel, par, i));
            }

            lv_obj_t *list = lv_dropdown_create(cont);
            view_register_object_default_callback_with_number(list, dd_id, num);
            lv_dropdown_set_options(list, options);
            lv_obj_set_width(list, 240);
            lv_dropdown_set_symbol(list, NULL);
            lv_dropdown_set_selected(list, parameter_to_index(par));
            if (max >= 8) {
                // lv_ddlist_set_fix_height(list, 280); style max height
            }

            lv_label_set_long_mode(lbl_description, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lbl_description, LV_PCT(95));

            lv_obj_align_to(list, lbl_description, LV_ALIGN_OUT_BOTTOM_MID, 0, 32);
            break;
        }

        case PTYPE_SWITCH: {
            lv_obj_t *sw = lv_switch_create(cont);
            if (parameter_to_bool(par)) {
                lv_obj_add_state(sw, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(sw, LV_STATE_CHECKED);
            }
            view_register_object_default_callback(sw, sw_id);
            lv_obj_set_size(sw, 120, 40);

            lv_label_set_long_mode(lbl_description, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lbl_description, LV_PCT(95));

            lv_obj_align(sw, LV_ALIGN_BOTTOM_MID, 0, -160);
            break;
        }

        case PTYPE_NUMBER: {
            char      string[64];
            lv_obj_t *ta = lv_textarea_create(cont);
            lv_textarea_set_one_line(ta, 1);
            snprintf(string, 64, "%li", parameter_to_long(par));
            lv_textarea_set_text(ta, string);
            lv_obj_set_size(ta, 180, 40);
            lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
            lv_obj_add_state(ta, LV_STATE_FOCUSED);
            *textarea = ta;

            lv_obj_t *msg = lv_label_create(cont);
            lv_label_set_recolor(msg, 1);
            lv_label_set_text_fmt(msg, "#fF9A9A %s#", view_intl_get_string(pmodel, STRINGS_LIMITE_PARAMETRO));
            lv_label_set_long_mode(msg, LV_LABEL_LONG_SCROLL);
            lv_obj_set_width(msg, LV_PCT(100));
            lv_obj_align_to(msg, ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
            lv_obj_add_flag(msg, LV_OBJ_FLAG_HIDDEN);

            view_register_object_default_callback(ta, ta_id);

            // clang-format off
            static const char *kbmap[] = {"1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
                                          "4", "5", "6", LV_SYMBOL_PLUS, "\n",
                                          "7", "8", "9", LV_SYMBOL_MINUS, "\n",
                                          LV_SYMBOL_LEFT, "0", LV_SYMBOL_RIGHT, "",};
            static const lv_btnmatrix_ctrl_t ctrl_map[] = {1, 1, 1, 1,
                                          1, 1, 1, 1, 
                                          1, 1, 1, 1,
                                          1, 1, 1,};
            // clang-format on

            lv_label_set_long_mode(lbl_description, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_width(lbl_description, LV_PCT(95));

            lv_obj_t *kb = lv_keyboard_create(cont);
            lv_keyboard_set_textarea(kb, ta);
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
            lv_obj_set_size(kb, 240, 210);
            lv_obj_align_to(kb, ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
            lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_NUMBER, kbmap, ctrl_map);
            view_register_keyboard_plus_minus_callback(kb, kb_id);
            break;
        }

#if 0
        case PRICE_PARAMETER: {
            char               string[64];
            lv_obj_user_data_t tadata = {.id = ta_id};
            lv_obj_t          *ta     = lv_ta_create(cont, NULL);
            lv_obj_set_width(ta, 180);
            lv_ta_set_one_line(ta, 1);
            lv_ta_set_max_length(ta, model->prog.parmac.cifre_prezzo + (model->prog.parmac.cifre_decimali_prezzo > 0));
            model_formatta_prezzo(string, model, par_get_uint(pdata));
            lv_ta_set_text(ta, string);
            lv_obj_align(ta, NULL, LV_ALIGN_IN_TOP_MID, 0, 40);
            lv_ta_set_cursor_pos(ta, 0);

            lv_obj_t *msg = lv_label_create(cont, NULL);
            lv_label_set_recolor(msg, 1);
            lv_label_set_text_fmt(msg, "#fF9A9A %s#", get_string(string, STRINGA_LIMITE_PAR, lingua));
            lv_label_set_long_mode(msg, LV_LABEL_LONG_SROLL);
            lv_obj_set_width(msg, lv_obj_get_width_fit(cont));
            lv_obj_align(msg, ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
            lv_obj_set_hidden(msg, 1);

            tadata.extra = msg;
            lv_obj_set_user_data(ta, tadata);
            lv_obj_t *kb = price_keyboard_create(cont, kb_id);
            lv_kb_set_ta(kb, ta);
            lv_obj_set_size(kb, 240, 190);
            lv_obj_align(kb, ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
            break;
        }

#endif
        case PTYPE_TIME: {
            lv_obj_t *roller1 = lv_roller_create(cont);
            lv_obj_t *roller2 = lv_roller_create(cont);

            view_common_roller_set_number(roller1, 60);
            view_common_roller_set_number(roller2, 60);

            lv_roller_set_visible_row_count(roller1, 4);
            lv_roller_set_visible_row_count(roller2, 4);

            lv_obj_set_width(roller1, 80);
            lv_obj_set_width(roller2, 80);

            lv_label_set_long_mode(lbl_description, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lbl_description, LV_PCT(95));

            lv_obj_align(roller1, LV_ALIGN_BOTTOM_MID, -45, -110);
            lv_obj_align(roller2, LV_ALIGN_BOTTOM_MID, 45, -110);

            view_register_object_default_callback(roller1, roller1_id);
            view_register_object_default_callback(roller2, roller2_id);

            unsigned int min = parameter_to_long(par) / 60;
            unsigned int sec = parameter_to_long(par) % 60;

            lv_roller_set_selected(roller1, min, LV_ANIM_OFF);
            lv_roller_set_selected(roller2, sec, LV_ANIM_OFF);
        } break;
    }
    return cont;
}


void view_common_roller_set_number(lv_obj_t *roller, int num) {
    int   cifre  = num / 10 > 3 ? num / 10 : 3;
    char *string = malloc((cifre + 1) * num);
    memset(string, 0, (cifre + 1) * num);

    for (int i = 0; i < num; i++) {
        char tmp[16] = {0};
        if (i == num - 1) {
            snprintf(tmp, sizeof(tmp), "%02i", i);
        } else {
            snprintf(tmp, sizeof(tmp), "%02i\n", i);
        }
        strcat(string, tmp);
    }

    lv_roller_set_options(roller, string, LV_ROLLER_MODE_NORMAL);
    free(string);
}



void view_common_select_btn_in_list(lv_obj_t *list, int selected) {
    for (size_t i = 0; i < lv_obj_get_child_cnt(list); i++) {
        lv_obj_t *btn = lv_obj_get_child(list, i);

        if (btn == NULL) {
            continue;
        }

        if ((size_t)selected == i) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    }
}


lv_obj_t *view_common_icon_button(lv_obj_t *parent, char *symbol, int id) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, symbol);
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    view_register_object_default_callback(btn, id);

    return btn;
}


lv_obj_t *view_common_toast(const char *msg) {
    lv_obj_t *obj = view_common_toast_with_parent(msg, lv_layer_top());
    return obj;
}


lv_obj_t *view_common_io_error_toast(model_t *pmodel) {
    lv_obj_t *obj = view_common_toast_with_parent(view_intl_get_string(pmodel, STRINGS_ERRORE_DISCO), lv_layer_top());
    return obj;
}


lv_obj_t *view_common_toast_with_parent(const char *msg, lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_width(obj, 400);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(obj);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(lbl, msg);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(lbl, 380);
    if (lv_obj_get_height(lbl) > 84) {
        lv_obj_set_height(obj, lv_obj_get_height(lbl) + 20);
    } else {
        lv_obj_set_height(obj, 64);
    }

    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -40);

    lv_timer_t *timer = lv_timer_create(delete_obj_timer, 5000, obj);
    lv_timer_set_repeat_count(timer, 1);

    return obj;
}


lv_obj_t *view_common_create_password_popup(lv_obj_t *parent, int kb_id, int hidden) {
    lv_obj_t *blanket = view_common_create_blanket(parent);

    lv_obj_t *cont = lv_obj_create(blanket);
    lv_obj_set_size(cont, 320, 480);

    lv_obj_t *ta = lv_textarea_create(cont);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_one_line(ta, 1);
    lv_textarea_set_password_mode(ta, hidden);
    lv_textarea_set_max_length(ta, PASSWORD_MAX_SIZE);
    lv_obj_set_height(ta, 56);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *kb = lv_keyboard_create(cont);
    view_register_object_default_callback(kb, kb_id);

    static const char *keys[] = {
        "1", "2", "3", LV_SYMBOL_BACKSPACE, "\n", "4", "5", "6", LV_SYMBOL_CLOSE, "\n",
        "7", "8", "9", LV_SYMBOL_OK,        "\n", "0", "",
    };
    static const lv_btnmatrix_ctrl_t ctrl_map[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_NUMBER, keys, ctrl_map);
    lv_keyboard_set_textarea(kb, ta);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_height(kb, 320);

    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    return blanket;
}


lv_obj_t *view_common_create_simple_image_button(lv_obj_t *parent, const lv_img_dsc_t *src_on,
                                                 const lv_img_dsc_t *src_off, int id) {
    lv_obj_t *btn = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, src_on, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, NULL, src_on, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_DISABLED, NULL, src_off, NULL);
    lv_obj_set_style_img_recolor(btn, lv_color_make(0xFF, 0xFF, 0xFF), LV_STATE_PRESSED);
    lv_obj_set_style_img_recolor_opa(btn, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_set_width(btn, src_on->header.w);
    view_register_object_default_callback(btn, id);

    return btn;
}


lv_obj_t *view_common_create_image_button(lv_obj_t *parent, const lv_img_dsc_t *src, const lv_img_dsc_t *src_pressed,
                                          const lv_img_dsc_t *src_checked, int id) {
    lv_obj_t *btn = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, src, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, NULL, src_pressed, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, src_pressed, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, src_checked, NULL);
    lv_obj_set_width(btn, src->header.w);
    view_register_object_default_callback(btn, id);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CHECKABLE);

    return btn;
}



static void delete_obj_timer(lv_timer_t *timer) {
    lv_obj_del(timer->user_data);
}
