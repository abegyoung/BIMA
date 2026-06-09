# Makefile for spectrometer server (sending to ARO catcher)

CC=gcc
CFLAGS=-I/usr/local/include -c -Wall -O2 -Wno-unused-result -Wno-unused-but-set-variable
LDFLAGS=-L/usr/local/lib -lcaclib 

TARGETS = ibob_control if_control vaned

all: $(TARGETS)

ibob_control: ibob_control.o
	$(CC) $(OBJS) -o ibob_control ibob_control.o $(LDFLAGS)

if_control: if_control.o
	$(CC) $(OBJS) -o if_control if_control.o $(LDFLAGS)

vaned: vaned.o
	$(CC) $(OBJS) -o vaned vaned.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(TARGETS) *.o
