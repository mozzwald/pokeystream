#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

#define DEFAULT_PORT "9000"
#define DEFAULT_PEER "127.0.0.1:9001"

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s [--port <port>] [--peer <host:port>]\n", prog);
}

static int parse_peer(const char* spec, struct sockaddr_storage* out_addr, socklen_t* out_len)
{
    char host[256];
    char port[32];
    const char* colon = strrchr(spec, ':');
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    int rc = 0;

    if (!colon || colon == spec || *(colon + 1) == '\0') {
        return -1;
    }

    memset(host, 0, sizeof(host));
    memset(port, 0, sizeof(port));

    if ((size_t)(colon - spec) >= sizeof(host)) {
        return -1;
    }

    memcpy(host, spec, (size_t)(colon - spec));
    strncpy(port, colon + 1, sizeof(port) - 1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        return -1;
    }

    if (res->ai_addrlen > sizeof(*out_addr)) {
        freeaddrinfo(res);
        return -1;
    }

    memcpy(out_addr, res->ai_addr, res->ai_addrlen);
    *out_len = (socklen_t)res->ai_addrlen;
    freeaddrinfo(res);
    return 0;
}

int main(int argc, char** argv)
{
    const char* port = DEFAULT_PORT;
    const char* peer_spec = DEFAULT_PEER;
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct sockaddr_storage peer_addr;
    socklen_t peer_len = 0;
    int sockfd = -1;
    int rc = 0;
    uint32_t rx_count = 0;
    uint32_t tx_count = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "--peer") == 0 && i + 1 < argc) {
            peer_spec = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (parse_peer(peer_spec, &peer_addr, &peer_len) != 0) {
        fprintf(stderr, "Invalid peer specification: %s\n", peer_spec);
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(NULL, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        freeaddrinfo(res);
        close(sockfd);
        return 1;
    }
    freeaddrinfo(res);

    fprintf(stderr, "Listening on UDP port %s, peer %s\n", port, peer_spec);

    while (1) {
        fd_set rfds;
        int maxfd = sockfd;
        struct timeval tv;

        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        if (STDIN_FILENO > maxfd) {
            maxfd = STDIN_FILENO;
        }

        tv.tv_sec = 0;
        tv.tv_usec = 20000;

        rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(sockfd, &rfds)) {
            uint8_t buf[512];
            ssize_t got = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
            if (got > 0) {
                for (ssize_t i = 0; i < got; ++i) {
                    uint8_t ch = buf[i];
                    if (ch == 0x9B) {
                        ch = '\n';
                    }
                    fputc(ch, stdout);
                }
                fflush(stdout);
                rx_count += (uint32_t)got;
                fprintf(stderr, "RX:%u TX:%u\n", rx_count, tx_count);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char line[256];
            if (fgets(line, sizeof(line), stdin) == NULL) {
                break;
            }

            size_t len = strlen(line);
            uint8_t out[256];
            size_t out_len = 0;
            for (size_t i = 0; i < len && out_len < sizeof(out); ++i) {
                if (line[i] == '\n') {
                    out[out_len++] = 0x9B;
                } else {
                    out[out_len++] = (uint8_t)line[i];
                }
            }

            if (out_len > 0) {
                ssize_t sent = sendto(sockfd, out, out_len, 0,
                                      (const struct sockaddr*)&peer_addr, peer_len);
                if (sent >= 0) {
                    tx_count += (uint32_t)sent;
                    fprintf(stderr, "RX:%u TX:%u\n", rx_count, tx_count);
                } else {
                    perror("sendto");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
