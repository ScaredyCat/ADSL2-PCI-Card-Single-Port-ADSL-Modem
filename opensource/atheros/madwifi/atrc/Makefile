#
# Makefile for tracing module
#

ifeq ($(obj),)
obj=	.
endif

DEPTH=	..

# release tag versioning
-include $(KERNELPATH)/ath_version.mk
export EXTRAVERSION

obj-m		+= atrcm.o
atrcm-objs	:= atrc.o
ifeq ($(findstring 2.6,$(KERNELRELEASE)),)
export-objs	:= atrc.o
list-multi	:= atrcm.o
endif

include $(obj)/${DEPTH}/Makefile.inc

ifneq ($(MAKING_MODULES),1)
obj-$(CONFIG_ATRC_ATHEROS) := $(atrcm-objs)
ifeq ($(findstring 2.6,$(KERNELRELEASE)),)
O_TARGET := atrcm.o
endif
endif

INCS+=	-I${HAL} -I${HAL}/${OS} -I${obj}/${ATH} -I${obj}/${WLAN} \
	-I${obj}/${DEPTH}

EXTRA_CFLAGS+= ${INCS} ${COPTS}

-include $(TOPDIR)/Rules.make

all:
	$(MAKE) -C $(KERNELPATH) SUBDIRS=$(shell pwd) modules

install: all
	test -d ${DESTDIR}/${MODULEPATH} || mkdir -p ${DESTDIR}/${MODULEPATH}
	${TOOLPREFIX}strip -S atrcm.$(KMODSUF)
	cp atrcm.${KMODSUF} ${DESTDIR}/${MODULEPATH}

clean:
	-rm -f *~ *.o *.ko *.mod.c
	-rm -f .depend .version .*.o.flags .*.o.d .*.o.cmd .*.ko.cmd

atrcm.o: ${atrcm-objs}
	$(LD) $(LDOPTS) -o atrcm.$(KMODSUF) -r $(atrcm-objs)
