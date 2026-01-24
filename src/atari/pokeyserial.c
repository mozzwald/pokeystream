#include <stdint.h>

#include "pokeyserial.h"

extern volatile uint8_t ps_serial_rx_rd;
extern volatile uint8_t ps_serial_rx_wr;
extern volatile uint8_t ps_serial_tx_rd;
extern volatile uint8_t ps_serial_tx_wr;
extern volatile uint8_t ps_serial_status_accum;
extern volatile uint8_t ps_serial_rx_buf[256];
extern volatile uint8_t ps_serial_tx_buf[256];

void ps_serial_enable_rx_irq(void);
void ps_serial_kick_tx_irq(void);

static void ps_serial_irq_disable(void) {
    __asm__("sei");
}

static void ps_serial_irq_enable(void) {
    __asm__("cli");
}

uint8_t ps_serial_rx_available(void) {
    uint8_t rd;
    uint8_t wr;

    ps_serial_irq_disable();
    rd = ps_serial_rx_rd;
    wr = ps_serial_rx_wr;
    ps_serial_irq_enable();

    return (uint8_t)(wr - rd);
}

int ps_serial_read_byte(void) {
    uint8_t rd;
    uint8_t wr;
    uint8_t value;

    ps_serial_irq_disable();
    rd = ps_serial_rx_rd;
    wr = ps_serial_rx_wr;
    if (rd == wr) {
        ps_serial_irq_enable();
        return -1;
    }

    value = ps_serial_rx_buf[rd];
    ps_serial_rx_rd = (uint8_t)(rd + 1);
    ps_serial_irq_enable();

    return (int)value;
}

uint8_t ps_serial_read(uint8_t* dst, uint8_t maxlen) {
    uint8_t rd;
    uint8_t wr;
    uint8_t count;
    uint8_t i;

    if (!dst || maxlen == 0) {
        return 0;
    }

    ps_serial_irq_disable();
    rd = ps_serial_rx_rd;
    wr = ps_serial_rx_wr;
    count = (uint8_t)(wr - rd);
    if (count > maxlen) {
        count = maxlen;
    }

    for (i = 0; i < count; ++i) {
        dst[i] = ps_serial_rx_buf[rd];
        rd = (uint8_t)(rd + 1);
    }
    ps_serial_rx_rd = rd;
    ps_serial_irq_enable();

    return count;
}

uint8_t ps_serial_tx_space(void) {
    uint8_t rd;
    uint8_t wr;

    ps_serial_irq_disable();
    rd = ps_serial_tx_rd;
    wr = ps_serial_tx_wr;
    ps_serial_irq_enable();

    return (uint8_t)(rd - wr - 1);
}

int ps_serial_write_byte(uint8_t b) {
    uint8_t rd;
    uint8_t wr;
    uint8_t space;

    ps_serial_irq_disable();
    rd = ps_serial_tx_rd;
    wr = ps_serial_tx_wr;
    space = (uint8_t)(rd - wr - 1);
    if (space == 0) {
        ps_serial_irq_enable();
        return -1;
    }

    ps_serial_tx_buf[wr] = b;
    ps_serial_tx_wr = (uint8_t)(wr + 1);
    ps_serial_irq_enable();

    ps_serial_kick_tx_irq();
    return 0;
}

uint8_t ps_serial_write(const uint8_t* src, uint8_t len) {
    uint8_t rd;
    uint8_t wr;
    uint8_t space;
    uint8_t count;
    uint8_t i;

    if (!src || len == 0) {
        return 0;
    }

    ps_serial_irq_disable();
    rd = ps_serial_tx_rd;
    wr = ps_serial_tx_wr;
    space = (uint8_t)(rd - wr - 1);
    count = len;
    if (count > space) {
        count = space;
    }

    for (i = 0; i < count; ++i) {
        ps_serial_tx_buf[wr] = src[i];
        wr = (uint8_t)(wr + 1);
    }
    ps_serial_tx_wr = wr;
    ps_serial_irq_enable();

    if (count != 0) {
        ps_serial_kick_tx_irq();
    }

    return count;
}

uint8_t ps_serial_get_status(void) {
    uint8_t status;

    ps_serial_irq_disable();
    status = ps_serial_status_accum;
    ps_serial_status_accum = 0;
    ps_serial_irq_enable();

    return status;
}
