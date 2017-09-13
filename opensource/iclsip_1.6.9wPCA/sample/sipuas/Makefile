NAME	= sipuas

SRC	= sipuas.c
	  
OBJ	= $(SRC:.c=.o)

#CC	= gcc

ROOT	= ../../src
include ../../src/make/common.mk
OUT = $(NAME)

OFLAG	= # -O2
CFLAGS	= $(IFX_CFLAGS) -pipe $(OFLAG) -Wall -g \
      	  -I. -I$(ROOT)/
      	  -DVERSION=\"$(VERSION)\" \
      	  -DUNIX

LFLAGS	= $(IFX_LFLAGS) -L$(ROOT)/Linux/lib
	  #-L../../../third-party/lib/cppunit

LIBS	= -lcclsip  -lpthread

MKDEP	= makedepend
MKDIR	= mkdir -p
CP	= cp
LN	= ln
RM	= rm -f

all: .depend $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(OUT) $(OBJ) $(LIBS)

.SUFFIXES: .o .c .cpp
.cpp.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: dep depned
dep depend .depend: $(SRC)
	$(MKDEP) -f- -- $(CFLAGS) -- $(SRC) >.depend 2>/dev/null

clean:
	-rm -f $(OUT) $(OBJ) core* .depend

cleancore:
	-rm -f core*

-include .depend
# DO NOT DELETE
