INCLUDES= -I./inc  -I./common 

ifdef TARGET
DIR_PATH=$(TARGET)/
CROSS=$(TARGET)-

CC=$(CROSS)gcc
AR=$(CROSS)ar
LD=$(CROSS)ld
STRIP=$(CROSS)strip
else
CC = gcc
AR = ar
LD = ld
STRIP = strip
endif

DEBUG_OPTION =
OPT = -O2
LIBS= -lpthread libupnp.a libthreadutil.a libixml.a

CFLAGS += -Wall -static $(OPT)

APPS = upnp_igd_ctrlpt

all: $(APPS)

upnp_igd_ctrlpt: upnp_igd_ctrlpt.o ./common/sample_util.o upnp_igd_ctrlpt_main.o signalpipe.o $(LIBS)
	$(CC)  $(CFLAGS) upnp_igd_ctrlpt.o sample_util.o  upnp_igd_ctrlpt_main.o  signalpipe.o -o  $@ $(LIBS) 
	$(STRIP) -s upnp_igd_ctrlpt
	@echo "make $@ finished on `date`"

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o *.a $(APPS)
	
