CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)

CFLAGS = -g

all: nhwenc

clean:
	rm -f $(OBJS) nhwenc

nhwenc: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng
