NAME	= sipUa

OUT	= $(NAME)
SRC	= CallManager.c MediaManager.c SDPManager.c cclRtp.c lib_vmmc.c common.c tapidemo.c cid.c voip.c qos.c pcm.c abstract.c	event_handling_msg.c tapi_signal.c conference.c	feature.c analog.c
OBJ	= $(SRC:.c=.o)

#CC	= gcc

ROOT	= ../../src

LIBATMPATH=../../../ifx/IFXAPIs/lib/
LIBIFXCOMMON=IFXAPIs
IFXCOMMONPATH=../../../ifx/IFXAPIs/
OFLAG	= -O2
CFLAGS	= $(IFX_CFLAGS) -pipe $(OFLAG) -g \
      	  -I. -I$(ROOT)/ \
      	  -I./system/ \
          -I/Danube/3.0.5/source/kernel/opensource/linux-2.4.31/include \
          -I/Danube/3.0.5/source/user/ifx//common/src/ \
          -I/Danube/3.0.5/source/user/ifx//drv_vmmc/src/ \
          -I/Danube/3.0.5/source/user/ifx//drv_tapi/src/ \
          -I/Danube/3.0.5/source/user/ifx/drv_tapi/src/common/src \
      	  -DVERSION=\"$(VERSION)\" \
      	  -DUNIX -DLINUX -DENABLE_TRACE -DVMMC -DFXO -Wimplicit -Wreturn-type -Wswitch -Wcomment -Wparentheses -Wpointer-arith -DUSE_FILESYSTEM 
      	  
LFLAGS	= $(IFX_LFLAGS)  -L$(ROOT)/Linux/lib
	  #-L../../../third-party/lib/cppunit
	  
LDFLAGS+= $(IFX_LDFLAGS)

LIBS	= -lcclsip -lpthread -lm

MKDEP	= makedepend
MKDIR	= mkdir -p
CP	= cp
LN	= ln
RM	= rm -f

all: .depend $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) $(LDFLAGS) -o $(OUT) $(OBJ) $(LIBS) -l$(LIBIFXCOMMON)

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
