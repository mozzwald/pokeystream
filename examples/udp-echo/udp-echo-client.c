#include <atari.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pokeystream.h"

#define POKMSK  (*(volatile unsigned char*)0x0010)
#define SSKCTL  (*(volatile unsigned char*)0x0232)
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

static void render_hex_line(const uint8_t* buf,
                            unsigned len,
                            unsigned char y,
                            unsigned char width)
{
    static const char hex[] = "0123456789ABCDEF";
    unsigned i = 0;
    unsigned col = 0;

    cclearxy(0, y, width);
    gotoxy(0, y);
    cputs("RX:");
    col = 3;
    for (i = 0; i < len && (col + 3) < width; ++i) {
        unsigned char b = buf[i];
        gotoxy((unsigned char)col, y);
        cputc(hex[b >> 4]);
        cputc(hex[b & 0x0F]);
        col += 3;
    }
}

static void put_hex_byte(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    cputc(hex[v >> 4]);
    cputc(hex[v & 0x0F]);
}

static void render_status_line(unsigned char y, unsigned char width)
{
    cclearxy(0, y, width);
    gotoxy(0, y);
    cputs("OV:");
    put_hex_byte((unsigned char)(ps_rx_overflow >> 8));
    put_hex_byte((unsigned char)(ps_rx_overflow & 0xFF));
    cputs(" ER:");
    put_hex_byte(ps_last_skstat_error);
}

static void render_top_status(unsigned char width)
{
    cclearxy(0, 0, width);
    gotoxy(0, 0);
    cputs("SK:");
    put_hex_byte((unsigned char)SSKCTL);
    cputs(" PM:");
    put_hex_byte((unsigned char)POKMSK);
    cputs(" RX:");
    put_hex_byte((unsigned char)(ps_rx_bytes >> 8));
    put_hex_byte((unsigned char)(ps_rx_bytes & 0xFF));
    cputs(" TX:");
    put_hex_byte((unsigned char)(ps_tx_bytes >> 8));
    put_hex_byte((unsigned char)(ps_tx_bytes & 0xFF));
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
    uint8_t rx_hex[16];
    unsigned rx_hex_len = 0;

    clrscr();
    screensize(&screen_w, &screen_h);
    if (screen_w == 0) screen_w = 40;
    if (screen_h < 2) screen_h = 24;
    gotoxy(0, 0);
    cputs("POKEYStream UDP Echo Live Client");

    ps_init_stream();

    render_top_status(screen_w);
    input_y = 1;
    rx_top = 4;
    rx_x = 0;
    rx_y = rx_top;
    render_status_line(2, screen_w);
    render_hex_line(rx_hex, rx_hex_len, 3, screen_w);

    for (;;) {
        uint8_t b;

        line_len = 0;
        line[0] = '\0';
        render_input_line(line, line_len, input_y, screen_w);

        for (;;) {
            while (ps_recv_byte(&b)) {
                if (rx_hex_len < sizeof(rx_hex)) {
                    rx_hex[rx_hex_len++] = b;
                } else {
                    unsigned i = 0;
                    for (i = 0; i + 1 < sizeof(rx_hex); ++i) {
                        rx_hex[i] = rx_hex[i + 1];
                    }
                    rx_hex[sizeof(rx_hex) - 1] = b;
                }
                render_hex_line(rx_hex, rx_hex_len, 3, screen_w);
                remote_putc((char)b, &rx_x, &rx_y, rx_top,
                            (unsigned char)(screen_h - 1), screen_w);
                render_input_line(line, line_len, input_y, screen_w);
                render_top_status(screen_w);
                render_status_line(2, screen_w);
            }

            if (kbhit()) {
                char ch = cgetc();
                if (ch == '\r' || ch == '\n') {
                    ps_send_byte((uint8_t)0x9B);
                    //ps_flush_tx();
                    line_len = 0;
                    line[0] = '\0';
                    render_input_line(line, line_len, input_y, screen_w);
                    render_top_status(screen_w);
                    render_status_line(2, screen_w);
                    break;
                }
                if (line_len + 1 < sizeof(line)) {
                    line[line_len++] = ch;
                    ps_send_byte((uint8_t)ch);
                    //ps_flush_tx();
                    render_input_line(line, line_len, input_y, screen_w);
                    render_top_status(screen_w);
                    render_status_line(2, screen_w);
                }
            }
        }
    }
}
