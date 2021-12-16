#include <unistd.h>

#include "lvgl.h"
#include "sdl/sdl.h"
#include "display/fbdev.h"
#include "indev/evdev.h"
#include "view/view.h"
#include "model/model.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "config/app_conf.h"


int main(int argc, char *argv[]) {
    model_t model;

    model_init(&model);

    lv_init();
#if USE_SDL
    sdl_init();
    view_init(&model, sdl_display_flush, sdl_mouse_read);
#endif
#if USE_FBDEV
    fbdev_init();
    evdev_init();
    view_init(&model, fbdev_flush, evdev_read);
#endif

    controller_init(&model);

    for (;;) {
        controller_gui_manage(&model);
        controller_manage(&model);

        usleep(5 * 1000);
    }

    return 0;
}
