/* main.c - 主程序：装配各层 + 主循环 + 命令处理
 *
 * 依赖方向 flow -> algo -> base，本文件是唯一知道"全貌"的地方。
 * 完全脱离 UI 独立运行；UI 随时接入/断开互不影响。
 */
#include "flow.h"
#include "detect.h"
#include "log.h"
#include "net.h"
#include "plc_sim.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#  define sleep_ms(ms) Sleep(ms)
#else
#  include <unistd.h>
#  define sleep_ms(ms) usleep((ms) * 1000)
#endif

#ifndef BUILD_TAG
#  define BUILD_TAG "unknown"
#endif

#define PORT        9500
#define TICK_MS     500

/* 条件编译的落点：关掉的客户流程连符号都不存在 */
static const flow_t *const g_flows[] = {
    &FLOW_COMMON,
#ifdef FLOW_A
    &FLOW_CUSTOMER_A,
#endif
#ifdef FLOW_B
    &FLOW_CUSTOMER_B,
#endif
};
#define FLOW_COUNT ((int)(sizeof(g_flows) / sizeof(g_flows[0])))

static int g_running = 1;

static void on_cmd(const char *cmd, char *reply, size_t n)
{
    if (strcmp(cmd, "start") == 0) {
        g_running = 1;
        log_msg("main", "cmd start -> running");
        snprintf(reply, n, "{\"type\":\"ack\",\"cmd\":\"start\",\"running\":true}");
    } else if (strcmp(cmd, "stop") == 0) {
        g_running = 0;
        log_msg("main", "cmd stop -> idle");
        snprintf(reply, n, "{\"type\":\"ack\",\"cmd\":\"stop\",\"running\":false}");
    } else if (strcmp(cmd, "status") == 0) {
        snprintf(reply, n,
                 "{\"type\":\"status\",\"running\":%s,\"flow_count\":%d,"
                 "\"build\":\"%s\"}",
                 g_running ? "true" : "false", FLOW_COUNT, BUILD_TAG);
    } else {
        snprintf(reply, n, "{\"type\":\"error\",\"msg\":\"unknown cmd\"}");
    }
}

static void tick(void)
{
    frame_t         f;
    detect_result_t r;
    char            json[512];
    int             i;

    plc_sim_next(&f);                              /* base : 取一帧 */
    detect_run(f.px, PLC_FRAME_PIXELS, &r);        /* algo : 算 score */

    for (i = 0; i < FLOW_COUNT; i++) {             /* flow : 各自判定 */
        g_flows[i]->run(f.frame_id, &r, json, sizeof(json));
        log_msg("flow", "%s", json);
        net_broadcast(json);
    }
}

int main(void)
{
    int i;

    log_msg("main", "IndustryLibDemo core   build=%s", BUILD_TAG);
    log_msg("main", "flows compiled in: %d", FLOW_COUNT);
    for (i = 0; i < FLOW_COUNT; i++)
        log_msg("main", "  [%d] %s", i + 1, g_flows[i]->name);

    plc_sim_init(20260716u);
    if (net_start(PORT, on_cmd) != 0) {
        log_msg("main", "net_start failed, exit");
        return 1;
    }

    for (;;) {
        net_poll();            /* UI 接入/断开/发命令，不阻塞 */
        if (g_running) tick(); /* 核心业务，与 UI 无关 */
        sleep_ms(TICK_MS);
    }

    net_stop();
    return 0;
}
