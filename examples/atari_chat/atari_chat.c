#include <conio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pokeyserial.h"
#include "fujinet-fuji.h"

#define SCREEN_COLS 40
#define SCREEN_ROWS 24

#define COUNTER_Y 0
#define QUEUE_Y 1
#define PROMPT_Y 2
#define PROMPT_LINES 2
#define DIVIDER_Y (PROMPT_Y + PROMPT_LINES)
#define RX_START_Y (DIVIDER_Y + 1)

#define PROMPT_MAX 78

#define HOST_BUF_LEN 64
#define NETSTREAM_PORT 9000
#define NETSTREAM_HOST "localhost"
#define NETSTREAM_TRANSPORT_UDP 1
#define HOST_PREFIX_TCP "tcp:"
#define HOST_PREFIX_UDP "udp:"
#define HOST_PREFIX_LEN 4
#define HOSTNAME_MAX_LEN (HOST_BUF_LEN - HOST_PREFIX_LEN - 2)

#define SEI() __asm__("sei")
#define CLI() __asm__("cli")

static char host_buf[HOST_BUF_LEN];
static bool use_udp = true;

static char input_buf[PROMPT_MAX + 1];
static uint8_t input_len;

static uint8_t rx_x = 0;
static uint8_t rx_y = RX_START_Y;

static uint8_t input_cursor_x = 2;
static uint8_t input_cursor_y = PROMPT_Y;

static uint32_t app_rx_count;
static uint32_t app_tx_count;

static void update_counters(void)
{
    uint32_t rx = 0;
    uint32_t tx = 0;
    uint8_t status = 0;
    volatile uint8_t* irqst_reg = (volatile uint8_t*)0xD20E;
    volatile uint8_t* pokmsk_reg = (volatile uint8_t*)0x0010;
    uint8_t irqst = *irqst_reg;
    uint8_t pokmsk = *pokmsk_reg;
    uint8_t rxq = 0;
    uint8_t txf = 0;

    SEI();
    rx = app_rx_count;
    tx = app_tx_count;
    CLI();

    rxq = ps_serial_rx_available();
    txf = ps_serial_tx_space();
    status = ps_serial_get_status();

    gotoxy(0, COUNTER_Y);
    printf("RX:%lu TX:%lu ST:%02X IRQ:%02X PK:%02X",
           (unsigned long)rx, (unsigned long)tx,
           (unsigned int)status, (unsigned int)irqst, (unsigned int)pokmsk);

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
        ps_serial_shutdown();
        ps_serial_init();
        draw_ui();
        return;
    }

    if (ch == 0x9B || ch == 0x0D) {
        if (input_len > 0) {
            uint8_t sent = ps_serial_write((const uint8_t*)input_buf, input_len);
            if (sent != 0) {
                app_tx_count += sent;
                if (ps_serial_write_byte(0x9B) == 0) {
                    app_tx_count += 1;
                }
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

static uint16_t swap16(uint16_t value)
{
    return (uint16_t)(((uint32_t)value << 8) | ((uint32_t)value >> 8));
}

static void strip_newline(char *str)
{
    size_t len;

    if (!str) {
        return;
    }

    len = strlen(str);
    if (len == 0) {
        return;
    }

    if (str[len - 1] == '\n' || str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }
    len = strlen(str);
    if (len != 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
    }
}

static void trim_spaces(char *str)
{
    size_t len;
    size_t start = 0;
    size_t i = 0;

    if (!str) {
        return;
    }

    len = strlen(str);
    while (start < len && (str[start] == ' ' || str[start] == '\t')) {
        ++start;
    }
    if (start != 0) {
        for (i = 0; i + start <= len; ++i) {
            str[i] = str[i + start];
        }
        len = strlen(str);
    }

    while (len != 0 && (str[len - 1] == ' ' || str[len - 1] == '\t')) {
        str[len - 1] = '\0';
        len--;
    }
}

static uint16_t parse_port(const char *str, uint16_t fallback)
{
    uint16_t value = 0;
    bool any = false;

    if (!str) {
        return fallback;
    }

    while (*str != '\0') {
        if (*str < '0' || *str > '9') {
            break;
        }
        any = true;
        value = (uint16_t)(value * 10u + (uint16_t)(*str - '0'));
        ++str;
    }

    if (!any || value == 0) {
        return fallback;
    }

    return value;
}

static void build_host_buffer(const char *hostname, bool tcp_transport, uint8_t flags)
{
    size_t len;
    const char *prefix = tcp_transport ? HOST_PREFIX_TCP : HOST_PREFIX_UDP;

    memset(host_buf, 0, sizeof(host_buf));
    strncpy(host_buf, prefix, HOST_BUF_LEN - 1);
    strncat(host_buf, hostname, HOSTNAME_MAX_LEN);
    host_buf[HOST_BUF_LEN - 2] = '\0';

    len = strlen(host_buf);
    if (len + 1 < HOST_BUF_LEN)
    {
        host_buf[len + 1] = (char)flags;
    }
}

static void debug_netstream_config(const char *host, uint16_t port,
                                   bool use_udp, uint8_t flags)
{
    clrscr();
    printf("FujiNet NETStream config\n");
    printf("Host buffer: %s\n", host_buf);
    printf("Host: %s\n", host);
    printf("Port: %u\n", (unsigned)port);
    printf("Transport: %s\n", use_udp ? "udp" : "tcp");
    printf("Flags: $%02X (bit0=TCP, bit1=REGISTER)\n", (unsigned)flags);
    printf("\nPress any key to continue...");
    cgetc();
}

static void prompt_netstream_settings(char *host_out, size_t host_len,
                                      uint16_t *port_out)
{
    char input[HOST_BUF_LEN];
    char transport[16];
    uint16_t port = NETSTREAM_PORT;
    use_udp = true;

    printf("Host [%s]: ", NETSTREAM_HOST);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        strip_newline(input);
        trim_spaces(input);
        if (input[0] != '\0') {
            strncpy(host_out, input, host_len - 1);
            host_out[host_len - 1] = '\0';
        } else {
            strncpy(host_out, NETSTREAM_HOST, host_len - 1);
            host_out[host_len - 1] = '\0';
        }
    } else {
        strncpy(host_out, NETSTREAM_HOST, host_len - 1);
        host_out[host_len - 1] = '\0';
    }

    printf("Port [%u]: ", (unsigned)NETSTREAM_PORT);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        strip_newline(input);
        trim_spaces(input);
        port = parse_port(input, NETSTREAM_PORT);
    }

    printf("Transport (udp/tcp) [udp]: ");
    if (fgets(transport, sizeof(transport), stdin) != NULL) {
        strip_newline(transport);
        trim_spaces(transport);
        if (transport[0] == 't' || transport[0] == 'T') {
            use_udp = false;
        } else if (transport[0] == 'u' || transport[0] == 'U' || transport[0] == '\0') {
            use_udp = true;
        }
    }

    *port_out = port;
}

int main(void)
{
    bool ok;
    uint8_t flags = 0;
    uint16_t port = NETSTREAM_PORT;
    char host[HOST_BUF_LEN];

    clrscr();
    prompt_netstream_settings(host, sizeof(host), &port);
    draw_ui();

    /* Enable FujiNet NETStream */
    if (!use_udp) {
        flags |= (1u << 0);
    }
    flags |= (1u << 1); /* REGISTER enabled */
    build_host_buffer(host, !use_udp, flags);
    //debug_netstream_config(host, port, use_udp, flags);
    ok = fuji_enable_udpstream(swap16(port), host_buf);
    (void)ok;

    ps_serial_install_cio();
    ps_serial_init();

    while (1) {
        uint8_t rx_buf[64];
        uint8_t got = ps_serial_read(rx_buf, sizeof(rx_buf));
        uint8_t i = 0;

        if (got != 0) {
            app_rx_count += got;
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
