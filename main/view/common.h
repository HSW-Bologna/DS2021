#ifndef VIEW_COMMON_H_INCLUDED
#define VIEW_COMMON_H_INCLUDED


#include "lvgl.h"
#include "model/model.h"


lv_obj_t *view_common_create_blanket(lv_obj_t *parent);
lv_obj_t *view_common_create_popup(lv_obj_t *parent, int covering);
lv_obj_t *view_common_create_info_popup(lv_obj_t *parent, int covering, const lv_img_dsc_t *src, const char *text,
                                        const char *confirm_text, int confirm_id);

lv_obj_t *view_common_build_param_editor(lv_obj_t *root, lv_obj_t **textarea, model_t *pmodel, parameter_handle_t *par,
                                         size_t num, int dd_id, int sw_id, int ta_id, int kb_id, int roller1_id,
                                         int roller2_id, int cancel_id, int confirm_id, int default_id);

lv_obj_t *view_common_create_choice_popup(lv_obj_t *parent, int covering, const char *text, const char *confirm_text,
                                          int confirm_id, const char *refuse_text, int refuse_id);

void      view_common_roller_set_number(lv_obj_t *roller, int num);
lv_obj_t *view_common_back_button(lv_obj_t *parent, int id);
lv_obj_t *view_common_create_title(lv_obj_t *parent, const char *title, int back_id);
void      view_common_select_btn_in_list(lv_obj_t *list, int selected);
lv_obj_t *view_common_icon_button(lv_obj_t *parent, char *symbol, int id);
lv_obj_t *view_common_toast_with_parent(const char *msg, lv_obj_t *parent);
lv_obj_t *view_common_toast(const char *msg);
lv_obj_t *view_common_create_password_popup(lv_obj_t *parent, int kb_id, int hidden);
lv_obj_t *view_common_create_simple_image_button(lv_obj_t *parent, const lv_img_dsc_t *src_on,
                                                 const lv_img_dsc_t *src_off, int id);
lv_obj_t *view_common_create_image_button(lv_obj_t *parent, const lv_img_dsc_t *src, const lv_img_dsc_t *src_pressed,
                                          const lv_img_dsc_t *src_checked, int id);
lv_obj_t *view_common_io_error_toast(model_t *pmodel);


#endif