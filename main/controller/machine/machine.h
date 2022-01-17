#ifndef MACHINE_H_INCLUDED
#define MACHINE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include "model/model.h"


#define COMMAND_REGISTER_RUN_STEP     1
#define COMMAND_REGISTER_STOP         2
#define COMMAND_REGISTER_PAUSE        3
#define COMMAND_REGISTER_CLEAR_ALARMS 4
#define COMMAND_REGISTER_CLEAR_COINS  5
#define COMMAND_REGISTER_ENTER_TEST   6
#define COMMAND_REGISTER_EXIT_TEST    7
#define COMMAND_REGISTER_INITIALIZE   8



typedef enum {
    MACHINE_RESPONSE_MESSAGE_CODE_ERROR,
    MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE,
    MACHINE_RESPONSE_MESSAGE_CODE_READ_EXTENDED_STATE,
    MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT,
    MACHINE_RESPONSE_MESSAGE_CODE_READ_SENSORS,
    MACHINE_RESPONSE_MESSAGE_CODE_VERSION,
} machine_response_message_code_t;


typedef struct {
    machine_response_message_code_t code;

    union {
        uint16_t value;
        struct {
            uint16_t version_major;
            uint16_t version_minor;
            uint16_t version_patch;
            uint8_t  build_day;
            uint8_t  build_month;
            uint8_t  build_year;
        };
        struct {
            uint16_t state;
            uint16_t alarms;
            uint16_t remaining;
            uint16_t flags;
            uint16_t program_number;
            uint16_t step_number;
            uint16_t step_type;
        };

        struct {
            uint16_t coins[COIN_LINES];
            uint16_t payment;
            uint16_t t_rs485;
            uint16_t h_rs485;
            uint16_t t1_adc;
            uint16_t t2_adc;
            uint16_t t1;
            uint16_t t2;
            uint16_t configured_temperature;
        };
    };
} machine_response_message_t;


void machine_init(void);
void machine_test_rele(size_t coil, int value);
void machine_restart_communication(void);
void machine_read_version(void);
void machine_refresh_test_values(void);
int  machine_get_response(machine_response_message_t *msg);
void machine_refresh_state(void);
void machine_send_command(uint16_t command);
void machine_test_pwm(size_t pwm, int speed);
void machine_send_parmac(parmac_t *parmac);
void machine_send_step(parameters_step_t *step, size_t prog_num, size_t step_num, int start);
void machine_refresh_sensors(void);
void machine_get_extended_state(void);

#endif