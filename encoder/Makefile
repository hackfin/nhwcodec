CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)

CFLAGS = -O3

all: nhwenc

nhwenc: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng
