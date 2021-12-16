#include <unistd.h>
#include "controller.h"
#include "machine/machine.h"
#include "view/view.h"
#include "gel/timer/timecheck.h"
#include "utils/system_time.h"
#include "model/parmac.h"
#include "log.h"


void controller_init(model_t *pmodel) {
    machine_init();
    view_change_page(pmodel, &page_splash);

    /*for(;;) {
        machine_test_rele(0, 1);
        usleep(1000UL*1000UL);
        machine_test_rele(0, 0);
        usleep(1000UL*1000UL);
    }*/
}


void controller_process_msg(view_controller_message_t *msg, model_t *pmodel) {
    switch (msg->code) {
        case VIEW_CONTROLLER_MESSAGE_CODE_NOTHING:
            break;

        case VIEW_CONTROLLER_MESSAGE_TEST_RELE:
            machine_test_rele(msg->rele, msg->value);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_RESTART_COMMUNICATION:
            model_set_machine_communication_error(pmodel, 0);
            machine_restart_communication();
            view_event((view_event_t){.code = VIEW_EVENT_CODE_ALARM});
            break;
    }
}


void controller_manage(model_t *pmodel) {
    static unsigned long inputts = 0;

    if (is_expired(inputts, get_millis(), 500UL)) {
        machine_refresh_inputs();
        machine_refresh_state();
        inputts = get_millis();
    }


    machine_response_message_t msg;
    while (machine_get_response(&msg)) {
        switch (msg.code) {
            case MACHINE_RESPONSE_MESSAGE_CODE_ERROR:
                model_set_machine_communication_error(pmodel, 1);
                view_event((view_event_t){.code = VIEW_EVENT_CODE_ALARM});
                break;

            case MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT:
                view_event((view_event_t){.code = VIEW_EVENT_CODE_TEST_INPUT_VALUES, .digital_inputs = msg.value});
                break;

            case MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE:
                model_set_machine_state(pmodel, msg.state);
                view_event((view_event_t){.code = VIEW_EVENT_CODE_STATE_CHANGED});
                break;
        }
    }
}