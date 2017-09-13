EXTRA_CFLAGS+=-I. -DLINUX -DEXPORT_SYMTAB

export-objs     :=	mps_linux.o
obj-$(CONFIG_DANUBE_MPS) += mps_linux.o mps_device.o 
#obj-$(CONFIG_DANUBE_MPS) += mps.o

#mps-objs := mps_linux.o mps_device.o

#mps.o: $(mps-objs)
#	$(LD) -r -o $@ $(mps-objs)

O_TARGET:= mps.o

include $(TOPDIR)/Rules.make
