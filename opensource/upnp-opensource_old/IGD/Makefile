#CC=gcc
#INCLUDES= -I/usr/include/upnp 
#LIBS= -lpthread -lupnp -lixml -lthreadutil

INCLUDES= -I../libupnp-1.2.1/upnp/inc -I/inc
ifeq ($(DEBUG),1)
ULIB=/work/matarj/libupnp-1.2.1/upnp/bin/debug
else
ULIB=../libupnp-1.2.1/upnp/bin/mips-linux
endif
LIBS= -lpthread -L$(ULIB) -L. -lupnp -lthreadutil -lixml

ifeq ($(DEBUG),1)
OPT = -ggdb
else
OPT = -Os
endif
CROSSCFLAGS=-mips32 -mtune=4kc
CFLAGS += -Wall $(OPT) $(CROSSCFLAGS)
#CFLAGS += -Wall  $(OPT)

all: upnpd

upnpd: main.o gatedevice.o sample_util.o pmlist.o util.o
	$(CC)  $(CFLAGS) -Xlinker -rpath-link . main.o gatedevice.o sample_util.o pmlist.o util.o $(LIBS) -o  $@ 
	@echo "make $@ finished on `date`"

%.o:	%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o upnpd *.a

install: upnpd
#	@install -d /etc/linuxigd
#	@install etc/* /etc/linuxigd
#	@install upnpd /usr/sbin

