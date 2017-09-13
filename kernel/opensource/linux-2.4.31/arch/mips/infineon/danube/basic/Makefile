#
# Makefile for the DANUBE specific kernel interface routines under Linux.
#

all: danube.a

O_TARGET	:= danube.a

obj-y	 := setup.o prom.o interrupt.o danubeIRQ.o reset.o kgdb_serial.o ikos.o dma-core.o

obj-$(CONFIG_PCI) += pci.o

export-objs     := setup.o interrupt.o

EXTRA_AFLAGS := $(CFLAGS)

ifeq ($(CONFIG_NFS_FS),y)
EXTRA_AFLAGS +=-DIFX_NFS_PARA
endif

danubeIRQ.o: danubeIRQ.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c -o $@ $<

piggy.gz: $(TOPDIR)/vmlinux
	$(OBJCOPY) -O binary -R .note -R .comment -S $(TOPDIR)/vmlinux piggy
	gzip -9 < piggy > piggy.gz

uImage \
pImage: piggy.gz
	mkimage -A mips -O linux -T kernel -C gzip -a 0x80002000 -e \
	0x$(shell grep kernel_entry $(TOPDIR)/System.map | awk '{ printf "%s", $$1 }') \
	-n 'MIPS Linux-$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)' \
	-d piggy.gz $@

clean:
	rm -f uImage pImage piggy piggy.gz *.o *.a

include $(TOPDIR)/Rules.make
