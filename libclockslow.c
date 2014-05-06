/*
    Clockslow -- A tool to trick app to let it think time goes slower or faster
    Copyright (C) 2014  StarBrilliant <m13253@hotmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include "config.h"
#include <dlfcn.h>
#include <math.h>
#include <inttypes.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>

static double app_timestart = NAN;
static double app_timefactor = 1;
static double app_timefactor_intercept = NAN; // (1-app_timefactor)*app_timestart
static int app_verbose = 0;

static void printf_verbose(const char *format, ...);

static void init_clockslow(void) {
    if(isnan(app_timestart))
    {
        const char *timestart = getenv(APP_ENV_PREFIX "_START");
        const char *timefactor = getenv(APP_ENV_PREFIX "_FACTOR");
        const char *verbose = getenv(APP_ENV_PREFIX "_VERBOSE");
        if(verbose && *verbose)
            app_verbose = 1;
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
            fprintf(stderr, "%s: set " APP_ENV_PREFIX "_START to %lf\n", APP_NAME, app_timestart);
        }
        if(timefactor) {
            char *endptr;
            double res = strtod(timefactor, &endptr);
            if(timefactor != endptr && isnormal(res) && res > 0) {
                app_timefactor = res;
                printf_verbose("%s: set " APP_ENV_PREFIX "_FACTOR to %.3f\n", APP_NAME, app_timefactor);
            }
        }
        if(app_timefactor == 1.0)
            fprintf(stderr, "%s: set " APP_ENV_PREFIX "_FACTOR to 1.0\n", APP_NAME);
        app_timefactor_intercept = (1-app_timefactor)*app_timestart;
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

#ifdef DISABLE_VERBOSE
#define printf_verbose(...)
#define printf_verbose_timespec(ts)
#define printf_verbose_timeval(tv)
#endif

static float fastrand() {
    static uint32_t mirand = 1;
    uint32_t a;
    mirand *= 16807;
    a = (mirand & 0x7fffff) | 0x3f800000;
    return *(float *) &a - 1;
}

static long long randround(double x) {
    double xint;
    double xfrac;
    if(!isnormal(x)) return 0;
    xfrac = modf(x, &xint);
    if(xfrac > 0)
        if(fastrand() > xfrac) return (long long) xint + 1; else;
    else if(xfrac < 0)
        if(fastrand() < -xfrac) return (long long) xint - 1;
    return (long long) xint;
}

struct largertimespec {
    time_t    tv_sec;
    long long tv_nsec; // longer tv_nsec to avoid overflow on 32-bit systems */
};

static void timespec2larger(const struct timespec *ts, struct largertimespec *lts) {
    if(lts)
        if(ts) {
            lts->tv_sec = ts->tv_sec;
            lts->tv_nsec = ts->tv_nsec;
        } else {
            lts->tv_sec = 0;
            lts->tv_nsec = 0;
        }
    else;
}

static void larger2timespec(const struct largertimespec *lts, struct timespec *ts) {
    if(ts)
        if(lts) {
            ts->tv_sec = lts->tv_sec;
            ts->tv_nsec = lts->tv_nsec;
        } else {
            ts->tv_sec = 0;
            ts->tv_nsec = 0;
        }
    else;
}

static void timespec_norm(struct largertimespec *ts) {
    if(ts->tv_nsec >= 1000000000 || ts->tv_nsec < 0) {
        ts->tv_sec += ts->tv_nsec/1000000000;
        ts->tv_nsec %= 1000000000;
    }
    if(ts->tv_nsec < 0) {
        ts->tv_sec--;
        ts->tv_nsec += 1000000000;
    }
}

static void timespec_add_double(struct largertimespec *ts1, double ts2, int use_randround) {
    double i, f;
    if(!ts1) return;
    f = modf(ts2, &i);
    ts1->tv_sec += i;
    if(use_randround)
        ts1->tv_nsec += randround(f*1000000000);
    else
        ts1->tv_nsec += round(f*1000000000);
    timespec_norm(ts1);
}

static void timespec_mul(struct largertimespec *ts, int use_randround) {
    double i, f;
    if(!ts) return;
    f = modf(ts->tv_sec*app_timefactor, &i);
    ts->tv_sec = i;
    if(use_randround)
        ts->tv_nsec = randround(ts->tv_nsec*app_timefactor+f*1000000000);
    else
        ts->tv_nsec = round(ts->tv_nsec*app_timefactor+f*1000000000);
    timespec_norm(ts);
}

static void timespec_div(struct largertimespec *ts, int use_randround) {
    double i, f;
    if(!ts) return;
    f = modf(ts->tv_sec/app_timefactor, &i);
    ts->tv_sec = i;
    if(use_randround)
        ts->tv_nsec = randround(ts->tv_nsec/app_timefactor+f*1000000000);
    else
        ts->tv_nsec = round(ts->tv_nsec/app_timefactor+f*1000000000);
    timespec_norm(ts);
}

static void timeval_norm(struct timeval *tv) {
    if(tv->tv_usec >= 1000000 || tv->tv_usec < 0) {
        tv->tv_sec += tv->tv_usec/1000000;
        tv->tv_usec %= 1000000;
    }
    if(tv->tv_usec < 0) {
        tv->tv_sec--;
        tv->tv_usec += 1000000;
    }
}

static void timeval_add_double(struct timeval *tv1, double tv2, int use_randround) {
    double i, f;
    if(!tv1) return;
    f = modf(tv2, &i);
    tv1->tv_sec += i;
    if(use_randround)
        tv1->tv_usec += randround(f*1000000);
    else
        tv1->tv_usec += round(f*1000000);
    timeval_norm(tv1);
}

static void timeval_mul(struct timeval *tv, int use_randround) {
    double i, f;
    if(!tv) return;
    f = modf(tv->tv_sec*app_timefactor, &i);
    tv->tv_sec = i;
    if(use_randround)
        tv->tv_usec = randround(tv->tv_usec*app_timefactor+f*1000000);
    else
        tv->tv_usec = round(tv->tv_usec*app_timefactor+f*1000000);
    timeval_norm(tv);
}

static void timeval_div(struct timeval *tv, int use_randround) {
    double i, f;
    if(!tv) return;
    f = modf(tv->tv_sec/app_timefactor, &i);
    tv->tv_sec = i;
    if(use_randround)
        tv->tv_usec = randround(tv->tv_usec/app_timefactor+f*1000000);
    else
        tv->tv_usec = round(tv->tv_usec/app_timefactor+f*1000000);
    timeval_norm(tv);
}

#define load_real(x) { \
    init_clockslow(); \
    if(!real_ ## x ) \
        real_ ## x = dlsym(RTLD_NEXT, #x ); \
    if(!real_ ## x ) \
        fprintf(stderr, "%s: %s\n", APP_NAME, dlerror()); \
}

int alarm(unsigned seconds) {
    static int (*real_alarm)(unsigned) = NULL;
    int res;
    int seconds_ = seconds;
    load_real(alarm);
    printf_verbose("alarm(%u);\n", seconds);
    seconds_ = randround(seconds_ * app_timefactor);
    if(seconds_ <= 0)
        seconds_ = seconds;
    res = real_alarm(seconds_);
    return res;
}

clock_t clock(void) {
    static clock_t (*real_clock)(void) = NULL;
    clock_t res;
    load_real(clock);
    res = real_clock();
    if(res != -1)
        res = round(res*app_timefactor);
    printf_verbose("clock() = %ld;\n", (long) res);
    return res;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    static int (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
    int res;
    load_real(clock_gettime);
    res = real_clock_gettime(clk_id, tp);
    if(tp) {
        struct largertimespec ltp;
        timespec2larger(tp, &ltp);
        timespec_mul(&ltp, 0);
        if(clk_id == CLOCK_REALTIME || clk_id == CLOCK_REALTIME_COARSE)
            timespec_add_double(&ltp, app_timefactor_intercept, 0);
        larger2timespec(&ltp, tp);
    }
    printf_verbose("clock_gettime(%" PRId32 ", ", (int32_t) clk_id);
    printf_verbose_timespec(tp);
    printf_verbose(");\n");
    return res;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    static int (*real_nanosleep)(const struct timespec *, struct timespec *) = NULL;
    struct timespec req_;
    struct largertimespec ltmp;
    int res;
    load_real(nanosleep);
    timespec2larger(req, &ltmp);
    timespec_div(&ltmp, 1);
    larger2timespec(&ltmp, &req_);
    res = real_nanosleep(&req_, rem);
    if(rem) {
        timespec2larger(rem, &ltmp);
        timespec_mul(&ltmp, 1);
        larger2timespec(&ltmp, rem);
    }
    printf_verbose("nanosleep(");
    printf_verbose_timespec(req);
    printf_verbose(", ");
    printf_verbose_timespec(rem);
    printf_verbose(");\n");
    return res;
}

int gettimeofday(struct timeval *tv, void *tz) {
    static int (*real_gettimeofday)(struct timeval *, void *) = NULL;
    int res;
    load_real(gettimeofday);
    res = real_gettimeofday(tv, tz);
    if(tv) {
        timeval_mul(tv, 0);
        timeval_add_double(tv, app_timefactor_intercept, 0);
    }
    printf_verbose("gettimeofday(");
    printf_verbose_timeval(tv);
    printf_verbose(", %p);\n", tz);
    return res;
}

unsigned int sleep(unsigned int seconds) {
    struct timespec req;
    struct timespec rem;
    unsigned int res;
    req.tv_sec = seconds;
    req.tv_nsec = 0;
    nanosleep(&req, &rem);
    res = rem.tv_sec+randround(rem.tv_nsec/1000000000);
    printf_verbose("sleep(%u) = %u;\n", seconds, res);
    return res;
}

time_t time(time_t *t) {
    static int (*real_time)(time_t *) = NULL;
    time_t res;
    load_real(time);
    res = real_time(t);
    if(res != -1)
        res = round(res*app_timefactor+app_timefactor_intercept);
    printf_verbose("time(%p) = %" PRId64 ";\n", t, (int64_t) res);
    if(t)
        *t = res;
    return res;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    static int (*real_select)(int, void *, void *, void *, struct timeval *) = NULL;
    int res;
    load_real(select);
    printf_verbose("select(..., ");
    printf_verbose_timeval(timeout);
    printf_verbose(");\n");
    timeval_div(timeout, 1);
    res = real_select(nfds, readfds, writefds, exceptfds, timeout);
    timeval_mul(timeout, 1);
    return res;
}

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask) {
    static int (*real_pselect)(int, void *, void *, void *, const struct timespec *, const void *) = NULL;
    int res;
    load_real(pselect);
    printf_verbose("pselect(..., ");
    printf_verbose_timespec(timeout);
    printf_verbose(", ...);\n");
    if(timeout) {
        struct timespec timeout_;
        struct largertimespec ltmp;
        timespec2larger(timeout, &ltmp);
        timespec_div(&ltmp, 1);
        larger2timespec(&ltmp, &timeout_);
        res = real_pselect(nfds, readfds, writefds, exceptfds, &timeout_, sigmask);
    } else
        res = real_pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
    return res;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    static int (*real_poll)(struct pollfd *, nfds_t, int) = NULL;
    int res;
    load_real(poll);
    printf_verbose("poll(..., %d);", timeout);
    if(timeout > 0)
        timeout = randround(timeout/app_timefactor);
    res = real_poll(fds, nfds, timeout);
    return res;
}

int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask) {
    static int (*real_ppoll)(struct pollfd *, nfds_t, const struct timespec *, const sigset_t *) = NULL;
    int res;
    load_real(ppoll);
    printf_verbose("ppoll(..., ");
    printf_verbose_timespec(timeout_ts);
    printf_verbose(", ...);\n");
    if(timeout_ts) {
        struct timespec timeout_ts_;
        struct largertimespec ltmp;
        timespec2larger(timeout_ts, &ltmp);
        timespec_div(&ltmp, 1);
        larger2timespec(&ltmp, &timeout_ts_);
        res = real_ppoll(fds, nfds, &timeout_ts_, sigmask);
    } else
        res = real_ppoll(fds, nfds, timeout_ts, sigmask);
    return res;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    static int (*real_epoll_wait)(int, struct epoll_event *, int, int) = NULL;
    int res;
    load_real(epoll_wait);
    printf_verbose("epoll_wait(..., %d);\n", timeout);
    if(timeout != -1)
        timeout = randround(timeout/app_timefactor);
    res = real_epoll_wait(epfd, events, maxevents, timeout);
    return res;
}

int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask) {
    static int (*real_epoll_pwait)(int, struct epoll_event *, int, int, const sigset_t *) = NULL;
    int res;
    load_real(epoll_pwait);
    printf_verbose("epoll_pwait(..., %d, ...);\n", timeout);
    if(timeout > 0)
        timeout = randround(timeout/app_timefactor);
    res = real_epoll_pwait(epfd, events, maxevents, timeout, sigmask);
    return res;
}

int snd_pcm_open(void) {
    printf_verbose("snd_pcm_open(...) = BLOCKED;");
    return -1;
}

int snd_pcm_open_lconf(void) {
    printf_verbose("snd_pcm_open_lconf(...) = BLOCKED;");
    return -1;
}

int snd_pcm_open_fallback(void) {
    printf_verbose("snd_pcm_open_fallback(...) = BLOCKED;");
    return -1;
}

void *pa_simple_new(void) {
    printf_verbose("pa_simple_new(...) = BLOCKED;");
    return NULL;
}

void *pa_context_new(void) {
    printf_verbose("pa_context_new(...) = BLOCKED;");
    return NULL;
}

void *jack_client_open(void) {
    printf_verbose("jack_client_open(...) = BLOCKED;");
    return NULL;
}

int oss_pcm_open(void) {
    printf_verbose("oss_pcm_open(...) = BLOCKED;");
    return -1;
}
