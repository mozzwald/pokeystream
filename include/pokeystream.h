#ifndef POKEYSTREAM_H
#define POKEYSTREAM_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize POKEYStream and hook OS serial vectors. */
int ps_init(void);

/* Restore hardware state and vectors. */
void ps_shutdown(void);

/* Queue a byte for transmit; return false if TX ring is full. */
bool ps_try_send_byte(uint8_t b);

/* Queue a byte for transmit; may spin if TX ring is full. */
void ps_send_byte(uint8_t b);

/* Queue up to len bytes; returns number queued. */
uint16_t ps_send(const void* data, uint16_t len);

/* Return the number of bytes available in the RX ring. */
uint8_t ps_rx_available(void);

/* Read a byte from RX ring; return false if empty. */
bool ps_read_byte(uint8_t* out);

/* Read up to maxlen bytes into dst; returns number read. */
uint16_t ps_read(void* dst, uint16_t maxlen);

/* Block until TX ring is empty. */
void ps_flush_tx(void);

#endif
