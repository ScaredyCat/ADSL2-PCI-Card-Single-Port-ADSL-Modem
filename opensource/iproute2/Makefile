# Path to parent kernel include files directory
KERNEL_INCLUDE=/usr/src/linux/include
LIBC_INCLUDE=/usr/include

DEFINES= -DRESOLVE_HOSTNAMES

#options if you have a bind>=4.9.4 libresolv (or, maybe, glibc)
LDLIBS=-lresolv
ADDLIB=

#options if you compile with libc5, and without a bind>=4.9.4 libresolv
#LDLIBS=
#ADDLIB=inet_ntop.o inet_pton.o

#options for decnet
ADDLIB+=dnet_ntop.o dnet_pton.o

#options for ipx
ADDLIB+=ipx_ntop.o ipx_pton.o

ifeq ($(LIBC_INCLUDE)/socketbits.h,$(wildcard $(LIBC_INCLUDE)/socketbits.h))
  ifeq ($(LIBC_INCLUDE)/net/if_packet.h,$(wildcard $(LIBC_INCLUDE)/net/if_packet.h))
    GLIBCFIX=-I../include-glibc -include ../include-glibc/glibc-bugs.h
  endif
endif
ifeq ($(LIBC_INCLUDE)/bits/socket.h,$(wildcard $(LIBC_INCLUDE)/bits/socket.h))
  GLIBCFIX=-I../include-glibc -include ../include-glibc/glibc-bugs.h
endif

CC = mips-linux-gcc
CCOPTS = -D_GNU_SOURCE -Wstrict-prototypes -Wall $(RPM_OPT_FLAGS) -g
CFLAGS = $(CCOPTS) -I../include $(GLIBCFIX) $(DEFINES) $(IFX_CFLAGS)
RANLIB=mips-linux-ranlib

LDLIBS += -L../lib -lnetlink -lutil

SUBDIRS=lib ip tc

LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a

all: check-kernel
	@set -e; \
	for i in $(SUBDIRS); \
	do $(MAKE) -C $$i; done

check-kernel:

clean:
	for i in $(SUBDIRS) doc; \
	do $(MAKE) -C $$i clean; done

.EXPORT_ALL_VARIABLES:
