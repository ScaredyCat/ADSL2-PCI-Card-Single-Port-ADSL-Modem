#
# Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
# Copyright (c) 2002-2005 Atheros Communications, Inc.
# All rights reserved.
#
# $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/diag/Makefile#4 $
#
DEPTH=	..

ATHDIR=	${DEPTH}/../../madwifi/ath
#ATHDIR=	/data/freebsd/head/sys/dev/ath
DESTDIR=
ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif

CC = $(CROSS)gcc

CFLAGS=	-I. -I${DEPTH} -I${ATHDIR} -g

ALL=	eeprom rfgain ani txpow dumpregs key reg

all:	${ALL}

eeprom:	eeprom.c
	${CC} -o eeprom ${CFLAGS} eeprom.c
rfgain:	rfgain.o diag.o
	${CC} -o rfgain ${CFLAGS} rfgain.o diag.o
ani:	ani.o diag.o
	${CC} -o ani ${CFLAGS} ani.o diag.o
txpow:	txpow.c
	${CC} -o txpow ${CFLAGS} txpow.c
dumpregs: dumpregs.o dumpregs_5210.o dumpregs_5211.o dumpregs_5212.o dumpregs_5416.o
	${CC} -o dumpregs ${CFLAGS} dumpregs.o \
		dumpregs_5210.o dumpregs_5211.o dumpregs_5212.o dumpregs_5416.o
key:	key.c
	${CC} -o key ${CFLAGS} key.c
reg:	reg.c
	${CC} -o reg ${CFLAGS} reg.c

install: ${ALL}
	for i in ${ALL}; do \
		rm -f /usr/local/bin/$$i; \
		cp $$i /usr/local/bin/$$i; \
	done

clean:
	rm -f ${ALL} *.o core
