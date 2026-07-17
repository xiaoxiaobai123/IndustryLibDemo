/* 客户A项目：苹果外观检测
 *   硬件：面阵相机 CAM_A     算法：algo.detect_surface（表面质量评分）
 *   需求：合格线更严(150)，且要按外观分 A/B/C 等级出货
 * 调用链：base.plc_sim(CAM_A) -> algo.detect_surface -> base.imgproc */
#include "flow.h"
#include "detect.h"  /* algo：这个项目要的是"表面评分" */
#include "plc_sim.h" /* base：这个项目的硬件是 CAM_A */
#include "log.h"
#include <stdio.h>

#define A_THRESHOLD 150

static const char *grade_of(int score)
{
    if (score >= 180) return "A";
    if (score >= 150) return "B";
    return "C";
}

static void run(char *json, size_t n)
{
    frame_t         f;
    detect_result_t r;
    const char     *verdict;

    log_trace("flow.customer_a", "苹果外观项目：取图 CAM_A -> algo.detect_surface()");
    plc_sim_grab(PLC_CAM_A, &f);
    detect_surface(f.px, PLC_FRAME_PIXELS, &r);

    verdict = (r.score >= A_THRESHOLD) ? "OK" : "NG";
    log_trace("flow.customer_a", "判定 score=%d vs %d -> %s 等级=%s",
              r.score, A_THRESHOLD, verdict, grade_of(r.score));

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"customer_a\",\"frame\":%d,"
             "\"score\":%d,\"verdict\":\"%s\",\"threshold\":%d,\"grade\":\"%s\"}",
             f.frame_id, r.score, verdict, A_THRESHOLD, grade_of(r.score));
}

const flow_t FLOW_CUSTOMER_A = {"customer_a", run};
