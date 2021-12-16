#include <stddef.h>
#include <assert.h>
#include "model.h"
#include "parmac.h"


void model_init(model_t *pmodel) {
    assert(pmodel != NULL);

    pmodel->machine.communication_error   = 0;
    pmodel->machine.communication_enabled = 1;
    pmodel->machine.state                 = MACHINE_STATE_STOPPED;

    pmodel->parmac.lingua = LINGUA_ITALIANO;

    parmac_init(pmodel);
}


int model_is_machine_communication_ok(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.communication_enabled && !pmodel->machine.communication_error;
}


void model_set_machine_communication_error(model_t *pmodel, int error) {
    assert(pmodel != NULL);
    pmodel->machine.communication_error = error;
}


void model_set_machine_communication(model_t *pmodel, int enabled) {
    assert(pmodel != NULL);
    pmodel->machine.communication_enabled = enabled;
}


size_t model_get_language(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->parmac.lingua;
}


unsigned int model_get_access_level(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->parmac.access_level == 0) {
        return USER_ACCESS_LEVEL;
    } else {
        return TECHNICIAN_ACCESS_LEVEL;
    }
}


const char *model_parameter_get_description(model_t *pmodel, parameter_handle_t *par) {
    assert(pmodel != NULL);
    pudata_t data = parameter_get_user_data(par);
    return data.desc[model_get_language(pmodel)];
}


const char *model_parameter_to_string(model_t *pmodel, parameter_handle_t *par, size_t index) {
    assert(pmodel != NULL);

    pudata_t data               = parameter_get_user_data(par);
    char *(*values)[NUM_LINGUE] = (char *(*)[NUM_LINGUE])data.values;

    return values[index][model_get_language(pmodel)];
}


int model_parameter_set_value(parameter_handle_t *par, uint16_t value) {
    *(uint16_t *)par->pointer = value;
    return parameter_check_ranges(par, 1);
}


int model_set_machine_state(model_t *pmodel, uint16_t state) {
    assert(pmodel != NULL);
    if (pmodel->machine.state != state) {
        pmodel->machine.state = state;
        return 1;
    } else {
        return 0;
    }
}


uint16_t model_get_machine_state(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.state;
}