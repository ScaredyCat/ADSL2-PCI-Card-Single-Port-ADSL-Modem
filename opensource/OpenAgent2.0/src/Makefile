TOPDIR := ..

SUBDIRS = tools auth CLI comm device event handler init res bin APIforWeb

O_TARGET = 

SOURCES =

#**********************
#bin_PROG = prog
#prog_SOURCE = prog.c xxx.c yyy.c
#prog_LIB = prog.o xxx.a yyy.a
#**********************
bin_PROG = agent
agent_SOURCE = cpe_agent.c
agent_LIB = auth/libauth.o CLI/libcli.o comm/libcomm.o \
            device/libdev.o event/libevent.o handler/libhandler.o \
            init/libinit.o res/libres.o tools/libtools.o


#**********************
#test_PROG = mytest
#mytestg_SOURCE = prog.c xxx.c yyy.c
#mytest_LIB = prog.o xxx.a yyy.a
#**********************
test_PROG =


CFLAGS =
DEFS   = 
INCLUDE = 

LDFLAGS =
LDLIBS  = 

include $(TOPDIR)/rules.mk
