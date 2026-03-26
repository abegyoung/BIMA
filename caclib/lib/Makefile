            CC = gcc
 GCCVERSIONGT4 = $(shell expr `gcc -dumpversion | cut -f1 -d.` \> 4)
   DEBUG_FLAGS = -g -Wall
 FEDORA_CFLAGS = -Wformat-overflow=0 -Wno-stringop-truncation -Wno-old-style-definition
 CENTOS_CFLAGS =
        CFLAGS += $(DEBUG_FLAGS) -D_DEFAULT_SOURCE
      INCLUDES = -I../include 
          LIBS = -L. -lcaclib -lm
   FEDORA_LIBS = 
   CENTOS_LIBS =
          SRCS = cacstr.c deg_trig.c socklib.c mbdef.c socknoxvlib.c timeLib.c loglib.c sem.c swapdata.c findversion.c wr_sdd.c
          OBJS = cacstr.o deg_trig.o socklib.o mbdef.o socknoxvlib.o timeLib.o loglib.o sem.o swapdata.o findversion.o wr_sdd.o
        RANLIB = ranlib
            AR = ar cq

all: libcaclib.a sock sockGTKlib.o

ifeq "$(GCCVERSIONGT4)" "1"
  CFLAGS += $(FEDORA_CFLAGS)
    LIBS += $(FEDORA_LIBS)
else
  CFLAGS += $(CENTOS_CFLAGS)
    LIBS += $(CENTOS_LIBS)
endif

sockGTKlib.o: sockGTKlib.c
	gcc $(CFLAGS) -Wno-deprecated-declarations -I../include `pkg-config --cflags gtk+-2.0` -c sockGTKlib.c


sock: sock.o libcaclib.a
	$(CC) $(CFLAGS) -o sock sock.o $(LIBS)

.c.o: 
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c

libcaclib.a: $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
	$(RANLIB) $@
	$(_NULLCMD_)

install:
	mkdir -p /usr/local/lib
	install libcaclib.a sockGTKlib.o /usr/local/lib
#
	mkdir -p /usr/local/bin
	install sock /usr/local/bin
#
	mkdir -p /usr/local/include
	install *.h /usr/local/include

proto:
	echo " " > tmp_proto.h
	echo "#include \"caclib_sock.h\"" >> tmp_proto.h
	echo " " >> tmp_proto.h
	cproto -DPROTO $(INCLUDES) $(SRCS) >> tmp_proto.h
	mv tmp_proto.h caclib_proto.h

clean:
	rm -f core.* *.o *.a sock

realclean:
	make clean

.depends:
	touch .depends

depend::
	touch .depends
	makedepend -f.depends -- $(INCLUDES)  -I/usr/include -I/usr/include/gtk-2.0 $(SRCS) sock.c 

include .depends

