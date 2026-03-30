# Makefile for stripped down socket server

CC=gcc
CFLAGS=-c -Wall -O2 -Wno-unused-result -Wno-unused-variable
LIBFLAGS=

OBJS = Commands.o \
        ../coreServer/hardware/serial.o \
        ../coreServer/hardware/can.o \
	../coreServer/Server.o \
	../coreServer/Logging.o \
	../coreServer/CmdArr.o \
	../coreServer/Dispatch.o \

all: server

server: $(OBJS)
	$(CC) $(OBJS) -o Server $(LIBFLAGS) -Wno-unused-result
	strip Server

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm -f ../coreServer/*.o *.o ../coreServer/*~ *~ *.core Server
