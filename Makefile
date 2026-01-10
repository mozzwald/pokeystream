export CC65_HOME = /usr/share/cc65

CC      = cl65
HOSTCC  = gcc
AS      = ca65
TARGET  = atari

PROGRAM = udp-echo-client
OUT     = examples/udp-echo/$(PROGRAM).xex
SERVER  = examples/udp-echo/udp-echo-server

CFLAGS  = -O -t $(TARGET) --cpu 6502 -Iinclude
ASFLAGS = --cpu 6502

LIB_C   = src/pokeystream.c
LIB_S   = src/pokeystream_isr.s
EX_C    = examples/udp-echo/udp-echo-client.c

OBJ     = src/pokeystream.o src/pokeystream_isr.o examples/udp-echo/udp-echo-client.o

all: $(OUT) $(SERVER)

src/pokeystream.o: $(LIB_C) include/pokeystream.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/pokeystream_isr.o: $(LIB_S)
	$(AS) $(ASFLAGS) -o $@ $<

examples/udp-echo/udp-echo-client.o: $(EX_C) include/pokeystream.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(SERVER): examples/udp-echo/udp-echo-server.c
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -o $@ $<

clean:
	rm -f $(OUT) $(SERVER) src/*.o examples/udp-echo/*.o

.PHONY: all clean
