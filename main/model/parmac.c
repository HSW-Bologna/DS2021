#include <assert.h>
#include <stdio.h>
#include "gel/parameter/parameter.h"
#include "model.h"
#include "parmac.h"
#include "descriptions/AUTOGEN_FILE_parameters.h"
#include "gel/parameter/parameter.h"



#define USER_BITS USER_ACCESS_LEVEL
#define TECH_BITS 0x02


static const char *secfmt = "%i s";
static const char *msfmt  = "%02i:%02i";



void parmac_init(model_t *pmodel) {
    assert(pmodel != NULL);
#define DESC parameters_desc

    parmac_t           *p    = &pmodel->configuration.parmac;
    parameter_handle_t *pars = pmodel->parameter_mac;
    size_t              i    = 0;

    // clang-format off
    pars[i++] = PARAMETER(&p->lingua,                                   LINGUA_ITALIANO,                NUM_LINGUE - 1,             LINGUA_ITALIANO,                ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_LINGUA], .values = (const char***)parameters_lingue}),                                        USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_visualizzazione_temperatura,      0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_VISUALIZZAZIONE_TEMPERATURA], .values = (const char***)parameters_abilitazione}),               USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_tasto_menu,                       0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_TASTO_MENU], .values = (const char***)parameters_abilitazione}),                                USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_pressione_tasto_pausa,              0,                              10,                         0,                              ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_PRESSIONE_PAUSA], .fmt = secfmt}),                                                          USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_pressione_tasto_stop,               0,                              10,                         5,                              ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_PRESSIONE_STOP], .fmt = secfmt}),                                                           USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_stop_automatico,                    0,                              10,                         0,                              ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_STOP_AUTOMATICO], .fmt = secfmt}),                                                          USER_BITS);
    pars[i++] = PARAMETER(&p->stop_tempo_ciclo,                         0,                              1,                          1,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_FERMA_TEMPO], .values = (const char***)parameters_abilitazione}),                               USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_attesa_partenza_ciclo,              0,                              600,                        0,                              ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_ATTESA_PARTENZA], .fmt = msfmt, .min_sec = 1}),                                             USER_BITS);
    pars[i++] = PARAMETER(&p->tipo_sonda_temperatura,                   SONDA_TEMPERATURA_PTC_1,        SONDA_TEMPERATURA_SHT,      SONDA_TEMPERATURA_PTC_1,        ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_SONDA_TEMPERATURA], .values = (const char***)parameters_sonde_temperatura}),             USER_BITS);
    pars[i++] = PARAMETER(&p->posizione_sonda_temperatura,              POSIZIONE_SONDA_INGRESSO,       POSIZIONE_SONDA_USCITA,     POSIZIONE_SONDA_INGRESSO,       ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_POSIZIONE_SONDA_TEMPERATURA], .values = (const char***)parameters_posizione}),                USER_BITS);
    pars[i++] = PARAMETER(&p->temperatura_massima_ingresso,             0,                              MAX_TEMPERATURE,            0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA_MAX_INGRESSO], .fmt = "%i C"}),                                                      USER_BITS);
    pars[i++] = PARAMETER(&p->temperatura_massima_uscita,               0,                              MAX_TEMPERATURE,            0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA_MAX_USCITA], .fmt = "%i C"}),                                                       USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_espansione_rs485,                 0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ABILITA_EXP_RS485], .values = (const char***)parameters_abilitazione}),                         USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_gas,                              0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ABILITA_GAS], .values = (const char***)parameters_abilitazione}),                               USER_BITS);
    pars[i++] = PARAMETER(&p->velocita_minima,                          0,                              MAX_SPEED,                  0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA_MINIMA], .fmt = "%i RPM"}),                                                            USER_BITS);
    pars[i++] = PARAMETER(&p->velocita_massima,                         0,                              MAX_SPEED,                  0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA_MASSIMA], .fmt = "%i RPM"}),                                                           USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_gettone,                            5,                              300,                        30,                             ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_GETTONE], .fmt = msfmt, .min_sec = 1}),                                                     USER_BITS);
    pars[i++] = PARAMETER(&p->max_programs,                             1,                              MAX_PROGRAMS,               (MAX_PROGRAMS/2),               ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_MAX_PROGRAMMI]}),                                                                                 USER_BITS);
    pars[i++] = PARAMETER(&p->temperatura_sicurezza_in,                 0,                              100,                        50,                            ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_TEMPERATURA_SICUREZZA_IN], .fmt = "%i C"}),                                                       USER_BITS);
    pars[i++] = PARAMETER(&p->temperatura_sicurezza_out,                0,                              100,                        50,                            ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_TEMPERATURA_SICUREZZA_OUT], .fmt = "%i C"}),                                                      USER_BITS);
    pars[i++] = PARAMETER(&p->tempo_allarme_temperatura,                60,                             1200,                       300,                            ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_ALLARME_TEMPERATURA], .fmt = secfmt}),                                                      USER_BITS);
    pars[i++] = PARAMETER(&p->allarme_inverter_off_on,                  0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ALLARME_INVERTER], .values = (const char***)parameters_abilitazione}),                          USER_BITS);
    pars[i++] = PARAMETER(&p->allarme_filtro_off_on,                    0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ALLARME_FILTRO], .values = (const char***)parameters_abilitazione}),                          USER_BITS);
    pars[i++] = PARAMETER(&p->inverti_macchina_occupata,                0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_INVERTI_MACCHINA_OCCUPATA], .values = (const char***)parameters_abilitazione}),                 USER_BITS);
    pars[i++] = PARAMETER(&p->tipo_macchina_occupata,                   0,                              2,                          0,                              ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_MACCHINA_OCCUPATA], .values = (const char***)parameters_tipo_macchina_occupata}),        USER_BITS);
    pars[i++] = PARAMETER(&p->tipo_riscaldamento,                       TIPO_RISCALDAMENTO_GAS,         TIPO_RISCALDAMENTO_VAPORE,  TIPO_RISCALDAMENTO_ELETTRICO,   ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_RISCALDAMENTO], .values = (const char***)parameters_riscaldamento}),                     USER_BITS);
    pars[i++] = PARAMETER(&p->access_level,                             0,                              1,                          0,                              ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_LIVELLO_ACCESSO], .values = (const char***)parameters_livello_accesso}),                      TECH_BITS);
    pars[i++] = PARAMETER(&p->autoavvio,                                0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_AUTOAVVIO], .values = (const char***)parameters_abilitazione}),                                 USER_BITS);
    pars[i++] = PARAMETER(&p->abilita_allarmi,                          0,                              1,                          1,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_GESTIONE_ALLARMI], .values = (const char***)parameters_abilitazione}),                          TECH_BITS);
    // clang-format on

    parameter_check_ranges(pars, i);

#undef DESC
}


void parmac_reset_to_defaults(model_t *pmodel) {
    assert(pmodel != NULL);
    parameter_reset_to_defaults(pmodel->parameter_mac, NUM_PARMAC);
}


void parmac_check_ranges(model_t *pmodel) {
    assert(pmodel != NULL);
    parameter_check_ranges(pmodel->parameter_mac, NUM_PARMAC);
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
    return model_format_parameter_value(pmodel, pmodel->parameter_mac, NUM_PARMAC, string, len, num);
}


parameter_handle_t *parmac_get_handle(model_t *pmodel, size_t num) {
    return parameter_get_handle(pmodel->parameter_mac, NUM_PARMAC, num, model_get_access_level(pmodel));
}
