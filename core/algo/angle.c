#include "angle.h"
#include "imgproc.h" /* 只允许向下依赖 base */
#include "log.h"

static int clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void angle_measure(const unsigned char *px, size_t n, angle_result_t *out)
{
    size_t half = n / 2;
    double left, right, noise, diff;

    log_trace("algo.angle", " angle_measure() -> 调 base.imgproc 取左右半区亮度");

    /* 自研部分：左右半区亮度差 ≈ 扫描方向的倾角 */
    left  = imgproc_mean(px, half);
    right = imgproc_mean(px + half, n - half);
    noise = imgproc_stddev(px, n);
    diff  = right - left;

    out->angle      = clamp((int)(diff + (diff >= 0 ? 0.5 : -0.5)), -90, 90);
    out->confidence = clamp(100 - (int)(noise * 3.0), 0, 100);

    log_trace("algo.angle", " angle = right %.1f - left %.1f = %d 度 (conf=%d)",
              right, left, out->angle, out->confidence);
}
