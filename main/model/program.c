#include <assert.h>
#include "model.h"
#include "gel/serializer/serializer.h"


static parameters_step_t default_step(int tipo);
static size_t            serialize_step(uint8_t *buffer, parameters_step_t *s);
static size_t            deserialize_step(parameters_step_t *s, uint8_t *buffer);


void program_init(dryer_program_t *p, name_t *names, int num) {
    assert(p != NULL);
    p->filename[0] = '\0';

    for (size_t i = 0; i < NUM_LINGUE; i++) {
        strcpy(p->nomi[i], names[i]);
    }

    p->num_steps = 0;
}


uint16_t program_get_total_time(dryer_program_t *p) {
    uint16_t total = 0;

    for (size_t i = 0; i < p->num_steps; i++) {
        parameters_step_t *s = &p->steps[i];
        switch (s->type) {
            case DRYER_PROGRAM_STEP_TYPE_DRYING:
                total += s->drying.duration;
                break;

            case DRYER_PROGRAM_STEP_TYPE_COOLING:
                total += s->cooling.duration;
                break;

            case DRYER_PROGRAM_STEP_TYPE_UNFOLDING:
                total += s->unfolding.max_duration;
                break;
        }
    }

    return total;
}


size_t program_serialize(uint8_t *buffer, dryer_program_t *p) {
    assert(p != NULL && buffer != NULL);
    size_t i = 0;

    for (size_t j = 0; j < NUM_LINGUE; j++) {
        memcpy(&buffer[i], p->nomi[j], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += serialize_uint16_be(&buffer[i], p->type);
    i += serialize_uint16_be(&buffer[i], p->num_steps);

    for (size_t j = 0; j < p->num_steps; j++) {
        i += serialize_step(&buffer[i], &p->steps[j]);
    }

    return i;
}


size_t program_deserialize(dryer_program_t *p, uint8_t *buffer) {
    assert(p != NULL && buffer != NULL);
    size_t i = 0;

    for (size_t j = 0; j < NUM_LINGUE; j++) {
        memcpy(p->nomi[j], &buffer[i], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += deserialize_uint16_be(&p->type, &buffer[i]);
    i += deserialize_uint16_be(&p->num_steps, &buffer[i]);

    for (size_t j = 0; j < p->num_steps; j++) {
        i += deserialize_step(&p->steps[j], &buffer[i]);
    }

    return i;
}


void program_add_step(dryer_program_t *p, int tipo) {
    program_insert_step(p, tipo, p->num_steps);
}


void program_insert_step(dryer_program_t *p, int tipo, size_t index) {
    if (p->num_steps < MAX_STEPS && index <= p->num_steps) {
        for (int i = (int)p->num_steps - 1; i >= (int)index; i--)
            p->steps[i + 1] = p->steps[i];

        p->steps[index] = default_step(tipo);
        p->num_steps++;
    }
}


void program_remove_step(dryer_program_t *p, int index) {
    int len = p->num_steps;

    for (int i = index; i < len - 1; i++) {
        p->steps[i] = p->steps[i + 1];
    }

    if (p->num_steps > 0) {
        p->num_steps--;
    }
}


void program_swap_steps(dryer_program_t *p, int first, int second) {
    if (first != second) {
        parameters_step_t s = p->steps[first];
        p->steps[first]     = p->steps[second];
        p->steps[second]    = s;
    }
}


void program_copy_step(dryer_program_t *p, size_t src, size_t pos) {
    assert(p != NULL);
    if (p->num_steps >= MAX_STEPS) {
        return;
    }

    assert(pos <= p->num_steps);
    assert(src < p->num_steps);

    // Copia il sorgente
    parameters_step_t from = p->steps[src];

    // Aggiungi uno step in fondo
    p->num_steps++;
    // Dopo questo loop pos contiene uno step libero
    for (size_t i = p->num_steps - 1; i > pos; i--) {
        program_swap_steps(p, i, i - 1);
    }

    parameters_step_t *new = &p->steps[pos];
    *new                   = from;
}


static parameters_step_t default_step(int tipo) {
    parameters_step_t step = {.type = tipo};
    return step;
}


static size_t serialize_step(uint8_t *buffer, parameters_step_t *s) {
    size_t i = 0;
    i += serialize_uint16_be(&buffer[i], s->type);

    switch (s->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            i += serialize_uint16_be(&buffer[i], s->drying.type);
            i += serialize_uint16_be(&buffer[i], s->drying.duration);
            i += serialize_uint16_be(&buffer[i], s->drying.enable_waiting_for_temperature);
            i += serialize_uint16_be(&buffer[i], s->drying.enable_reverse);
            i += serialize_uint16_be(&buffer[i], s->drying.rotation_time);
            i += serialize_uint16_be(&buffer[i], s->drying.pause_time);
            i += serialize_uint16_be(&buffer[i], s->drying.speed);
            i += serialize_uint16_be(&buffer[i], s->drying.temperature);
            i += serialize_uint16_be(&buffer[i], s->drying.humidity);
            i += serialize_uint16_be(&buffer[i], s->drying.cooling_hysteresis);
            i += serialize_uint16_be(&buffer[i], s->drying.heating_hysteresis);
            i += serialize_uint16_be(&buffer[i], s->drying.vaporization_temperature);
            i += serialize_uint16_be(&buffer[i], s->drying.vaporization_duration);
            break;

        case DRYER_PROGRAM_STEP_TYPE_COOLING:
            i += serialize_uint16_be(&buffer[i], s->cooling.type);
            i += serialize_uint16_be(&buffer[i], s->cooling.duration);
            i += serialize_uint16_be(&buffer[i], s->cooling.enable_reverse);
            i += serialize_uint16_be(&buffer[i], s->cooling.rotation_time);
            i += serialize_uint16_be(&buffer[i], s->cooling.pause_time);
            i += serialize_uint16_be(&buffer[i], s->cooling.temperature);
            i += serialize_uint16_be(&buffer[i], s->cooling.deodorant_delay);
            i += serialize_uint16_be(&buffer[i], s->cooling.deodorant_duration);
            break;

        case DRYER_PROGRAM_STEP_TYPE_UNFOLDING:
            i += serialize_uint16_be(&buffer[i], s->unfolding.max_duration);
            i += serialize_uint16_be(&buffer[i], s->unfolding.max_cycles);
            i += serialize_uint16_be(&buffer[i], s->unfolding.rotation_time);
            i += serialize_uint16_be(&buffer[i], s->unfolding.pause_time);
            i += serialize_uint16_be(&buffer[i], s->unfolding.speed);
            i += serialize_uint16_be(&buffer[i], s->unfolding.start_delay);
            break;

        default:
            break;
    }

    assert(i <= STEP_SIZE);
    return STEP_SIZE;
}


static size_t deserialize_step(parameters_step_t *s, uint8_t *buffer) {
    size_t i = 0;
    i += deserialize_uint16_be(&s->type, &buffer[i]);

    switch (s->type) {
        case DRYER_PROGRAM_STEP_TYPE_DRYING:
            i += deserialize_uint16_be(&s->drying.type, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.duration, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.enable_waiting_for_temperature, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.enable_reverse, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.rotation_time, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.pause_time, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.speed, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.temperature, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.humidity, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.cooling_hysteresis, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.heating_hysteresis, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.vaporization_temperature, &buffer[i]);
            i += deserialize_uint16_be(&s->drying.vaporization_duration, &buffer[i]);
            break;

        case DRYER_PROGRAM_STEP_TYPE_COOLING:
            i += deserialize_uint16_be(&s->cooling.type, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.duration, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.enable_reverse, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.rotation_time, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.pause_time, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.temperature, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.deodorant_delay, &buffer[i]);
            i += deserialize_uint16_be(&s->cooling.deodorant_duration, &buffer[i]);
            break;

        case DRYER_PROGRAM_STEP_TYPE_UNFOLDING:
            i += deserialize_uint16_be(&s->unfolding.max_duration, &buffer[i]);
            i += deserialize_uint16_be(&s->unfolding.max_cycles, &buffer[i]);
            i += deserialize_uint16_be(&s->unfolding.rotation_time, &buffer[i]);
            i += deserialize_uint16_be(&s->unfolding.pause_time, &buffer[i]);
            i += deserialize_uint16_be(&s->unfolding.speed, &buffer[i]);
            i += deserialize_uint16_be(&s->unfolding.start_delay, &buffer[i]);
            break;

        default:
            break;
    }

    assert(i <= STEP_SIZE);
    return STEP_SIZE;
}