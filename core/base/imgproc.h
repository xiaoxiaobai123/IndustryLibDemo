/* base/imgproc.h - 基础图像算法原语
 *
 * 这一层模拟真实项目里 OpenCV 所处的位置：第三方 / 通用基础算法块。
 * 只提供无业务含义的通用运算，不知道"检测""良品"这些概念。
 */
#ifndef BASE_IMGPROC_H
#define BASE_IMGPROC_H

#include <stddef.h>

double        imgproc_mean(const unsigned char *px, size_t n);
unsigned char imgproc_min(const unsigned char *px, size_t n);
unsigned char imgproc_max(const unsigned char *px, size_t n);
double        imgproc_stddev(const unsigned char *px, size_t n);

#endif /* BASE_IMGPROC_H */
