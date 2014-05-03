#include <config.h>
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    static int (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
    int res;
    if(!real_clock_gettime)
        real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    if(!real_clock_gettime)
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror());
    res = real_clock_gettime(clk_id, tp);
    tp->tv_sec = 0;
    tp->tv_nsec = 0;
    return 0;
}

