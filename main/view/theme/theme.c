#include "../widgets/custom_tabview.h"
#include "theme.h"

#define PAD_DEF   lv_disp_dpx(th->disp, 24)
#define PAD_SMALL lv_disp_dpx(th->disp, 14)

#define TRANSITION_TIME  LV_THEME_DEFAULT_TRANSITION_TIME
#define DARK_COLOR_SCR   lv_color_hex(0x15171A)
#define DARK_COLOR_CARD  lv_color_hex(0x282b30)
#define DARK_COLOR_TEXT  lv_palette_lighten(LV_PALETTE_GREY, 5)
#define DARK_COLOR_GREY  lv_color_hex(0x2f3237)
#define LIGHT_COLOR_SCR  lv_palette_lighten(LV_PALETTE_GREY, 4)
#define LIGHT_COLOR_CARD lv_color_white()
#define LIGHT_COLOR_TEXT lv_palette_darken(LV_PALETTE_GREY, 4)
#define LIGHT_COLOR_GREY lv_palette_lighten(LV_PALETTE_GREY, 2)
#define OUTLINE_WIDTH    lv_disp_dpx(th->disp, 3)
#define BORDER_WIDTH     lv_disp_dpx(th->disp, 2)

static void       theme_apply_cb(lv_theme_t *th, lv_obj_t *obj);
static lv_color_t dark_color_filter_cb(const lv_color_filter_dsc_t *f, lv_color_t c, lv_opa_t opa);

static const lv_style_const_prop_t style_btn_props[] = {
    LV_STYLE_CONST_HEIGHT(60),
};
static LV_STYLE_CONST_INIT(style_btn, style_btn_props);

static const lv_style_const_prop_t style_label_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    {.prop = LV_STYLE_PROP_INV},
};
static LV_STYLE_CONST_INIT(style_label, style_label_props);

static const lv_style_const_prop_t style_all_props[] = {
    {.prop = LV_STYLE_PROP_INV},
};
static LV_STYLE_CONST_INIT(style_all, style_all_props);

static const lv_style_const_prop_t style_transparent_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_TEXT_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),
    {.prop = LV_STYLE_PROP_INV},
};
static LV_STYLE_CONST_INIT(style_transparent, style_transparent_props);


static lv_style_t pad_normal;
static lv_style_t scrollbar;
static lv_style_t scrollbar_scrolled;
static lv_style_t bg_color_white;
static lv_style_t outline_primary;
static lv_style_t tab_bg_focus;
static lv_style_t pressed;
static lv_style_t bg_color_primary_muted;
static lv_style_t tab_btn;
static lv_style_t outline_primary;
static lv_style_t outline_secondary;
static lv_style_t tab_bg_focus;
static lv_style_t scr;
static lv_style_t pad_zero;


void theme_init(lv_disp_t *disp) {
    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_GREY), lv_palette_main(LV_PALETTE_BLUE),
                                           LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);

    /*Initialize the styles*/
    static const lv_style_prop_t trans_props[] = {LV_STYLE_BG_OPA,
                                                  LV_STYLE_BG_COLOR,
                                                  LV_STYLE_TRANSFORM_WIDTH,
                                                  LV_STYLE_TRANSFORM_HEIGHT,
                                                  LV_STYLE_TRANSLATE_Y,
                                                  LV_STYLE_TRANSLATE_X,
                                                  LV_STYLE_TRANSFORM_ZOOM,
                                                  LV_STYLE_TRANSFORM_ANGLE,
                                                  LV_STYLE_COLOR_FILTER_OPA,
                                                  LV_STYLE_COLOR_FILTER_DSC,
                                                  0};

    static lv_style_transition_dsc_t trans_normal;
    lv_style_transition_dsc_init(&trans_normal, trans_props, lv_anim_path_linear, TRANSITION_TIME, 0, NULL);

    lv_style_init(&pad_normal);
    lv_style_set_pad_all(&pad_normal, PAD_DEF);
    lv_style_set_pad_row(&pad_normal, PAD_DEF);
    lv_style_set_pad_column(&pad_normal, PAD_DEF);

    lv_style_init(&scrollbar);
    lv_style_set_bg_color(&scrollbar, lv_palette_darken(LV_PALETTE_GREY, 2));
    lv_style_set_radius(&scrollbar, LV_RADIUS_CIRCLE);
    lv_style_set_pad_right(&scrollbar, lv_disp_dpx(th->disp, 7));
    lv_style_set_pad_top(&scrollbar, lv_disp_dpx(th->disp, 7));
    lv_style_set_size(&scrollbar, lv_disp_dpx(th->disp, 5));
    lv_style_set_bg_opa(&scrollbar, LV_OPA_40);
    lv_style_set_transition(&scrollbar, &trans_normal);

    lv_style_init(&scrollbar_scrolled);
    lv_style_set_bg_opa(&scrollbar_scrolled, LV_OPA_COVER);

    lv_style_init(&bg_color_white);
    lv_style_set_bg_color(&bg_color_white, LIGHT_COLOR_CARD);
    lv_style_set_bg_opa(&bg_color_white, LV_OPA_COVER);
    lv_style_set_text_color(&bg_color_white, LIGHT_COLOR_TEXT);

    lv_style_init(&outline_primary);
    lv_style_set_outline_color(&outline_primary, th->color_primary);
    lv_style_set_outline_width(&outline_primary, OUTLINE_WIDTH);
    lv_style_set_outline_pad(&outline_primary, OUTLINE_WIDTH);
    lv_style_set_outline_opa(&outline_primary, LV_OPA_50);

    lv_style_init(&tab_btn);
    lv_style_set_border_color(&tab_btn, th->color_primary);
    lv_style_set_border_width(&tab_btn, BORDER_WIDTH * 2);
    lv_style_set_border_side(&tab_btn, LV_BORDER_SIDE_BOTTOM);

    lv_style_init(&tab_bg_focus);
    lv_style_set_outline_pad(&tab_bg_focus, -BORDER_WIDTH);

    static lv_color_filter_dsc_t dark_filter;
    lv_color_filter_dsc_init(&dark_filter, dark_color_filter_cb);

    lv_style_init(&pressed);
    lv_style_set_color_filter_dsc(&pressed, &dark_filter);
    lv_style_set_color_filter_opa(&pressed, 35);

    lv_style_init(&bg_color_primary_muted);
    lv_style_set_bg_color(&bg_color_primary_muted, th->color_primary);
    lv_style_set_text_color(&bg_color_primary_muted, th->color_primary);
    lv_style_set_bg_opa(&bg_color_primary_muted, LV_OPA_20);

    lv_style_init(&outline_primary);
    lv_style_set_outline_color(&outline_primary, th->color_primary);
    lv_style_set_outline_width(&outline_primary, OUTLINE_WIDTH);
    lv_style_set_outline_pad(&outline_primary, OUTLINE_WIDTH);
    lv_style_set_outline_opa(&outline_primary, LV_OPA_50);

    lv_style_init(&outline_secondary);
    lv_style_set_outline_color(&outline_secondary, th->color_secondary);
    lv_style_set_outline_width(&outline_secondary, OUTLINE_WIDTH);
    lv_style_set_outline_opa(&outline_secondary, LV_OPA_50);

    lv_style_init(&tab_bg_focus);
    lv_style_set_outline_pad(&tab_bg_focus, -BORDER_WIDTH);

    lv_style_init(&scr);
    lv_style_set_bg_opa(&scr, LV_OPA_COVER);
    lv_style_set_bg_color(&scr, LIGHT_COLOR_SCR);
    lv_style_set_text_color(&scr, LIGHT_COLOR_TEXT);
    lv_style_set_pad_row(&scr, PAD_SMALL);
    lv_style_set_pad_column(&scr, PAD_SMALL);

    lv_style_init(&pad_zero);
    lv_style_set_pad_all(&pad_zero, 0);
    lv_style_set_pad_row(&pad_zero, 0);
    lv_style_set_pad_column(&pad_zero, 0);

    /*Initialize the new theme from the current theme*/
    static lv_theme_t th_new;
    th_new = *th;

    /*Set the parent theme and the style apply callback for the new theme*/
    lv_theme_set_parent(&th_new, th);
    lv_theme_set_apply_cb(&th_new, theme_apply_cb);

    /*Assign the new theme the the current display*/
    lv_disp_set_theme(disp, &th_new);
}


static void theme_apply_cb(lv_theme_t *th, lv_obj_t *obj) {
    LV_UNUSED(th);

    if (lv_obj_check_type(obj, &lv_obj_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        /*Tabview content area*/
        if (lv_obj_check_type(parent, &custom_tabview_class)) {
            lv_obj_remove_style_all(obj);
            return;
        }
        /*Tabview pages*/
        else if (lv_obj_check_type(lv_obj_get_parent(parent), &custom_tabview_class)) {
            lv_obj_remove_style_all(obj);
            lv_obj_add_style(obj, &pad_normal, 0);
            lv_obj_add_style(obj, &scrollbar, LV_PART_SCROLLBAR);
            lv_obj_add_style(obj, &scrollbar_scrolled, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
            return;
        }
    } else if (lv_obj_check_type(obj, &lv_btnmatrix_class)) {
        if (lv_obj_check_type(lv_obj_get_parent(obj), &custom_tabview_class)) {
            lv_obj_remove_style_all(obj);
            lv_obj_add_style(obj, &bg_color_white, 0);
            lv_obj_add_style(obj, &outline_primary, LV_STATE_FOCUS_KEY);
            lv_obj_add_style(obj, &tab_bg_focus, LV_STATE_FOCUS_KEY);
            lv_obj_add_style(obj, &pressed, LV_PART_ITEMS | LV_STATE_PRESSED);
            lv_obj_add_style(obj, &bg_color_primary_muted, LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_add_style(obj, &tab_btn, LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_add_style(obj, &outline_primary, LV_PART_ITEMS | LV_STATE_FOCUS_KEY);
            lv_obj_add_style(obj, &outline_secondary, LV_PART_ITEMS | LV_STATE_EDITED);
            lv_obj_add_style(obj, &tab_bg_focus, LV_PART_ITEMS | LV_STATE_FOCUS_KEY);
            return;
        }
    } else if (lv_obj_check_type(obj, &custom_tabview_class)) {
        lv_obj_add_style(obj, &scr, 0);
        lv_obj_add_style(obj, &pad_zero, 0);
    } else if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_add_style(obj, (lv_style_t *)&style_label, LV_STATE_DEFAULT);
    } else if (lv_obj_check_type(obj, &lv_btn_class)) {
        lv_obj_add_style(obj, (lv_style_t *)&style_btn, LV_STATE_DEFAULT);
    } else if (lv_obj_check_type(obj, &lv_meter_class)) {
        lv_obj_add_style(obj, (lv_style_t *)&style_transparent, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(obj, (lv_style_t *)&style_transparent, LV_PART_TICKS | LV_STATE_DEFAULT);
        lv_obj_add_style(obj, (lv_style_t *)&style_transparent, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    lv_obj_add_style(obj, (lv_style_t *)&style_all, LV_STATE_DEFAULT);
}


static lv_color_t dark_color_filter_cb(const lv_color_filter_dsc_t *f, lv_color_t c, lv_opa_t opa) {
    LV_UNUSED(f);
    return lv_color_darken(c, opa);
}