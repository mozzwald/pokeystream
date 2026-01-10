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
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
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

static struct termios orig_term;
static int term_rows = 24;
static int term_cols = 80;

static void restore_term(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

static void set_raw_term(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig_term);
    t = orig_term;
    t.c_lflag &= (unsigned)(~(ICANON | ECHO));
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    atexit(restore_term);
}

static void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
}

static void show_prompt(const char* buf, size_t len, int input_row)
{
    move_cursor(input_row, 1);
    printf("\033[K> ");
    if (len > 0) fwrite(buf, 1, len, stdout);
    fflush(stdout);
}

static void clear_rx_region(int rx_top, int rx_bottom)
{
    for (int r = rx_top; r <= rx_bottom; ++r) {
        move_cursor(r, 1);
        printf("\033[K");
    }
    fflush(stdout);
}

static void log_message(const char* msg, int rx_top, int rx_bottom, int* rx_row)
{
    move_cursor(*rx_row, 1);
    printf("\033[K%s", msg);
    if (*rx_row >= rx_bottom) {
        clear_rx_region(rx_top, rx_bottom);
        *rx_row = rx_top;
    } else {
        ++(*rx_row);
    }
    fflush(stdout);
}

static void render_remote_char(unsigned char c,
                               int* rx_row,
                               int* rx_col,
                               int rx_top,
                               int rx_bottom,
                               const char* input,
                               size_t input_len,
                               int input_row)
{
    if (c == 0x9B) {
        *rx_col = 1;
        if (*rx_row >= rx_bottom) {
            clear_rx_region(rx_top, rx_bottom);
            *rx_row = rx_top;
        } else {
            ++(*rx_row);
        }
        show_prompt(input, input_len, input_row);
        return;
    }

    move_cursor(*rx_row, *rx_col);
    putchar((int)c);
    if (*rx_col >= term_cols) {
        *rx_col = 1;
        if (*rx_row >= rx_bottom) {
            clear_rx_region(rx_top, rx_bottom);
            *rx_row = rx_top;
        } else {
            ++(*rx_row);
        }
    } else {
        ++(*rx_col);
    }
    fflush(stdout);
    show_prompt(input, input_len, input_row);
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

    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            if (ws.ws_row > 0) term_rows = ws.ws_row;
            if (ws.ws_col > 0) term_cols = ws.ws_col;
        }
        if (term_rows < 6) term_rows = 6;
        if (term_rows > 60) term_rows = 24;
        if (term_cols < 20) term_cols = 40;
        if (term_cols > 120) term_cols = 80;
    }

    set_raw_term();
    printf("\033[2J");
    move_cursor(1, 1);
    printf("POKEYStream UDP Echo Demo Server\n");
    printf("Listening on UDP port %ld...\n", port);

    bool have_peer = false;
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    char input[256];
    size_t input_len = 0;
    int rx_top = 3;
    int rx_bottom = term_rows - 2;
    int input_row = term_rows;
    int rx_row = rx_top;
    int rx_col = 1;

    clear_rx_region(rx_top, rx_bottom);
    move_cursor(rx_bottom + 1, 1);
    for (int i = 0; i < term_cols; ++i) putchar('-');
    show_prompt(input, input_len, input_row);

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
            char ch;
            ssize_t rn = read(STDIN_FILENO, &ch, 1);
            if (rn == 1) {
                if (ch == '\r' || ch == '\n') {
                    if (have_peer) {
                        unsigned char eol = 0x9B;
                        if (sendto(fd, &eol, 1, 0,
                                   (struct sockaddr*)&peer, peer_len) < 0) {
                            perror("sendto");
                        }
                    }
                    input_len = 0;
                    show_prompt(input, input_len, input_row);
                } else {
                    if (input_len + 1 < sizeof(input)) {
                        input[input_len++] = ch;
                    }
                    if (have_peer) {
                        if (sendto(fd, &ch, 1, 0,
                                   (struct sockaddr*)&peer, peer_len) < 0) {
                            perror("sendto");
                        }
                    }
                    show_prompt(input, input_len, input_row);
                }
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

            /* FujiNet Client Registered */
            if (msg_len == 8 && !memcmp(buf, "REGISTER", 8)) {
                log_message("Client Registered", rx_top, rx_bottom, &rx_row);
                rx_col = 1;
                fflush(stdout);
                peer = from;
                peer_len = from_len;
                have_peer = true;
                rx_col = 1;
                show_prompt(input, input_len, input_row);
                /* Break out since we don't need to show the packet info for registration */
                continue;
            }

            /* Echo back to whoever spoke last */
            peer = from;
            peer_len = from_len;
            have_peer = true;
            if (n > 0) {
                size_t payload_off = 0;
                if (n >= 3 && (unsigned char)buf[1] == 0x00) {
                    payload_off = 2;
                }
                for (ssize_t i = (ssize_t)payload_off; i < n; ++i) {
                    unsigned char c = (unsigned char)buf[i];
                    render_remote_char(c, &rx_row, &rx_col, rx_top, rx_bottom,
                                       input, input_len, input_row);
                }
            }
        }
    }

    close(fd);
    return 0;
}
