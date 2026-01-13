#include <conio.h>
#include <stdint.h>
#include <stdio.h>

#include "pokeystream.h"

#define SCREEN_COLS 40
#define SCREEN_ROWS 24

#define COUNTER_Y 0
#define QUEUE_Y 1
#define PROMPT_Y 2
#define PROMPT_LINES 2
#define DIVIDER_Y (PROMPT_Y + PROMPT_LINES)
#define RX_START_Y (DIVIDER_Y + 1)

#define PROMPT_MAX 78

#define SEI() __asm__("sei")
#define CLI() __asm__("cli")

static char input_buf[PROMPT_MAX + 1];
static uint8_t input_len;

static uint8_t rx_x = 0;
static uint8_t rx_y = RX_START_Y;

static uint8_t input_cursor_x = 2;
static uint8_t input_cursor_y = PROMPT_Y;

static void read_counters(uint32_t* rx, uint32_t* tx, uint32_t* ovf, uint8_t* sk)
{
    SEI();
    *rx = ps_rx_count;
    *tx = ps_tx_count;
    *ovf = ps_rx_overflow;
    *sk = ps_last_skstat;
    CLI();
}

static void update_counters(void)
{
    uint32_t rx = 0;
    uint32_t tx = 0;
    uint32_t ovf = 0;
    uint8_t sk = 0;
    volatile uint8_t* irqst_reg = (volatile uint8_t*)0xD20E;
    volatile uint8_t* pokmsk_reg = (volatile uint8_t*)0x0010;
    uint8_t irqst = *irqst_reg;
    uint8_t pokmsk = *pokmsk_reg;
    uint16_t rxq = 0;
    uint16_t txf = 0;

    read_counters(&rx, &tx, &ovf, &sk);
    rxq = ps_rx_available();
    txf = ps_tx_free();

    gotoxy(0, COUNTER_Y);
    printf("RX:%lu TX:%lu OVF:%lu SK:%02X IRQ:%02X PK:%02X",
           (unsigned long)rx, (unsigned long)tx, (unsigned long)ovf,
           (unsigned int)sk, (unsigned int)irqst, (unsigned int)pokmsk);

    gotoxy(0, QUEUE_Y);
    printf("RXQ:%u TXF:%u   ", (unsigned)rxq, (unsigned)txf);
}

static void render_prompt(void)
{
    uint8_t i = 0;
    uint8_t max_first = SCREEN_COLS - 2;

    gotoxy(0, PROMPT_Y);
    printf("> ");
    for (i = 0; i < max_first; ++i) {
        if (i < input_len) {
            putchar(input_buf[i]);
        } else {
            putchar(' ');
        }
    }

    gotoxy(0, PROMPT_Y + 1);
    for (i = 0; i < SCREEN_COLS; ++i) {
        uint8_t idx = (uint8_t)(max_first + i);
        if (idx < input_len) {
            putchar(input_buf[idx]);
        } else {
            putchar(' ');
        }
    }

    if (input_len < max_first) {
        input_cursor_x = (uint8_t)(2 + input_len);
        input_cursor_y = PROMPT_Y;
    } else {
        uint8_t second_pos = (uint8_t)(input_len - max_first);
        if (second_pos >= SCREEN_COLS) {
            second_pos = (uint8_t)(SCREEN_COLS - 1);
        }
        input_cursor_x = second_pos;
        input_cursor_y = (uint8_t)(PROMPT_Y + 1);
    }
}

static void draw_ui(void)
{
    uint8_t i = 0;

    clrscr();
    update_counters();
    render_prompt();

    gotoxy(0, DIVIDER_Y);
    for (i = 0; i < SCREEN_COLS; ++i) {
        putchar('-');
    }

    rx_x = 0;
    rx_y = RX_START_Y;
}

static void rx_put(uint8_t ch)
{
    gotoxy(rx_x, rx_y);
    putchar(ch);

    if (ch == 0x9B) {
        rx_x = 0;
        ++rx_y;
    } else {
        ++rx_x;
        if (rx_x >= SCREEN_COLS) {
            rx_x = 0;
            ++rx_y;
        }
    }

    if (rx_y >= SCREEN_ROWS) {
        draw_ui();
    }
}

static void handle_key(uint8_t ch)
{
    if (ch == 0x12) {
        ps_shutdown();
        ps_init();
        draw_ui();
        return;
    }

    if (ch == 0x9B || ch == 0x0D) {
        if (input_len > 0) {
            if (ps_send((const uint8_t*)input_buf, input_len) >= 0) {
                ps_send_byte(0x9B);
            }
        }
        input_len = 0;
        input_buf[0] = '\0';
        render_prompt();
        return;
    }

    if (ch == 0x7E || ch == 0x08) {
        if (input_len > 0) {
            --input_len;
            input_buf[input_len] = '\0';
            render_prompt();
        }
        return;
    }

    if (ch >= 0x20 && input_len < PROMPT_MAX) {
        input_buf[input_len++] = (char)ch;
        input_buf[input_len] = '\0';
        render_prompt();
    }
}

int main(void)
{
    ps_init();
    draw_ui();

    while (1) {
        uint8_t rx_buf[64];
        size_t got = ps_recv(rx_buf, sizeof(rx_buf));
        size_t i = 0;

        if (got != 0) {
            for (i = 0; i < got; ++i) {
                uint8_t ch = rx_buf[i];
                if (ch == '\n') {
                    ch = 0x9B;
                }
                rx_put(ch);
            }
        }

        if (kbhit()) {
            uint8_t ch = (uint8_t)cgetc();
            handle_key(ch);
        }

        update_counters();
        gotoxy(input_cursor_x, input_cursor_y);
    }

    return 0;
}
