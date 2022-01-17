#ifndef PARCICLO_H_INCLUDED
#define PARCICLO_H_INCLUDED

#include "model.h"


void                parciclo_init(model_t *pmodel, size_t num_prog, size_t num_step);
size_t              parciclo_get_total_parameters(model_t *pmodel);
const char         *parciclo_get_description(model_t *pmodel, size_t num);
const char         *parciclo_to_string(model_t *pmodel, char *string, size_t len, size_t num);
parameter_handle_t *parciclo_get_handle(model_t *pmodel, size_t num);


#endif