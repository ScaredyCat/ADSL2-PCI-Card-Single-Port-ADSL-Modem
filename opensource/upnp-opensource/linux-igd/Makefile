CC=$(CROSS_COMPILE_PREFIX)gcc
INCLUDES= -I. -I./upnp
ifeq ($(DEBUG),1)
ULIB=/work/matarj/libupnp-1.2.1/upnp/bin/debug
else
ULIB=.
endif
LIBS= -lpthread -L$(ULIB) -L. -lupnp -lthreadutil -lixml


ifeq ($(DEBUG),1)
OPT = -ggdb
else
OPT = -Os
endif

CFLAGS += -Wall $(OPT) 

ifeq ($(PRODUCT_CONFIG),4)
CFLAGS += -D'_VENDERID="DLINK"' -D'_PRODUCTNAME="DVG-5121SP"' -D'_WEBURL="www.dlink.com"'
else
CFLAGS += -D'_VENDERID="OEM"' -D'_PRODUCTNAME="ADSL2+ Router"' -D'_WEBURL="127.0.0.1"'
endif

APPS = upnpd

all: $(APPS)

#upnpd:  gate.o gateway.o sample_util.o ipcon.o portmap.o pmlist.o
upnpd:  gate.o gateway.o ipcon.o portmap.o pmlist.o
	$(CC)  $(CFLAGS) -Xlinker -rpath-link . gate.o gateway.o ipcon.o portmap.o pmlist.o $(LIBS) -o  $@ 
#	strip -s upnpd
	@echo "make $@ finished on `date`"

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o $(APPS) *.a

install: upnpd
	@install -d /etc/linuxigd
	@install etc/* /etc/linux-igd
	@install upnpd /usr/bin

distclean: clean
