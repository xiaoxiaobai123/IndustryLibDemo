/* flow/flow.h - 业务流程层接口
 *
 * 每个流程就是一个 {name, run} 结构体；run 输出一行结果 JSON。
 * 流程只依赖 algo 的结果结构，不认识 base、不认识网络。
 */
#ifndef FLOW_FLOW_H
#define FLOW_FLOW_H

#include "detect.h"
#include <stddef.h>

typedef struct {
    const char *name;
    void (*run)(int frame_id, const detect_result_t *r, char *json, size_t n);
} flow_t;

extern const flow_t FLOW_COMMON;
#ifdef FLOW_A
extern const flow_t FLOW_CUSTOMER_A;
#endif
#ifdef FLOW_B
extern const flow_t FLOW_CUSTOMER_B;
#endif

#endif /* FLOW_FLOW_H */
