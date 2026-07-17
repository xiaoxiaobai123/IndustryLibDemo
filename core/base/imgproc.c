#include "imgproc.h"
#include <math.h>

double imgproc_mean(const unsigned char *px, size_t n)
{
    double sum = 0.0;
    size_t i;

    if (n == 0) return 0.0;
    for (i = 0; i < n; i++) sum += px[i];
    return sum / (double)n;
}

unsigned char imgproc_min(const unsigned char *px, size_t n)
{
    unsigned char v = 255;
    size_t        i;

    if (n == 0) return 0;
    for (i = 0; i < n; i++)
        if (px[i] < v) v = px[i];
    return v;
}

unsigned char imgproc_max(const unsigned char *px, size_t n)
{
    unsigned char v = 0;
    size_t        i;

    if (n == 0) return 0;
    for (i = 0; i < n; i++)
        if (px[i] > v) v = px[i];
    return v;
}

double imgproc_stddev(const unsigned char *px, size_t n)
{
    double m = imgproc_mean(px, n);
    double acc = 0.0;
    size_t i;

    if (n == 0) return 0.0;
    for (i = 0; i < n; i++) {
        double d = px[i] - m;
        acc += d * d;
    }
    return sqrt(acc / (double)n);
}
