#
# Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
# Copyright (c) 2002-2005 Atheros Communications, Inc.
# All rights reserved.
#
# $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ardecode/Makefile#1 $
#
DESTDIR=

CFLAGS=	-I.. -g

OBJS=	main.o ar5210.o ar5211.o ar5212.o

ardecode: ${OBJS}
	${CC} -o ardecode ${OBJS}

install:
	rm -f ${DESTDIR}/usr/local/bin/ardecode
	cp ardecode ${DESTDIR}/usr/local/bin

clean:
	rm -f ardecode ${OBJS} core
