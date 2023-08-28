#include <assert.h>
#include "model_updater.h"



struct model_updater {
    model_t *pmodel;
};


model_updater_t model_updater_init(model_t *pmodel, static_model_updater_t *buffer) {
    assert(sizeof(struct model_updater) == sizeof(static_model_updater_t));
    model_updater_t updater = (model_updater_t)buffer;
    updater->pmodel         = pmodel;
    return updater;
}


const model_t *model_updater_get(model_updater_t updater) {
    assert(updater != NULL);
    return (const model_t *)updater->pmodel;
}
