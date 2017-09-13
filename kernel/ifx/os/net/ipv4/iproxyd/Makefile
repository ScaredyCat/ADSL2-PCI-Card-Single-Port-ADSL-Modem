#
# Makefile for the Linux TCP/IP (INET) layer.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

O_TARGET := iproxyd.o
obj-m    := $(O_TARGET)

#CONFIG_FULL_PACKAGE=y
ifeq ($(CONFIG_FULL_PACKAGE),y)
obj-y 	 := mMain.o mUtil.o

include $(TOPDIR)/Rules.make

else

include $(TOPDIR)/Rules.make

iproxyd.o: iproxyd.oo
	cp iproxyd.oo iproxyd.o
endif
