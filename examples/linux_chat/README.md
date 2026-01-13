# Linux Chat Demo

Build from repo root:

```
make
```

Run:

```
./examples/linux_chat/linux_chat --port 9001
```

Notes:
- Converts Atari EOL (0x9B) to '\n' on receive.
- Sends '\n' as 0x9B to match Atari behavior.
- Peer is set automatically on the first received packet.
