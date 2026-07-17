/* 通用型流程：始终编入。score >= 140 判 OK */
#include "flow.h"
#include <stdio.h>

#define COMMON_THRESHOLD 140

static void run(int frame_id, const detect_result_t *r, char *json, size_t n)
{
    const char *verdict = (r->score >= COMMON_THRESHOLD) ? "OK" : "NG";

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"common\",\"frame\":%d,"
             "\"score\":%d,\"verdict\":\"%s\",\"threshold\":%d}",
             frame_id, r->score, verdict, COMMON_THRESHOLD);
}

const flow_t FLOW_COMMON = {"common", run};
