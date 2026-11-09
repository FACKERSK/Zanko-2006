// net.c — UDP sockets for multiplayer, kept apart from raylib (windows.h and raylib clash). Built as its own unit.
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t; typedef int socklen_t2;
#define BADSOCK INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int sock_t; typedef socklen_t socklen_t2;
#define BADSOCK (-1)
#define closesocket close
#endif
typedef struct { unsigned int ip; unsigned short port; } NetAddr;   // network byte order
static sock_t sk = BADSOCK;
static int wsaUp = 0;
int netOpen(int port) {   // port 0: any (clients)
#ifdef _WIN32
    if (!wsaUp) { WSADATA w; if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return 0; wsaUp = 1; }
#endif
    sk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); if (sk == BADSOCK) return 0;
    struct sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons((unsigned short)port);
    int yes = 1; setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);
    if (bind(sk, (struct sockaddr *)&a, sizeof a) != 0) { closesocket(sk); sk = BADSOCK; return 0; }
#ifdef _WIN32
    u_long nb = 1; ioctlsocket(sk, FIONBIO, &nb);
#else
    fcntl(sk, F_SETFL, fcntl(sk, F_GETFL, 0) | O_NONBLOCK);
#endif
    int buf = 1 << 20; setsockopt(sk, SOL_SOCKET, SO_RCVBUF, (const char *)&buf, sizeof buf); setsockopt(sk, SOL_SOCKET, SO_SNDBUF, (const char *)&buf, sizeof buf);
    return 1;
}
void netClose(void) { if (sk != BADSOCK) closesocket(sk); sk = BADSOCK; }
int netResolve(const char *host, int port, NetAddr *out) {
#ifdef _WIN32
    if (!wsaUp) { WSADATA w; if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return 0; wsaUp = 1; }
#endif
    struct addrinfo hints, *res = NULL; memset(&hints, 0, sizeof hints); hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res) return 0;
    out->ip = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr; out->port = htons((unsigned short)port); freeaddrinfo(res); return 1;
}
int netSend(const NetAddr *to, const void *buf, int len) {
    if (sk == BADSOCK) return 0;
    struct sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_addr.s_addr = to->ip; a.sin_port = to->port;
    return sendto(sk, (const char *)buf, len, 0, (struct sockaddr *)&a, sizeof a) == len;
}
int netRecv(void *buf, int max, NetAddr *from) {   // -> bytes, 0 when nothing is waiting
    if (sk == BADSOCK) return 0;
    struct sockaddr_in a; socklen_t2 al = sizeof a;
    int n = (int)recvfrom(sk, (char *)buf, max, 0, (struct sockaddr *)&a, &al);
    if (n <= 0) return 0;
    from->ip = a.sin_addr.s_addr; from->port = a.sin_port; return n;
}
void netAddrStr(const NetAddr *a, char *out, int max) { struct in_addr ia; ia.s_addr = a->ip; snprintf(out, max, "%s:%d", inet_ntoa(ia), ntohs(a->port)); }
