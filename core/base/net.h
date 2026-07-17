/* base/net.h - TCP 服务端（行分隔文本协议）
 *
 * 非阻塞：net_poll() 由主循环调用，UI 随时接入/断开都不影响核心运行。
 */
#ifndef BASE_NET_H
#define BASE_NET_H

#include <stddef.h>

/* 收到一条命令时回调；把回复写进 reply（写空串表示不回复） */
typedef void (*net_cmd_cb)(const char *cmd, char *reply, size_t reply_size);

int  net_start(int port, net_cmd_cb cb);
/* 处理新连接 / 读命令 / 清理断开的客户端，不阻塞 */
void net_poll(void);
/* 向所有已连接客户端推一行（函数内部补 '\n'） */
void net_broadcast(const char *line);
int  net_client_count(void);
void net_stop(void);

#endif /* BASE_NET_H */
