CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)
OBJS += png_r.o png_w.o filters.o

# Create this for local config:
-include config.mk

CFLAGS  += -I../include -Wall
LDFLAGS += -lm -lpng 
ifdef DEBUG
	CFLAGS  += -g
else
	CFLAGS  += -O3
endif

all: nhwenc

clean:
	rm -f $(OBJS) nhwenc

%.o: ../utils/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o: ../common/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

nhw_encoder.o: $(wildcard inline/*.c)

nhwenc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
