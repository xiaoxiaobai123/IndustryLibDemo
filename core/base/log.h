/* base/log.h - 日志原语：时间戳 + 模块名 */
#ifndef BASE_LOG_H
#define BASE_LOG_H

/* 打印一行日志：[HH:MM:SS][module] message */
void log_msg(const char *module, const char *fmt, ...);

/* 调用链跟踪：默认关闭，开了之后每一层被调用时自己打一行，
 * 用来在演示时直观看到 flow -> algo -> base 的真实调用顺序。 */
void log_set_trace(int on);
int  log_trace_enabled(void);
void log_trace(const char *module, const char *fmt, ...);

#endif /* BASE_LOG_H */
