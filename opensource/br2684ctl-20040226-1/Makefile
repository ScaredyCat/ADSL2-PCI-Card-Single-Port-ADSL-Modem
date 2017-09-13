#CROSS=mips-linux-
#CROSS=
#CC=$(CROSS)gcc

#CROSSCFLAGS=-mips2 -mtune=4kc
#CROSSCFLAGS=
#CFLAGS=-Os $(CROSSCFLAGS) -Wall
CFLAGS=-Wall $(IFX_CFLAGS)
LDFLAGS+= $(IFX_LDFLAGS)

LIBATM=atm
#LIBATMPATH=../../common/lib
LIBATMPATH=../../ifx/IFXAPIs/lib/
LIBIFXCOMMON=IFXAPIs
IFXCOMMONPATH=../../ifx/IFXAPIs/
#INCPATH=-I../../../kernel/opensource/linux_2_4_20_mvl_ifx/include -I$(IFXCOMMONPATH)include
INCPATH=-I../../../kernel/opensource/linux-2.4.31/include

all:br2684ctl br2684ctld
	
#	$(CC) $(INCPATH) $(CFLAGS) -o $@  br2684ctl.c -l$(LIBATM) -L$(LIBATMPATH) -l$(LIBIFXCOMMON) -L$(IFXCOMMONPATH)lib
br2684ctl:br2684ctl.c
	$(CC) $(INCPATH) $(CFLAGS) -DIFX_BR2684CLIENT $(LDFLAGS) -o $@  br2684ctl.c -l$(LIBATM) -l$(LIBIFXCOMMON) 

br2684ctld:br2684ctl.c
	rm -f br2684ctl.o
	$(CC) $(INCPATH) $(CFLAGS) -DIFX_BR2684DEAMON $(LDFLAGS) -o $@  br2684ctl.c -l$(LIBATM) -l$(LIBIFXCOMMON) 

clean:
	-rm -f br2684ctl
	-rm -f br2684ctld

