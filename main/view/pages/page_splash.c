#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"


static void *create_page(lv_pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    return NULL;
}


static void open_page(lv_pman_handle_t handle, void *state) {
    (void)state;

    lv_pman_timer_t *timer = LV_PMAN_REGISTER_TIMER_ID(handle, 500, 0);
    lv_pman_timer_set_repeat_count(timer, 1);
    lv_pman_timer_resume(timer);
}


static lv_pman_msg_t process_page_event(lv_pman_handle_t handle, void *state, lv_pman_event_t event) {
    lv_pman_msg_t msg = LV_PMAN_MSG_NULL;

    switch (event.tag) {
        case LV_PMAN_EVENT_TAG_TIMER:
            msg.vmsg.tag = LV_PMAN_STACK_MSG_TAG_REBASE;
            msg.vmsg.as.destination.page = (void*)&page_main;
            break;

        default:
            break;
    }

    return msg;
}



const lv_pman_page_t page_splash = {
    .close         = lv_pman_close_all,
    .destroy       = lv_pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
