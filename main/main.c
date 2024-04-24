#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl.h"
#include "sdl/sdl.h"
#include "display/fbdev.h"
#include "indev/evdev.h"
#include "view/view.h"
#include "model/model.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "config/app_conf.h"
#include "utils/system_time.h"
#include "log.h"


static void log_file_init(void);
static void log_lock(void *arg, int op);


static pthread_mutex_t lock;


int main(int argc, char *argv[]) {
    static_model_updater_t model_updater_buffer;
    model_t                model;
    model_updater_t        model_updater = model_updater_init(&model, &model_updater_buffer);

    log_set_level(CONFIG_LOG_LEVEL);
    log_file_init();

    log_info("App version %s, %s", SOFTWARE_VERSION, SOFTWARE_BUILD_DATE);

    model_init(&model);

    lv_init();
#if USE_SDL
    sdl_init();
    view_init(model_updater, controller_manage_message, sdl_display_flush, sdl_mouse_read);
#endif
#if USE_FBDEV
    fbdev_init();
    evdev_init();
    view_init(model_updater, controller_manage_message, fbdev_flush, evdev_read);
#endif

    controller_init(&model);

    for (;;) {
        controller_gui_manage(&model);
        controller_manage(&model);

        usleep(5 * 1000);
    }

    return 0;
}


static void log_file_init(void) {
    assert(pthread_mutex_init(&lock, NULL) == 0);
    log_set_lock(log_lock);
    log_set_udata(&lock);

    log_set_fp(fopen(LOGFILE, "a+"));
    log_set_fileinfo(0);
}


static void log_lock(void *arg, int op) {
    op ? pthread_mutex_lock(arg) : pthread_mutex_unlock(arg);
}
