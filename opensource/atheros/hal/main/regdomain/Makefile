#
# Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
# Copyright (c) 2002-2005 Atheros Communications, Inc.
# All rights reserved.
#
# $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/regdomain/Makefile#1 $
#
DEPTH=	..

DESTDIR=

CFLAGS=	-I. -I${DEPTH} -g

OBJS=	main.o ah_regdomain.o

regdomain: ${OBJS}
	${CC} -o regdomain ${CFLAGS} ${OBJS}

regtest: test.o ah_regdomain.o
	${CC} -o regtest ${CFLAGS} test.o ah_regdomain.o

${OBJS} test.o:	opt_ah.h

ah_regdomain.o: ${DEPTH}/ah_regdomain.c
	${CC} -c -o ah_regdomain.o ${CFLAGS} ${DEPTH}/ah_regdomain.c

opt_ah.h:
	echo "#define AH_DEBUG 1" > opt_ah.h
	echo "#define AH_ENABLE_AP_SUPPORT 1" >> opt_ah.h

install:

clean:
	rm -f regdomain regtest test.o ${OBJS} core opt_ah.h
