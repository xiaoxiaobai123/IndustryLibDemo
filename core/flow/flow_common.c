/* 通用型流程：始终编入。标准表面检测，score >= 140 判 OK。
 * 调用链：base.plc_sim(CAM_A) -> algo.detect_surface -> base.imgproc */
#include "flow.h"
#include "detect.h"  /* algo */
#include "plc_sim.h" /* base：自己选硬件 */
#include "log.h"
#include <stdio.h>

#define COMMON_THRESHOLD 140

static void run(char *json, size_t n)
{
    frame_t         f;
    detect_result_t r;
    const char     *verdict;

    log_trace("flow.common", "取图 CAM_A -> algo.detect_surface()");
    plc_sim_grab(PLC_CAM_A, &f);
    detect_surface(f.px, PLC_FRAME_PIXELS, &r);

    verdict = (r.score >= COMMON_THRESHOLD) ? "OK" : "NG";
    log_trace("flow.common", "判定 score=%d vs %d -> %s", r.score, COMMON_THRESHOLD, verdict);

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"common\",\"frame\":%d,"
             "\"score\":%d,\"verdict\":\"%s\",\"threshold\":%d}",
             f.frame_id, r.score, verdict, COMMON_THRESHOLD);
}

const flow_t FLOW_COMMON = {"common", run};
