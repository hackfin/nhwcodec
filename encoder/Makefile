CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)
OBJS += png_r.o png_w.o filters.o

OBJS += new/enc_new.o new/processing.o new/quantize.o
OBJS += dec_compress_pixel.o dec_wavelet_filterbank.o dec_nhw_decoder.o

all: nhwenc

# Create this for local config:
-include config.mk

CFLAGS  += -I../include -Wall -I.

SWAP = y

CFLAGS += -DUSE_OPCODES
# When defined, swap out some legacy routines
ifdef SWAP
CFLAGS += -DSWAPOUT
endif



LDFLAGS += -lm -lpng 
ifdef DEBUG
	CFLAGS  += -g
else
	CFLAGS  += -O3
endif



REGISTER_OUTPUT_OPTIONS = --param convertBitfields 1

DUTIES += $(OBJS)

clean::
	rm -f $(DUTIES) nhwenc

%.o: ../utils/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o: ../common/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

# Decoder items:
dec_%.o: ../decoder/%.c
	$(CC) -c -o $@ $(CFLAGS) $<


nhw_encoder.o: $(wildcard inline/*.c)

nhwenc: $(DUTIES)
	$(CC) -o $@ $^ $(LDFLAGS)
