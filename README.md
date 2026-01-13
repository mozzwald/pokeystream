# POKEYStream

POKEYStream is a small Atari-only library for raw POKEY serial streaming that
targets FujiNet UDP Stream / MIDIMaze-style communication. It configures POKEY
with the MIDI Maze timing and installs a VIMIRQ wrapper for IRQ-driven RX/TX.

## Build the demo

From the `pokeystream/` directory:

```
make
```

The output is `pokeystream/examples/udp-echo/udp-echo-client.xex`.

## API summary

```
int ps_init_stream(void);
void ps_shutdown_stream(void);
bool ps_send_byte(uint8_t b);
uint16_t ps_send(const void* data, uint16_t len);
uint8_t ps_rx_available(void);
uint8_t ps_tx_free(void);
bool ps_recv_byte(uint8_t* out);
uint16_t ps_recv(void* dst, uint16_t maxlen);
```

## Notes

- RX is interrupt-driven via a VIMIRQ wrapper and a 256-byte ring buffer.
- TX is queued into a 256-byte ring; TX IRQ is enabled only when data is pending.
- POKEY init uses AUDF3=$15, AUDF4=$00, AUDCTL=$28, and SKCTL/SSKCTL=$13.
