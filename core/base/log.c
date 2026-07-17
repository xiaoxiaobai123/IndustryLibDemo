#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static int s_trace = 0;

static void emit(const char *module, const char *fmt, va_list ap)
{
    char      stamp[16];
    time_t    now = time(NULL);
    struct tm tmv;

#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    strftime(stamp, sizeof(stamp), "%H:%M:%S", &tmv);

    printf("[%s][%s] ", stamp, module);
    vprintf(fmt, ap);
    printf("\n");
    fflush(stdout);
}

void log_msg(const char *module, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    emit(module, fmt, ap);
    va_end(ap);
}

void log_set_trace(int on) { s_trace = on; }

int log_trace_enabled(void) { return s_trace; }

void log_trace(const char *module, const char *fmt, ...)
{
    va_list ap;
    if (!s_trace) return;
    va_start(ap, fmt);
    emit(module, fmt, ap);
    va_end(ap);
}
