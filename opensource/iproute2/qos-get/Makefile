include ../Config
#CC=gcc
#CFLAGS=-g -rdynamic

#RANLIB=ranlib
LIBPATH=/home/chandrav/iproute2/lib
TCPATH=/home/chandrav/iproute2/tc
QOSLIBPATH=-L$(LIBPATH) -L$(TCPATH)
QOSLIB=-lnetlink -lutil -ltc

#testget:ifx_get.c
#	$(CC) $(CFLAGS) -o $@ $^ $(TCPATH)/q_fifo.o $(TCPATH)/tc_util.o $(QOSLIB) $(QOSLIBPATH) -lc -lm -ldl 

TCOBJ=ifx_get.o $(TCPATH)/q_fifo.o $(TCPATH)/tc_util.o

all: testget

testget: $(TCOBJ) $(LIBNETLINK) $(LIBUTIL) 

clean:
	-rm -f  testget *.o
