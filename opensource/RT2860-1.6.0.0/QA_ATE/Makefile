EXEC = ated
SRCS = ate.c

#PLATFORM = 5VT
PLATFORM = PC
#PLATFORM = STAR

ifeq ($(PLATFORM),5VT)
CROSS_COMPILE = /opt/crosstool/uClibc/bin/arm-linux-
endif

ifeq ($(PLATFORM),STAR)
CROSS_COMPILE = /opt/star/tools/arm-linux/bin/arm-linux-
endif

ifeq ($(PLATFORM),PC)
	#PATCHLEVEL := 6
	ifeq ($(PATCHLEVEL),)
	PATCHLEVEL := $(shell uname -r | cut -d. -f2)
	endif

	ifeq ($(PATCHLEVEL), 4)
	INCLUDE= /usr/src/linux-$(shell uname -r)/include/
	else
	INCLUDE = /lib/modules/$(shell uname -r)/build/include/
	endif
	CROSS_COMPILE =
endif

# If you want to debug daemon, add following line
#EXTRA_CFLAGS += -DDBG

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

all: $(EXEC)

ifeq ($(PLATFORM),PC)
$(EXEC): $(SRCS) 
	$(CC) -v -I$(INCLUDE) $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LDLIBS)
else
$(EXEC): $(SRCS) 
	$(CC) -v $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LDLIBS)
	cp ated /tftpboot
endif

clean:
	-rm -f $(EXEC) *.o

