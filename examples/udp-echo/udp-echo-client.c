#include <atari.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pokeystream.h"

static uint16_t append_eol(char* dst, uint16_t len, uint16_t cap)
{
    if (len < cap) dst[len++] = (char)0x9B;
    if (len < cap) dst[len] = '\0';
    return len;
}

static uint16_t to_atascii_line(const char* line, char* out, uint16_t cap)
{
    uint16_t i = 0;
    while (line[i] && i + 1 < cap) {
        if (line[i] == '\r') {
            ++i;
            continue;
        }
        if (line[i] == '\n') break;
        out[i] = line[i];
        ++i;
    }
    return append_eol(out, i, cap);
}

static void send_line(const char* line)
{
    char buf[128];
    uint16_t len = to_atascii_line(line, buf, (uint16_t)sizeof(buf) - 1);
    ps_send(buf, len);
    ps_flush_tx();
}

static void print_prompt(const char* input, unsigned len)
{
    unsigned i = 0;
    printf("\n> ");
    while (i < len) {
        cputc(input[i]);
        ++i;
    }
}

int main(void)
{
    char line[128];
    char rx_line[128];
    unsigned rx_len = 0;
    unsigned line_len = 0;

    clrscr();
    cprintf("POKEYStream UDP Echo Demo Atari Client\r\n");
    cprintf("Type a line and press ENTER.\r\n\r\n");

    ps_init();

    for (;;) {
        uint8_t b;

        line_len = 0;
        line[0] = '\0';
        print_prompt(line, line_len);

        for (;;) {
            while (ps_read_byte(&b)) {
                if (b == 0x9B) {
                    rx_line[rx_len] = '\0';
                    printf("\n%s\n", rx_line);
                    rx_len = 0;
                    print_prompt(line, line_len);
                } else if (rx_len + 1 < sizeof(rx_line)) {
                    rx_line[rx_len++] = (char)b;
                }
            }

            if (kbhit()) {
                char ch = cgetc();
                if (ch == '\r' || ch == '\n') {
                    cputc('\n');
                    line[line_len] = '\0';
                    send_line(line);
                    rx_len = 0;
                    break;
                }
                if ((ch == 8 || ch == 127) && line_len > 0) {
                    --line_len;
                    gotox(wherex() - 1);
                    cputc(' ');
                    gotox(wherex() - 1);
                    continue;
                }
                if (line_len + 1 < sizeof(line)) {
                    line[line_len++] = ch;
                    cputc(ch);
                }
            }
        }
    }
}
