DEBUG = y

CFLAGS += -I$(NETPP)/include -I$(NETPP)/devices
CFLAGS += -DCOMPILE_WITHOUT_BOUNDARY_CHECKS

SWAP = y

ifdef SWAP
CFLAGS += -DSWAPOUT
CFLAGS += -I$(NETPP)/devices
CFLAGS += -I.
OBJS += new/enc_new.o new/remote.o
endif

LDFLAGS += -L$(NETPP)/devices/libmaster/Debug -lmaster

