#include "model/model.h"
#include "controller.h"
#include "utils/system_time.h"
#include "lvgl.h"
#include "gel/timer/timecheck.h"
#include "view/view.h"
#include "buzzer.h"


void controller_gui_manage(model_t *pmodel) {
    static unsigned long last_invoked = 0;
    //view_message_t       umsg;
    //view_event_t         event;

    if (last_invoked != get_millis()) {
        if (last_invoked > 0) {
            lv_tick_inc(time_interval(last_invoked, get_millis()));
        }
        last_invoked = get_millis();
    }
    lv_timer_handler();

#if 0
    while (view_get_next_msg(pmodel, &umsg, &event)) {
        if (event.code == VIEW_EVENT_CODE_LVGL && event.event == LV_EVENT_CLICKED) {
            buzzer_beep(1, 50);
        }

        controller_process_msg(&umsg.cmsg, pmodel);
        view_process_msg(umsg.vmsg, pmodel);
    }
#endif
}
