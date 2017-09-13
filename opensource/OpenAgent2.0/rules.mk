
#You can modify it for your compiler
TOOLCHAIN_DIR=/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin/
CROSS_COMPILER_PREFIX=mips-linux-
CC = $(TOOLCHAIN_DIR)$(CROSS_COMPILER_PREFIX)gcc
LD = $(TOOLCHAIN_DIR)$(CROSS_COMPILER_PREFIX)ld
STRIP = $(TOOLCHAIN_DIR)$(CROSS_COMPILER_PREFIX)strip
#LDFLAGS += -s -L/root/danube_build_anr/root_filesystem/lib -L/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/lib
LDFLAGS += -s  
#DEFS    = -D_KERNEL_ -DUSE_SSL
DEFS    = -D_KERNEL_
#INCLUDE = -I/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/include -I/Danube/3.0.5/source/kernel/opensource/linux-2.4.31/include
INCLUDE = -I/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/include 
#LDLIBS  = -L/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/lib -lpthread -ldl -lssl 
LDLIBS  = -L/Danube/3.0.5/source/opt_uclibc_lib/lib -lpthread -ldl
CFLAGS += -Os -mips32 -mtune=4kc -I/Danube/3.0.5/source/user/ifx/IFXAPIs/include -Wall -g3 -O2


objs := $(SOURCES:.c=.o)
dirs := $(patsubst %, %_sub, $(SUBDIRS))
dirclean := $(patsubst %, %_clr, $(SUBDIRS))

all: $(dirs) $(objs) O_TAR bin_prog
	@echo Build successufully...

%.o:%.c
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) -c $< -o $@

$(dirs):
	make -C $(patsubst %_sub, %, $@) -f Makefile

O_TAR: $(objs)
ifdef O_TARGET

	$(LD) $(LDFLAGS) $(LDLIB) -r $(objs) $(LIBS) -o $(join lib, $(O_TARGET))

endif

bin_prog: clrbin $(bin_PROG)

clrbin: $($(join $@, _LIB))
ifdef bin_PROG
	rm -f $(bin_PROG)
endif


$(bin_PROG):

	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(LDFLAGS) $(LDLIBS) $($(join $@, _LIB)) -o $@ $($(join $@, _SOURCE))
	$(STRIP) $@


test: clrtst $(test_PROG)

clrtst:
ifdef test_PROG
	rm -f $(test_PROG)
endif

$(test_PROG):
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(LDFLAGS) $(LDLIBS) $($(join $@, _LIB)) -o $@ $($(join $@, _SOURCE))

clean: $(dirclean)
	rm -f $(objs)  $(patsubst %, lib%, $(O_TARGET)) $(bin_PROG) $(test_PROG)

$(dirclean):
	make -C $(patsubst %_clr, %, $@) -f Makefile clean

