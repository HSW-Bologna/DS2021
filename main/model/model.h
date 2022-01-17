#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED


#include <stdlib.h>
#include <stdint.h>
#include "gel/parameter/parameter.h"
#include "program.h"


#define PASSWORD_MAX_SIZE 10


#define MODE_MANUAL 0
#define MODE_AUTO   1

#define COIN_LINES 5

#define FUNCTION_FLAGS_INITIALIZED 0x01
#define FUNCTION_FLAGS_TEST        0x02


#define MAX_NUM_PARCICLO    16
#define NUM_PARMAC          20
#define NUM_INTERNAL_RELES  9
#define NUM_RELES           17
#define NUM_INTERNAL_INPUTS 12
#define NUM_INPUTS          24

#define PARMAC_SIZE 44

#define USER_ACCESS_LEVEL       1
#define TECHNICIAN_ACCESS_LEVEL 3
#define NUM_ACCESS_LEVELS       2


#define MACHINE_STATE_STOPPED 0
#define MACHINE_STATE_ACTIVE  1
#define MACHINE_STATE_RUNNING 2
#define MACHINE_STATE_PAUSED  3


#define TIPO_RISCALDAMENTO_GAS       0
#define TIPO_RISCALDAMENTO_ELETTRICO 1
#define TIPO_RISCALDAMENTO_VAPORE    2


enum {
    TIPO_PAGAMENTO_GRATUITO,
    TIPO_PAGAMENTO_GETTONE,
    TIPO_PAGAMENTO_MONETA,
    TIPO_PAGAMENTO_CASSA,
};


typedef enum {
    WIFI_CONNECTED,
    WIFI_CONNECTING,
    WIFI_SCANNING,
    WIFI_INACTIVE,
} wifi_status_t;


typedef struct {
    int  signal;
    char ssid[STRING_NAME_SIZE];
} wifi_network_t;


typedef enum {
    ALARM_CODE_EMERGENZA = 0,
    ALARM_CODE_OBLO_APERTO,
} alarm_code_t;


typedef struct {
    uint16_t lingua;
    uint16_t abilita_visualizzazione_temperatura;
    uint16_t tempo_pressione_tasto_pausa;
    uint16_t tempo_pressione_tasto_stop;
    uint16_t tempo_stop_automatico;

    uint16_t abilita_espansione_rs485;
    uint16_t abilita_gas;
    uint16_t velocita_minima;
    uint16_t tempo_gettone;
    uint16_t velocita_antipiega;
    uint16_t tipo_pagamento;
    uint16_t access_level;
    uint16_t max_programs;

    /* Parametri da inviare alla macchina */
    uint16_t tipo_sonda_temperatura;
    uint16_t temperatura_sicurezza_in;
    uint16_t temperatura_sicurezza_out;
    uint16_t tempo_allarme_temperatura;     // se non arriva in temperatura in quel tempo
    uint16_t allarme_inverter_off_on;
    uint16_t allarme_filtro_off_on;
    uint16_t tipo_macchina_occupata;
    uint16_t inverti_macchina_occupata;
    uint16_t tipo_riscaldamento;
    uint16_t autoavvio;

    /*
abilita_visualizzazione_cicli_totali
abilita_blocco_no_aria
numero_cicli_manutenzione
tempo_cadenza_avviso_manutenzione
tempo_durata_avviso_manutenzione
temperatura_stop_tempo_ciclo
tempo_attesa_partenza_ciclo // All'inizio del ciclo aspetta questo tempo con le ventole accese
tempo_antigelo

abilita_visualizzazione_temperatura
lingua_max
tempo_reset_lingua
logo_ditta
abilita_tasto_menu
abilita_gettoniera (gettone, moneta, cassa)
numero_gettoni_consenso
tempo_gettone
velocita_min_lavoro
velocita_max_lavoro
sonda_temperatura_in_out
temperatura_max_in
temperatura_max_out
percentuale_velocita_min_ventola
percentuale_anticipo_temperatura_ventola
durata_ventilazione_gas
numero_tentativi_reset_gas
abilita_reset_gas_esteso
abilita_stop_tempo_ciclo // se il tempo deve scorrere in pausa
tempo_uscita_pagine
abilita_parametri_ridotti
abilita_autoavvio
tempo_allarme_flusso_aria   // isteresi del flusso aria (ingresso ventilazione)
tempo_azzeramento_ciclo_pausa //pausa
// tempo_stacco_gruppo_1
// tempo_riattacco_gruppo_2
     */


} parmac_t;


typedef struct {
    struct {
        int communication_error;
        int communication_enabled;

        uint16_t state;
        uint16_t remaining;
        uint16_t reported_step_type;
        uint16_t alarms;
        uint16_t function_flags;

        uint16_t coins[COIN_LINES];
        uint16_t payment;
        uint16_t adc_ptc1;
        uint16_t adc_ptc2;
        int      temperature_ptc1;
        int      temperature_ptc2;
        int      configured_temperature;

        char version[32];
        char date[32];
    } machine;

    struct {
        parmac_t        parmac;
        size_t          num_programs;
        dryer_program_t programs[MAX_PROGRAMS];
        uint64_t        programs_to_save;
        int             parmac_to_save;
        char            password[PASSWORD_MAX_SIZE + 1];
    } configuration;

    struct {
        char            wifi_ipaddr[16];
        char            eth_ipaddr[16];
        char            ssid[33];
        wifi_status_t   net_status;
        int             num_networks;
        wifi_network_t *networks;
        int             connected;
    } system;

    struct {
        dryer_program_t program;
        size_t          program_number;
        size_t          step_number;
        unsigned long   autostop_ts;
    } run;

    parameter_handle_t parameter_mac[NUM_PARMAC];
    size_t             num_parciclo;
    parameter_handle_t parameter_ciclo[MAX_NUM_PARCICLO];
} model_t;


void             model_init(model_t *pmodel);
void             model_set_machine_communication_error(model_t *pmodel, int error);
int              model_is_machine_communication_ok(model_t *pmodel);
size_t           model_get_language(model_t *pmodel);
unsigned int     model_get_access_level(model_t *pmodel);
const char      *model_parameter_get_description(model_t *pmodel, parameter_handle_t *par);
const char      *model_parameter_value_to_string(model_t *pmodel, parameter_handle_t *par, size_t index);
void             model_set_machine_communication(model_t *pmodel, int enabled);
int              model_parameter_set_value(parameter_handle_t *par, uint16_t value);
uint16_t         model_get_machine_state(model_t *pmodel);
int              model_update_machine_state(model_t *pmodel, uint16_t state, uint16_t step_type);
size_t           model_get_effective_outputs(model_t *pmodel);
size_t           model_get_effective_inputs(model_t *pmodel);
size_t           model_serialize_parmac(uint8_t *buffer, parmac_t *parmac);
size_t           model_deserialize_parmac(parmac_t *parmac, uint8_t *buffer);
size_t           model_get_num_programs(model_t *pmodel);
const char      *model_get_program_name(model_t *pmodel, size_t num);
size_t           model_get_max_programs(model_t *pmodel);
dryer_program_t *model_create_new_program(model_t *pmodel, const char *name, size_t lingua, unsigned long timestamp,
                                          size_t *index);
dryer_program_t *model_get_program(model_t *pmodel, size_t num);
void             model_remove_program(model_t *pmodel, size_t num);
dryer_program_t *model_create_new_program_from(model_t *pmodel, size_t src, size_t pos, unsigned long timestamp);
void             model_swap_programs(model_t *pmodel, size_t first, size_t second);
void             model_mark_program_to_save(model_t *pmodel, size_t num);
uint64_t         model_get_programs_to_save(model_t *pmodel);
void             model_clear_marks_to_save(model_t *pmodel);
const char      *model_format_parameter_value(model_t *pmodel, parameter_handle_t *pars, size_t len_pars, char *string,
                                              size_t len, size_t num);
uint16_t         model_get_remaining(model_t *pmodel);
void             model_set_remaining(model_t *pmodel, uint16_t remaining);
void             model_start_program(model_t *pmodel, size_t num);
dryer_program_t *model_get_current_program(model_t *pmodel);
parameters_step_t *model_get_current_step(model_t *pmodel);
int                model_is_valid_program(model_t *pmodel, size_t num);
int                model_is_program_running(model_t *pmodel);
int                model_is_machine_running(model_t *pmodel);
int                model_is_machine_active(model_t *pmodel);
int                model_is_machine_stopped(model_t *pmodel);
int                model_next_step(model_t *pmodel);
int                model_get_machine_current_step_type(model_t *pmodel, uint16_t *step_type);
int                model_get_worst_alarm(model_t *pmodel, alarm_code_t *code, uint16_t exclude_mask);
int                model_is_any_alarm_active(model_t *pmodel);
uint16_t           model_get_alarms(model_t *pmodel);
void               model_stop_program(model_t *pmodel);
int                model_is_in_test(model_t *pmodel);
int         model_update_sensors(model_t *pmodel, uint16_t *coins, uint16_t payment, uint16_t t1_adc, uint16_t t2_adc,
                                 uint16_t t1, uint16_t t2, uint16_t configured_temperature);
int         model_current_setpoint(model_t *pmodel);
int         model_is_machine_initialized(model_t *pmodel);
size_t      model_get_current_program_number(model_t *pmodel);
size_t      model_get_current_step_number(model_t *pmodel);
void        model_resume_program(model_t *pmodel, size_t num, size_t step_num);
int         model_pick_up_machine_state(model_t *pmodel, uint16_t state, uint16_t program_number, uint16_t step_number);
int         model_wifi_status_changed(model_t *pmodel, wifi_status_t status);
const char *model_get_password(model_t *pmodel);
void        model_set_password(model_t *pmodel, const char *password);
int         model_current_temperature(model_t *pmodel);
const char *model_get_program_name_in_language(model_t *pmodel, uint16_t language, size_t num);
uint16_t    model_get_access_level_code(model_t *pmodel);
void        model_set_access_level_code(model_t *pmodel, uint16_t access_level);
int         model_get_parmac_to_save(model_t *pmodel);
void        model_mark_parmac_to_save(model_t *pmodel);
int         model_display_temperature(model_t *pmodel);
uint16_t    model_get_pause_press_time(model_t *pmodel);
uint16_t    model_get_stop_press_time(model_t *pmodel);
int         model_should_autostop(model_t *pmodel);
int         model_is_porthole_open(model_t *pmodel);
int         model_update_flags(model_t *pmodel, uint16_t alarms, uint16_t flags);


#endif