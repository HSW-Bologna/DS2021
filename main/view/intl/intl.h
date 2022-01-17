#ifndef INTL_H_INCLUDED
#define INTL_H_INCLUDED

#include "AUTOGEN_FILE_strings.h"
#include "model/model.h"

const char *view_intl_get_string(model_t *pmodel, strings_t id);
const char *view_intl_get_string_in_language(uint16_t language, strings_t id);
const char *view_intl_step_description(model_t *pmodel, dryer_program_step_type_t step);

#endif