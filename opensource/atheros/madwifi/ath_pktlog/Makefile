
#
# Makefile for the Packet logging module.
#
ifeq ($(obj),)
obj=	.
endif

DEPTH=	..

obj-m		+= ath_pktlog.o
ath_pktlog-objs := pktlog.o
ifeq ($(findstring 2.6,$(KERNELRELEASE)),)
export-obs      := pktlog.o
list-multi	:= ath_pktlog.o
endif

include $(obj)/${DEPTH}/Makefile.inc

INCS+=	-I${HAL} -I${HAL}/${OS} -I${obj}/${ATH_HAL} -I${obj}/${DEPTH} \
	-I${obj}/${ATH} -I${obj}/${ATH_PKTLOG} \
	-I${ATH_RATE}

EXTRA_CFLAGS+= ${INCS} ${COPTS}

-include $(TOPDIR)/Rules.make

all:    	
	$(MAKE) -C $(KERNELPATH) SUBDIRS=$(shell pwd) modules

install: all
	test -d ${DESTDIR}/${MODULEPATH} || mkdir -p ${DESTDIR}/${MODULEPATH}
	$(STRIP) -S ath_pktlog.$(KMODSUF)
	cp ath_pktlog.${KMODSUF} ${DESTDIR}/${MODULEPATH}

clean:
	rm -f *~ *.o *.ko *.mod.c
	rm -f .depend .version .*.o.flags .*.o.d .*.o.cmd .*.ko.cmd


ath_pktlog.o: ${ath_pktlog-objs}
	$(LD) $(LDOPTS) -o ath_pktlog.$(KMODSUF) -r ${ath_pktlog-objs}

