#include <unistd.h>
#include "controller.h"
#include "machine/machine.h"
#include "view/view.h"
#include "gel/timer/timecheck.h"
#include "utils/system_time.h"
#include "model/parmac.h"
#include "storage/disk_op.h"
#include "storage/storage.h"
#include "model/parciclo.h"
#include "controller/network/wifi.h"
#include "config/app_conf.h"
#include "log.h"
#include "buzzer.h"


static void load_parmac_callback(model_t *pmodel, void *data, void *arg);
static void load_parmac_error_callback(model_t *pmodel, void *arg);
static void disk_io_callback(model_t *pmodel, void *data, void *arg);
static void disk_io_error_callback(model_t *pmodel, void *arg);
static void load_programs_callback(model_t *pmodel, void *data, void *arg);
static void load_password_callback(model_t *pmodel, void *data, void *arg);


static int pending_start = 0;


void controller_init(model_t *pmodel) {
    buzzer_init();
    wifi_init();
    machine_init();
    disk_op_init();

    disk_op_load_parmac(load_parmac_callback, load_parmac_error_callback, NULL);
    while (disk_op_manage_response(pmodel) == 0) {
        usleep(1000);
    }
    disk_op_load_programs(load_programs_callback, NULL, NULL);
    while (disk_op_manage_response(pmodel) == 0) {
        usleep(1000);
    }

    disk_op_read_file(DEFAULT_PATH_FILE_PASSWORD, load_password_callback, NULL, NULL);
    while (disk_op_manage_response(pmodel) == 0) {
        usleep(1000);
    }

    machine_read_version();
    machine_send_parmac(&pmodel->configuration.parmac);
    machine_send_command(COMMAND_REGISTER_EXIT_TEST);
    buzzer_beep(2, 500);
    view_change_page(pmodel, &page_splash);
}


void controller_process_msg(view_controller_message_t *msg, model_t *pmodel) {
    switch (msg->code) {
        case VIEW_CONTROLLER_MESSAGE_CODE_NOTHING:
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_READ_LOG_FILE:
            disk_op_read_file(LOGFILE, disk_io_callback, disk_io_error_callback, NULL);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_CLEAR_ALARMS:
            machine_send_command(COMMAND_REGISTER_CLEAR_ALARMS);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_START_PROGRAM:
            if (!model_is_program_running(pmodel)) {
                model_start_program(pmodel, msg->program);
                machine_send_step(model_get_current_step(pmodel), model_get_current_program_number(pmodel),
                                  model_get_current_step_number(pmodel), 1);
                pending_start = 1;
            } else {
                machine_send_command(COMMAND_REGISTER_RUN_STEP);
            }
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_STOP_MACHINE:
            model_stop_program(pmodel);
            machine_send_command(COMMAND_REGISTER_STOP);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_PAUSE_MACHINE:
            machine_send_command(COMMAND_REGISTER_PAUSE);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_ENTER_TEST:
            machine_send_command(COMMAND_REGISTER_ENTER_TEST);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_EXIT_TEST:
            machine_send_command(COMMAND_REGISTER_EXIT_TEST);
            break;

        case VIEW_CONTROLLER_MESSAGE_TEST_RELE:
            machine_test_rele(msg->rele, msg->value);
            break;

        case VIEW_CONTROLLER_MESSAGE_CLEAR_COINS:
            machine_send_command(COMMAND_REGISTER_CLEAR_COINS);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_TEST_PWM:
            machine_test_pwm(msg->pwm, msg->speed);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_RESTART_COMMUNICATION:
            model_set_machine_communication_error(pmodel, 0);
            machine_restart_communication();
            view_event((view_event_t){.code = VIEW_EVENT_CODE_ALARM});
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_REMOVE_PROGRAM:
            disk_op_remove_program(msg->name, disk_io_callback, disk_io_error_callback, NULL);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_SAVE_ALL:
            if (msg->password) {
                disk_op_save_password((char *)model_get_password(pmodel), disk_io_callback, disk_io_error_callback,
                                      (void *)(uintptr_t)msg->save_all_io_op);
            }
            if (msg->parmac) {
                machine_send_parmac(&pmodel->configuration.parmac);
                disk_op_save_parmac(&pmodel->configuration.parmac, disk_io_callback, disk_io_error_callback,
                                    (void *)(uintptr_t)msg->save_all_io_op);
            }
            if (msg->index) {
                disk_op_save_program_index(pmodel, disk_io_callback, disk_io_error_callback,
                                           (void *)(uintptr_t)msg->save_all_io_op);
            }
            for (size_t i = 0; i < MAX_PROGRAMS; i++) {
                if (msg->programs & (1 << i)) {
                    disk_op_save_program(model_get_program(pmodel, i), disk_io_callback, disk_io_error_callback,
                                         (void *)(uintptr_t)msg->save_all_io_op);
                }
            }
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_CONNECT_TO_WIFI_NETWORK:
            log_info("Connessione a %s...", msg->ssid);
            wifi_connect(msg->ssid, msg->psk);
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_WIFI_SCAN:
            wifi_scan();
            break;

        case VIEW_CONTROLLER_MESSAGE_CODE_SAVE_WIFI_CONFIG:
            disk_op_save_wifi_config(disk_io_callback, disk_io_error_callback, NULL);
            break;
    }
}


void controller_manage(model_t *pmodel) {
    static unsigned long fastts     = 0;
    static unsigned long slowts     = 0;
    static unsigned long wifits     = 0;
    static int           first_sync = 1;

    disk_op_manage_response(pmodel);

    if (is_expired(fastts, get_millis(), 300UL)) {
        if (model_is_in_test(pmodel)) {
            machine_refresh_test_values();
        }
        machine_refresh_state();
        fastts = get_millis();
    } else if (is_expired(slowts, get_millis(), 600UL)) {
        machine_refresh_sensors();
        slowts = get_millis();
    } else if (is_expired(wifits, get_millis(), 2000)) {
        pmodel->system.num_networks = wifi_read_scan(&pmodel->system.networks);
        model_wifi_status_changed(pmodel, wifi_status(pmodel->system.ssid));

        view_event((view_event_t){.code = VIEW_EVENT_CODE_WIFI});
        int ip1 = 0, ip2 = 0;
        if (pmodel->system.net_status == WIFI_CONNECTED) {
            ip1 = wifi_get_ip_address(IFWIFI, pmodel->system.wifi_ipaddr);
        } else {
            strcpy(pmodel->system.wifi_ipaddr, "_._._._");
        }

        ip2 = wifi_get_ip_address(IFETH, pmodel->system.eth_ipaddr);

        pmodel->system.connected = ip1 || ip2;

        wifits = get_millis();
    }


    if (model_is_machine_communication_ok(pmodel) && model_is_program_running(pmodel)) {
        if (model_is_machine_active(pmodel) && !pending_start) {
            if (model_next_step(pmodel)) {
                pending_start = 1;
                machine_send_step(model_get_current_step(pmodel), model_get_current_program_number(pmodel),
                                  model_get_current_step_number(pmodel), 1);
            } else {
                model_stop_program(pmodel);
                machine_send_command(COMMAND_REGISTER_STOP);
            }
        }
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

            case MACHINE_RESPONSE_MESSAGE_CODE_READ_SENSORS:
                if (model_update_sensors(pmodel, msg.coins, msg.payment, msg.t1_adc, msg.t2_adc, msg.t1, msg.t2,
                                         msg.configured_temperature)) {
                    view_event((view_event_t){.code = VIEW_EVENT_CODE_SENSORS_CHANGED});
                }
                break;

            case MACHINE_RESPONSE_MESSAGE_CODE_READ_EXTENDED_STATE:
                log_warn("Machine sync");
                model_update_flags(pmodel, msg.alarms, msg.flags);
                machine_send_parmac(&pmodel->configuration.parmac);

                if (model_pick_up_machine_state(pmodel, msg.state, msg.program_number, msg.step_number)) {
                    machine_send_step(model_get_current_step(pmodel), model_get_current_program_number(pmodel),
                                      model_get_current_step_number(pmodel), pmodel->configuration.parmac.autoavvio);
                    view_event((view_event_t){.code = VIEW_EVENT_CODE_STATE_CHANGED});
                }
                first_sync = 0;
                break;

            case MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE:
                model_update_flags(pmodel, msg.alarms, msg.flags);

                if (!model_is_machine_initialized(pmodel) || first_sync) {
                    machine_get_extended_state();
                    first_sync = 0;
                    break;
                }

                if (model_update_machine_state(pmodel, msg.state, msg.step_type)) {
                    if (pending_start && model_is_machine_running(pmodel)) {
                        pending_start = 0;
                    }

                    view_event((view_event_t){.code = VIEW_EVENT_CODE_STATE_CHANGED});
                }

                model_set_remaining(pmodel, msg.remaining);
                break;

            case MACHINE_RESPONSE_MESSAGE_CODE_VERSION:
                snprintf(pmodel->machine.version, sizeof(pmodel->machine.version), "%i.%c.%i", msg.version_major,
                         msg.version_minor, msg.version_patch);
                snprintf(pmodel->machine.date, sizeof(pmodel->machine.date), "%i/%i/%i", msg.build_day, msg.build_month,
                         msg.build_year);
                break;
        }
    }

    if (model_should_autostop(pmodel)) {
        model_stop_program(pmodel);
        machine_send_command(COMMAND_REGISTER_STOP);
    }
}



static void load_parmac_callback(model_t *pmodel, void *data, void *arg) {
    (void)arg;
    parmac_t *parmac = data;

    pmodel->configuration.parmac = *parmac;
    parmac_check_ranges(pmodel);
}


static void load_parmac_error_callback(model_t *pmodel, void *arg) {
    (void)arg;
    parmac_reset_to_defaults(pmodel);
}


static void load_programs_callback(model_t *pmodel, void *data, void *arg) {
    (void)arg;
    storage_program_list_t *list = data;

    memcpy(pmodel->configuration.programs, list->programs, sizeof(list->programs[0]) * list->num_programs);
    pmodel->configuration.num_programs = list->num_programs;
    for (size_t i = 0; i < model_get_num_programs(pmodel); i++) {
        dryer_program_t *p = model_get_program(pmodel, i);
        for (size_t j = 0; j < p->num_steps; j++) {
            // Limit check
            parciclo_init(pmodel, i, j);
        }
    }
}


static void load_password_callback(model_t *pmodel, void *data, void *arg) {
    if (data != NULL) {
        model_set_password(pmodel, data);
        free(data);
    }
}

static void disk_io_callback(model_t *pmodel, void *data, void *arg) {
    (void)pmodel;
    view_event((view_event_t){.code = VIEW_EVENT_CODE_IO_DONE, .io_data = data, .io_op = (int)(uintptr_t)arg});
}


static void disk_io_error_callback(model_t *pmodel, void *arg) {
    (void)pmodel;
    view_event((view_event_t){.code = VIEW_EVENT_CODE_IO_DONE, .io_op = (int)(uintptr_t)arg, .error = 1});
}