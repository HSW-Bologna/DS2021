#include <assert.h>
#include <stdio.h>
#include "gel/parameter/parameter.h"
#include "model.h"
#include "parciclo.h"
#include "descriptions/AUTOGEN_FILE_parameters.h"


#define USER_BITS USER_ACCESS_LEVEL
#define TECH_BITS 0x02


static const char *secfmt = "%i s";
static const char *msfmt  = "%02i:%02i";


void parciclo_init(model_t *pmodel, size_t num_prog, size_t num_step) {
    assert(pmodel != NULL);

    dryer_program_t *p = model_get_program(pmodel, num_prog);
    assert(p != NULL);
    assert(num_step < p->num_steps);
    parameters_step_t *s = &p->steps[num_step];
#define DESC parameters_desc

    parameter_handle_t *pars = pmodel->parameter_ciclo;
    size_t              i    = 0;

    switch (s->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            // clang-format off
            pars[i++] = PARAMETER(&s->drying.type,                  MODE_MANUAL,     MODE_AUTO,     MODE_MANUAL,    ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_ASCIUGATURA], .values = (const char***)parameters_modalita}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->drying.duration,              0,     MAX_STEP_DURATION,    60,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_DURATA], .fmt=msfmt, .min_sec = 1}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->drying.enable_waiting_for_temperature,              0,     1,    0,    ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ATTESA_TEMPERATURA], .values = (const char***)parameters_abilitazione}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->drying.enable_reverse,              0,     1,    0,    ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_INVERSIONE], .values = (const char***)parameters_abilitazione}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->drying.rotation_time,         0,     60,       4,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_MARCIA], .fmt=secfmt}),               USER_BITS);
            pars[i++] = PARAMETER(&s->drying.pause_time,            0,     60,       2,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_DI_PAUSA], .fmt=secfmt}),               USER_BITS);
            pars[i++] = PARAMETER_DLIMITS(&s->drying.speed,     model_get_minimum_speed(pmodel),    model_get_maximum_speed(pmodel),    0,     MAX_SPEED,       40,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA], .fmt="%i rpm"}),               USER_BITS);
            pars[i++] = PARAMETER(&s->drying.temperature,            0,     MAX_TEMPERATURE,       40,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA], .fmt="%i C"}),               USER_BITS);
            pars[i++] = PARAMETER(&s->drying.humidity,            0,     MAX_HUMIDITY,       40,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_UMIDITA], .fmt="%i%%"}),               USER_BITS);
            // clang-format on
            break;

        case DRYER_PROGRAM_STEP_TYPE_COOLING:
            // clang-format off
            pars[i++] = PARAMETER(&s->cooling.type,                  MODE_MANUAL,     MODE_AUTO,     MODE_MANUAL,    ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_RAFFREDDAMENTO], .values = (const char***)parameters_modalita}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.duration,              0,     MAX_STEP_DURATION,    60,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_DURATA], .fmt=msfmt, .min_sec = 1}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.enable_reverse,              0,     1,    0,    ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_INVERSIONE], .values = (const char***)parameters_abilitazione}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.rotation_time,         0,     60,       4,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_MARCIA], .fmt=secfmt}),               USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.pause_time,            0,     60,       2,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_DI_PAUSA], .fmt=secfmt}),               USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.temperature,            0,     MAX_TEMPERATURE,       40,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA], .fmt="%i C"}),               USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.deodorant_delay,         0,     60,       0,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_ATTESA_DEODORIZZAZIONE], .fmt=msfmt, .min_sec = 1}),               USER_BITS);
            pars[i++] = PARAMETER(&s->cooling.deodorant_duration,         0,     60,       0,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_DEODORIZZAZIONE], .fmt=msfmt, .min_sec = 1}),               USER_BITS);
            // clang-format on
            break;

        case DRYER_PROGRAM_STEP_TYPE_UNFOLDING:
            // clang-format off
            pars[i++] = PARAMETER(&s->unfolding.max_duration,             60,     MAX_STEP_DURATION,     120,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_DURATA_MASSIMA], .fmt = msfmt, .min_sec = 1}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->unfolding.max_cycles,             100,     1000,     120,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_CICLI_MASSIMI]}),                     USER_BITS);
            pars[i++] = PARAMETER_DLIMITS(&s->unfolding.speed,     model_get_minimum_speed(pmodel), model_get_maximum_speed(pmodel),       0,     MAX_SPEED,       40,    ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA], .fmt="%i rpm"}),               USER_BITS);
            pars[i++] = PARAMETER(&s->unfolding.rotation_time,             0,     60,     2,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_MARCIA], .fmt =secfmt}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->unfolding.pause_time,             0,     60,     2,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_MARCIA], .fmt =secfmt}),                     USER_BITS);
            pars[i++] = PARAMETER(&s->unfolding.start_delay,             0,     60,     2,    ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_RITARDO_IN_PARTENZA], .fmt =msfmt, .min_sec = 1}),                     USER_BITS);
            // clang-format on
            break;
    }

    parameter_check_ranges(pars, i);
    pmodel->num_parciclo = i;

#undef DESC
}


size_t parciclo_get_total_parameters(model_t *pmodel) {
    assert(pmodel != NULL);
    return parameter_get_count(pmodel->parameter_ciclo, pmodel->num_parciclo, model_get_access_level(pmodel));
}


const char *parciclo_get_description(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    return model_parameter_get_description(pmodel, parameter_get_handle(pmodel->parameter_ciclo, pmodel->num_parciclo,
                                                                        num, model_get_access_level(pmodel)));
}


const char *parciclo_to_string(model_t *pmodel, char *string, size_t len, size_t num) {
    return model_format_parameter_value(pmodel, pmodel->parameter_ciclo, pmodel->num_parciclo, string, len, num);
}


parameter_handle_t *parciclo_get_handle(model_t *pmodel, size_t num) {
    return parameter_get_handle(pmodel->parameter_ciclo, pmodel->num_parciclo, num, model_get_access_level(pmodel));
}
