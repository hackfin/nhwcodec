CSRCS = $(wildcard *.c)

LIBPNG ?= /usr/local

OBJS = $(CSRCS:%.c=%.o)
OBJS += png_w.o filters.o wavelet_filterbank.o

CFLAGS = -I../include -I$(LIBPNG)/include
CFLAGS += -Wall -g

all: nhwdec

nhwdec: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng -L$(LIBPNG)/lib -lz

clean:
	rm -f nhwdec $(OBJS)

%.o: ../utils/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o: ../common/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

