DEBUG = y

include $(NETPP)/xml/prophandler.mk

CFLAGS += -Wall
CFLAGS += -I$(NETPP)/include -I$(NETPP)/devices
CFLAGS += -DCOMPILE_WITHOUT_BOUNDARY_CHECKS
ifdef SIMPLE
CFLAGS += -DSIMPLIFIED
endif

SWAP = y

ifdef SWAP
CFLAGS += -DSWAPOUT -DUSE_OPCODES
CFLAGS += -I$(NETPP)/devices
CFLAGS += -I.
OBJS += new/enc_new.o new/remote.o new/processing.o new/quantize.o
OBJS += dec_compress_pixel.o dec_wavelet_filterbank.o dec_nhw_decoder.o
endif

LDFLAGS += -L$(NETPP)/devices/libmaster/Debug -lmaster

new/opcodes_register.h new/opcodes_register_modes.h: new/opcodes.xml

DUTIES = new/opcodes_register.h new/opcodes_register_modes.h

dec_%.o: ../decoder/%.c
	$(CC) -c -o $@ $(CFLAGS) $<
