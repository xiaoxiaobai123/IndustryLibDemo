/* base/log.h - 日志原语：时间戳 + 模块名 */
#ifndef BASE_LOG_H
#define BASE_LOG_H

/* 打印一行日志：[HH:MM:SS][module] message */
void log_msg(const char *module, const char *fmt, ...);

#endif /* BASE_LOG_H */
