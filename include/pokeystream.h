#ifndef POKEYSTREAM_H
#define POKEYSTREAM_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize streaming IRQ handler and POKEY serial. */
int ps_init_stream(void);

/* Restore IRQ handler and hardware state. */
void ps_shutdown_stream(void);

/* Queue a byte for transmit; return false if TX ring is full. */
bool ps_send_byte(uint8_t b);

/* Queue up to len bytes; returns number queued. */
uint16_t ps_send(const void* data, uint16_t len);

/* Return the number of bytes available in the RX ring. */
uint8_t ps_rx_available(void);

/* Return the number of free slots in the TX ring. */
uint8_t ps_tx_free(void);

/* Read a byte from RX ring; return false if empty. */
bool ps_recv_byte(uint8_t* out);

/* Read up to maxlen bytes into dst; returns number read. */
uint16_t ps_recv(void* dst, uint16_t maxlen);

/* Debug counters (optional for demos). */
extern volatile uint16_t ps_rx_bytes;
extern volatile uint16_t ps_tx_bytes;
extern volatile uint16_t ps_rx_overflow;
extern volatile uint8_t ps_last_skstat_error;

/* Compatibility aliases (optional). */
int ps_init(void);
void ps_shutdown(void);
bool ps_try_send_byte(uint8_t b);
bool ps_read_byte(uint8_t* out);
uint16_t ps_read(void* dst, uint16_t maxlen);
void ps_flush_tx(void);

#endif
