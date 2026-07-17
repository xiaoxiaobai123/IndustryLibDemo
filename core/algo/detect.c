#include "detect.h"
#include "imgproc.h" /* 只允许向下依赖 base */
#include "log.h"

static int clamp255(double v)
{
    if (v < 0.0)   return 0;
    if (v > 255.0) return 255;
    return (int)(v + 0.5);
}

void detect_surface(const unsigned char *px, size_t n, detect_result_t *out)
{
    double mean, noise, score;
    int    contrast;

    log_trace("algo.detect", " detect_surface() -> 调 base.imgproc 取特征");

    /* 1) 调 base 原语取特征 */
    mean     = imgproc_mean(px, n);
    contrast = imgproc_max(px, n) - imgproc_min(px, n);
    noise    = imgproc_stddev(px, n);

    /* 2) 自研部分：把原语结果加权组合成 score */
    score = mean + contrast * 0.20 - noise * 0.60;

    out->mean     = clamp255(mean);
    out->contrast = contrast;
    out->noise    = clamp255(noise);
    out->score    = clamp255(score);

    log_trace("algo.detect", " score = mean %.1f + contrast %d*0.2 - noise %.1f*0.6 = %d",
              mean, contrast, noise, out->score);
}
