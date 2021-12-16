#include "lvgl.h"
#include "view/view.h"
#include "gel/pagemanager/page_manager.h"


static void *create_page(model_t *pmodel, void *extra) {
    (void)pmodel;
    (void)extra;
    return NULL;
}


static void open_page(model_t *pmodel, void *arg) {
    (void)pmodel;
    (void)arg;


    lv_timer_t *timer = view_register_periodic_timer(500, 0);
    lv_timer_set_repeat_count(timer, 1);
}


static view_message_t process_page_event(model_t *pmodel, void *arg, view_event_t event) {
    view_message_t msg = VIEW_NULL_MESSAGE;

    switch (event.code) {
        case VIEW_EVENT_CODE_TIMER:
            msg.vmsg.code = VIEW_PAGE_MESSAGE_CODE_REBASE;
            msg.vmsg.page = (void *)&page_main;
            break;

        default:
            break;
    }

    return msg;
}



const pman_page_t page_splash = {
    .close         = view_close_all,
    .destroy       = view_destroy_all,
    .process_event = process_page_event,
    .open          = open_page,
};