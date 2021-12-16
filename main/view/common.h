#ifndef VIEW_COMMON_H_INCLUDED
#define VIEW_COMMON_H_INCLUDED


#include "lvgl.h"
#include "model/model.h"


lv_obj_t *view_common_create_popup(lv_obj_t *parent, int covering);
lv_obj_t *view_common_create_info_popup(lv_obj_t *parent, int covering, const char *text, const char *confirm_text,
                                        int confirm_id);

lv_obj_t *view_common_build_param_editor(lv_obj_t *root, model_t *pmodel, parameter_handle_t *par, size_t num,
                                         int dd_id, int sw_id, int ta_id, int kb_id, int roller1_id, int roller2_id,
                                         int cancel_id, int confirm_id, int default_id);

lv_obj_t *view_common_create_choice_popup(lv_obj_t *parent, int covering, const char *text, const char *confirm_text,
                                          int confirm_id, const char *refuse_text, int refuse_id);

void view_common_roller_set_number(lv_obj_t *roller, int num);

#endif