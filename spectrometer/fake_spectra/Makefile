# Makefile for watchForRef

CC=gcc
CFLAGS=-I/home/corona/cactus/spectral/include -I/home/corona/cactus/optical/lib -I/home/corona/cactus/optical/include -I/home/corona/cactus/include -I/home/corona/cactus/dbe/include -I/home/corona/cactus/AZA/include -c -Wall -O2
LDFLAGS=-L/home/corona/cactus/lib -L/home/corona/cactus/AZA/lib  -ltracker -lcactus


TARGETS = watchForRef


all: $(TARGETS)

watchForRef: watchForRef.o
	$(CC) $(OBJS) -o watchForRef watchForRef.o -lm $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(TARGETS) *.o
