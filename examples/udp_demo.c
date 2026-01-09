#include <atari.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pokeystream.h"

static void send_line(const char* line)
{
    char buf[128];
    size_t i = 0;

    /* Convert newline to ATASCII EOL (0x9B). */
    while (line[i] && i + 1 < sizeof(buf)) {
        if (line[i] == '\r') {
            ++i;
            continue;
        }
        if (line[i] == '\n') {
            buf[i] = (char)0x9B;
            ++i;
            break;
        }
        buf[i] = line[i];
        ++i;
    }
    if (i == 0 || buf[i - 1] != (char)0x9B) {
        buf[i++] = (char)0x9B;
    }

    ps_send(buf, (uint16_t)i);
    ps_flush_tx();
}

int main(void)
{
    char line[128];
    char rx_line[128];
    unsigned rx_len = 0;

    clrscr();
    cprintf("POKEYStream UDP Demo\r\n");
    cprintf("Enter a line to send.\r\n\r\n");

    ps_init();

    cprintf("> ");
    fgets(line, sizeof(line), stdin);
    send_line(line);

    for (;;) {
        uint8_t b;
        while (ps_read_byte(&b)) {
            if (b == 0x9B) {
                rx_line[rx_len] = '\0';
                printf("%s\n", rx_line);
                rx_len = 0;
            } else if (rx_len + 1 < sizeof(rx_line)) {
                rx_line[rx_len++] = (char)b;
            }
        }
    }
}
