#if 0
#include <stdio.h>
#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"
#include "view/common.h"
#include "view/intl/intl.h"


enum {
    BACK_BTN_ID,
    WIFI_BTN_ID,
    PASSWORD_KB_ID,
    SCAN_TIMER_ID,
    SAVE_BTN_ID,
};


struct page_data {
    lv_obj_t *lwifi;
    lv_obj_t *leth;
    lv_obj_t *lstate;
    lv_obj_t *netlist;
    lv_obj_t *kb;

    size_t      selected_network;
    lv_timer_t *timer;
};


static lv_obj_t *create_password_kb(lv_obj_t *root, model_t *pmodel) {
    lv_obj_t *blanket = view_common_create_blanket(lv_scr_act());
    lv_obj_t *kb      = lv_keyboard_create(blanket);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *ta = lv_textarea_create(blanket);
    lv_obj_set_size(ta, LV_PCT(70), 56);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 120);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_password_mode(ta, 1);
    lv_textarea_set_placeholder_text(ta, view_intl_get_string(pmodel, STRINGS_PASSWORD));
    lv_textarea_set_max_length(ta, 32);
    lv_textarea_set_one_line(ta, 1);
    lv_keyboard_set_textarea(kb, ta);
    view_register_object_default_callback(kb, PASSWORD_KB_ID);

    return blanket;
}


static lv_obj_t *update_network_list(struct page_data *data, wifi_network_t *networks, int num) {
    lv_obj_t *list = data->netlist;

    while (lv_obj_get_child_cnt(list)) {
        lv_obj_del(lv_obj_get_child(list, 0));
    }

    lv_obj_set_size(list, 480, 380);
    lv_obj_align(list, LV_ALIGN_TOP_RIGHT, -8, 80);

    for (int i = 0; i < num; i++) {
        char str[128];
        snprintf(str, sizeof(str), "     %s", networks[i].ssid);
        // lv_obj_t *btn = lv_list_add_btn(list, get_wifi_icon(networks[i].signal), str);
        lv_obj_t *btn = lv_list_add_btn(list, NULL, str);
        view_register_object_default_callback_with_number(btn, WIFI_BTN_ID, i);
    }

    return list;
}


static void update_data(model_t *pmodel, struct page_data *data) {
    lv_label_set_text_fmt(data->lwifi, "WiFi - %s", pmodel->system.wifi_ipaddr);
    lv_label_set_text_fmt(data->leth, "Ethernet - %s", pmodel->system.eth_ipaddr);
    switch (pmodel->system.net_status) {
        case WIFI_INACTIVE:
            lv_label_set_text(data->lstate, view_intl_get_string(pmodel, STRINGS_NON_CONNESSO));
            break;

        case WIFI_CONNECTED:
            lv_label_set_text_fmt(data->lstate, "%s %s", view_intl_get_string(pmodel, STRINGS_CONNESSO_A),
                                  pmodel->system.ssid);
            break;

        case WIFI_CONNECTING:
            lv_label_set_text(data->lstate, view_intl_get_string(pmodel, STRINGS_CONNESSIONE_IN_CORSO));
            break;

        default:
            break;
    }
}


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    struct page_data *data = malloc(sizeof(struct page_data));
    data->timer            = view_register_periodic_timer(10000UL, SCAN_TIMER_ID);
    return data;
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    struct page_data *data = arg;

    view_common_create_title(lv_scr_act(), view_intl_get_string(pmodel, STRINGS_IMPOSTAZIONI_DI_RETE), BACK_BTN_ID);

    data->netlist = lv_list_create(lv_scr_act());

    data->lwifi = lv_label_create(lv_scr_act());
    lv_obj_align(data->lwifi, LV_ALIGN_TOP_LEFT, 8, 80);

    data->lstate = lv_label_create(lv_scr_act());
    lv_obj_align_to(data->lstate, data->lwifi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    data->leth = lv_label_create(lv_scr_act());
    lv_obj_align_to(data->leth, data->lwifi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);

    lv_obj_t *btn = view_common_icon_button(lv_scr_act(), LV_SYMBOL_SAVE, SAVE_BTN_ID);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    update_network_list(data, pmodel->system.networks, pmodel->system.num_networks);
    update_data(pmodel, data);
    lv_timer_resume(data->timer);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t    msg  = VIEW_NULL_MESSAGE;
    struct page_data *data = arg;

    switch (event.code) {
        case VIEW_EVENT_CODE_TIMER:
            msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_WIFI_SCAN;
            break;

        case VIEW_EVENT_CODE_WIFI:
            update_network_list(data, pmodel->system.networks, pmodel->system.num_networks);
            update_data(pmodel, data);
            break;

        case VIEW_EVENT_CODE_IO_DONE:
            if (event.error) {
                view_common_io_error_toast(pmodel);
            } else {
                view_common_toast(view_intl_get_string(pmodel, STRINGS_CONFIGURAZIONE_SALVATA));
            }
            break;

        case VIEW_EVENT_CODE_LVGL:
            if (event.event == LV_EVENT_CLICKED) {
                switch (event.data.id) {
                    case BACK_BTN_ID:
                        msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_BACK;
                        break;

                    case WIFI_BTN_ID:
                        data->selected_network = event.data.number;
                        data->kb               = create_password_kb(lv_scr_act(), pmodel);
                        break;

                    case SAVE_BTN_ID:
                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_SAVE_WIFI_CONFIG;
                        break;

                    default:
                        break;
                }
            } else if (event.event == LV_EVENT_CANCEL) {
                switch (event.data.id) {
                    case PASSWORD_KB_ID:
                        lv_obj_del(data->kb);
                        data->kb = NULL;
                        break;
                }
            } else if (event.data.id) {
                switch (event.data.id) {
                    case PASSWORD_KB_ID:
                        lv_obj_del(data->kb);
                        data->kb = NULL;

                        msg.cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_CONNECT_TO_WIFI_NETWORK;
                        strcpy(msg.cmsg.ssid, pmodel->system.networks[data->selected_network].ssid);
                        strcpy(msg.cmsg.psk, event.string_value);
                        break;
                }
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
    free(data);
}


static void close_page(void *arg) {
    struct page_data *data = arg;
    lv_timer_pause(data->timer);
    lv_obj_clean(lv_scr_act());
}


const pman_page_t page_network = {
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
    .close         = close_page,
    .destroy       = destroy_page,
};
#endif