#include "net.h"
#include "log.h"
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef SOCKET sock_t;
#  define SOCK_BAD   INVALID_SOCKET
#  define sock_close closesocket
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
   typedef int sock_t;
#  define SOCK_BAD   (-1)
#  define sock_close close
#endif

#define MAX_CLIENTS 8
#define RXBUF       256

typedef struct {
    sock_t s;
    char   rx[RXBUF];
    size_t rxlen;
} client_t;

static sock_t     s_listen = SOCK_BAD;
static client_t   s_cli[MAX_CLIENTS];
static net_cmd_cb s_cb = NULL;

static void client_drop(int i)
{
    sock_close(s_cli[i].s);
    s_cli[i].s = SOCK_BAD;
    s_cli[i].rxlen = 0;
    log_msg("net", "client disconnected (%d online)", net_client_count());
}

int net_start(int port, net_cmd_cb cb)
{
    struct sockaddr_in addr;
    int                yes = 1, i;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        log_msg("net", "WSAStartup failed");
        return -1;
    }
#endif
    for (i = 0; i < MAX_CLIENTS; i++) s_cli[i].s = SOCK_BAD;
    s_cb = cb;

    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen == SOCK_BAD) return -1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(s_listen, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        log_msg("net", "bind :%d failed", port);
        sock_close(s_listen);
        s_listen = SOCK_BAD;
        return -1;
    }
    if (listen(s_listen, 4) != 0) {
        sock_close(s_listen);
        s_listen = SOCK_BAD;
        return -1;
    }
    log_msg("net", "listening on 127.0.0.1:%d", port);
    return 0;
}

static void handle_line(int i, char *line)
{
    char reply[256];

    if (line[0] == '\0' || s_cb == NULL) return;
    reply[0] = '\0';
    s_cb(line, reply, sizeof(reply));
    if (reply[0] != '\0') {
        char out[288];
        int  n = snprintf(out, sizeof(out), "%s\n", reply);
        send(s_cli[i].s, out, n, 0);
    }
}

/* 从客户端缓冲里切出整行并逐条处理 */
static void drain_lines(int i)
{
    char *nl;

    while ((nl = memchr(s_cli[i].rx, '\n', s_cli[i].rxlen)) != NULL) {
        size_t used = (size_t)(nl - s_cli[i].rx) + 1; /* 含 '\n' */
        size_t len  = used - 1;
        char   line[RXBUF];

        memcpy(line, s_cli[i].rx, len);
        line[len] = '\0';
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' '))
            line[--len] = '\0';

        s_cli[i].rxlen -= used;
        memmove(s_cli[i].rx, nl + 1, s_cli[i].rxlen);
        handle_line(i, line);
    }
}

void net_poll(void)
{
    fd_set         rf;
    struct timeval tv;
    sock_t         maxfd = s_listen;
    int            i;

    if (s_listen == SOCK_BAD) return;

    FD_ZERO(&rf);
    FD_SET(s_listen, &rf);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (s_cli[i].s == SOCK_BAD) continue;
        FD_SET(s_cli[i].s, &rf);
        if (s_cli[i].s > maxfd) maxfd = s_cli[i].s;
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (select((int)maxfd + 1, &rf, NULL, NULL, &tv) <= 0) return;

    if (FD_ISSET(s_listen, &rf)) {
        sock_t c = accept(s_listen, NULL, NULL);
        if (c != SOCK_BAD) {
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (s_cli[i].s == SOCK_BAD) {
                    s_cli[i].s = c;
                    s_cli[i].rxlen = 0;
                    log_msg("net", "client connected (%d online)", net_client_count());
                    break;
                }
            }
            if (i == MAX_CLIENTS) sock_close(c); /* 满了直接拒 */
        }
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        int n;
        if (s_cli[i].s == SOCK_BAD || !FD_ISSET(s_cli[i].s, &rf)) continue;

        n = recv(s_cli[i].s, s_cli[i].rx + s_cli[i].rxlen,
                 (int)(RXBUF - s_cli[i].rxlen), 0);
        if (n <= 0) {
            client_drop(i);
            continue;
        }
        s_cli[i].rxlen += (size_t)n;
        drain_lines(i);
        if (s_cli[i].rxlen >= RXBUF) s_cli[i].rxlen = 0; /* 超长行丢弃 */
    }
}

void net_broadcast(const char *line)
{
    char out[512];
    int  n, i;

    n = snprintf(out, sizeof(out), "%s\n", line);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (s_cli[i].s == SOCK_BAD) continue;
        if (send(s_cli[i].s, out, n, 0) < 0) client_drop(i);
    }
}

int net_client_count(void)
{
    int i, c = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
        if (s_cli[i].s != SOCK_BAD) c++;
    return c;
}

void net_stop(void)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
        if (s_cli[i].s != SOCK_BAD) sock_close(s_cli[i].s);
    if (s_listen != SOCK_BAD) sock_close(s_listen);
    s_listen = SOCK_BAD;
#ifdef _WIN32
    WSACleanup();
#endif
}
