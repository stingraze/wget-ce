/* mini_wget_ce.c ─────────────────────────────────────────────────────────
   Build:  arm-mingw32ce-gcc -O2 -DWINCE mini_wget_ce.c -o mini-wget.exe -lws2
   Notes:  • IPv4 / plain HTTP only           • ≈6 kB after strip
           • No libmingwex dependency (custom perror stub below)
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ───────────────────── minimal perror replacement ───────────────────── */
static void wce_perror(const char *msg)
{
    fprintf(stderr, "%s: error %lu\n", msg, (unsigned long)GetLastError());
}
#define perror(x) wce_perror(x)

/* ───────────────────── tiny http:// URL parser ──────────────────────── */
static int split_url(const char *url,
                     char *host, size_t hsz,
                     char *path, size_t psz,
                     int  *port)
{
    if (strncmp(url, "http://", 7) != 0) return -1;
    url += 7;                                    /* skip “http://”        */

    const char *slash = strchr(url, '/');
    const char *colon = strchr(url, ':');

    if (colon && (!slash || colon < slash)) {
        size_t len = colon - url;
        if (len >= hsz) return -1;
        memcpy(host, url, len); host[len] = 0;
        *port = atoi(colon + 1);
    } else {
        size_t len = slash ? (size_t)(slash - url) : strlen(url);
        if (len >= hsz) return -1;
        memcpy(host, url, len); host[len] = 0;
        *port = 80;
    }
    snprintf(path, psz, "%s", slash ? slash : "/");
    return 0;
}

/* ───────────────────────── single-shot GET ──────────────────────────── */
static int http_get(const char *host, const char *path, int port)
{
    WSADATA wsa;
    SOCKET  s = INVALID_SOCKET;
    struct hostent *hp;
    struct sockaddr_in sa = {0};
    char req[256], buf[1024];
    int  n, total = 0;

    if (WSAStartup(MAKEWORD(2,2), &wsa)) { perror("WSAStartup"); return 1; }

    hp = gethostbyname(host);
    if (!hp) { perror("gethostbyname"); goto done; }

    sa.sin_family = AF_INET;
    sa.sin_port   = htons((u_short)port);
    memcpy(&sa.sin_addr, hp->h_addr_list[0], hp->h_length);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { perror("socket"); goto done; }
    if (connect(s, (struct sockaddr*)&sa, sizeof sa)) { perror("connect"); goto done; }

    snprintf(req, sizeof req,
             "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: mini-wget-ce\r\n\r\n",
             path, host);
    send(s, req, (int)strlen(req), 0);

    while ((n = recv(s, buf, sizeof buf, 0)) > 0) {
        fwrite(buf, 1, n, stdout);
        total += n;
    }
    fprintf(stderr, "\n[%d bytes]\n", total);

done:
    if (s != INVALID_SOCKET) closesocket(s);
    WSACleanup();
    return 0;
}

/* ─────────────────────────── entry point ────────────────────────────── */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        puts("usage: mini-wget http://host[:port]/path");
        return 0;
    }

    char host[128] = {0}, path[256] = {0};
    int  port = 80;

    if (split_url(argv[1], host, sizeof host, path, sizeof path, &port) != 0) {
        fputs("only http:// URLs supported\n", stderr);
        return 1;
    }
    return http_get(host, path, port);
}
