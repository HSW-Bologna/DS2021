#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED


#include <stdint.h>
#include <stdlib.h>


#define MAX_STEPS           16
#define MAX_PROGRAMS        32
#define MAX_NAME_SIZE       32
#define STEP_SIZE           32
#define PROGRAM_SIZE(steps) ((size_t)(338 + STEP_SIZE * steps))
#define MAX_PROGRAM_SIZE    PROGRAM_SIZE(MAX_STEPS)
#define STRING_NAME_SIZE    (MAX_NAME_SIZE + 1)

#define MAX_STEP_DURATION (3600)
#define MAX_TEMPERATURE   (100)
#define MAX_HUMIDITY      (100)
#define MAX_SPEED         (100)


typedef enum {
    DRYER_PROGRAM_STEP_TYPE_DRYING = 0,
    DRYER_PROGRAM_STEP_TYPE_COOLING,
    DRYER_PROGRAM_STEP_TYPE_UNFOLDING,
    NUM_DRYER_PROGRAM_STEP_TYPES,
} dryer_program_step_type_t;


enum {
    LINGUA_ITALIANO = 0,
    LINGUA_INGLESE,
    NUM_LINGUE,
};

typedef char name_t[STRING_NAME_SIZE];

typedef struct {
    uint16_t type;

    union {
        struct {
            uint16_t type;
            uint16_t duration;
            uint16_t enable_waiting_for_temperature;
            uint16_t enable_reverse;
            uint16_t rotation_time;
            uint16_t pause_time;
            uint16_t speed;
            uint16_t temperature;
            uint16_t humidity;

            uint16_t cooling_hysteresis;
            uint16_t heating_hysteresis;
            uint16_t vaporization_temperature;
            uint16_t vaporization_duration;
        } drying;

        struct {
            uint16_t type;
            uint16_t duration;
            uint16_t enable_reverse;
            uint16_t rotation_time;
            uint16_t pause_time;
            uint16_t temperature;
            uint16_t deodorant_delay;
            uint16_t deodorant_duration;
        } cooling;

        struct {
            uint16_t max_duration;
            uint16_t max_cycles;
            uint16_t speed;
            uint16_t rotation_time;
            uint16_t pause_time;
            uint16_t start_delay;
        } unfolding;
    };
} parameters_step_t;


typedef struct {
    name_t            filename;
    name_t            nomi[NUM_LINGUE];
    uint16_t          type;
    uint16_t          num_steps;
    parameters_step_t steps[MAX_STEPS];
} dryer_program_t;


void     program_init(dryer_program_t *p, name_t *names, int num);
size_t   program_deserialize(dryer_program_t *p, uint8_t *buffer);
size_t   program_serialize(uint8_t *buffer, dryer_program_t *p);
void     program_remove_step(dryer_program_t *p, int index);
void     program_insert_step(dryer_program_t *p, int tipo, size_t index);
void     program_add_step(dryer_program_t *p, int tipo);
void     program_swap_steps(dryer_program_t *p, int first, int second);
void     program_copy_step(dryer_program_t *p, size_t src, size_t pos);
uint16_t program_get_total_time(dryer_program_t *p);

#endif