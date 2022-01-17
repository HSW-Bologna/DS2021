#include <assert.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/limits.h>

#include "log.h"
#include "gel/timer/timecheck.h"
#include "utils/system_time.h"

#define GPIO_FOLDER "/sys/class/gpio"
#define BUZZER_IO   12
#define OUT         "out"
#define VALUE_ON    "1"
#define VALUE_OFF   "0"

#define BUZZER_CMD(x, y) ((buzzer_cmd_t){x, y})


typedef struct {
    int repeat;
    int duration;
} buzzer_cmd_t;


static int writepipe;


static int set_gpio_output(int gpio) {
    char string[PATH_MAX];
    snprintf(string, PATH_MAX, "%s/gpio%i/direction", GPIO_FOLDER, gpio);

    int fd = open(string, O_WRONLY);
    if (fd < 0) {
        log_warn("Non sono riuscito ad aprire il file %s: %s", string, strerror(errno));
        return -1;
    } else if (write(fd, OUT, strlen(OUT)) == 0) {
        log_warn("Non sono riuscito ad impostare il GPIO %i come output: %s", gpio, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


static int enable_gpio(int gpio) {
    char string[PATH_MAX + 1];
    char number[9];
    snprintf(string, PATH_MAX, "%s/export", GPIO_FOLDER);
    snprintf(number, 8, "%i", gpio);

    int fd = open(string, O_WRONLY);
    if (fd < 0) {
        log_warn("Non sono riuscito ad aprire il file %s: %s", string, strerror(errno));
        return -1;
    } else if (write(fd, number, strlen(number)) == 0) {
        log_warn("Non sono riuscito ad attivare il GPIO %i", gpio, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


static int gpio_update(int gpio, int value) {
    char string[PATH_MAX + 1];
    char svalue[8] = {0};
    snprintf(string, PATH_MAX, "%s/gpio%i/value", GPIO_FOLDER, gpio);
    strcpy(svalue, value ? VALUE_ON : VALUE_OFF);

    int fd = open(string, O_WRONLY);
    if (fd < 0) {
        log_warn("Non sono riuscito ad aprire il file %s: %s", string, strerror(errno));
        return -1;
    } else if (write(fd, svalue, strlen(svalue)) == 0) {
        log_warn("Non sono riuscito ad impostare il GPIO %i", gpio, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


static void *buzzer_task(void *args) {
    unsigned long timestamp = 0;
    int           value     = 1;
    buzzer_cmd_t  cmd       = {0};
    int           readpipe  = (int)(uintptr_t)((void **)args)[0];
    free(args);

    for (;;) {
        struct timeval delay = {.tv_sec = 0, .tv_usec = 1000};
        fd_set         readfd;

        FD_ZERO(&readfd);
        FD_SET(readpipe, &readfd);
        int res = select(readpipe + 1, &readfd, NULL, NULL, &delay);
        if (res > 0) {
            if (FD_ISSET(readpipe, &readfd)) {
                read(readpipe, &cmd, sizeof(buzzer_cmd_t));
                cmd.repeat--;
                timestamp = get_millis();
                value     = 1;
#ifndef TARGET_DEBUG
                gpio_update(BUZZER_IO, value);
#endif
            }
        } else if (res < 0) {
            log_error("Errore select: %s", strerror(errno));
        }

        if (cmd.repeat > 0 && is_expired(timestamp, get_millis(), cmd.duration)) {
            cmd.repeat--;
            value     = !value;
            timestamp = get_millis();
#ifndef TARGET_DEBUG
            gpio_update(BUZZER_IO, value);
#endif
        } else if (cmd.repeat == 0) {
            value = 0;
#ifndef TARGET_DEBUG
            gpio_update(BUZZER_IO, value);
#endif
        }

        usleep(delay.tv_usec);
    }

    pthread_exit(NULL);
    return NULL;
}


void buzzer_beep(int repeat, int duration) {
    assert(sizeof(buzzer_cmd_t) < PIPE_BUF);

    buzzer_cmd_t cmd = BUZZER_CMD(repeat * 2, duration);
    write(writepipe, &cmd, sizeof(buzzer_cmd_t));
}


pthread_t buzzer_init(void) {
    pthread_t id;
    int       pipes[2];

    assert(pipe(pipes) == 0);
    writepipe = pipes[1];

#ifndef TARGET_DEBUG
    enable_gpio(BUZZER_IO);
    set_gpio_output(BUZZER_IO);
#endif

    void **args = (void **)malloc(sizeof(void *) * 1);
    args[0]     = (void *)(uintptr_t)pipes[0];
    pthread_create(&id, NULL, buzzer_task, (void *)args);

    log_info("Buzzer initializzato");

    return id;
}