CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)
OBJS += png_r.o

CFLAGS = -g -Wall -I../include

all: nhwenc

clean:
	rm -f $(OBJS) nhwenc

%.o: ../utils/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

nhwenc: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng
