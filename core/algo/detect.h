/* algo/detect.h - 自研算法：表面质量评分
 *
 * 通用能力，不绑定任何客户、任何工件。谁的项目要"看表面好不好"就调它。
 * 自研 = 基础原语之上的二次开发：本层不碰像素细节，只组合 base/imgproc。
 */
#ifndef ALGO_DETECT_H
#define ALGO_DETECT_H

#include <stddef.h>

typedef struct {
    int score;    /* 0..255 表面质量综合评分 */
    int mean;
    int contrast; /* max - min */
    int noise;    /* stddev 取整 */
} detect_result_t;

void detect_surface(const unsigned char *px, size_t n, detect_result_t *out);

#endif /* ALGO_DETECT_H */
