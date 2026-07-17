/* 客户B流程：区间判定 130~170，输出与 A 完全不同的字段 */
#include "flow.h"
#include <stdio.h>

#define B_LOW  130
#define B_HIGH 170

static void run(int frame_id, const detect_result_t *r, char *json, size_t n)
{
    int         in_band = (r->score >= B_LOW && r->score <= B_HIGH);
    const char *band    = in_band ? "in_range" : (r->score < B_LOW ? "under" : "over");

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"customer_b\",\"frame\":%d,"
             "\"score\":%d,\"verdict\":\"%s\",\"band\":\"%s\","
             "\"low\":%d,\"high\":%d,\"noise\":%d}",
             frame_id, r->score, in_band ? "OK" : "NG", band,
             B_LOW, B_HIGH, r->noise);
}

const flow_t FLOW_CUSTOMER_B = {"customer_b", run};
