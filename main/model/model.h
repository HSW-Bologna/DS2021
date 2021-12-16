#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED


#include <stdlib.h>
#include <stdint.h>
#include "gel/parameter/parameter.h"


#define NUM_PARMAC 4
#define NUM_RELES  17
#define NUM_INPUTS 5

#define USER_ACCESS_LEVEL       1
#define TECHNICIAN_ACCESS_LEVEL 3


#define MACHINE_STATE_STOPPED 0
#define MACHINE_STATE_RUNNING  1


enum {
    LINGUA_ITALIANO = 0,
    LINGUA_INGLESE,
    NUM_LINGUE,
};


typedef struct {
    uint16_t lingua;
    uint16_t abilita_gas;
    uint16_t velocita_minima;
    uint16_t tempo_gettone;
    uint16_t access_level;
} parmac_t;


typedef struct {
    struct {
        int communication_error;
        int communication_enabled;

        uint16_t state;
    } machine;

    parmac_t parmac;

    parameter_handle_t parameter_mac[NUM_PARMAC];
} model_t;


void         model_init(model_t *pmodel);
void         model_set_machine_communication_error(model_t *pmodel, int error);
int          model_is_machine_communication_ok(model_t *pmodel);
size_t       model_get_language(model_t *pmodel);
unsigned int model_get_access_level(model_t *pmodel);
const char  *model_parameter_get_description(model_t *pmodel, parameter_handle_t *par);
const char  *model_parameter_to_string(model_t *pmodel, parameter_handle_t *par, size_t index);
void         model_set_machine_communication(model_t *pmodel, int enabled);
int          model_parameter_set_value(parameter_handle_t *par, uint16_t value);
uint16_t     model_get_machine_state(model_t *pmodel);
int          model_set_machine_state(model_t *pmodel, uint16_t state);

#endif