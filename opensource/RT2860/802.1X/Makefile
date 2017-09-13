PLATFORM = INF_DANUBE

ifeq ($(PLATFORM),INF_DANUBE)
CROSS_COMPILE = /opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin/mips-linux-
endif

CC = $(CROSS_COMPILE)gcc

ifndef CFLAGS
CFLAGS = -O2 -Wall -g -DDBG
endif

# If you want to support multiple radius server for multiple bssid, add following line
#CFLAGS +=  -DMULTIPLE_RADIUS

OBJS =	rt2860apd.o eloop.o eapol_sm.o radius.o md5.o  \
	config.o ieee802_1x.o  \
	sta_info.o   radius_client.o

all: rt2860apd

rt2860apd: $(OBJS)
	$(CC) -o rt2860apd $(OBJS)
	cp rt2860apd /tftpboot

clean:
	rm -f core *~ *.o rt2860apd *.d

-include $(OBJS:%.o=%.d)
