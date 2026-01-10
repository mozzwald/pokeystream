#include <atari.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pokeystream.h"

static void render_input_line(const char* input,
                              unsigned len,
                              unsigned char y,
                              unsigned char width)
{
    unsigned i = 0;
    if (width < 2) return;
    cputcxy(0, y, '>');
    cputcxy(1, y, ' ');
    for (i = 0; i + 2 < width; ++i) {
        char ch = (i < len) ? input[i] : ' ';
        cputcxy((unsigned char)(2 + i), y, ch);
    }
    if (y + 1 < 24) {
        for (i = 0; i < width; ++i) {
            char ch = (i + (width - 2) < len) ? input[i + (width - 2)] : ' ';
            cputcxy((unsigned char)i, (unsigned char)(y + 1), ch);
        }
    }
}

static void scroll_region(unsigned char top, unsigned char bottom, unsigned char width)
{
    unsigned char y = top;
    if (bottom <= top) return;
    while (y < bottom) {
        unsigned char x = 0;
        while (x < width) {
            gotoxy(x, (unsigned char)(y + 1));
            {
                char ch = cpeekc();
                gotoxy(x, y);
                cputc(ch);
            }
            ++x;
        }
        ++y;
    }
    cclearxy(0, bottom, width);
}

static void remote_putc(char c,
                        unsigned char* cur_x,
                        unsigned char* cur_y,
                        unsigned char top,
                        unsigned char bottom,
                        unsigned char width)
{
    if (c == (char)0x9B) {
        *cur_x = 0;
        if (*cur_y >= bottom) {
            scroll_region(top, bottom, width);
        } else {
            ++(*cur_y);
        }
        return;
    }

    gotoxy(*cur_x, *cur_y);
    cputc(c);
    if (++(*cur_x) >= width) {
        *cur_x = 0;
        if (*cur_y >= bottom) {
            scroll_region(top, bottom, width);
        } else {
            ++(*cur_y);
        }
    }
}

int main(void)
{
    char line[128];
    unsigned line_len = 0;
    unsigned char screen_w = 0;
    unsigned char screen_h = 0;
    unsigned char input_y = 0;
    unsigned char rx_x = 0;
    unsigned char rx_y = 0;
    unsigned char rx_top = 0;

    clrscr();
    gotoxy(0, 0);
    cputs("POKEYStream UDP Echo Live Client");

    ps_init();

    screensize(&screen_w, &screen_h);
    if (screen_w == 0) screen_w = 40;
    if (screen_h < 2) screen_h = 24;
    input_y = 1;
    rx_top = 3;
    rx_x = 0;
    rx_y = rx_top;
    gotoxy(0, 2);
    cclearxy(0, 2, screen_w);
    cputs("-------------------------------");

    for (;;) {
        uint8_t b;

        line_len = 0;
        line[0] = '\0';
        render_input_line(line, line_len, input_y, screen_w);

        for (;;) {
            while (ps_read_byte(&b)) {
                remote_putc((char)b, &rx_x, &rx_y, rx_top,
                            (unsigned char)(screen_h - 1), screen_w);
                render_input_line(line, line_len, input_y, screen_w);
            }

            if (kbhit()) {
                char ch = cgetc();
                if (ch == '\r' || ch == '\n') {
                    ps_send_byte((uint8_t)0x9B);
                    ps_flush_tx();
                    line_len = 0;
                    line[0] = '\0';
                    render_input_line(line, line_len, input_y, screen_w);
                    break;
                }
                if (line_len + 1 < sizeof(line)) {
                    line[line_len++] = ch;
                    ps_send_byte((uint8_t)ch);
                    ps_flush_tx();
                    render_input_line(line, line_len, input_y, screen_w);
                }
            }
        }
    }
}
