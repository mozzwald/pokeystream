# POKEYStream

POKEYStream is a small Atari-side library that implements 850-style concurrent
serial IRQ handling for raw FujiNet stream mode. It does **not** issue any SIO
commands; it only configures POKEY, installs IRQ vectors, and provides RX/TX
rings.

## Build the demos

From the `pokeystream/` directory:

```
make
```

Outputs:
- `examples/atari_chat/atari_chat.xex`
- `examples/linux_chat/linux_chat`

## API summary

```
void ps_init(void);
void ps_shutdown(void);
int  ps_send_byte(uint8_t b);
int  ps_send(const uint8_t* buf, size_t n);
int  ps_recv_byte(uint8_t* out);
size_t ps_recv(uint8_t* buf, size_t maxn);
uint16_t ps_rx_available(void);
uint16_t ps_tx_free(void);
```

Debug counters:

```
volatile uint32_t ps_rx_count;
volatile uint32_t ps_tx_count;
volatile uint32_t ps_rx_overflow;
volatile uint8_t  ps_last_skstat;
```

## Notes

- POKEY is hardcoded to 19200 baud with AUDF/AUDC/AUDCTL bytes:
  `{0x28, 0xA0, 0x00, 0xA0, 0x28, 0xA0, 0x00, 0xA0, 0x78}`.
- SKCTL/SSKCTL routing follows the Altirra 850 handler pattern:
  `(SSKCTL & 0x07) | 0x70`.
- IRQ masks for serial input/output are based on `refs/Altirra-850-handler.asm`.
