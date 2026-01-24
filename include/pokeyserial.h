#ifndef POKEYSERIAL_H
#define POKEYSERIAL_H

#include <stdint.h>

/*
 * POKEYStream raw SIO serial stream API.
 *
 * This module extracts the core "raw serial over SIO" logic from the MIDI Mate
 * handler: POKEY init, ring buffers, and RX/TX ISR behavior, without CIO/HATABS.
 *
 * Call order: ps_serial_init() -> read/write -> ps_serial_shutdown().
 *
 * Intended for raw FujiNet NETStream-style usage where you want a clean serial
 * byte stream and control of buffering outside CIO.
 */

void ps_serial_init(void);
void ps_serial_shutdown(void);

void ps_serial_install_cio(void);
void ps_serial_remove_cio(void);

uint8_t ps_serial_rx_available(void);
int ps_serial_read_byte(void);
uint8_t ps_serial_read(uint8_t* dst, uint8_t maxlen);

uint8_t ps_serial_tx_space(void);
int ps_serial_write_byte(uint8_t b);
uint8_t ps_serial_write(const uint8_t* src, uint8_t len);

uint8_t ps_serial_get_status(void);

#endif
