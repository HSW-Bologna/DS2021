#include "lvgl.h"
#include "style.h"


lv_style_t style_icon_button;


void style_init(void) {
    lv_style_init(&style_icon_button);
    lv_style_set_text_font(&style_icon_button, &lv_font_montserrat_28);
}