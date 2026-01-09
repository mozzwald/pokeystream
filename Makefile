export CC65_HOME = /usr/share/cc65

CC      = cl65
AS      = ca65
TARGET  = atari

PROGRAM = udp_demo
OUT     = examples/$(PROGRAM).xex

CFLAGS  = -O -t $(TARGET) --cpu 6502 -Iinclude
ASFLAGS = --cpu 6502

LIB_C   = src/pokeystream.c
LIB_S   = src/pokeystream_isr.s
EX_C    = examples/udp_demo.c

OBJ     = src/pokeystream.o src/pokeystream_isr.o examples/udp_demo.o

all: $(OUT)

src/pokeystream.o: $(LIB_C) include/pokeystream.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/pokeystream_isr.o: $(LIB_S)
	$(AS) $(ASFLAGS) -o $@ $<

examples/udp_demo.o: $(EX_C) include/pokeystream.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OUT) src/*.o examples/*.o

.PHONY: all clean
