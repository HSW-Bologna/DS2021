#ifndef MODEL_UPDATER_H_INCLUDED
#define MODEL_UPDATER_H_INCLUDED


#include "model.h"


typedef struct {
    uint8_t _padding[8];
} static_model_updater_t;
typedef struct model_updater *model_updater_t;


model_updater_t model_updater_init(model_t *pmodel, static_model_updater_t *buffer);
const model_t  *model_updater_get(model_updater_t updater);


#endif
