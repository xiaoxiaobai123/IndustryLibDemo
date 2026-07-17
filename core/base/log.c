#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_msg(const char *module, const char *fmt, ...)
{
    char      stamp[16];
    time_t    now = time(NULL);
    struct tm tmv;
    va_list   ap;

#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    strftime(stamp, sizeof(stamp), "%H:%M:%S", &tmv);

    printf("[%s][%s] ", stamp, module);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}
