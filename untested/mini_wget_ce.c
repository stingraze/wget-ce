/* mini_wget_ce.c ─────────────────────────────────────────────────────────
   (C)Tsubasa Kato - 7/2/2025 - Untested Version, created with help of GitHub Copilot.
   Build:  arm-mingw32ce-gcc -O2 -DWINCE mini_wget_ce.c -o mini-wget.exe -lws2
   Notes:  • IPv4 / plain HTTP only           • ≈6 kB after strip
           • No libmingwex dependency (custom perror stub below)
           • Now supports -O <outputfile> for file output
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
static int http_get(const char *host, const char *path, int port, FILE *out)
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

    s*

