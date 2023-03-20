#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "model.h"
#include "parmac.h"
#include "gel/serializer/serializer.h"
#include "gel/timer/timecheck.h"
#include "utils/system_time.h"


static char *new_unique_filename(model_t *pmodel, name_t filename, unsigned long seed);
static int   name_intersection(name_t *as, int numa, name_t *bs, int numb);


static const name_t default_program_names[NUM_LINGUE] = {
    "Programma ",
    "Program ",
};


void model_init(model_t *pmodel) {
    assert(pmodel != NULL);

    pmodel->machine.communication_error   = 0;
    pmodel->machine.communication_enabled = 1;
    pmodel->machine.state                 = MACHINE_STATE_STOPPED;
    pmodel->machine.reported_step_type    = 0;

    pmodel->configuration.parmac.lingua    = LINGUA_ITALIANO;
    pmodel->configuration.num_programs     = 0;
    pmodel->configuration.programs_to_save = 0;
    pmodel->configuration.parmac_to_save   = 0;
    strcpy(pmodel->configuration.password, "");

    pmodel->run.program_number = 0;
    pmodel->run.step_number    = 0;

    pmodel->system.networks           = 0;
    pmodel->system.num_networks       = 0;
    pmodel->system.connected          = 0;
    pmodel->system.drive_machines     = NULL;
    pmodel->system.num_drive_machines = 0;

    parmac_init(pmodel);
}


const char *model_get_password(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.password;
}


void model_set_password(model_t *pmodel, const char *password) {
    assert(pmodel != NULL);
    strncpy(pmodel->configuration.password, password, PASSWORD_MAX_SIZE);
}


int model_is_in_test(model_t *pmodel) {
    assert(pmodel != NULL);
    return (pmodel->machine.function_flags & FUNCTION_FLAGS_TEST) > 0;
}


int model_is_machine_communication_active(model_t *pmodel) {
    assert(pmodel != NULL);
    return (pmodel->machine.communication_enabled && !pmodel->machine.communication_error);
}


int model_is_machine_communication_ok(model_t *pmodel) {
    assert(pmodel != NULL);
    return (!pmodel->machine.communication_enabled || !pmodel->machine.communication_error);
}


void model_set_machine_communication_error(model_t *pmodel, int error) {
    assert(pmodel != NULL);
    pmodel->machine.communication_error = error;
}


void model_set_machine_communication(model_t *pmodel, int enabled) {
    assert(pmodel != NULL);
    pmodel->machine.communication_enabled = enabled;
}


int model_is_machine_communication_enabled(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.communication_enabled;
}


size_t model_get_language(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.lingua;
}


unsigned int model_get_access_level(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->configuration.parmac.access_level == 0) {
        return USER_ACCESS_LEVEL;
    } else {
        return TECHNICIAN_ACCESS_LEVEL;
    }
}


void model_set_access_level_code(model_t *pmodel, uint16_t access_level) {
    assert(pmodel != NULL);
    pmodel->configuration.parmac.access_level = access_level;
}


uint16_t model_get_access_level_code(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.access_level;
}


const char *model_parameter_get_description(model_t *pmodel, parameter_handle_t *par) {
    assert(pmodel != NULL);
    pudata_t data = parameter_get_user_data(par);
    return data.desc[model_get_language(pmodel)];
}


const char *model_parameter_value_to_string(model_t *pmodel, parameter_handle_t *par, size_t index) {
    assert(pmodel != NULL);

    pudata_t data               = parameter_get_user_data(par);
    char *(*values)[NUM_LINGUE] = (char *(*)[NUM_LINGUE])data.values;

    return values[index][model_get_language(pmodel)];
}


int model_parameter_set_value(parameter_handle_t *par, uint16_t value) {
    *(uint16_t *)par->pointer = value;
    return parameter_check_ranges(par, 1);
}


int model_pick_up_machine_state(model_t *pmodel, uint16_t state, uint16_t program_number, uint16_t step_number) {
    if (state != MACHINE_STATE_STOPPED) {
        dryer_program_t *p = model_get_program(pmodel, program_number);
        if (p != NULL && step_number < p->num_steps) {
            model_resume_program(pmodel, program_number, step_number);
            pmodel->machine.state = state;
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


int model_update_flags(model_t *pmodel, uint16_t alarms, uint16_t flags) {
    assert(pmodel != NULL);

    if (pmodel->machine.function_flags != flags || pmodel->machine.alarms != alarms) {
        pmodel->machine.alarms         = alarms;
        pmodel->machine.function_flags = flags;
        return 1;
    } else {
        return 0;
    }
}


int model_update_machine_state(model_t *pmodel, uint16_t state, uint16_t step_type) {
    assert(pmodel != NULL);
    int res = 0;

    if (pmodel->machine.state != state) {
        pmodel->machine.state = state;
        res                   = 1;

        if (pmodel->machine.state == MACHINE_STATE_PAUSED) {
            pmodel->run.autostop_ts = get_millis();
        }
    }

    if (pmodel->machine.reported_step_type != step_type) {
        pmodel->machine.reported_step_type = step_type;
        res                                = 1;
    }

    return res;
}


int model_should_autostop(model_t *pmodel) {
    assert(pmodel != NULL);

    alarm_code_t code = ALARM_CODE_EMERGENZA;
    if (model_get_worst_alarm(pmodel, &code, 0)) {
        return code == ALARM_CODE_OBLO_APERTO && pmodel->machine.state == MACHINE_STATE_PAUSED &&
               pmodel->configuration.parmac.tempo_stop_automatico > 0 &&
               is_expired(pmodel->run.autostop_ts, get_millis(),
                          pmodel->configuration.parmac.tempo_stop_automatico * 1000UL);
    } else {
        return 0;
    }
}


uint16_t model_get_machine_state(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.state;
}


int model_is_machine_initialized(model_t *pmodel) {
    assert(pmodel != NULL);
    return (pmodel->machine.function_flags & FUNCTION_FLAGS_INITIALIZED) > 0;
}


int model_is_machine_running(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.state == MACHINE_STATE_RUNNING;
}


int model_is_machine_active(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.state == MACHINE_STATE_ACTIVE;
}


int model_is_machine_stopped(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.state == MACHINE_STATE_STOPPED;
}


void model_start_program(model_t *pmodel, size_t num) {
    model_resume_program(pmodel, num, 0);
}


void model_resume_program(model_t *pmodel, size_t num, size_t step_num) {
    assert(pmodel != NULL);
    assert(num < pmodel->configuration.num_programs);
    assert(pmodel->configuration.programs[num].num_steps > step_num);

    pmodel->run.program        = pmodel->configuration.programs[num];
    pmodel->run.program_number = num;
    pmodel->run.step_number    = step_num;
}


void model_stop_program(model_t *pmodel) {
    assert(pmodel != NULL);
    pmodel->run.step_number = 0;
}


int model_next_step(model_t *pmodel) {
    assert(pmodel != NULL);

    pmodel->run.step_number++;
    if (model_get_current_step(pmodel) == NULL) {
        return 0;
    } else {
        return 1;
    }
}


dryer_program_t *model_get_current_program(model_t *pmodel) {
    assert(pmodel != NULL);
    return &pmodel->run.program;
}


parameters_step_t *model_get_current_step(model_t *pmodel) {
    assert(pmodel != NULL);
    dryer_program_t *p = model_get_current_program(pmodel);
    if (pmodel->run.step_number < p->num_steps) {
        return &p->steps[pmodel->run.step_number];
    } else {
        return NULL;
    }
}


size_t model_get_current_program_number(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->run.program_number;
}


size_t model_get_current_step_number(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->run.step_number;
}


int model_is_valid_program(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    if (num >= pmodel->configuration.num_programs) {
        return 0;
    }
    return pmodel->configuration.programs[num].num_steps > 0;
}


int model_is_program_running(model_t *pmodel) {
    return pmodel->machine.state == MACHINE_STATE_RUNNING || pmodel->machine.state == MACHINE_STATE_ACTIVE ||
           pmodel->machine.state == MACHINE_STATE_PAUSED;
}


void model_set_remaining(model_t *pmodel, uint16_t remaining) {
    assert(pmodel != NULL);
    pmodel->machine.remaining = remaining;
}


uint16_t model_get_remaining(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.remaining;
}


uint16_t *model_get_minimum_speed(model_t *pmodel) {
    assert(pmodel != NULL);
    return &pmodel->configuration.parmac.velocita_minima;
}


uint16_t *model_get_maximum_speed(model_t *pmodel) {
    assert(pmodel != NULL);
    return &pmodel->configuration.parmac.velocita_massima;
}


uint16_t *model_get_current_setpoint_parameter(model_t *pmodel) {
    assert(pmodel != NULL);
    parameters_step_t *step = model_get_current_step(pmodel);
    if (step == NULL) {
        return NULL;
    }

    switch (step->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            return &step->drying.temperature;
        case DRYER_PROGRAM_STEP_TYPE_COOLING:
            return &step->cooling.temperature;
        default:
            return NULL;
    }
}


uint16_t *model_get_current_speed_parameter(model_t *pmodel) {
    assert(pmodel != NULL);
    parameters_step_t *step = model_get_current_step(pmodel);
    if (step == NULL) {
        return NULL;
    }

    switch (step->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            return &step->drying.speed;
        case DRYER_PROGRAM_STEP_TYPE_UNFOLDING:
            return &step->unfolding.speed;
        default:
            return NULL;
    }
}


uint16_t *model_get_current_humidity_parameter(model_t *pmodel) {
    assert(pmodel != NULL);
    parameters_step_t *step = model_get_current_step(pmodel);
    if (step == NULL) {
        return NULL;
    }

    switch (step->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            return &step->drying.humidity;
        default:
            return NULL;
    }
}


void model_swap_programs(model_t *pmodel, size_t first, size_t second) {
    if (first >= model_get_num_programs(pmodel))
        return;
    if (second >= model_get_num_programs(pmodel))
        return;
    if (first == second)
        return;

    dryer_program_t p                      = pmodel->configuration.programs[first];
    pmodel->configuration.programs[first]  = pmodel->configuration.programs[second];
    pmodel->configuration.programs[second] = p;
}


size_t model_get_effective_inputs(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->configuration.parmac.abilita_espansione_rs485) {
        return NUM_INPUTS;
    } else {
        return NUM_INTERNAL_INPUTS;
    }
}


size_t model_get_effective_outputs(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->configuration.parmac.abilita_espansione_rs485) {
        return NUM_RELES;
    } else {
        return NUM_INTERNAL_RELES;
    }
}


int model_is_drive_mounted(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->system.drive_mounted;
}


int model_update_drive_status(model_t *pmodel, int mounted) {
    assert(pmodel != NULL);
    if (pmodel->system.drive_mounted != mounted) {
        pmodel->system.drive_mounted = mounted;
        return 1;
    } else {
        return 0;
    }
}


size_t model_serialize_parmac(uint8_t *buffer, parmac_t *parmac) {
    assert(parmac != NULL && buffer != NULL);
    size_t i = 0;

    memcpy(&buffer[i], parmac->nome, sizeof(name_t));
    i += sizeof(name_t);

    i += serialize_uint16_be(&buffer[i], parmac->lingua);
    i += serialize_uint16_be(&buffer[i], parmac->abilita_visualizzazione_temperatura);
    i += serialize_uint16_be(&buffer[i], parmac->abilita_tasto_menu);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_pressione_tasto_pausa);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_pressione_tasto_stop);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_stop_automatico);
    i += serialize_uint16_be(&buffer[i], parmac->stop_tempo_ciclo);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_attesa_partenza_ciclo);
    i += serialize_uint16_be(&buffer[i], parmac->abilita_espansione_rs485);
    i += serialize_uint16_be(&buffer[i], parmac->abilita_gas);
    i += serialize_uint16_be(&buffer[i], parmac->velocita_minima);
    i += serialize_uint16_be(&buffer[i], parmac->velocita_massima);
    i += serialize_uint16_be(&buffer[i], parmac->velocita_antipiega);
    i += serialize_uint16_be(&buffer[i], parmac->tipo_pagamento);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_gettone);

    i += serialize_uint16_be(&buffer[i], parmac->tipo_sonda_temperatura);
    i += serialize_uint16_be(&buffer[i], parmac->posizione_sonda_temperatura);
    i += serialize_uint16_be(&buffer[i], parmac->temperatura_massima_ingresso);
    i += serialize_uint16_be(&buffer[i], parmac->temperatura_massima_uscita);
    i += serialize_uint16_be(&buffer[i], parmac->temperatura_sicurezza_in);
    i += serialize_uint16_be(&buffer[i], parmac->temperatura_sicurezza_out);
    i += serialize_uint16_be(&buffer[i], parmac->tempo_allarme_temperatura);
    i += serialize_uint16_be(&buffer[i], parmac->allarme_inverter_off_on);
    i += serialize_uint16_be(&buffer[i], parmac->allarme_filtro_off_on);
    i += serialize_uint16_be(&buffer[i], parmac->tipo_macchina_occupata);
    i += serialize_uint16_be(&buffer[i], parmac->inverti_macchina_occupata);
    i += serialize_uint16_be(&buffer[i], parmac->tipo_riscaldamento);

    i += serialize_uint16_be(&buffer[i], parmac->access_level);
    i += serialize_uint16_be(&buffer[i], parmac->autoavvio);
    i += serialize_uint16_be(&buffer[i], parmac->abilita_allarmi);

    assert(i == PARMAC_SIZE);
    return i;
}


size_t model_deserialize_parmac(parmac_t *parmac, uint8_t *buffer) {
    assert(parmac != NULL && buffer != NULL);
    size_t i = 0;

    memcpy(parmac->nome, &buffer[i], sizeof(name_t));
    i += sizeof(name_t);

    i += deserialize_uint16_be(&parmac->lingua, &buffer[i]);
    i += deserialize_uint16_be(&parmac->abilita_visualizzazione_temperatura, &buffer[i]);
    i += deserialize_uint16_be(&parmac->abilita_tasto_menu, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_pressione_tasto_pausa, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_pressione_tasto_stop, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_stop_automatico, &buffer[i]);
    i += deserialize_uint16_be(&parmac->stop_tempo_ciclo, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_attesa_partenza_ciclo, &buffer[i]);
    i += deserialize_uint16_be(&parmac->abilita_espansione_rs485, &buffer[i]);
    i += deserialize_uint16_be(&parmac->abilita_gas, &buffer[i]);
    i += deserialize_uint16_be(&parmac->velocita_minima, &buffer[i]);
    i += deserialize_uint16_be(&parmac->velocita_massima, &buffer[i]);
    i += deserialize_uint16_be(&parmac->velocita_antipiega, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tipo_pagamento, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_gettone, &buffer[i]);

    i += deserialize_uint16_be(&parmac->tipo_sonda_temperatura, &buffer[i]);
    i += deserialize_uint16_be(&parmac->posizione_sonda_temperatura, &buffer[i]);
    i += deserialize_uint16_be(&parmac->temperatura_massima_ingresso, &buffer[i]);
    i += deserialize_uint16_be(&parmac->temperatura_massima_uscita, &buffer[i]);
    i += deserialize_uint16_be(&parmac->temperatura_sicurezza_in, &buffer[i]);
    i += deserialize_uint16_be(&parmac->temperatura_sicurezza_out, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tempo_allarme_temperatura, &buffer[i]);
    i += deserialize_uint16_be(&parmac->allarme_inverter_off_on, &buffer[i]);
    i += deserialize_uint16_be(&parmac->allarme_filtro_off_on, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tipo_macchina_occupata, &buffer[i]);
    i += deserialize_uint16_be(&parmac->inverti_macchina_occupata, &buffer[i]);
    i += deserialize_uint16_be(&parmac->tipo_riscaldamento, &buffer[i]);

    i += deserialize_uint16_be(&parmac->access_level, &buffer[i]);
    i += deserialize_uint16_be(&parmac->autoavvio, &buffer[i]);
    i += deserialize_uint16_be(&parmac->abilita_allarmi, &buffer[i]);

    assert(i == PARMAC_SIZE);
    return i;
}


size_t model_get_num_programs(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.num_programs;
}


const char *model_get_program_name_in_language(model_t *pmodel, uint16_t language, size_t num) {
    assert(pmodel != NULL);
    assert(num < model_get_num_programs(pmodel));
    return model_get_program(pmodel, num)->nomi[language];
}


const char *model_get_program_name(model_t *pmodel, size_t num) {
    return model_get_program_name_in_language(pmodel, model_get_language(pmodel), num);
}


size_t model_get_max_programs(model_t *pmodel) {
    assert(pmodel != NULL);

    return 10;
}


void model_remove_program(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    size_t len = model_get_num_programs(pmodel);

    if (num >= len) {
        return;
    }

    for (size_t i = num; i < len - 1; i++) {
        pmodel->configuration.programs[i] = pmodel->configuration.programs[i + 1];
    }

    if (model_get_num_programs(pmodel) > 0) {
        pmodel->configuration.num_programs--;
    }
}


dryer_program_t *model_create_new_program_from(model_t *pmodel, size_t src, size_t pos, unsigned long timestamp) {
    assert(pmodel != NULL);
    if (model_get_num_programs(pmodel) >= model_get_max_programs(pmodel)) {
        return NULL;
    }

    assert(pos <= model_get_num_programs(pmodel));
    assert(src < model_get_num_programs(pmodel));

    // Copia il sorgente
    dryer_program_t from = *model_get_program(pmodel, src);
    new_unique_filename(pmodel, from.filename, timestamp);

    // Aggiungi un programma in fondo
    pmodel->configuration.num_programs++;
    // Dopo questo loop pos contiene un programma libero
    for (size_t i = model_get_num_programs(pmodel) - 1; i > pos; i--) {
        model_swap_programs(pmodel, i, i - 1);
    }

    dryer_program_t *new = &pmodel->configuration.programs[pos];
    *new                 = from;

    return new;
}



dryer_program_t *model_create_new_program(model_t *pmodel, const char *name, size_t lingua, unsigned long timestamp,
                                          size_t *index) {
    assert(pmodel != NULL && lingua < NUM_LINGUE);

    if (pmodel->configuration.num_programs >= pmodel->configuration.parmac.max_programs)
        return NULL;

    dryer_program_t *p = &pmodel->configuration.programs[pmodel->configuration.num_programs];
    if (index != NULL) {
        *index = pmodel->configuration.num_programs;
    }
    program_init(p, (name_t *)default_program_names, pmodel->configuration.num_programs + 1);
    new_unique_filename(pmodel, &p->filename[0], timestamp);
    strncpy(p->nomi[lingua], name, sizeof(p->nomi[lingua]));
    p->nomi[lingua][sizeof(p->nomi[lingua]) - 1] = '\0';
    pmodel->configuration.num_programs++;
    return p;
}


dryer_program_t *model_get_program(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);

    if (num >= model_get_num_programs(pmodel)) {
        return NULL;
    } else {
        return &pmodel->configuration.programs[num];
    }
}


void model_mark_parmac_to_save(model_t *pmodel) {
    assert(pmodel != NULL);
    pmodel->configuration.parmac_to_save = 1;
}


void model_mark_program_to_save(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    pmodel->configuration.programs_to_save |= (1 << num);
}


uint64_t model_get_programs_to_save(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.programs_to_save;
}


int model_get_parmac_to_save(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac_to_save;
}


void model_clear_marks_to_save(model_t *pmodel) {
    assert(pmodel != NULL);
    pmodel->configuration.programs_to_save = 0;
    pmodel->configuration.parmac_to_save   = 0;
}


const char *model_format_parameter_value(model_t *pmodel, parameter_handle_t *pars, size_t len_pars, char *string,
                                         size_t len, size_t num) {
    assert(pmodel != NULL);
    parameter_handle_t *par = parameter_get_handle(pars, len_pars, num, model_get_access_level(pmodel));

    pudata_t data = parameter_get_user_data(par);

    switch (data.t) {
        case PTYPE_SWITCH:
        case PTYPE_DROPDOWN: {
            size_t index = parameter_to_index(par);
            return model_parameter_value_to_string(pmodel, par, index);
        }

        case PTYPE_NUMBER: {
            long value = parameter_to_long(par);
            if (data.fmt == NULL) {
                snprintf(string, len, "%li", value);
            } else if (data.min_sec) {
                snprintf(string, len, data.fmt, value / 60, value % 60);
            } else {
                snprintf(string, len, data.fmt, value);
            }

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


int model_get_machine_current_step_type(model_t *pmodel, uint16_t *step_type) {
    assert(pmodel != NULL);
    if (model_is_program_running(pmodel)) {
        *step_type = pmodel->machine.reported_step_type;
        return 1;
    } else {
        return 0;
    }
}


int model_is_any_alarm_active(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.alarms > 0;
}


int model_get_worst_alarm(model_t *pmodel, alarm_code_t *code, uint16_t exclude_mask) {
    assert(pmodel != NULL);
    for (size_t i = 0; i < sizeof(pmodel->machine.alarms); i++) {
        if ((exclude_mask & (1 << i)) > 0) {
            continue;
        }

        if ((pmodel->machine.alarms & (1 << i)) > 0) {
            *code = i;
            return 1;
        }
    }
    return 0;
}


int model_is_porthole_open(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.alarms & (1 << ALARM_CODE_OBLO_APERTO);
}


uint16_t model_get_alarms(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.alarms;
}


int model_update_sensors(model_t *pmodel, uint16_t *coins, uint16_t payment, uint16_t t1_adc, uint16_t t2_adc,
                         uint16_t t1, uint16_t t2, uint16_t actual_temperature, uint16_t actual_humidity) {
    assert(pmodel != NULL);
    int res = 0;

    if (memcmp(pmodel->machine.coins, coins, sizeof(pmodel->machine.coins)) || pmodel->machine.payment != payment) {
        memcpy(pmodel->machine.coins, coins, sizeof(pmodel->machine.coins));
        pmodel->machine.payment = payment;
        res                     = 1;
    }

    if (pmodel->machine.adc_ptc1 != t1_adc || pmodel->machine.adc_ptc2 != t2_adc ||
        t1 != pmodel->machine.temperature_ptc1 || t2 != pmodel->machine.temperature_ptc2 ||
        actual_temperature != pmodel->machine.actual_temperature ||
        actual_humidity != pmodel->machine.actual_humidity) {
        pmodel->machine.adc_ptc1           = t1_adc;
        pmodel->machine.adc_ptc2           = t2_adc;
        pmodel->machine.temperature_ptc1   = (int16_t)t1;
        pmodel->machine.temperature_ptc2   = (int16_t)t2;
        pmodel->machine.actual_temperature = (int16_t)actual_temperature;
        pmodel->machine.actual_humidity    = (int16_t)actual_humidity / 100;
        res                                = 1;
    }

    return res;
}


int model_current_temperature_setpoint(model_t *pmodel) {
    assert(pmodel != NULL);
    if (model_is_program_running(pmodel)) {
        parameters_step_t *s = model_get_current_step(pmodel);
        if (s == NULL) {
            return 0;
        }
        switch (s->type) {
            case DRYER_PROGRAM_STEP_TYPE_DRYING:
                return s->drying.temperature;
            case DRYER_PROGRAM_STEP_TYPE_COOLING:
                return s->cooling.temperature;
            default:
                return 0;
        }
    } else {
        return 0;
    }
}


int model_current_humidity_setpoint(model_t *pmodel) {
    assert(pmodel != NULL);
    if (model_is_program_running(pmodel)) {
        parameters_step_t *s = model_get_current_step(pmodel);
        if (s == NULL) {
            return 0;
        }
        switch (s->type) {
            case DRYER_PROGRAM_STEP_TYPE_DRYING:
                return s->drying.humidity;
            default:
                return 0;
        }
    } else {
        return 0;
    }
}


int model_current_temperature(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.actual_temperature;
}


int model_current_humidity(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->machine.actual_humidity;
}


int model_wifi_status_changed(model_t *pmodel, wifi_status_t status) {
    assert(pmodel != NULL);
    if (status != WIFI_SCANNING && pmodel->system.net_status != status) {
        pmodel->system.net_status = status;
        return 1;
    } else {
        return 0;
    }
}


int model_display_temperature(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.abilita_visualizzazione_temperatura;
}


int model_display_humidity(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.abilita_visualizzazione_temperatura &&
           pmodel->configuration.parmac.tipo_sonda_temperatura == SONDA_TEMPERATURA_SHT;
}


uint16_t model_get_pause_press_time(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.tempo_pressione_tasto_pausa;
}


uint16_t model_get_stop_press_time(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.parmac.tempo_pressione_tasto_stop;
}


int model_is_current_archive_in_drive(model_t *pmodel) {
    assert(pmodel != NULL);
    return name_intersection(pmodel->system.drive_machines, pmodel->system.num_drive_machines,
                             &pmodel->configuration.parmac.nome, 1);
}


int model_menu_view_enabled(model_t *pmodel) {
    assert(pmodel != NULL);
    return model_is_program_running(pmodel) && pmodel->configuration.parmac.abilita_tasto_menu;
}


void model_update_statistics(model_t *pmodel, statistics_t stats) {
    assert(pmodel != NULL);
    pmodel->statistics = stats;
}


/*
 *  STATIC FUNCTIONS
 */

static char *new_unique_filename(model_t *pmodel, name_t filename, unsigned long seed) {
    assert(pmodel != NULL);
    unsigned long now = seed;
    int           found;

    do {
        found = 0;
        snprintf(filename, STRING_NAME_SIZE, "%lu.bin", now);

        for (size_t i = 0; i < pmodel->configuration.num_programs; i++) {
            if (strcmp(filename, pmodel->configuration.programs[i].filename) == 0) {
                now++;
                found = 1;
                break;
            }
        }
    } while (found);

    return filename;
}


static int name_intersection(name_t *as, int numa, name_t *bs, int numb) {
    for (int i = 0; i < numa; i++) {
        for (int j = 0; j < numb; j++) {
            if (strcmp(as[i], bs[j]) == 0) {
                return 1;
            }
        }
    }

    return 0;
}