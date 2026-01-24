export CC65_HOME = /usr/share/cc65

CC      = cl65
HOSTCC  = gcc
AS      = ca65
TARGET  = atari

BUILD_DIR     = build
ATARI_PROGRAM = atari_chat
ATARI_OUT     = $(BUILD_DIR)/$(ATARI_PROGRAM).xex
LINUX_PROGRAM = $(BUILD_DIR)/linux_chat

# FujiNet Library
FUJINET_LIB_VERSION = 4.9.0
FUJINET_LIB_DIR = fujinet-lib
FUJINET_LIB = $(FUJINET_LIB_DIR)/fujinet-atari-$(FUJINET_LIB_VERSION).lib
FUJINET_INCLUDES = -I$(FUJINET_LIB_DIR)

CFLAGS  = -O -t $(TARGET) --cpu 6502 -Iinclude $(FUJINET_INCLUDES)
ASFLAGS = --cpu 6502 -Isrc/atari

LIB_S   = src/atari/pokeystream_atari.s src/atari/pokeystream_isr.s src/atari/pokeystream_buffers.s src/atari/pokeyserial_core.s src/atari/pokeyserial_cio.s
EX_C    = examples/atari_chat/atari_chat.c

OBJ     = src/atari/pokeystream_atari.o src/atari/pokeystream_isr.o src/atari/pokeystream_buffers.o src/atari/pokeyserial_core.o src/atari/pokeyserial_cio.o src/atari/pokeyserial.o examples/atari_chat/atari_chat.o

all: $(ATARI_OUT) $(LINUX_PROGRAM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

src/atari/pokeystream_atari.o: src/atari/pokeystream_atari.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeystream_isr.o: src/atari/pokeystream_isr.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeystream_buffers.o: src/atari/pokeystream_buffers.s
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeyserial_core.o: src/atari/pokeyserial_core.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeyserial_cio.o: src/atari/pokeyserial_cio.s src/atari/pokey_regs.inc
	$(AS) $(ASFLAGS) -o $@ $<

src/atari/pokeyserial.o: src/atari/pokeyserial.c include/pokeyserial.h
	$(CC) $(CFLAGS) -c -o $@ $<

examples/atari_chat/atari_chat.o: $(EX_C) include/pokeyserial.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(ATARI_OUT): $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(FUJINET_LIB)

$(LINUX_PROGRAM): examples/linux_chat/linux_chat.c | $(BUILD_DIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -o $@ $<

clean:
	rm -f $(BUILD_DIR)/* src/atari/*.o examples/atari_chat/*.o

.PHONY: all clean
