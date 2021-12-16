#ifndef MACHINE_H_INCLUDED
#define MACHINE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>


#define COMMAND_REGISTER_START 1
#define COMMAND_REGISTER_STOP  2


typedef enum {
    MACHINE_RESPONSE_MESSAGE_CODE_ERROR,
    MACHINE_RESPONSE_MESSAGE_CODE_READ_STATE,
    MACHINE_RESPONSE_MESSAGE_CODE_TEST_READ_INPUT,
} machine_response_message_code_t;


typedef struct {
    machine_response_message_code_t code;

    union {
        uint16_t value;
        struct {
            uint16_t state;
        };
    };
} machine_response_message_t;


void machine_init(void);
void machine_test_rele(size_t coil, int value);
void machine_restart_communication(void);
void machine_refresh_inputs(void);
int  machine_get_response(machine_response_message_t *msg);
void machine_refresh_state(void);

#endif