#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include "log.h"


#define RTC_PATH     "/dev/rtc0"
#define RTC_ATTEMPTS 5


static int change_rtc_time(time_t seconds) {
#ifndef TARGET_DEBUG
    struct tm tm = *localtime(&seconds);

    // Nel RTC salvo la data senza DST
    if (tm.tm_isdst) {
        seconds -= 60 * 60;
        tm = *localtime(&seconds);
    }

    struct rtc_time rt = {
        .tm_sec  = tm.tm_sec,
        .tm_min  = tm.tm_min,
        .tm_hour = tm.tm_hour,
        .tm_mday = tm.tm_mday,
        .tm_mon  = tm.tm_mon,
        .tm_year = tm.tm_year,
    };

    int counter = 0;

    do {
        int fd;

        if ((fd = open(RTC_PATH, O_RDONLY)) < 0) {
            log_warn("Non riesco a interrogare il RTC: %s", strerror(errno));
            return -1;
        }

        if (ioctl(fd, RTC_SET_TIME, &rt) < 0) {
            log_warn("Non sono riuscito a impostare l'ora (tentativo %i): %s", counter, strerror(errno));
            close(fd);
            continue;
        }

        close(fd);
        break;
    } while (counter++ < RTC_ATTEMPTS);

    if (counter >= RTC_ATTEMPTS)
        return -1;
#endif

    return 0;
}


static int change_system_time(time_t seconds) {
    int res = 0;

#ifndef TARGET_DEBUG
    struct timespec tv = {.tv_sec = seconds, .tv_nsec = 0};

    if ((res = clock_settime(CLOCK_REALTIME, &tv))) {
        log_warn("Non sono riuscito a impostare data e ora: %s", strerror(errno));
    } else {
        char *t          = ctime((const time_t *)&seconds);
        t[strlen(t) - 1] = '\0';
        log_info("Aggiornamento di data e ora: %s", t);
    }
#endif

    return res;
}


int update_system_rtc_time(time_t seconds) {
    int res1 = change_system_time(seconds);
    int res2 = change_rtc_time(seconds);
    return res1 || res2;
}


int init_system_time_from_rtc(void) {
#ifndef TARGET_DEBUG
    struct rtc_time rt;
    int             counter = 0;

    do {
        int fd;

        if ((fd = open(RTC_PATH, O_RDONLY)) < 0) {
            log_warn("Non riesco a interrogare il RTC: %s", strerror(errno));
            return -1;
        }

        if (ioctl(fd, RTC_RD_TIME, &rt) < 0) {
            log_warn("Non sono riuscito a leggere l'ora (tentativo %i): %s", counter, strerror(errno));
            close(fd);
            continue;
        }

        close(fd);
        break;
    } while (counter++ < RTC_ATTEMPTS);

    if (counter >= RTC_ATTEMPTS)
        return -1;

    struct tm tm = {
        .tm_sec  = rt.tm_sec,
        .tm_min  = rt.tm_min,
        .tm_hour = rt.tm_hour,
        .tm_mday = rt.tm_mday,
        .tm_mon  = rt.tm_mon,
        .tm_year = rt.tm_year,
        // Nel RTC non metto mai i DST
        .tm_isdst = 0,
    };

    log_info("Ora caricata: %i:%i:%i (%i)", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_isdst);
    time_t new = mktime(&tm);
    log_info("Ora corretta %i:%i:%i (%i)", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_isdst);

#define FIRST_JAN_2020 1577836800
    if (tm.tm_year < 120)     // Non accettare date prima del 2020
        new = FIRST_JAN_2020;

    return change_system_time(new);
#else
    return 0;
#endif
}


/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
unsigned long get_millis(void) {
    unsigned long   now_ms;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    now_ms = ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;

    return now_ms;
}


time_t mktime_autodst(struct tm *tm) {
    // struct tm autodst = *tm;
    // mktime(&autodst);
    // tm->tm_isdst = autodst.tm_isdst;
    log_info("Ora da impostare %i:%i:%i (%i)", tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
    time_t res = mktime(tm);
    log_info("Ora impostata %i:%i:%i (%i)", tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
    return res;
}