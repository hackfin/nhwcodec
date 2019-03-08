CSRCS = $(wildcard *.c)

OBJS = $(CSRCS:%.c=%.o)

CFLAGS = -O3

all: nhwdec

nhwdec: $(OBJS)
	$(CC) -o $@ $^ -lm -lpng
