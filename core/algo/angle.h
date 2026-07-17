/* algo/angle.h - 自研算法：角度测量
 *
 * 和 detect 平级的另一个能力：客户B的项目要"找角度"，就调它。
 * 同样只依赖 base/imgproc —— 新增算法不需要动 base。
 */
#ifndef ALGO_ANGLE_H
#define ALGO_ANGLE_H

#include <stddef.h>

typedef struct {
    int angle;      /* -90..90 度，正=右倾 */
    int confidence; /* 0..100，噪声越大越低 */
} angle_result_t;

void angle_measure(const unsigned char *px, size_t n, angle_result_t *out);

#endif /* ALGO_ANGLE_H */
