#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "view/view.h"
#include "view/intl/intl.h"
#include "view/common.h"
#include "model/model.h"


enum {
    BACK_BTN_ID,
};


struct page_data {
    lv_obj_t *table;

    view_controller_message_t cmsg;
};


static void *create_page(pman_handle_t handle, void *extra) {
    struct page_data *data = (struct page_data *)malloc(sizeof(struct page_data));
    return data;
}


static char *format_num(char *string, size_t len, uint32_t num) {
    memset(string, 0, len);
    snprintf(string, len, "%i", num);
    return string;
}


static char *format_hms(char *string, size_t len, uint32_t seconds) {
    memset(string, 0, len);
    snprintf(string, len, "%i h %i m %i s", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    return string;
}


static void update_stats(model_t *pmodel, struct page_data *data) {
    char string[32] = {0};
    lv_table_set_cell_value(data->table, 0, 1, format_num(string, sizeof(string), pmodel->statistics.complete_cycles));
    lv_table_set_cell_value(data->table, 1, 1, format_num(string, sizeof(string), pmodel->statistics.partial_cycles));
    lv_table_set_cell_value(data->table, 2, 1, format_hms(string, sizeof(string), pmodel->statistics.active_time));
    lv_table_set_cell_value(data->table, 3, 1, format_hms(string, sizeof(string), pmodel->statistics.work_time));
    lv_table_set_cell_value(data->table, 4, 1, format_hms(string, sizeof(string), pmodel->statistics.rotation_time));
    lv_table_set_cell_value(data->table, 5, 1, format_hms(string, sizeof(string), pmodel->statistics.ventilation_time));
    lv_table_set_cell_value(data->table, 6, 1, format_hms(string, sizeof(string), pmodel->statistics.heating_time));
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *data = state;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    view_common_create_title(lv_scr_act(), view_intl_get_string(pmodel, STRINGS_STATISTICHE), BACK_BTN_ID);

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(88));
    lv_obj_align(page, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *table = lv_table_create(page);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);

    lv_table_set_col_cnt(table, 2);
    lv_table_set_col_width(table, 0, 480);
    lv_table_set_col_width(table, 1, 480);
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_YELLOW), LV_STATE_DEFAULT | LV_PART_ITEMS);

    lv_table_set_cell_value(table, 0, 0, view_intl_get_string(pmodel, STRINGS_CICLI_COMPLETI));
    lv_table_set_cell_value(table, 1, 0, view_intl_get_string(pmodel, STRINGS_CICLI_PARZIALI));
    lv_table_set_cell_value(table, 2, 0, view_intl_get_string(pmodel, STRINGS_TEMPO_ACCESO));
    lv_table_set_cell_value(table, 3, 0, view_intl_get_string(pmodel, STRINGS_TEMPO_DI_LAVORO));
    lv_table_set_cell_value(table, 4, 0, view_intl_get_string(pmodel, STRINGS_TEMPO_IN_MOTO));
    lv_table_set_cell_value(table, 5, 0, view_intl_get_string(pmodel, STRINGS_TEMPO_DI_VENTILAZIONE));
    lv_table_set_cell_value(table, 6, 0, view_intl_get_string(pmodel, STRINGS_TEMPO_IN_RISCALDAMENTO));

    data->table = table;

    update_stats(pmodel, data);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t        msg  = PMAN_MSG_NULL;
    struct page_data *data = state;
    (void)data;

    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = (model_t *)model_updater_get(updater);

    data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_NOTHING;
    msg.user_msg    = &data->cmsg;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            data->cmsg.code = VIEW_CONTROLLER_MESSAGE_CODE_READ_STATISTICS;
            break;

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target  = lv_event_get_current_target(event.as.lvgl);
            view_object_data_t *objdata = lv_obj_get_user_data(target);

            if (lv_event_get_code(event.as.lvgl) == LV_EVENT_CLICKED) {
                switch (objdata->id) {
                    case BACK_BTN_ID:
                        msg.stack_msg.tag = PMAN_STACK_MSG_TAG_BACK;
                        break;
                }
            }
            break;
        }


        case PMAN_EVENT_TAG_USER: {
            view_event_t *user_event = event.as.user;
            switch (user_event->code) {
                case VIEW_EVENT_CODE_STATS_READ:
                    update_stats(pmodel, data);
                    break;
                default:
                    break;
            }
        }

        default:
            break;
    }

    return msg;
}


const pman_page_t page_stats = {
    .create        = create_page,
    .open          = open_page,
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
};
