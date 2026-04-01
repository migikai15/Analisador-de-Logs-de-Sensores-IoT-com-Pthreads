
CC = gcc

CFLAGS = -Wall -O3

LIBS = -lm -lpthread

SRCS = $(wildcard *.c)

PROGS = $(SRCS:.c=)

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)


clean:
	rm -f $(PROGS)

.PHONY: all clean