# Makefile for spectrometer server (sending to ARO catcher)

CC=gcc
CFLAGS=-I../caclib/lib -I../caclib/include -c -Wall -O2
LDFLAGS=-L../caclib/lib -lcaclib

TARGETS = ibob_control if_control

all: $(TARGETS)

ibob_control: ibob_control.o
	$(CC) $(OBJS) -o ibob_control ibob_control.o $(LDFLAGS)

if_control: if_control.o
	$(CC) $(OBJS) -o if_control if_control.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(TARGETS) *.o
