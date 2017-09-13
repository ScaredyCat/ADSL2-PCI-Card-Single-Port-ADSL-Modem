# Oracle proc
#
# %W% %E% %U%
#

.SUFFIXES: .pc

RM = \rm -f

PC = proc

CPPFLAGS =
CFLAGS = -Wall
PCFLAGS = mode=oracle ireclen=160 lines=yes lname=/dev/null

.pc.o:
	$(PC) $(PCFLAGS) code=ansi_c iname=$< oname=_kgw.c
	$(CC) -I$(ORACLE_HOME)/precomp/public $(CPPFLAGS) -o $@ -c _kgw.c
	$(RM) _kgw.c

SRC.c = ora_load.c
SRC.pc = oracle.pc

SRC = $(SRC.c) $(SRC.pc)

SRC.h =

OBJ = $(SRC.c:.c=.o) $(SRC.pc:.pc=.o)

LDFLAGS = -L$(ORACLE_HOME)/lib

ORALIBS = -lclntsh $(ORACLE_HOME)/lib/scorept.o $(ORACLE_HOME)/lib/sscoreed.o \
	$(ORACLE_HOME)/rdbms/lib/kpudfo.o \
	-lclient -lsql $(ORACLE_HOME)/lib/nautab.o $(ORACLE_HOME)/lib/naeet.o \
	$(ORACLE_HOME)/lib/naect.o $(ORACLE_HOME)/lib/naedhs.o \
	`cat $(ORACLE_HOME)/lib/naldflgs` \
	-lnetv2 -lnttcp -lnetwork -lncr -lnetv2 -lnttcp -lnetwork \
	-lclient -lvsn -lcommon -lgeneric -lmm -lnlsrtl3 -lcore4 \
	-lnlsrtl3 -lcore4 -lnlsrtl3 -lnetv2 -lnttcp -lnetwork -lncr \
	-lnetv2 -lnttcp -lnetwork -lclient -lvsn -lcommon -lgeneric \
	$(ORACLE_HOME)/lib/libplsf.a $(ORACLE_HOME)/lib/libplsb.a \
	$(ORACLE_HOME)/lib/libextp.a \
	-lepc -lnlsrtl3  -lcore4 -lnlsrtl3 -lcore4 -lnlsrtl3 -lclient \
	-lvsn -lcommon -lgeneric -lnlsrtl3  -lcore4 -lnlsrtl3 -lcore4 -lnlsrtl3 \
	`cat $(ORACLE_HOME)/lib/sysliblist` -ldl -lm

LDLIBS = $(ORALIBS)

ora_load: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LDLIBS)

ifeq (.depend,$(wildcard .depend))
include .depend
endif

clean:
	$(RM) $(OBJ)

tags: $(SRC)
	ctags *.c *.pc

dep depend:
	./makedep.g++ $(SRC) > .depend
