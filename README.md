# POKEYStream

NOTE: This is a work in progress and currently has issues

POKEYStream is a small Atari-only library for raw POKEY serial streaming that
targets FujiNet UDP Stream / MIDIMaze-style communication. It configures POKEY
with the MIDI Maze timing and hooks the OS serial vectors for IRQ-driven RX/TX.

## Build the demo

From the `pokeystream/` directory:

```
make
```

The output is `pokeystream/examples/udp_demo.xex`.

## API summary

```
int ps_init(void);
void ps_shutdown(void);
bool ps_try_send_byte(uint8_t b);
void ps_send_byte(uint8_t b);
uint16_t ps_send(const void* data, uint16_t len);
uint8_t ps_rx_available(void);
bool ps_read_byte(uint8_t* out);
uint16_t ps_read(void* dst, uint16_t maxlen);
void ps_flush_tx(void);
```

## Notes

- RX is interrupt-driven via VSERIN and a 256-byte ring buffer.
- TX is queued into a 256-byte ring and flushed with `ps_flush_tx()`.
- POKEY init uses AUDF3=$15, AUDF4=$00, AUDCTL=$39, and the SKCTL/SSKCTL
  pattern observed in the MIDI Maze ROM disassembly.
