#include "imgproc.h"
#include "log.h"
#include <math.h>

double imgproc_mean(const unsigned char *px, size_t n)
{
    double sum = 0.0;
    size_t i;

    if (n == 0) return 0.0;
    for (i = 0; i < n; i++) sum += px[i];
    sum /= (double)n;
    log_trace("base.imgproc", "  mean(n=%u) = %.1f", (unsigned)n, sum);
    return sum;
}

unsigned char imgproc_min(const unsigned char *px, size_t n)
{
    unsigned char v = 255;
    size_t        i;

    if (n == 0) return 0;
    for (i = 0; i < n; i++)
        if (px[i] < v) v = px[i];
    log_trace("base.imgproc", "  min(n=%u) = %d", (unsigned)n, v);
    return v;
}

unsigned char imgproc_max(const unsigned char *px, size_t n)
{
    unsigned char v = 0;
    size_t        i;

    if (n == 0) return 0;
    for (i = 0; i < n; i++)
        if (px[i] > v) v = px[i];
    log_trace("base.imgproc", "  max(n=%u) = %d", (unsigned)n, v);
    return v;
}

double imgproc_stddev(const unsigned char *px, size_t n)
{
    double m, acc = 0.0;
    size_t i;

    if (n == 0) return 0.0;
    m = imgproc_mean(px, n);
    for (i = 0; i < n; i++) {
        double d = px[i] - m;
        acc += d * d;
    }
    acc = sqrt(acc / (double)n);
    log_trace("base.imgproc", "  stddev(n=%u) = %.1f", (unsigned)n, acc);
    return acc;
}
