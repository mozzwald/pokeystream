# Linux Chat Demo

Build from repo root:

```
make
```

Run:

```
./examples/linux_chat/linux_chat --port 9001 --peer 127.0.0.1:9000
```

Notes:
- Converts Atari EOL (0x9B) to '\n' on receive.
- Sends '\n' as 0x9B to match Atari behavior.
