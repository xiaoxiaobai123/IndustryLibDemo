/* base/plc_sim.h - 模拟数据源
 *
 * 真实项目里这里是相机 SDK / PLC 采集卡；Demo 里用假数据顶替，
 * 上层算法完全感知不到差别 —— 这就是"数据源可替换"。
 */
#ifndef BASE_PLC_SIM_H
#define BASE_PLC_SIM_H

#define PLC_FRAME_PIXELS 64

typedef struct {
    int           frame_id;
    unsigned char px[PLC_FRAME_PIXELS];
} frame_t;

void plc_sim_init(unsigned int seed);
/* 产生下一帧，frame_id 自增 */
void plc_sim_next(frame_t *out);

#endif /* BASE_PLC_SIM_H */
