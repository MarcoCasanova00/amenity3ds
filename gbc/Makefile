# Makefile — amenclk.gb (Game Boy clock-out ROM)
GBDK  := gbdk
LCC   := $(GBDK)/bin/lcc

all: amenclk.gb

amenclk.gb: main.c
	$(LCC) -o $@ main.c
	@ls -la $@

clean:
	rm -f *.gb *.ihx *.o *.map

.PHONY: all clean
