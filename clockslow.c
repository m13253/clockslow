#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include "config.h"
#include <dlfcn.h>
#include <math.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double app_timestart = NAN;
static double app_timefactor = 1;
static int app_verbose = 0;

static void printf_verbose(const char *format, ...);

static void init_clockslow(void) {
    if(isnan(app_timestart))
    {
        const char *timestart = getenv(APP_ENV_PREFIX "_START");
        const char *timefactor = getenv(APP_ENV_PREFIX "_FACTOR");
        const char *verbose = getenv(APP_ENV_PREFIX "_VERBOSE");
        if(timestart) {
            char *endptr;
            double res = strtod(timestart, &endptr);
            if(timestart == endptr || !isnormal(res)) // Error
                timestart = NULL;
            else
                app_timestart = res;
        }
        if(!timestart) {
            int (*real_gettimeofday)(struct timeval *, void *) = dlsym(RTLD_NEXT, "gettimeofday");
            struct timeval tv;
            real_gettimeofday(&tv, NULL);
            app_timestart = tv.tv_sec + tv.tv_usec/1000000.0;
            printf_verbose("%s: set " APP_ENV_PREFIX "_START to %lf\n", APP_NAME, app_timestart);
        }
        if(timefactor) {
            char *endptr;
            double res = strtod(timefactor, &endptr);
            if(timefactor != endptr && !isnormal(res))
                app_timefactor = res;
        }
        if(verbose && *verbose)
            app_verbose = 1;
    }
}

static void printf_verbose(const char *format, ...) {
    va_list ap;
    if(!app_verbose)
        return;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

static void printf_verbose_timespec(const struct timespec *ts) {
    if(ts)
        printf_verbose("{%" PRId64 ", %ld}", (int64_t) ts->tv_sec, (long) ts->tv_nsec);
    else
        printf_verbose("NULL");
}

static void printf_verbose_timeval(const struct timeval *tv) {
    if(tv)
        printf_verbose("{%" PRId64 ", %ld}", (int64_t) tv->tv_sec, (long) tv->tv_usec);
    else
        printf_verbose("NULL");
}

int alarm(unsigned seconds) {
    static int (*real_alarm)(unsigned) = NULL;
    int res;
    int seconds_ = seconds;
    init_clockslow();
    if(!real_alarm)
        real_alarm = dlsym(RTLD_NEXT, "alarm");
    if(!real_alarm)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    printf_verbose("alarm(%u);\n", seconds);
    seconds_ *= app_timefactor;
    if(seconds_ <= 0)
        seconds_ = seconds;
    res = real_alarm(seconds_);
    return res;
}

clock_t clock(void) {
    static clock_t (*real_clock)(void) = NULL;
    clock_t res;
    init_clockslow();
    if(!real_clock)
        real_clock = dlsym(RTLD_NEXT, "clock");
    if(!real_clock)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    res = real_clock();
    if(res != -1)
        res *= app_timefactor;
    printf_verbose("clock() = %ld;\n", (long) res);
    return res;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    static int (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
    int res;
    init_clockslow();
    if(!real_clock_gettime)
        real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    if(!real_clock_gettime)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    res = real_clock_gettime(clk_id, tp);
    if(tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 0;
    }
    printf_verbose("clock_gettime(%" PRId32 ", ", (int32_t) clk_id);
    printf_verbose_timespec(tp);
    printf_verbose(");\n");
    return res;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    static int (*real_nanosleep)(const struct timespec *, struct timespec *) = NULL;
    struct timespec *req_ = (struct timespec *) req;
    int res;
    init_clockslow();
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
    printf_verbose("nanosleep(");
    printf_verbose_timespec(req);
    printf_verbose(", ");
    printf_verbose_timespec(rem);
    printf_verbose(");\n");
    return res;
}
