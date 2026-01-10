/*
 * UDP echo server for FujiNet UDP Stream mode.
 *
 * Build:
 *   gcc -std=c99 -O2 -Wall -Wextra -o udp-echo-server udp-echo-server.c
 *
 * Run:
 *   ./udp-echo-server -p 5004
 */

#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s -p <port>\n", prog);
}

static long parse_port(const char* s)
{
    char* end = NULL;
    long p = strtol(s, &end, 10);
    if (!s[0] || (end && *end) || p < 1 || p > 65535) return -1;
    return p;
}

static size_t build_line(char* out, size_t cap, const char* payload, size_t len)
{
    size_t i = 0;
    for (size_t j = 0; j < len && i + 1 < cap; ++j) {
        if ((unsigned char)payload[j] == 0x9B) break;
        out[i++] = payload[j];
    }
    if (i + 1 < cap) out[i++] = (char)0x9B;
    if (i < cap) out[i] = '\0';
    return i;
}

int main(int argc, char** argv)
{
    long port = -1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    if (port < 0) {
        usage(argv[0]);
        return 2;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    printf("POKEYStream UDP Echo Demo Server\n");
    printf("Listening on UDP port %ld...\n", port);
    printf("> ");
    fflush(stdout);

    bool have_peer = false;
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    char rx_line[512];
    size_t rx_len = 0;

    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        /* Wake up periodically so we can handle stdin */
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int maxfd = fd > STDIN_FILENO ? fd : STDIN_FILENO;
        int r = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char line[256];
            if (fgets(line, sizeof(line), stdin)) {
                if (have_peer) {
                    char msg[512];
                    size_t len = 0;
                    while (line[len] && line[len] != '\n' &&
                           line[len] != '\r' && len + 1 < sizeof(msg)) {
                        msg[len] = line[len];
                        ++len;
                    }
                    if (len + 1 < sizeof(msg)) {
                        msg[len++] = (char)0x9B;
                    }
                    if (len < sizeof(msg)) msg[len] = '\0';
                    if (sendto(fd, msg, len, 0,
                               (struct sockaddr*)&peer, peer_len) < 0) {
                        perror("sendto");
                    }
                } else {
                    printf("No client registered yet.\n");
                }
                printf("> ");
                fflush(stdout);
            }
        }

        if (r > 0 && FD_ISSET(fd, &rfds)) {
            char buf[2048];
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                                 (struct sockaddr*)&from, &from_len);
            if (n < 0) {
                perror("recvfrom");
                continue;
            }
            buf[n] = '\0';

            size_t msg_len = (size_t)n;
            while (msg_len > 0 &&
                   (buf[msg_len - 1] == '\r' || buf[msg_len - 1] == '\n')) {
                buf[--msg_len] = '\0';
            }

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));

            /* FujiNet Client Registered */
            if (msg_len == 8 && !memcmp(buf, "REGISTER", 8)) {
                printf("\nClient Registered\n");
                fflush(stdout);
                peer = from;
                peer_len = from_len;
                have_peer = true;
                printf("> ");
                fflush(stdout);
                /* Break out since we don't need to show the packet info for registration */
                continue;
            }

            (void)ip;

            /* Echo back to whoever spoke last */
            peer = from;
            peer_len = from_len;
            have_peer = true;
            if (n > 2) {
                const char* payload = buf + 2;
                size_t plen = (size_t)(n - 2);
                bool printed = false;
                for (size_t i = 0; i < plen; ++i) {
                    unsigned char c = (unsigned char)payload[i];
                    if (c == 0x9B) {
                        rx_line[rx_len] = '\0';
                        printf("\n%s\n", rx_line);
                        rx_len = 0;
                        printed = true;
                    } else if (rx_len + 1 < sizeof(rx_line)) {
                        rx_line[rx_len++] = (char)c;
                    }
                }
                if (printed) {
                    printf("> ");
                    fflush(stdout);
                }
            }
        }
    }

    close(fd);
    return 0;
}
