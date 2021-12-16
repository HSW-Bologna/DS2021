#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/widgets/custom_tabview.h"
#include "view/theme/style.h"
#include "view/common.h"
#include "view/intl/intl.h"


enum {
    LED_BTN_ID,
    BACK_BTN_ID,
    RETRY_COMM_BTN_ID,
    DISABLE_COMM_BTN_ID,
};


struct page_data {
    lv_obj_t *blanket;

    struct {
        lv_obj_t   *leds[NUM_RELES];
        lv_timer_t *timer;
        size_t      current;
    } output;

    struct {
        uint8_t   inputs;
        lv_obj_t *leds[NUM_INPUTS];
    } input;
};


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return (struct page_data *)malloc(sizeof(struct page_data));
}


static void create_tab_input(lv_obj_t *tab, struct page_data *data) {
    const int sizex = 120, sizey = 80;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    for (int i = 0; i < NUM_INPUTS; i++) {
        lv_obj_t *c = lv_obj_create(tab);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_layout(c, LV_LAYOUT_FLEX);
        lv_obj_t *led   = lv_led_create(c);
        lv_obj_t *label = lv_label_create(c);
        lv_label_set_text_fmt(label, "%02i", i + 1);
        lv_obj_set_size(c, sizex, sizey);
        data->input.leds[i] = led;
        lv_led_off(led);
    }
}


static void create_tab_output(lv_obj_t *tab, struct page_data *data) {
    const int sizex = 140, sizey = 80;
    data->output.timer   = NULL;
    data->output.current = 0;
    lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW_WRAP);

    for (int i = 0; i < NUM_RELES; i++) {
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
        lv_led_off(led);

        lv_label_set_text_fmt(label, "%02i", i + 1);
    }
}


void update_input_leds(struct page_data *data) {
    for (size_t i = 0; i < NUM_INPUTS; i++) {
        if ((data->input.inputs & (1 << i)) > 0) {
            lv_led_on(data->input.leds[i]);
        } else {
            lv_led_off(data->input.leds[i]);
        }
    }
}


void update_output_leds(struct page_data *data, int led_on) {
    for (size_t i = 0; i < NUM_RELES; i++) {
        if (led_on == (int)i) {
            lv_led_toggle(data->output.leds[i]);
        } else {
            lv_led_off(data->output.leds[i]);
        }
    }
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;
    data->blanket          = NULL;

    lv_obj_t *tab = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t *in_tab = lv_tabview_add_tab(tab, "Ingressi");
    create_tab_input(in_tab, data);

    lv_obj_t *out_tab = lv_tabview_add_tab(tab, "Uscite");
    create_tab_output(out_tab, data);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_add_style(btn, &style_icon_button, LV_STATE_DEFAULT);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(btn, BACK_BTN_ID);

    update_input_leds(data);
    update_output_leds(data, -1);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_LVGL: {
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID: {
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;
                    }

                    case LED_BTN_ID: {
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_TEST_RELE;
                        msg.cmsg.rele = event.data.number;
                        msg.cmsg.value =
                            lv_led_get_brightness(data->output.leds[event.data.number]) <= LV_LED_BRIGHT_MIN;
                        update_output_leds(data, event.data.number);
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
            }
            break;
        }

        case VIEW_EVENT_CODE_TEST_INPUT_VALUES: {
            data->input.inputs = event.digital_inputs;
            update_input_leds(data);
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