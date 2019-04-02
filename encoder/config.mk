DEBUG = y

CFLAGS += -Wall
CFLAGS += -I$(NETPP)/include -I$(NETPP)/devices
CFLAGS += -DCOMPILE_WITHOUT_BOUNDARY_CHECKS

SWAP = y

ifdef SWAP
CFLAGS += -DSWAPOUT
CFLAGS += -I$(NETPP)/devices
CFLAGS += -I.
OBJS += new/enc_new.o new/remote.o
OBJS += dec_compress_pixel.o dec_wavelet_filterbank.o dec_nhw_decoder.o
endif

LDFLAGS += -L$(NETPP)/devices/libmaster/Debug -lmaster


dec_%.o: ../decoder/%.c
	$(CC) -c -o $@ $(CFLAGS) $<
