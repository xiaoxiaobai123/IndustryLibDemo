#include "plc_sim.h"
#include "log.h"
#include <stdlib.h>

#define PLC_SOURCE_COUNT 2

static int s_frame_id[PLC_SOURCE_COUNT];
static int s_inited = 0;

static int clamp_px(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return v;
}

/* 面阵相机：整体亮度 + 随机噪声，亮度高=表面好 */
static void grab_cam_a(unsigned char *px)
{
    int base = 120 + rand() % 70;
    int i;
    for (i = 0; i < PLC_FRAME_PIXELS; i++)
        px[i] = (unsigned char)clamp_px(base + (rand() % 41) - 20);
}

/* 线扫相机：沿扫描方向有一个倾角，表现为亮度梯度 */
static void grab_cam_b(unsigned char *px)
{
    int    tilt = (rand() % 61) - 30; /* 本帧真实倾角 -30..30 度 */
    double half = (PLC_FRAME_PIXELS - 1) / 2.0;
    int    i;

    for (i = 0; i < PLC_FRAME_PIXELS; i++) {
        double pos = (i - half) / half; /* -1 .. +1 */
        px[i] = (unsigned char)clamp_px(150 + (int)(tilt * pos) + (rand() % 11) - 5);
    }
}

void plc_sim_grab(plc_source_t src, frame_t *out)
{
    if (!s_inited) {
        srand(20260716u);
        s_inited = 1;
    }
    if (src == PLC_CAM_A) grab_cam_a(out->px);
    else                  grab_cam_b(out->px);

    out->frame_id = ++s_frame_id[src];
    log_trace("base.plc_sim", "grab CAM_%c -> frame=%d",
              src == PLC_CAM_A ? 'A' : 'B', out->frame_id);
}
