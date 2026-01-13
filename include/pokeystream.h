#ifndef POKEYSTREAM_H
#define POKEYSTREAM_H

#include <stddef.h>
#include <stdint.h>

void ps_init(void);
void ps_shutdown(void);

int ps_send_byte(uint8_t b);
int ps_send(const uint8_t* buf, size_t n);

int ps_recv_byte(uint8_t* out);
size_t ps_recv(uint8_t* buf, size_t maxn);

uint16_t ps_rx_available(void);
uint16_t ps_tx_free(void);

extern volatile uint32_t ps_rx_count;
extern volatile uint32_t ps_tx_count;
extern volatile uint32_t ps_rx_overflow;
extern volatile uint8_t ps_last_skstat;

#endif
