# Atari Chat Demo

Build from repo root:

```
make
```

Output:
- `examples/atari_chat/atari_chat.xex`

Run notes:
- This demo assumes FujiNet is already in raw stream mode at 19200 baud.
- The app uses 850-style concurrent IRQ handling without any SIO commands.

Controls:
- Type a line and press Enter to send.
- Ctrl+R reinitializes the stream (ps_shutdown + ps_init).
