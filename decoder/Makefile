CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)
OBJS += png_w.o filters.o

CFLAGS = -I../include
CFLAGS += -Wall -g

all: nhwdec

nhwdec: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng

clean:
	rm -f nhwdec $(OBJS)

%.o: ../utils/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o: ../common/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

