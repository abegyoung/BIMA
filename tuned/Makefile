# Makefile for instrument tuner (accepts sock commands and sets up LO)

CC=gcc
CFLAGS=-I/usr/local/include -c -Wall -O2 -Wno-unused-result -Wno-unused-but-set-variable
LDFLAGS=-L/usr/local/lib -lcaclib 

TARGETS = tunemrid

all: $(TARGETS)

tunemrid: tunemrid.o
	$(CC) $(OBJS) -o tunemrid tunemrid.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(TARGETS) *.o
