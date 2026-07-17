/* algo/detect.h - 自研检测算法
 *
 * 自研算法 = 基础算法之上的二次开发：
 * 本层不碰像素细节，只把 base/imgproc 的原语组合成有业务价值的 score。
 */
#ifndef ALGO_DETECT_H
#define ALGO_DETECT_H

#include <stddef.h>

typedef struct {
    int score;      /* 0..255，综合评分，供上层流程判定 */
    int mean;
    int contrast;   /* max - min */
    int noise;      /* stddev 取整 */
} detect_result_t;

void detect_run(const unsigned char *px, size_t n, detect_result_t *out);

#endif /* ALGO_DETECT_H */
