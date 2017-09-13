# Establish working directory and pathnames.
TOP:=$(shell pwd)

# The Madwifi Makefiles must have environment varibles
# HAL and ATH_RATE defined for properly compiling
export HAL=$(TOP)/hal/main


export TOOLPREFIX=mips-linux-
export KERNELRELEASE=2.4.31
#export KERNELPATH=${TOPDIR}/source/kernel/opensource/linux-2.4.31/
export TARGET=mips4kc-be-elf
#export TARGET= mips-be-elf

#export TARGET=i386-elf

all: driver tools

clean:
	( make -C $(HAL)/linux clean)
	( make -C $(TOP)/ratectrl clean)
#	( make -C $(TOP)/dfs clean)
	( make -C $(TOP)/madwifi clean)
	( make -C $(TOP)/madwifi/tools clean)
	( make -C $(TOP)/hostapd clean)

driver:
# Build Hal 
	( make -C $(HAL)/linux release )
# Build Madwifi using Hal
	if ( test -e $(TOP)/ratectrl) then export ATH_RATE=$(TOP)/ratectrl; fi; \
	if ( test -e $(TOP)/dfs) then export ATH_PHYERR=$(TOP)/dfs; fi; \
	make -C $(TOP)/madwifi

tools:
	make -C $(TOP)/madwifi/tools
	make -C $(TOP)/hostapd
	make -C $(TOP)/hostapd-config clean
	make -C $(TOP)/hostapd-config
	./hostapd-config/hostap-config
