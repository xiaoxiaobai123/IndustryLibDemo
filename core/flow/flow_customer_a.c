/* 客户A流程：阈值更严（>=150），额外输出 A/B/C 等级 */
#include "flow.h"
#include <stdio.h>

#define A_THRESHOLD 150

static const char *grade_of(int score)
{
    if (score >= 180) return "A";
    if (score >= 150) return "B";
    return "C";
}

static void run(int frame_id, const detect_result_t *r, char *json, size_t n)
{
    const char *verdict = (r->score >= A_THRESHOLD) ? "OK" : "NG";

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"customer_a\",\"frame\":%d,"
             "\"score\":%d,\"verdict\":\"%s\",\"threshold\":%d,\"grade\":\"%s\"}",
             frame_id, r->score, verdict, A_THRESHOLD, grade_of(r->score));
}

const flow_t FLOW_CUSTOMER_A = {"customer_a", run};
