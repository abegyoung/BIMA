# Makefile for spectrometer server (sending to ARO catcher)

CC=gcc
CFLAGS=-I../caclib/lib -I../caclib/include -c -Wall -O2
LDFLAGS=-L../caclib/lib -lcaclib

OBJS = ibob_control.o

server: $(OBJS)
	$(CC) $(OBJS) -o ibob_control $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f ibob_control.o ibob_control
