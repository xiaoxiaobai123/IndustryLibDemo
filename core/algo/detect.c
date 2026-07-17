#include "detect.h"
#include "imgproc.h" /* 只允许向下依赖 base */

static int clamp255(double v)
{
    if (v < 0.0)   return 0;
    if (v > 255.0) return 255;
    return (int)(v + 0.5);
}

void detect_run(const unsigned char *px, size_t n, detect_result_t *out)
{
    /* 1) 调用 base 原语取特征 */
    double mean     = imgproc_mean(px, n);
    int    contrast = imgproc_max(px, n) - imgproc_min(px, n);
    double noise    = imgproc_stddev(px, n);

    /* 2) 自研部分：把原语结果加权组合成 score
     *    亮度为主，对比度加分，噪声扣分 */
    double score = mean + contrast * 0.20 - noise * 0.60;

    out->mean     = clamp255(mean);
    out->contrast = contrast;
    out->noise    = clamp255(noise);
    out->score    = clamp255(score);
}
