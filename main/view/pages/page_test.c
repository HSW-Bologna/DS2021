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
};


struct page_data {
    lv_obj_t *blanket;

    struct {
        lv_obj_t *lbl_temp1;
    } temp;

    struct {
        lv_obj_t *table;
    } coin;

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
};


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return (struct page_data *)malloc(sizeof(struct page_data));
}


static void create_tab_input(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    const int sizex = 120, sizey = 80;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    for (size_t i = 0; i < model_get_effective_inputs(pmodel); i++) {
        lv_obj_t *c = lv_obj_create(tab);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_layout(c, LV_LAYOUT_FLEX);
        lv_obj_t *led   = lv_led_create(c);
        lv_obj_t *label = lv_label_create(c);
        lv_label_set_text_fmt(label, "%02zu", i + 1);
        lv_obj_set_size(c, sizex, sizey);
        data->input.leds[i] = led;
        lv_led_off(led);
    }
}


static void create_tab_output(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    const int sizex = 140, sizey = 80;
    data->output.timer   = NULL;
    data->output.current = 0;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t *lbl = lv_label_create(tab);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_color(lbl, lv_color_darken(lv_color_make(0xff, 0, 0), LV_OPA_30), LV_STATE_DEFAULT);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_MACCHINA_IN_FUNZIONE_CONTROLLO_DISABILITATO));
    data->output.lbl_msg = lbl;

    for (size_t i = 0; i < model_get_effective_outputs(pmodel); i++) {
        lv_obj_t *c = lv_obj_create(tab);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *btn = lv_btn_create(c);
        view_register_object_default_callback_with_number(btn, LED_BTN_ID, i);
        lv_obj_t *led   = lv_led_create(c);
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_size(c, sizex, sizey);
        lv_obj_set_size(btn, 60, 20);
        lv_obj_set_height(btn, sizey * 2 / 3);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(led, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_align(btn, LV_ALIGN_LEFT_MID, 0, 0);
        data->output.leds[i] = led;
        data->output.btns[i] = btn;
        lv_led_off(led);

        lv_label_set_text_fmt(label, "%02zu", i + 1);
    }
}


static void create_tab_cesto(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *slider = lv_slider_create(tab);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, -100);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 640, 80);
    view_register_object_default_callback_with_number(slider, PWM_SLIDER_ID, 0);

    slider = lv_slider_create(tab);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 100);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 640, 80);
    view_register_object_default_callback_with_number(slider, PWM_SLIDER_ID, 1);
}


static void create_tab_temp(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *lbl        = lv_label_create(tab);
    data->temp.lbl_temp1 = lbl;
}


static void create_tab_coin(model_t *pmodel, lv_obj_t *tab, struct page_data *data) {
    lv_obj_t *table = lv_table_create(tab);

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
    lv_obj_align_to(btn, table, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}


static void update_sensors(model_t *pmodel, struct page_data *data) {
    lv_label_set_text_fmt(data->temp.lbl_temp1, "%i C (%i)", pmodel->machine.temperature_ptc1,
                          pmodel->machine.adc_ptc1);

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
    for (size_t i = 0; i < model_get_effective_outputs(pmodel); i++) {
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


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;
    data->blanket          = NULL;

    lv_obj_t *tab = custom_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t *in_tab = custom_tabview_add_tab(tab, "Ingressi");
    create_tab_input(pmodel, in_tab, data);

    lv_obj_t *out_tab = custom_tabview_add_tab(tab, "Uscite");
    create_tab_output(pmodel, out_tab, data);

    lv_obj_t *cesto_tab = custom_tabview_add_tab(tab, "Cesto");
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
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_STATE_CHANGED:
            if (!model_is_in_test(pmodel)) {
                msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
            } else {
                update_output_state(pmodel, data);
            }
            break;

        case VIEW_EVENT_CODE_SENSORS_CHANGED:
            update_sensors(pmodel, data);
            break;

        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_EXIT_TEST;
                        break;

                    case CLEAR_COINS_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CLEAR_COINS;
                        break;

                    case LED_BTN_ID: {
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        msg.cmsg.rele = event.data.number;
                        msg.cmsg.value =
                            lv_led_get_brightness(data->output.leds[event.data.number]) <= LV_LED_BRIGHT_MIN;
                        update_output_leds(pmodel, data, event.data.number);
                        break;
                    }

                    case RETRY_COMM_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_RESTART_COMMUNICATION;
                        break;

                    case DISABLE_COMM_BTN_ID:
                        model_set_machine_communication(pmodel, 0);
                        lv_obj_del(data->blanket);
                        data->blanket = NULL;
                        break;
                }
            } else if (event.event == LV_EVENT_RELEASED) {
                switch (event.data.id) {
                    case PWM_SLIDER_ID: {
                        msg.cmsg.code  = VIEW_CONTROLLER_MESSAGE_CODE_TEST_PWM;
                        msg.cmsg.pwm   = event.data.number;
                        msg.cmsg.speed = event.value;
                        break;
                    }
                }
            }
            break;
        }

        case VIEW_EVENT_CODE_TEST_INPUT_VALUES: {
            data->input.inputs = event.digital_inputs;
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

    return msg;
}



const pman_page_t page_test = {
    .create        = create_page,
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};