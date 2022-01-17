#include <assert.h>
#include "intl.h"
#include "model/descriptions/AUTOGEN_FILE_parameters.h"


const char *view_intl_get_string_in_language(uint16_t language, strings_t id) {
    return strings[id][language];
}


const char *view_intl_get_string(model_t *pmodel, strings_t id) {
    return view_intl_get_string_in_language(model_get_language(pmodel), id);
}


const char *view_intl_step_description(model_t *pmodel, dryer_program_step_type_t step) {
    assert(step < NUM_DRYER_PROGRAM_STEP_TYPES);
    return parameters_tipi_step[step][model_get_language(pmodel)];
}