#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/widgets/custom_tabview.h"
#include "view/theme/style.h"
#include "view/common.h"
#include "view/intl/intl.h"
#include "../widgets/custom_tabview.h"


enum {
    BACK_BTN_ID,
    RETRY_COMM_BTN_ID,
    DISABLE_COMM_BTN_ID,
    PWM_SLIDER_ID,
    LED_BTN_ID,
    CLEAR_COINS_BTN_ID,
    TAB_ID,
    BTN_VENTILATION_ID,
    BTN_ROTATION_FORWARD_ID,
    BTN_ROTATION_BACKWARD_ID,
};


typedef enum {
    SPEED_TEST_NONE = 0,
    SPEED_TEST_VENTILATION,
    SPEED_TEST_ROTATION_FORWARD,
    SPEED_TEST_ROTATION_BACKWARD,
} speed_test_t;


struct page_data {
    lv_obj_t *blanket;

    struct {
        lv_obj_t *lbl_temp1;
        lv_obj_t *lbl_temp2;
    } temp;

    struct {
        lv_obj_t *table;
    } coin;

    struct {
        lv_obj_t *slider_rotation_speed;
        lv_obj_t *slider_ventilation_speed;
        lv_obj_t *lbl_rotation_speed;
        lv_obj_t *lbl_ventilation_speed;
        lv_obj_t *btn_rotation_forward;
        lv_obj_t *btn_rotation_backward;
        lv_obj_t *btn_ventilation;
        lv_obj_t *led_rotation_forward;
        lv_obj_t *led_rotation_backward;
        lv_obj_t *led_ventilation;


        speed_test_t speed_test;
    } speed;

    struct {
        lv_obj_t   *btns[NUM_RELES];
        lv_obj_t   *leds[NUM_RELES];
        lv_obj_t   *lbl_msg;
        lv_timer_t *timer;
        size_t      current;
    } output;

    struct {
        uint16_t  inputs;
        lv_obj_t *leds[NUM_INPUTS];
    } input;

    view_controller_message_t cmsg;
};


static void update_speed(struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    return (struct page_data *)malloc(sizeof(struct page_data));
}


static void create_tab_input(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    const int sizex = 170, sizey = 80;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    static const char *input_labels[NUM_INTERNAL_INPUTS] = {"IN-V1", "IN-V4", "IN-V3", "IN-V2", "CESTO", "VENTOLA",
                                                            "IN4",   "IN3",   "IN2",   "IN1",   "TERM",  "C-GAS"};

    for (size_t i = 0; i < model_get_effective_inputs(pmodel); i++) {
        lv_obj_t *c = lv_obj_create(tab);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_layout(c, LV_LAYOUT_FLEX);
        lv_obj_t *led   = lv_led_create(c);
        lv_obj_t *label = lv_label_create(c);
        lv_label_set_text(label, input_labels[i]);
        lv_obj_set_size(c, sizex, sizey);
        data->input.leds[i] = led;
        lv_led_off(led);
    }
}


static void create_tab_output(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    const int sizex = 150, sizey = 110;
    data->output.timer   = NULL;
    data->output.current = 0;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t *lbl = lv_label_create(tab);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_color(lbl, lv_color_darken(lv_color_make(0xff, 0, 0), LV_OPA_30), LV_STATE_DEFAULT);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_MACCHINA_IN_FUNZIONE_CONTROLLO_DISABILITATO));
    data->output.lbl_msg = lbl;

    const char *output_descriptions[NUM_INTERNAL_RELES] = {
        "Reset GAS", "ON Gas", "Avanti", "Indietro", "Ventola", "Macc. ON", "Risc. 1", "Risc. 2", "Vapore",
    };

    for (size_t i = 0; i < NUM_INTERNAL_RELES; i++) {
        lv_obj_t *c = lv_obj_create(tab);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *btn = lv_btn_create(c);
        view_register_object_default_callback_with_number(btn, LED_BTN_ID, i);
        lv_obj_t *led   = lv_led_create(c);
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_size(c, sizex, sizey);
        lv_obj_set_size(btn, 60, 40);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(led, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, -10);


        data->output.leds[i] = led;
        data->output.btns[i] = btn;
        lv_led_off(led);

        lv_label_set_text_fmt(label, "%02zu", i + 1);

        label = lv_label_create(c);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 10);
        lv_label_set_text(label, output_descriptions[i]);
    }
}


static void create_tab_cesto(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *slider, *lbl, *btn, *led;

    slider = lv_slider_create(tab);
    lv_obj_align(slider, LV_ALIGN_LEFT_MID, 20, -100);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 480, 70);
    view_register_object_default_callback_with_number(slider, PWM_SLIDER_ID, 1);
    data->speed.slider_rotation_speed = slider;

    lbl = lv_label_create(tab);
    lv_label_set_text(lbl, "Cesto");
    lv_obj_align_to(lbl, slider, LV_ALIGN_TOP_MID, 0, -32);

    lbl = lv_label_create(tab);
    lv_obj_align_to(lbl, slider, LV_ALIGN_BOTTOM_MID, 0, 32);
    data->speed.lbl_rotation_speed = lbl;

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 80, 80);
    view_register_object_default_callback(btn, BTN_ROTATION_BACKWARD_ID);
    lv_obj_align_to(btn, slider, LV_ALIGN_OUT_RIGHT_MID, 60, 0);
    data->speed.btn_rotation_backward = btn;

    lbl = lv_label_create(tab);
    lv_label_set_text(lbl, "Indietro");
    lv_obj_align_to(lbl, btn, LV_ALIGN_TOP_MID, 0, -40);

    led = lv_led_create(btn);
    lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(led);
    data->speed.led_rotation_backward = led;

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 80, 80);
    view_register_object_default_callback(btn, BTN_ROTATION_FORWARD_ID);
    lv_obj_align_to(btn, data->speed.btn_rotation_backward, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    data->speed.btn_rotation_forward = btn;

    lbl = lv_label_create(tab);
    lv_label_set_text(lbl, "Avanti");
    lv_obj_align_to(lbl, btn, LV_ALIGN_TOP_MID, 0, -40);

    led = lv_led_create(btn);
    lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(led);
    data->speed.led_rotation_forward = led;


    slider = lv_slider_create(tab);
    lv_obj_align(slider, LV_ALIGN_LEFT_MID, 20, 100);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 480, 70);
    view_register_object_default_callback_with_number(slider, PWM_SLIDER_ID, 0);
    data->speed.slider_ventilation_speed = slider;

    lbl = lv_label_create(tab);
    lv_label_set_text(lbl, "Ventilazione");
    lv_obj_align_to(lbl, slider, LV_ALIGN_TOP_MID, 0, -32);

    lbl = lv_label_create(tab);
    lv_obj_align_to(lbl, slider, LV_ALIGN_BOTTOM_MID, 0, 32);
    data->speed.lbl_ventilation_speed = lbl;

    btn = lv_btn_create(tab);
    lv_obj_set_size(btn, 80, 80);
    view_register_object_default_callback(btn, BTN_VENTILATION_ID);
    lv_obj_align_to(btn, slider, LV_ALIGN_OUT_RIGHT_MID, 60, 0);
    data->speed.btn_ventilation = btn;

    led = lv_led_create(btn);
    lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(led);
    data->speed.led_ventilation = led;
}


static void create_tab_temp(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *lbl = lv_label_create(tab);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -50);
    data->temp.lbl_temp1 = lbl;

    lbl = lv_label_create(tab);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 50);
    data->temp.lbl_temp2 = lbl;
}


static void create_tab_coin(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *table = lv_table_create(tab);

    lv_table_set_col_width(table, 0, 200);
    lv_table_set_col_width(table, 1, 150);
    for (size_t i = 0; i < COIN_LINES; i++) {
        char string[64] = {0};
        snprintf(string, sizeof(string), "%s %zu", view_intl_get_string(pmodel, STRINGS_GETTONE), i + 1);
        lv_table_set_cell_value(table, i, 0, string);
        lv_table_set_cell_value(table, i, 1, "0");
    }
    lv_table_set_cell_value(table, COIN_LINES, 0, view_intl_get_string(pmodel, STRINGS_CASSA));
    lv_table_set_cell_value(table, COIN_LINES, 1, "0");

    lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
    data->coin.table = table;

    lv_obj_t *btn = lv_btn_create(tab);
    view_register_object_default_callback(btn, CLEAR_COINS_BTN_ID);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_AZZERA));
    lv_obj_align_to(btn, table, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}


static void update_sensors(model_t *pmodel, struct page_data *data) {
    lv_label_set_text_fmt(data->temp.lbl_temp1, "Temp. 1: %i C (%i)", pmodel->machine.temperature_ptc1,
                          pmodel->machine.adc_ptc1);
    lv_label_set_text_fmt(data->temp.lbl_temp2, "Temp. 2: %i C (%i)", pmodel->machine.temperature_ptc2,
                          pmodel->machine.adc_ptc2);

    for (size_t i = 0; i < COIN_LINES; i++) {
        char string[64] = {0};
        snprintf(string, sizeof(string), "%i", pmodel->machine.coins[i]);
        lv_table_set_cell_value(data->coin.table, i, 1, string);
    }

    char string[64] = {0};
    snprintf(string, sizeof(string), "%i", pmodel->machine.payment);
    lv_table_set_cell_value(data->coin.table, COIN_LINES, 1, string);
}


static void update_input_leds(model_t *pmodel, struct page_data *data) {
    for (size_t i = 0; i < model_get_effective_inputs(pmodel); i++) {
        if ((data->input.inputs & (1 << i)) > 0) {
            lv_led_on(data->input.leds[i]);
        } else {
            lv_led_off(data->input.leds[i]);
        }
    }
}


static void update_output_leds(model_t *pmodel, struct page_data *data, int led_on) {
    for (size_t i = 0; i < NUM_INTERNAL_RELES; i++) {
        if (led_on == (int)i) {
            lv_led_toggle(data->output.leds[i]);
        } else {
            lv_led_off(data->output.leds[i]);
        }
    }
}


static void update_output_state(model_t *pmodel, struct page_data *data) {
    if (model_is_program_running(pmodel)) {
        lv_obj_clear_flag(data->output.lbl_msg, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < model_get_effective_outputs(pmodel); i++) {
            lv_obj_clear_flag(data->output.btns[i], LV_OBJ_FLAG_CLICKABLE);
        }
    } else {
        lv_obj_add_flag(data->output.lbl_msg, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < model_get_effective_outputs(pmodel); i++) {
            lv_obj_add_flag(data->output.btns[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }
}


static void open_page(pman_handle_t handle, void *state) {
    (void)handle;
    struct page_data *data = state;
    data->blanket          = NULL;
    data->speed.speed_test = SPEED_TEST_NONE;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    lv_obj_t *tab = custom_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
    view_register_object_default_callback(tab, TAB_ID);

    lv_obj_t *in_tab = custom_tabview_add_tab(tab, "Ingressi");
    create_tab_input(pmodel, in_tab, data);

    lv_obj_t *out_tab = custom_tabview_add_tab(tab, "Uscite");
    create_tab_output(pmodel, out_tab, data);

    lv_obj_t *cesto_tab = custom_tabview_add_tab(tab, "Velocita'");
    create_tab_cesto(pmodel, cesto_tab, data);

    lv_obj_t *temp_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_TEMPERATURA));
    create_tab_temp(pmodel, temp_tab, data);

    lv_obj_t *coin_tab = custom_tabview_add_tab(tab, view_intl_get_string(pmodel, STRINGS_PAGAMENTO));
    create_tab_coin(pmodel, coin_tab, data);

    view_common_back_button(lv_scr_act(), BACK_BTN_ID);

    update_input_leds(pmodel, data);
    update_output_leds(pmodel, data, -1);
    update_output_state(pmodel, data);
    update_sensors(pmodel, data);
    update_speed(data);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = state;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *user_event = event.as.user;
            switch (user_event->code) {
                case VIEW_EVENT_CODE_STATE_CHANGED:
                    if (!model_is_in_test(pmodel)) {
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                    } else {
                        update_output_state(pmodel, data);
                    }
                    break;

                case VIEW_EVENT_CODE_SENSORS_CHANGED:
                    update_sensors(pmodel, data);
                    break;

                case VIEW_EVENT_CODE_TEST_INPUT_VALUES: {
                    data->input.inputs = user_event->digital_inputs;
                    update_input_leds(pmodel, data);
                    break;
                }

                case VIEW_EVENT_CODE_OPEN:
                case VIEW_EVENT_CODE_ALARM:
                    if (!model_is_machine_communication_ok(pmodel) && data->blanket == NULL) {
                        lv_obj_t *popup = view_common_create_choice_popup(
                            lv_scr_act(), 1, view_intl_get_string(pmodel, STRINGS_ERRORE_DI_COMUNICAZIONE),
                            view_intl_get_string(pmodel, STRINGS_RIPROVA), RETRY_COMM_BTN_ID,
                            view_intl_get_string(pmodel, STRINGS_DISABILITA), DISABLE_COMM_BTN_ID);
                        data->blanket = lv_obj_get_parent(popup);
                    } else if (data->blanket != NULL) {
                        lv_obj_del(data->blanket);
                        data->blanket = NULL;
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
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXIT_TEST;
                        break;

                    case CLEAR_COINS_BTN_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CLEAR_COINS;
                        break;

                    case LED_BTN_ID: {
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        data->cmsg.rele = objdata->number;
                        data->cmsg.value =
                            lv_led_get_brightness(data->output.leds[objdata->number]) <= LV_LED_BRIGHT_MIN;
                        update_output_leds(pmodel, data, objdata->number);
                        break;
                    }

                    case BTN_ROTATION_BACKWARD_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        data->cmsg.rele = 2;
                        if (data->speed.speed_test == SPEED_TEST_ROTATION_BACKWARD) {
                            data->speed.speed_test = SPEED_TEST_NONE;
                            data->cmsg.value       = 0;
                        } else {
                            data->speed.speed_test = SPEED_TEST_ROTATION_BACKWARD;
                            data->cmsg.value       = 1;
                        }
                        update_speed(data);
                        break;

                    case BTN_ROTATION_FORWARD_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        data->cmsg.rele = 3;
                        if (data->speed.speed_test == SPEED_TEST_ROTATION_FORWARD) {
                            data->speed.speed_test = SPEED_TEST_NONE;
                            data->cmsg.value       = 0;
                        } else {
                            data->speed.speed_test = SPEED_TEST_ROTATION_FORWARD;
                            data->cmsg.value       = 1;
                        }
                        update_speed(data);
                        break;

                    case BTN_VENTILATION_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        data->cmsg.rele = 4;
                        if (data->speed.speed_test == SPEED_TEST_VENTILATION) {
                            data->speed.speed_test = SPEED_TEST_NONE;
                            data->cmsg.value       = 0;
                        } else {
                            data->speed.speed_test = SPEED_TEST_VENTILATION;
                            data->cmsg.value       = 1;
                        }
                        update_speed(data);
                        break;

                    case RETRY_COMM_BTN_ID:
                        data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_TOGGLE_COMMUNICATION;
                        break;

                    case DISABLE_COMM_BTN_ID:
                        model_set_machine_communication(pmodel, 0);
                        lv_obj_del(data->blanket);
                        data->blanket = NULL;
                        break;
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_RELEASED) {
                switch (objdata->id) {
                    case PWM_SLIDER_ID: {
                        data->cmsg.code  = VIEW_CONTROLLER_MESSAGE_CODE_TEST_PWM;
                        data->cmsg.pwm   = objdata->number;
                        data->cmsg.speed = lv_slider_get_value(target);
                        update_speed(data);
                        break;
                    }
                }
            } else if (lv_event_get_code(event.as.lvgl) == LV_EVENT_VALUE_CHANGED) {
                switch (objdata->id) {
                    case TAB_ID: {
                        data->cmsg.code  = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        data->cmsg.rele  = objdata->number;
                        data->cmsg.value = 0;
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


static void update_speed(struct page_data *pdata) {
    lv_label_set_text_fmt(pdata->speed.lbl_rotation_speed, "%i %%",
                          lv_slider_get_value(pdata->speed.slider_rotation_speed));
    lv_label_set_text_fmt(pdata->speed.lbl_ventilation_speed, "%i %%",
                          lv_slider_get_value(pdata->speed.slider_ventilation_speed));

    switch (pdata->speed.speed_test) {
        case SPEED_TEST_NONE:
            lv_obj_clear_state(pdata->speed.btn_rotation_backward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_rotation_forward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_ventilation, LV_STATE_CHECKED);
            lv_led_off(pdata->speed.led_rotation_backward);
            lv_led_off(pdata->speed.led_rotation_forward);
            lv_led_off(pdata->speed.led_ventilation);
            break;

        case SPEED_TEST_VENTILATION:
            lv_obj_clear_state(pdata->speed.btn_rotation_backward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_rotation_forward, LV_STATE_CHECKED);
            lv_obj_add_state(pdata->speed.btn_ventilation, LV_STATE_CHECKED);
            lv_led_off(pdata->speed.led_rotation_backward);
            lv_led_off(pdata->speed.led_rotation_forward);
            lv_led_on(pdata->speed.led_ventilation);
            break;

        case SPEED_TEST_ROTATION_FORWARD:
            lv_obj_clear_state(pdata->speed.btn_rotation_backward, LV_STATE_CHECKED);
            lv_obj_add_state(pdata->speed.btn_rotation_forward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_ventilation, LV_STATE_CHECKED);
            lv_led_off(pdata->speed.led_rotation_backward);
            lv_led_on(pdata->speed.led_rotation_forward);
            lv_led_off(pdata->speed.led_ventilation);
            break;

        case SPEED_TEST_ROTATION_BACKWARD:
            lv_obj_add_state(pdata->speed.btn_rotation_backward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_rotation_forward, LV_STATE_CHECKED);
            lv_obj_clear_state(pdata->speed.btn_ventilation, LV_STATE_CHECKED);
            lv_led_on(pdata->speed.led_rotation_backward);
            lv_led_off(pdata->speed.led_rotation_forward);
            lv_led_off(pdata->speed.led_ventilation);
            break;
    }
}


const pman_page_t page_test = {
    .create        = create_page,
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};
