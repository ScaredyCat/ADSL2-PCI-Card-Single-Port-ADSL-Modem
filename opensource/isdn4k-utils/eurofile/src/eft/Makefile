# $Id: Makefile,v 1.3 2002/01/31 18:55:23 paul Exp $

TDU_M_OBJ = master.o
TDU_S_OBJ = slave.o echo_pos.o
TDU_OBJ = tdu.o tdu_user.o fileheader.o transfer.o tdu_log.o sbv.o \
	tdu_buf.o tdu_stream.o access.o assoc.o

EFT_M_OBJ = eft_client.o 
EFT_S_OBJ = eft_server.o eft_i4l.o
EFT_OBJ = eft.o eft_dir.o eft_names.o eft_tmpfile.o

OBJECTS = $(TDU_M_OBJ) $(TDU_S_OBJ) $(TDU_OBJ) $(EFT_M_OBJ) $(EFT_S_OBJ) $(EFT_OBJ) 

CFLAGS += -Iinclude

all: libeft.a

libeft.a: $(OBJECTS)
	-rm -f libeft.a
	ar -cq libeft.a $(OBJECTS)

$(OBJECTS): include/tdu_user.h tdu.h

$(EFT_OBJ) $(EFT_M_OBJ) $(EFT_S_OBJ): include/eft.h eft_private.h

tdu_log.o tdu_stream.o fileheader.o eft.o eft_client.o eft_server.o eft_dir.o: fileheader.h

eft_i4l.o: ../config.h

tdu.o eft.o: extra_version_.h

extra_version_.h: make_extra_version.sh $(E4L_EXTRA_VERSION_DEP)
	./make_extra_version.sh $(E4L_EXTRA_VERSION_DEP) > extra_version_.h

clean:
	rm -f *.o *~ libeft.a extra_version_.h

new_version:
	sed 's/^#define[ 	]*E4L_VERSION.*/#define E4L_VERSION "$(NEW_VERSION)"/' < include/tdu_user.h > tmp.h
	mv tmp.h include/tdu_user.h
	./make_extra_version.sh - > extra_version_.h


.PHONY: new_version clean

