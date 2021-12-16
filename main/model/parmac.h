#ifndef PARMAC_H_INCLUDED
#define PARMAC_H_INCLUDED


#include "model.h"


void                parmac_init(model_t *p);
size_t              parmac_get_total_parameters(model_t *pmodel);
const char         *parmac_get_description(model_t *pmodel, size_t num);
const char         *parmac_to_string(model_t *pmodel, char *string, size_t len, size_t num);
parameter_handle_t *parmac_get_handle(model_t *pmodel, size_t num);

#endif