/* flow/flow.h - 业务流程层接口
 *
 * 一个流程 = 一个客户项目。它自己决定：
 *   从哪个硬件取图(base) -> 调哪个算法(algo) -> 怎么判 -> 输出什么字段
 * 宿主(main)只负责按节拍调 run()，不掺和上面任何一件事。
 */
#ifndef FLOW_FLOW_H
#define FLOW_FLOW_H

#include <stddef.h>

typedef struct {
    const char *name;
    void (*run)(char *json, size_t n); /* 输出一行结果 JSON */
} flow_t;

extern const flow_t FLOW_COMMON;
#ifdef FLOW_A
extern const flow_t FLOW_CUSTOMER_A;
#endif
#ifdef FLOW_B
extern const flow_t FLOW_CUSTOMER_B;
#endif

#endif /* FLOW_FLOW_H */
