#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;
    return NULL;
}


static void open_page(pman_handle_t handle, void *state) {
    (void)state;

    pman_timer_t *timer = PMAN_REGISTER_TIMER_ID(handle, 500, 0);
    pman_timer_set_repeat_count(timer, 1);
    pman_timer_resume(timer);
}


static pman_msg_t process_page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_TIMER:
            msg.stack_msg.tag = PMAN_STACK_MSG_TAG_REBASE;
            msg.stack_msg.as.destination.page = (void*)&page_main;
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_splash = {
    .close         = pman_close_all,
    .destroy       = pman_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
    .create        = create_page,
};
