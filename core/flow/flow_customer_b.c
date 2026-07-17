/* 客户B项目：工件角度测量  —— 和客户A是完全不同的项目
 *   硬件：线扫相机 CAM_B     算法：algo.angle_measure（角度测量）
 *   需求：角度偏差不超过 ±15 度判 OK，输出角度和朝向，根本不关心表面评分
 * 调用链：base.plc_sim(CAM_B) -> algo.angle_measure -> base.imgproc
 *
 * 注意：本文件一个字都没提 detect_surface —— 客户B的项目不需要那个能力。 */
#include "flow.h"
#include "angle.h"   /* algo：这个项目要的是"角度" */
#include "plc_sim.h" /* base：这个项目的硬件是 CAM_B */
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#define B_TOLERANCE 15

static const char *direction_of(int angle)
{
    if (angle < -2) return "left";
    if (angle > 2)  return "right";
    return "center";
}

static void run(char *json, size_t n)
{
    frame_t        f;
    angle_result_t a;
    const char    *verdict;

    log_trace("flow.customer_b", "角度测量项目：取图 CAM_B -> algo.angle_measure()");
    plc_sim_grab(PLC_CAM_B, &f);
    angle_measure(f.px, PLC_FRAME_PIXELS, &a);

    verdict = (abs(a.angle) <= B_TOLERANCE) ? "OK" : "NG";
    log_trace("flow.customer_b", "判定 |%d| vs ±%d -> %s", a.angle, B_TOLERANCE, verdict);

    snprintf(json, n,
             "{\"type\":\"result\",\"flow\":\"customer_b\",\"frame\":%d,"
             "\"angle\":%d,\"verdict\":\"%s\",\"tolerance\":%d,"
             "\"direction\":\"%s\",\"confidence\":%d}",
             f.frame_id, a.angle, verdict, B_TOLERANCE,
             direction_of(a.angle), a.confidence);
}

const flow_t FLOW_CUSTOMER_B = {"customer_b", run};
