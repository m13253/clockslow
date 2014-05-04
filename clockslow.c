#include "config.h"
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

int alarm(unsigned seconds) {
    static int (*real_alarm)(unsigned) = NULL;
    int res;
    if(!real_alarm)
        real_alarm = dlsym(RTLD_NEXT, "alarm");
    if(!real_alarm)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    seconds = seconds*2;
    res = real_alarm(seconds);
    return res;
}

clock_t clock(void) {
    static clock_t (*real_clock)(void) = NULL;
    clock_t res;
    if(!real_clock)
        real_clock = dlsym(RTLD_NEXT, "clock");
    if(!real_clock)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    res = real_clock();
    if(res != -1)
        res /= 2;
    return res;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    static int (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
    int res;
    if(!real_clock_gettime)
        real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    if(!real_clock_gettime)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    res = real_clock_gettime(clk_id, tp);
    if(tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 0;
    }
    return res;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    static int (*real_nanosleep)(const struct timespec *, struct timespec *) = NULL;
    struct timespec *req_ = (struct timespec *) req;
    int res;
    if(!real_nanosleep)
        real_nanosleep = dlsym(RTLD_NEXT, "nanosleep");
    if(!real_nanosleep)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    req_->tv_sec *= 2;
    req_->tv_nsec *= 2;
    res = real_nanosleep(req_, rem);
    if(rem) {
        rem->tv_sec /= 2;
        rem->tv_nsec /= 2;
    }
    return res;
}
