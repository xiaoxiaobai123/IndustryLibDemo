/* base/plc_sim.h - 模拟数据源（相机 / PLC 采集）
 *
 * 真实项目里这里是各家相机 SDK。不同客户项目用的硬件不一样，
 * 所以按通道区分：流程自己决定从哪个硬件取图。
 */
#ifndef BASE_PLC_SIM_H
#define BASE_PLC_SIM_H

#define PLC_FRAME_PIXELS 64

typedef enum {
    PLC_CAM_A = 0, /* 客户A产线：面阵相机，整幅亮度图（看表面质量） */
    PLC_CAM_B = 1  /* 客户B产线：线扫相机，带倾角的轮廓图（看角度）   */
} plc_source_t;

typedef struct {
    int           frame_id; /* 每个硬件通道各自计数 */
    unsigned char px[PLC_FRAME_PIXELS];
} frame_t;

/* 从指定硬件取一帧。首次调用自动初始化，宿主不需要管。 */
void plc_sim_grab(plc_source_t src, frame_t *out);

#endif /* BASE_PLC_SIM_H */
