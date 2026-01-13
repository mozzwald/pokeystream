export CC65_HOME = /usr/share/cc65

CC      = cl65
HOSTCC  = gcc
AS      = ca65
TARGET  = atari

BUILD_DIR     = build
ATARI_PROGRAM = atari_chat
ATARI_OUT     = $(BUILD_DIR)/$(ATARI_PROGRAM).xex
LINUX_PROGRAM = $(BUILD_DIR)/linux_chat

CFLAGS  = -O -t $(TARGET) --cpu 6502 -Iinclude
ASFLAGS = --cpu 6502 -Isrc/atari

LIB_S   = src/atari/pokeystream_atari.s src/atari/pokeystream_isr.s src/atari/pokeystream_buffers.s
EX_C    = examples/atari_chat/atari_chat.c

OBJ     = src/atari/pokeystream_atari.o src/atari/pokeystream_isr.o src/atari/pokeystream_buffers.o examples/atari_chat/atari_chat.o

all: $(ATARI_OUT) $(LINUX_PROGRAM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

src/atari/pokeystream_atari.o: src/atari/pokeystream_atari.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeystream_isr.o: src/atari/pokeystream_isr.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeystream_buffers.o: src/atari/pokeystream_buffers.s
	$(AS) $(ASFLAGS) -o $@ $<

examples/atari_chat/atari_chat.o: $(EX_C) include/pokeystream.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(ATARI_OUT): $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(LINUX_PROGRAM): examples/linux_chat/linux_chat.c | $(BUILD_DIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -o $@ $<

clean:
	rm -f $(BUILD_DIR)/* src/atari/*.o examples/atari_chat/*.o

.PHONY: all clean
