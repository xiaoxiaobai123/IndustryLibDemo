/* main.c - 宿主：起网络、跑主循环、转发命令
 *
 * 注意本文件 include 了什么：flow.h / log.h / net.h —— 没有算法，没有相机。
 * 宿主不知道"表面评分""角度"是什么，也不知道谁用哪个相机：
 * 那是每个客户项目(flow)自己的事。宿主只按节拍喊一嗓子 run()。
 */
#include "flow.h"
#include "log.h"
#include "net.h"
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

#define PORT    9500
#define TICK_MS 500

/* 条件编译的落点：没勾选的客户项目连符号都不存在 */
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
    } else if (strcmp(cmd, "trace") == 0) {
        int on = !log_trace_enabled();
        log_set_trace(on);
        log_msg("main", "cmd trace -> %s", on ? "on" : "off");
        snprintf(reply, n, "{\"type\":\"ack\",\"cmd\":\"trace\",\"trace\":%s}",
                 on ? "true" : "false");
    } else if (strcmp(cmd, "status") == 0) {
        snprintf(reply, n,
                 "{\"type\":\"status\",\"running\":%s,\"flow_count\":%d,"
                 "\"build\":\"%s\",\"trace\":%s}",
                 g_running ? "true" : "false", FLOW_COUNT, BUILD_TAG,
                 log_trace_enabled() ? "true" : "false");
    } else {
        snprintf(reply, n, "{\"type\":\"error\",\"msg\":\"unknown cmd\"}");
    }
}

static void tick(void)
{
    char json[512];
    int  i;

    /* 宿主对每个项目一视同仁：喊一声 run，拿回一行 JSON。
     * 取图/算法/判定全在 flow 里面发生，这里看不到也不需要看到。 */
    for (i = 0; i < FLOW_COUNT; i++) {
        if (log_trace_enabled())
            log_trace("main", "--- tick: 调 flow[%d] %s.run() ---", i, g_flows[i]->name);
        g_flows[i]->run(json, sizeof(json));
        log_msg("flow", "%s", json);
        net_broadcast(json);
    }
}

int main(int argc, char **argv)
{
    int i;

    if (argc > 1 && strcmp(argv[1], "--trace") == 0) log_set_trace(1);

    log_msg("main", "IndustryLibDemo core   build=%s", BUILD_TAG);
    log_msg("main", "flows compiled in: %d", FLOW_COUNT);
    for (i = 0; i < FLOW_COUNT; i++)
        log_msg("main", "  [%d] %s", i + 1, g_flows[i]->name);

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
