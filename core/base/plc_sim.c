#include "plc_sim.h"
#include <stdlib.h>

static int s_frame_id = 0;

void plc_sim_init(unsigned int seed)
{
    srand(seed);
    s_frame_id = 0;
}

void plc_sim_next(frame_t *out)
{
    /* 每帧一个基准亮度，叠加噪声：让 score 在 OK/NG 阈值两侧来回摆动 */
    int base = 120 + rand() % 70;
    int i;

    for (i = 0; i < PLC_FRAME_PIXELS; i++) {
        int v = base + (rand() % 41) - 20;
        if (v < 0)   v = 0;
        if (v > 255) v = 255;
        out->px[i] = (unsigned char)v;
    }
    out->frame_id = ++s_frame_id;
}
