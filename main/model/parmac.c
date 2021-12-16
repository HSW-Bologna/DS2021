#include <assert.h>
#include <stdio.h>
#include "gel/parameter/parameter.h"
#include "model.h"
#include "parmac.h"
#include "descriptions/AUTOGEN_FILE_parameters.h"
#include "gel/parameter/parameter.h"



#define USER_BITS USER_ACCESS_LEVEL
#define TECH_BITS 0x02




void parmac_init(model_t *pmodel) {
    assert(pmodel != NULL);
#define DESC parameters_parmac

    parmac_t           *p    = &pmodel->parmac;
    parameter_handle_t *pars = pmodel->parameter_mac;
    size_t              i    = 0;

    // clang-format off
    pars[i++] = PARAMETER(&p->lingua,                   LINGUA_ITALIANO,     NUM_LINGUE - 1,     LINGUA_ITALIANO,    ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_PARMAC_LINGUA], .values = (const char***)parameters_lingue}),                     USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_gas,              0,                   1,                  0,                  ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_PARMAC_ABILITA_GAS], .values = (const char***)parameters_abilitazione}),            USER_BITS);
    pars[i++] = PARAMETER(&p->velocita_minima,          0,                   100,                0,                  ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_PARMAC_VELOCITA_MINIMA], .fmt = "%i RPM"}),                                         USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_gettone,            5,                   300,                30,                 ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_PARMAC_TEMPO_GETTONE], .fmt = "%i s"}),                                               USER_BITS);
    pars[i++] = PARAMETER(&p->access_level,             0,                   1,                  0,                  ((pudata_t){.t = PTYPE_DROPDOWN}),                                                                                                         TECH_BITS);
    // clang-format on

    parameter_check_ranges(pars, i);

#undef DESC
}


size_t parmac_get_total_parameters(model_t *pmodel) {
    assert(pmodel != NULL);
    return parameter_get_count(pmodel->parameter_mac, NUM_PARMAC, model_get_access_level(pmodel));
}


const char *parmac_get_description(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    return model_parameter_get_description(
        pmodel, parameter_get_handle(pmodel->parameter_mac, NUM_PARMAC, num, model_get_access_level(pmodel)));
}


const char *parmac_to_string(model_t *pmodel, char *string, size_t len, size_t num) {
    assert(pmodel != NULL);
    parameter_handle_t *par =
        parameter_get_handle(pmodel->parameter_mac, NUM_PARMAC, num, model_get_access_level(pmodel));

    pudata_t data = parameter_get_user_data(par);

    switch (data.t) {
        case PTYPE_SWITCH:
        case PTYPE_DROPDOWN: {
            size_t index = parameter_to_index(par);
            return model_parameter_to_string(pmodel, par, index);
        }

        case PTYPE_NUMBER: {
            long value = parameter_to_long(par);
            snprintf(string, len, data.fmt, value);
            return (const char *)string;
        }

        case PTYPE_TIME: {
            long value = parameter_to_long(par);
            snprintf(string, len, "%02li:%02li", value / 60, value % 60);
            return (const char *)string;
        }

        default:
            assert(0);
    }
}


parameter_handle_t *parmac_get_handle(model_t *pmodel, size_t num) {
    return parameter_get_handle(pmodel->parameter_mac, NUM_PARMAC, num, model_get_access_level(pmodel));
}