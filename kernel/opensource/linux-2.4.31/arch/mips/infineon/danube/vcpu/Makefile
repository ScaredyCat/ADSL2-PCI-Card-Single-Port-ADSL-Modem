
EXTRA_CFLAGS+=-I. -I./common/src -DLINUX -DOAK_FIFO_BLOCKING_WRITE=0 -DEXPORT_SYMTAB

O_TARGET := drv_danube_vcpu.o

obj-$(CONFIG_DANUBE_VCPU) += drv_DANUBE_VCPU_Linux.o DANUBE_VCPU_DeviceDriver.o 

list-multi := danube_vcpu.o

include $(TOPDIR)/Rules.make
