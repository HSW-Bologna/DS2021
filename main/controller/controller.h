#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED


#include "model/model.h"
#include "view/view.h"


void controller_init(model_t *pmodel);
void controller_process_msg(view_controller_message_t *msg, model_t *pmodel);
void controller_manage(model_t *pmodel);
void controller_manage_message(lv_pman_handle_t handle, void *msg);

#endif
