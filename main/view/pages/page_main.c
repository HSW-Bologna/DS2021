#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "view/common.h"
#include "gel/pagemanager/page_manager.h"
#include "gel/timer/timecheck.h"
#include "view/widgets/custom_tabview.h"
#include "view/intl/intl.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"
#include "config/app_conf.h"
#include "utils/system_time.h"
#include "view/theme/style.h"


#define ALARM_DISPLAY_DELAY 2000


LV_IMG_DECLARE(img_background);
LV_IMG_DECLARE(img_gears);
LV_IMG_DECLARE(img_gears_disabled);
LV_IMG_DECLARE(img_gears_press);
LV_IMG_DECLARE(img_play);
LV_IMG_DECLARE(img_play_disabled);
LV_IMG_DECLARE(img_up);
LV_IMG_DECLARE(img_up_disabled);
LV_IMG_DECLARE(img_down);
LV_IMG_DECLARE(img_down_disabled);
LV_IMG_DECLARE(img_pause);
LV_IMG_DECLARE(img_stop);
LV_IMG_DECLARE(img_italiano);
LV_IMG_DECLARE(img_english);
LV_IMG_DECLARE(img_temperature);
LV_IMG_DECLARE(img_drops);
LV_IMG_DECLARE(img_speed);
LV_IMG_DECLARE(img_time);
LV_IMG_DECLARE(img_warn);
LV_IMG_DECLARE(img_menu);
LV_IMG_DECLARE(img_empty);
LV_IMG_DECLARE(img_empty_pressed);
LV_IMG_DECLARE(img_empty_toggled);


enum {
    SETTINGS_BTN_ID,
    RETRY_COMM_BTN_ID,
    DISABLE_COMM_BTN_ID,
    START_BTN_ID,
    STOP_BTN_ID,
    PAUSE_BTN_ID,
    PERIODIC_TIMER_ID,
    LANGUAGE_TIMER_ID,
    LANGUAGE_IMG_ID,
    PROGRAM_UP_BTN_ID,
    PROGRAM_DOWN_BTN_ID,
    PASSWORD_KEYBOARD_ID,
    ALARM_BTN_ID,
    BLANKET_ID,
    MENU_BTN_ID,
    PAR_PLUS_BTN_ID,
    PAR_MINUS_BTN_ID,
    PAR_HUMIDITY_BTN_ID,
    PAR_SPEED_BTN_ID,
    PAR_TEMP_BTN_ID,
    PAR_TIME_BTN_ID,
};


struct page_data {
    lv_obj_t *blanket;
    lv_obj_t *password_blanket;
    lv_obj_t *alarm_blanket;

    lv_obj_t   *lbl_step_num;
    lv_obj_t   *lbl_step_name;
    lv_obj_t   *lbl_temp;
    lv_obj_t   *lbl_hum;
    lv_obj_t   *lbl_status;
    lv_obj_t   *lbl_program_name;
    lv_obj_t   *lbl_time;
    lv_obj_t   *lbl_parametro;
    lv_timer_t *timer;
    lv_timer_t *lang_timer;

    lv_obj_t *btn_menu;
    lv_obj_t *btn_start;
    lv_obj_t *btn_stop;
    lv_obj_t *btn_pause;
    lv_obj_t *btn_up;
    lv_obj_t *btn_down;
    lv_obj_t *img_lingua;
    lv_obj_t *btn_par_temp, *btn_par_time, *btn_par_speed, *btn_par_hum;

    lv_obj_t *cont_menu;

    lv_obj_t             *temp_meter;
    lv_meter_indicator_t *temp_indicator;
    lv_obj_t             *hum_meter;
    lv_meter_indicator_t *hum_indicator;

    alarm_code_t  oldalarm;
    int           selected_prog;
    uint16_t      language;
    unsigned long press_timestamp;
    int           ignore_ui;
    int           menu_visible;

    uint16_t           par_buffer;
    uint16_t          *par_pointer;
    parameter_handle_t par;
    int                checked_menu_button;
};


static const lv_img_dsc_t *languages[NUM_LINGUE] = {
    &img_italiano,
    &img_english,
};


static view_controller_message_code_t parameter_change_code(struct page_data *data) {
    switch (data->checked_menu_button) {
        case PAR_TIME_BTN_ID:
            return VIEW_CONTROLLER_MESSAGE_CODE_CHANGE_REMAINING_TIME;
        case PAR_SPEED_BTN_ID:
            return VIEW_CONTROLLER_MESSAGE_CODE_CHANGE_SPEED;
        case PAR_HUMIDITY_BTN_ID:
            return VIEW_CONTROLLER_MESSAGE_CODE_CHANGE_HUMIDITY;
        case PAR_TEMP_BTN_ID:
            return VIEW_CONTROLLER_MESSAGE_CODE_CHANGE_TEMPERATURE;
        default:
            return VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    }
}


static void update_menu_parameter(struct page_data *data) {
    switch (data->checked_menu_button) {
        case PAR_TIME_BTN_ID:
            if (data->par_pointer == NULL) {
                break;
            }
            lv_label_set_text_fmt(data->lbl_parametro, "%02i:%02i", *data->par_pointer / 60, *data->par_pointer % 60);
            break;

        case PAR_SPEED_BTN_ID:
            if (data->par_pointer == NULL) {
                break;
            }
            lv_label_set_text_fmt(data->lbl_parametro, "%i rpm", *data->par_pointer);
            break;

        case PAR_TEMP_BTN_ID:
            if (data->par_pointer == NULL) {
                break;
            }
            lv_label_set_text_fmt(data->lbl_parametro, "%i C", *data->par_pointer);
            break;

        case PAR_HUMIDITY_BTN_ID:
            if (data->par_pointer == NULL) {
                break;
            }
            lv_label_set_text_fmt(data->lbl_parametro, "%i%%", *data->par_pointer);
            break;

        default:
            lv_label_set_text(data->lbl_parametro, "");
            break;
    }
}


static void update_menu(model_t *pmodel, struct page_data *data, int button) {
    data->checked_menu_button = button;
    pudata_t _udata           = {0};

    if (model_get_current_setpoint_parameter(pmodel) == NULL) {
        lv_obj_add_flag(data->btn_par_temp, LV_OBJ_FLAG_HIDDEN);
        if (button == PAR_TEMP_BTN_ID) {
            button = PAR_TIME_BTN_ID;
        }
    } else {
        lv_obj_clear_flag(data->btn_par_temp, LV_OBJ_FLAG_HIDDEN);
    }

    if (model_get_current_humidity_parameter(pmodel) == NULL) {
        lv_obj_add_flag(data->btn_par_hum, LV_OBJ_FLAG_HIDDEN);
        if (button == PAR_HUMIDITY_BTN_ID) {
            button = PAR_TIME_BTN_ID;
        }
    } else {
        lv_obj_clear_flag(data->btn_par_hum, LV_OBJ_FLAG_HIDDEN);
    }

    if (model_get_current_speed_parameter(pmodel) == NULL) {
        lv_obj_add_flag(data->btn_par_speed, LV_OBJ_FLAG_HIDDEN);
        if (button == PAR_SPEED_BTN_ID) {
            button = PAR_TIME_BTN_ID;
        }
    } else {
        lv_obj_clear_flag(data->btn_par_speed, LV_OBJ_FLAG_HIDDEN);
    }

    switch (button) {
        case PAR_TEMP_BTN_ID:
            data->par_pointer = model_get_current_setpoint_parameter(pmodel);
            data->par         = PARAMETER(data->par_pointer, 0, MAX_TEMPERATURE, 0, _udata, 0);
            lv_obj_add_state(data->btn_par_temp, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_time, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_speed, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_hum, LV_STATE_CHECKED);
            break;

        case PAR_TIME_BTN_ID:
            data->par_buffer  = model_get_remaining(pmodel);
            data->par_pointer = &data->par_buffer;
            data->par         = PARAMETER_STEP(&data->par_buffer, 0, MAX_STEP_DURATION, 0, 5, _udata, 0);
            lv_obj_add_state(data->btn_par_time, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_temp, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_speed, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_hum, LV_STATE_CHECKED);
            break;

        case PAR_SPEED_BTN_ID:
            data->par_pointer = model_get_current_speed_parameter(pmodel);
            data->par         = PARAMETER_DLIMITS(data->par_pointer, model_get_minimum_speed(pmodel),
                                                  model_get_maximum_speed(pmodel), 0, MAX_SPEED, 0, _udata, 0);
            lv_obj_add_state(data->btn_par_speed, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_temp, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_time, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_hum, LV_STATE_CHECKED);
            break;

        case PAR_HUMIDITY_BTN_ID:
            data->par_pointer = model_get_current_humidity_parameter(pmodel);
            data->par         = PARAMETER(data->par_pointer, 0, MAX_HUMIDITY, 0, _udata, 0);
            lv_obj_add_state(data->btn_par_hum, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_temp, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_time, LV_STATE_CHECKED);
            lv_obj_clear_state(data->btn_par_speed, LV_STATE_CHECKED);
            break;

        default:
            break;
    }

    lv_obj_invalidate(data->btn_par_hum);
    lv_obj_invalidate(data->btn_par_temp);
    lv_obj_invalidate(data->btn_par_time);
    lv_obj_invalidate(data->btn_par_speed);

    update_menu_parameter(data);
}


static void update_sensors(model_t *pmodel, struct page_data *data) {
    lv_label_set_text_fmt(data->lbl_temp, "%i C\n%i C", model_current_temperature_setpoint(pmodel),
                          model_current_temperature(pmodel));
    lv_label_set_text_fmt(data->lbl_hum, "%i%%\n%i%%", model_current_humidity_setpoint(pmodel),
                          model_current_humidity(pmodel));
    lv_meter_set_indicator_end_value(data->temp_meter, data->temp_indicator, model_current_temperature(pmodel));
    lv_meter_set_indicator_end_value(data->hum_meter, data->hum_indicator, model_current_humidity(pmodel));
}


static void update_program_data(model_t *pmodel, struct page_data *data) {
    lv_label_set_text_fmt(data->lbl_program_name, "%i - %s", data->selected_prog + 1,
                          model_get_program_name_in_language(pmodel, data->language, data->selected_prog));
    lv_img_set_src(data->img_lingua, languages[data->language]);

    if (model_get_program(pmodel, data->selected_prog)->num_steps == 0) {
        lv_obj_add_state(data->btn_start, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(data->btn_start, LV_STATE_DISABLED);
    }
}


static void update_time(model_t *pmodel, struct page_data *data) {
    uint16_t remaining = model_get_remaining(pmodel);
    uint16_t total     = program_get_total_time(model_get_current_program(pmodel));

    switch (model_get_machine_state(pmodel)) {
        case MACHINE_STATE_STOPPED:
            lv_label_set_text(data->lbl_time, "");
            break;

        case MACHINE_STATE_ACTIVE:
        case MACHINE_STATE_WAIT_START:
        case MACHINE_STATE_PAUSED:
        case MACHINE_STATE_RUNNING:
            lv_label_set_text_fmt(data->lbl_time, "%02i:%02i\n%02i:%02i", total / 60, total % 60, remaining / 60,
                                  remaining % 60);
            break;

        default:
            lv_label_set_text(data->lbl_time, "");
            break;
    }
}


static void update_alarm_popup(model_t *pmodel, struct page_data *data, int force) {
    const strings_t alarm_strings[] = {
        STRINGS_EMERGENZA,
        STRINGS_ALLARME_TEMPERATURA,
        STRINGS_ALLARME_RISCALDAMENTO,
        STRINGS_OBLO_APERTO,
    };

    alarm_code_t code;
    if (model_get_worst_alarm(pmodel, &code, 0)) {
        if (force || data->oldalarm != code) {
            data->oldalarm = code;

            if (data->alarm_blanket != NULL) {
                lv_obj_del(data->alarm_blanket);
            }

            char string[256] = {0};
            snprintf(string, sizeof(string), "%s: %i\n%s", view_intl_get_string(pmodel, STRINGS_CODICE_ALLARME),
                     code + 1, view_intl_get_string(pmodel, alarm_strings[code]));
            lv_obj_t *obj =
                view_common_create_info_popup(lv_scr_act(), 1, &img_warn, string, LV_SYMBOL_OK, ALARM_BTN_ID);
            data->alarm_blanket = lv_obj_get_parent(obj);
        }
    } else {
        data->oldalarm = -1;
    }
}


static void update_step_info(model_t *pmodel, struct page_data *data) {
    parameters_step_t *step = model_get_current_step(pmodel);
    if (step != NULL) {
        lv_label_set_text(data->lbl_step_name, parameters_tipi_step[step->type][model_get_language(pmodel)]);
    } else {
        lv_label_set_text(data->lbl_step_name, "");
    }
    lv_label_set_text_fmt(data->lbl_step_num, "%02zu/%02i", model_get_current_step_number(pmodel) + 1,
                          model_get_current_program(pmodel)->num_steps);
}


static void update_state(model_t *pmodel, struct page_data *data) {
    switch (model_get_machine_state(pmodel)) {
        case MACHINE_STATE_STOPPED:
            if (model_get_num_programs(pmodel) == 0) {
                lv_label_set_text(data->lbl_status,
                                  view_intl_get_string_in_language(data->language, STRINGS_NESSUN_PROGRAMMA));
                lv_obj_add_state(data->btn_start, LV_STATE_DISABLED);
                lv_obj_add_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_label_set_text(data->lbl_status,
                                  view_intl_get_string_in_language(data->language, STRINGS_SCELTA_PROGRAMMA));
                lv_obj_clear_state(data->btn_start, LV_STATE_DISABLED);

                if (model_get_num_programs(pmodel) > 1) {
                    lv_obj_clear_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
                }
            }

            lv_obj_add_flag(data->lbl_step_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->lbl_step_num, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_stop, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_pause, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->btn_start, LV_OBJ_FLAG_HIDDEN);
            break;

        case MACHINE_STATE_ACTIVE: {
            lv_label_set_text(data->lbl_status, "");
            update_step_info(pmodel, data);

            // lv_obj_clear_flag(data->btn_stop, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_clear_flag(data->btn_pause, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->lbl_step_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->lbl_step_num, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(data->btn_start, LV_STATE_DISABLED);
            lv_obj_add_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
            break;
        }

        case MACHINE_STATE_RUNNING: {
            lv_label_set_text(data->lbl_status, "");
            update_step_info(pmodel, data);

            lv_obj_clear_flag(data->btn_stop, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->btn_pause, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->lbl_step_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->lbl_step_num, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_start, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
            break;
        }

        case MACHINE_STATE_PAUSED: {
            lv_label_set_text(data->lbl_status, "");
            update_step_info(pmodel, data);

            lv_obj_clear_flag(data->lbl_step_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->lbl_step_num, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->btn_stop, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_pause, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->btn_start, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(data->btn_start, LV_STATE_DISABLED);
            lv_obj_add_flag(data->btn_up, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->btn_down, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }

    if (model_menu_view_enabled(pmodel)) {
        lv_obj_clear_flag(data->btn_menu, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(data->btn_menu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->cont_menu, LV_OBJ_FLAG_HIDDEN);
        data->menu_visible = 0;
    }
}


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    struct page_data *data = malloc(sizeof(struct page_data));
    data->timer            = view_register_periodic_timer(500, PERIODIC_TIMER_ID);
    data->lang_timer       = view_register_periodic_timer(LANGUAGE_RESET_DELAY, LANGUAGE_TIMER_ID);
    data->selected_prog    = 0;
    data->ignore_ui        = 0;
    data->oldalarm         = -1;
    data->menu_visible     = 0;
    return data;
}


static void open_page(model_t *pmodel, void *arg) {
    struct page_data *data = arg;
    data->blanket          = NULL;
    data->password_blanket = NULL;
    data->alarm_blanket    = NULL;
    data->language         = model_get_language(pmodel);
    lv_timer_resume(data->timer);

    lv_obj_t *background = lv_obj_create(lv_scr_act());
    lv_obj_set_size(background, LV_PCT(100), LV_PCT(100));
    lv_obj_align(background, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(background, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(background, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_background);
    lv_obj_set_style_radius(img, 0, LV_STATE_DEFAULT);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_temperature);
    lv_obj_align(img, LV_ALIGN_BOTTOM_LEFT, 270, -60);

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_obj_align_to(lbl, img, LV_ALIGN_OUT_LEFT_MID, -10, -15);
    data->lbl_temp = lbl;

    img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_drops);
    lv_obj_align(img, LV_ALIGN_TOP_LEFT, 250, 55);

    lbl = lv_label_create(lv_scr_act());
    lv_obj_align_to(lbl, img, LV_ALIGN_OUT_LEFT_MID, -10, -15);
    data->lbl_hum = lbl;

    lv_color_t meter_color = lv_palette_darken(LV_PALETTE_GREY, 2);

    lv_obj_t         *meter = lv_meter_create(lv_scr_act());
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_obj_set_size(meter, 555, 555);
    lv_meter_set_scale_range(meter, scale, 0, 100, 90, 90);
    lv_meter_set_scale_ticks(meter, scale, 10, 4, 20, meter_color);
    lv_meter_set_scale_major_ticks(meter, scale, 9, 5, 30, meter_color, 0);
    lv_meter_indicator_t *temp_indicator = lv_meter_add_arc(meter, scale, 16, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter, temp_indicator, 0);
    lv_obj_align(meter, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    data->temp_meter     = meter;
    data->temp_indicator = temp_indicator;

    if (!model_display_temperature(pmodel)) {
        lv_obj_add_flag(data->temp_meter, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->lbl_temp, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    }

    meter = lv_meter_create(lv_scr_act());
    scale = lv_meter_add_scale(meter);
    lv_obj_set_size(meter, 555, 555);
    lv_meter_set_scale_range(meter, scale, 0, 100, 90, 180);
    lv_meter_set_scale_ticks(meter, scale, 10, 4, 20, meter_color);
    lv_meter_set_scale_major_ticks(meter, scale, 9, 5, 30, meter_color, 0);
    lv_meter_indicator_t *hum_indicator = lv_meter_add_arc(meter, scale, 16, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, hum_indicator, 0);
    lv_obj_align(meter, LV_ALIGN_BOTTOM_LEFT, 20, -37);
    data->hum_meter     = meter;
    data->hum_indicator = hum_indicator;

    if (!model_display_humidity(pmodel)) {
        lv_obj_add_flag(data->hum_meter, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->lbl_hum, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, 440, 320);
    lv_obj_align(cont, LV_ALIGN_CENTER, -155, -4);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont, (lv_style_t *)&style_transparent_container, LV_STATE_DEFAULT);

    lv_obj_t *lbl_bg = lv_obj_create(cont);
    lv_obj_clear_flag(lbl_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(lbl_bg, 160, 48);
    lbl = lv_label_create(lbl_bg);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_obj_align(lbl_bg, LV_ALIGN_CENTER, -80, 0);
    data->lbl_parametro = lbl;

    lv_obj_t *up = view_common_create_simple_image_button(cont, &img_up, &img_up_disabled, PAR_PLUS_BTN_ID);
    lv_obj_align_to(up, lbl_bg, LV_ALIGN_OUT_TOP_MID, 0, -10);

    lv_obj_t *down = view_common_create_simple_image_button(cont, &img_down, &img_down_disabled, PAR_MINUS_BTN_ID);
    lv_obj_align_to(down, lbl_bg, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *hum =
        view_common_create_image_button(cont, &img_empty, &img_empty_pressed, &img_empty_toggled, PAR_HUMIDITY_BTN_ID);
    img = lv_img_create(hum);
    lv_img_set_src(img, &img_drops);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(hum, LV_ALIGN_TOP_RIGHT, -100, 0);
    data->btn_par_hum = hum;

    lv_obj_t *time =
        view_common_create_image_button(cont, &img_empty, &img_empty_pressed, &img_empty_toggled, PAR_TIME_BTN_ID);
    img = lv_img_create(time);
    lv_img_set_src(img, &img_time);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(time, LV_ALIGN_TOP_RIGHT, 10, 0);
    data->btn_par_time = time;

    lv_obj_t *speed =
        view_common_create_image_button(cont, &img_empty, &img_empty_pressed, &img_empty_toggled, PAR_SPEED_BTN_ID);
    img = lv_img_create(speed);
    lv_img_set_src(img, &img_speed);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(speed, LV_ALIGN_BOTTOM_RIGHT, -100, 0);
    data->btn_par_speed = speed;

    lv_obj_t *temp =
        view_common_create_image_button(cont, &img_empty, &img_empty_pressed, &img_empty_toggled, PAR_TEMP_BTN_ID);
    img = lv_img_create(temp);
    lv_img_set_src(img, &img_temperature);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(temp, LV_ALIGN_BOTTOM_RIGHT, 10, 0);
    data->btn_par_temp = temp;

    data->cont_menu = cont;

    lv_obj_t *btn = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &img_gears, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, NULL, &img_gears_press, NULL);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_DISABLED, NULL, &img_gears_disabled, NULL);
    lv_obj_set_width(btn, img_gears.header.w);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    view_register_object_default_callback(btn, SETTINGS_BTN_ID);

    img = lv_img_create(lv_scr_act());
    lv_obj_align(img, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    view_register_object_default_callback(img, LANGUAGE_IMG_ID);
    data->img_lingua = img;

    lbl = lv_label_create(lv_scr_act());
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, -40, 140);
    data->lbl_status = lbl;

    lbl = lv_label_create(lv_scr_act());
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_width(lbl, 400);
    data->lbl_program_name = lbl;

    lbl = lv_label_create(lv_scr_act());
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 20, 40);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_width(lbl, 300);
    data->lbl_step_name = lbl;

    lbl = lv_label_create(lv_scr_act());
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 120, 16);
    data->lbl_step_num = lbl;

    btn = view_common_create_simple_image_button(lv_scr_act(), &img_play, &img_play_disabled, START_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -142, -4);
    data->btn_start = btn;

    btn = view_common_create_simple_image_button(lv_scr_act(), &img_stop, &img_stop, STOP_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -25, -5);
    data->btn_stop = btn;

    btn = view_common_create_simple_image_button(lv_scr_act(), &img_pause, &img_pause, PAUSE_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -142, -4);
    data->btn_pause = btn;

    btn = view_common_create_simple_image_button(lv_scr_act(), &img_up, &img_up_disabled, PROGRAM_UP_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -135, -200);
    data->btn_up = btn;

    btn = view_common_create_simple_image_button(lv_scr_act(), &img_down, &img_down_disabled, PROGRAM_DOWN_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -135, 190);
    data->btn_down = btn;

    lbl            = lv_label_create(lv_scr_act());
    data->lbl_time = lbl;


    btn = view_common_create_simple_image_button(lv_scr_act(), &img_menu, &img_menu, MENU_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, 0);
    data->btn_menu = btn;

    update_state(pmodel, data);
    update_program_data(pmodel, data);
    update_time(pmodel, data);
    update_sensors(pmodel, data);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_SENSORS_CHANGED:
            update_sensors(pmodel, data);
            break;

        case VIEW_EVENT_CODE_STATE_SYNCED:
            data->selected_prog = model_get_current_program_number(pmodel);
            update_program_data(pmodel, data);
            update_state(pmodel, data);
            break;

        case VIEW_EVENT_CODE_STATE_CHANGED:
            update_state(pmodel, data);
            update_alarm_popup(pmodel, data, 0);
            break;

        case VIEW_EVENT_CODE_TIMER:
            switch (event.timer_code) {
                case PERIODIC_TIMER_ID:
                    update_time(pmodel, data);
                    update_alarm_popup(pmodel, data, 0);
                    if (model_get_machine_state(pmodel) != MACHINE_STATE_STOPPED) {
                        lv_timer_reset(data->lang_timer);
                    }
                    break;

                case LANGUAGE_TIMER_ID:
                    data->language = model_get_language(pmodel);
                    lv_timer_reset(data->lang_timer);
                    lv_timer_pause(data->lang_timer);
                    update_program_data(pmodel, data);
                    update_state(pmodel, data);
                    break;
            }
            break;

        case VIEW_EVENT_CODE_LVGL: {
            if (data->ignore_ui) {
                if (event.event == LV_EVENT_RELEASED) {
                    data->ignore_ui = 0;
                }
            } else if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case PAR_PLUS_BTN_ID:
                        parameter_operator(&data->par, +1);
                        update_menu_parameter(data);
                        msg.cmsg.code           = parameter_change_code(data);
                        msg.cmsg.register_value = *data->par_pointer;
                        break;

                    case PAR_MINUS_BTN_ID:
                        parameter_operator(&data->par, -1);
                        update_menu_parameter(data);
                        msg.cmsg.code           = parameter_change_code(data);
                        msg.cmsg.register_value = *data->par_pointer;
                        break;

                    case MENU_BTN_ID:
                        data->menu_visible = !data->menu_visible;
                        if (data->menu_visible) {
                            update_menu(pmodel, data, PAR_TIME_BTN_ID);
                            lv_obj_clear_flag(data->cont_menu, LV_OBJ_FLAG_HIDDEN);
                        } else {
                            lv_obj_add_flag(data->cont_menu, LV_OBJ_FLAG_HIDDEN);
                        }
                        break;

                    case ALARM_BTN_ID:
                        if (data->alarm_blanket) {
                            lv_obj_del(data->alarm_blanket);
                            data->alarm_blanket = NULL;
                        }
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_CLEAR_ALARMS;
                        break;

                    case BLANKET_ID:
                        if (data->password_blanket) {
                            lv_obj_del(data->password_blanket);
                            data->password_blanket = NULL;
                        }
                        break;

                    case LANGUAGE_IMG_ID: {
                        data->language = (data->language + 1) % NUM_LINGUE;
                        update_program_data(pmodel, data);
                        update_state(pmodel, data);
                        lv_timer_reset(data->lang_timer);
                        lv_timer_resume(data->lang_timer);
                        break;
                    }

                    case PROGRAM_UP_BTN_ID:
                        data->selected_prog = (data->selected_prog + 1) % model_get_num_programs(pmodel);
                        update_program_data(pmodel, data);
                        break;

                    case PROGRAM_DOWN_BTN_ID:
                        if (data->selected_prog > 0) {
                            data->selected_prog--;
                        } else {
                            data->selected_prog = model_get_num_programs(pmodel) - 1;
                        }
                        update_program_data(pmodel, data);
                        break;

                    case SETTINGS_BTN_ID: {
                        if (strlen(model_get_password(pmodel)) > 0) {
                            if (data->password_blanket != NULL) {
                                lv_obj_del(data->password_blanket);
                            }
                            data->password_blanket =
                                view_common_create_password_popup(lv_scr_act(), PASSWORD_KEYBOARD_ID, 1);
                            view_register_object_default_callback(data->password_blanket, BLANKET_ID);
                        } else {
                            msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE;
                            msg.vmsg.page = (void *)&page_settings;
                        }
                        break;
                    }

                    case RETRY_COMM_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_TOGGLE_COMMUNICATION;
                        break;

                    case DISABLE_COMM_BTN_ID:
                        model_set_machine_communication(pmodel, 0);
                        lv_obj_del(data->blanket);
                        data->blanket = NULL;
                        break;

                    case START_BTN_ID:
                        if (model_is_any_alarm_active(pmodel)) {
                            update_alarm_popup(pmodel, data, 1);
                        } else {
                            msg.cmsg.code    = VIEW_CONTROLLER_MESSAGE_CODE_START_PROGRAM;
                            msg.cmsg.program = data->selected_prog;
                        }
                        break;

                    case STOP_BTN_ID:
                        break;

                    case PAUSE_BTN_ID:
                        break;
                }
            } else if (event.event == LV_EVENT_RELEASED) {
                switch (event.data.id) {
                    case PAR_TIME_BTN_ID:
                    case PAR_SPEED_BTN_ID:
                    case PAR_HUMIDITY_BTN_ID:
                    case PAR_TEMP_BTN_ID:
                        update_menu(pmodel, data, event.data.id);
                        break;

                    case PAR_MINUS_BTN_ID:
                    case PAR_PLUS_BTN_ID:
                        if (data->par_pointer != NULL) {
                            msg.cmsg.code           = parameter_change_code(data);
                            msg.cmsg.register_value = *data->par_pointer;
                        }
                        break;
                }
            } else if (event.event == LV_EVENT_PRESSED) {
                switch (event.data.id) {
                    case STOP_BTN_ID:
                    case PAUSE_BTN_ID:
                        data->press_timestamp = get_millis();
                        break;
                }
            } else if (event.event == LV_EVENT_PRESSING) {
                switch (event.data.id) {
                    case STOP_BTN_ID:
                        if (is_expired(data->press_timestamp, get_millis(),
                                       model_get_stop_press_time(pmodel) * 1000UL) &&
                            !data->ignore_ui) {
                            msg.cmsg.code   = VIEW_CONTROLLER_MESSAGE_CODE_STOP_MACHINE;
                            data->ignore_ui = 1;
                        }
                        break;

                    case PAUSE_BTN_ID:
                        if (is_expired(data->press_timestamp, get_millis(),
                                       model_get_pause_press_time(pmodel) * 1000UL) &&
                            !data->ignore_ui) {
                            msg.cmsg.code   = VIEW_CONTROLLER_MESSAGE_CODE_PAUSE_MACHINE;
                            data->ignore_ui = 1;
                        }
                        break;
                }
            } else if (event.event == LV_EVENT_LONG_PRESSED_REPEAT) {
                switch (event.data.id) {
                    case PAR_PLUS_BTN_ID:
                        parameter_operator(&data->par, +1);
                        update_menu_parameter(data);
                        break;

                    case PAR_MINUS_BTN_ID:
                        parameter_operator(&data->par, -1);
                        update_menu_parameter(data);
                        break;
                }
            } else if (event.event == LV_EVENT_CANCEL) {
                switch (event.data.id) {
                    case PASSWORD_KEYBOARD_ID:
                        if (data->blanket != NULL) {
                            lv_obj_del(data->blanket);
                            data->blanket = NULL;
                        }
                        break;
                }
            } else if (event.event == LV_EVENT_READY) {
                switch (event.data.id) {
                    case PASSWORD_KEYBOARD_ID: {
                        if (strcmp(event.string_value, model_get_password(pmodel)) == 0 ||
                            strcmp(event.string_value, SKELETON_KEY) == 0) {
                            msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_CHANGE_PAGE;
                            msg.vmsg.page = (void *)&page_settings;
                        } else {
                            view_common_toast(
                                view_intl_get_string_in_language(data->language, STRINGS_PASSWORD_ERRATA));
                            if (data->password_blanket != NULL) {
                                lv_obj_del(data->password_blanket);
                                data->password_blanket = NULL;
                            }
                        }
                        break;
                    }
                }
            }

            break;
        }

        case VIEW_EVENT_CODE_OPEN:
        case VIEW_EVENT_CODE_ALARM:
            if (!model_is_machine_communication_ok(pmodel) && data->blanket == NULL) {
                lv_obj_t *popup = view_common_create_choice_popup(
                    lv_scr_act(), 1, view_intl_get_string_in_language(data->language, STRINGS_ERRORE_DI_COMUNICAZIONE),
                    view_intl_get_string_in_language(data->language, STRINGS_RIPROVA), RETRY_COMM_BTN_ID,
                    view_intl_get_string_in_language(data->language, STRINGS_DISABILITA), DISABLE_COMM_BTN_ID);
                data->blanket = lv_obj_get_parent(popup);
            } else if (data->blanket != NULL) {
                lv_obj_del(data->blanket);
                data->blanket = NULL;
            }
            break;

        default:
            break;
    }

    return msg;
}


static void destroy_page(void *arg, void *extra) {
    struct page_data *data = arg;
    lv_timer_del(data->timer);
    lv_timer_del(data->lang_timer);
    free(data);
    free(extra);
}


static void close_page(void *arg) {
    struct page_data *data = arg;
    lv_timer_pause(data->timer);
    lv_timer_reset(data->lang_timer);
    lv_timer_pause(data->lang_timer);
    lv_obj_clean(lv_scr_act());
}

const pman_page_t page_main = {
    .create        = create_page,
    .close         = close_page,
    .destroy       = destroy_page,
    .process_event = process_page_event,
    .open          = open_page,
};