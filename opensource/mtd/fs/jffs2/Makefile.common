#
# Makefile for the linux Journalling Flash FileSystem (JFFS) routines.
#
# $Id: Makefile.common,v 1.1 2003/05/27 09:34:41 dwmw2 Exp $
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...


obj-$(CONFIG_JFFS2_FS) := jffs2.o

COMPR_OBJS	:= compr.o compr_rubin.o compr_rtime.o compr_zlib.o
JFFS2_OBJS	:= dir.o file.o ioctl.o nodelist.o malloc.o \
	read.o nodemgmt.o readinode.o write.o scan.o gc.o \
	symlink.o build.o erase.o background.o fs.o writev.o

BELOW25		:= $(shell echo $(PATCHLEVEL) | sed s/[1234]/y/)

ifeq ($(BELOW25),y)
LINUX_OBJS	:= super-v24.o crc32.o rbtree.o
else
LINUX_OBJS	:= super.o
endif

NAND_OBJS-$(CONFIG_JFFS2_FS_NAND)	:= wbuf.o

jffs2-objs := $(COMPR_OBJS) $(JFFS2_OBJS) $(VERS_OBJS) $(NAND_OBJS-y) \
	$(LINUX_OBJS)


# 2.4 build compatibility
ifeq ($(BELOW25),y)
obj-y := $(jffs2-objs)
O_TARGET := jffs2.o
include $(TOPDIR)/Rules.make
endif

