#MODE  STA or AP
#RT2860_MODE = STA
RT2860_MODE = AP
#TARTET = LINUX or UCOS 
TARGET = LINUX
#TARGET = UCOS
#RT2860_DIR = home directory of RT2860 source code
RT2860_DIR = $(shell pwd)
#PLATFORM = 5VT
PLATFORM = PC
#PLATFORM = SIGMA
#PLATFORM = INIC
#PLATFORM = STAR
#PLATFORM = IXP
#PLATFORM = INF_TWINPASS
PLATFORM = INF_DANUBE
RELEASE = DPA
#RELEASE = DPB
ifeq ($(PLATFORM),5VT)
LINUX_SRC = /opt/fvt_11N_SDK_0807/fvt131x_SDK_11n/linux-2.6.17
CROSS_COMPILE = /opt/crosstool/uClibc_v5te_le_gcc_4_1_1/bin/arm-linux-
endif

ifeq ($(PLATFORM),SIGMA)
LINUX_SRC = /root/sigma/smp86xx_kernel_source_2.7.172.0/linux-2.6.15
CROSS_COMPILE = /root/sigma/smp86xx_toolchain_2.7.172.0/build_mipsel_nofpu/staging_dir/bin/mipsel-linux-
endif

ifeq ($(PLATFORM),INIC)
UCOS_SRC = /opt/uCOS/iNIC_rt2880
CROSS_COMPILE = /usr/bin/mipsel-linux-
endif

ifeq ($(PLATFORM),STAR)
LINUX_SRC = /opt/star/kernel/linux-2.4.27-star
CROSS_COMPILE = /opt/star/tools/arm-linux/bin/arm-linux-
endif

ifeq ($(PLATFORM),PC)
# Linux 2.6
LINUX_SRC = /lib/modules/$(shell uname -r)/build
# Linux 2.4 Change to your local setting
#LINUX_SRC = /usr/src/linux-2.4.20-8
CROSS_COMPILE = 
endif

ifeq ($(PLATFORM),IXP)
LINUX_SRC = /project/stable/Gmtek/snapgear-uclibc/linux-2.6.x
CROSS_COMPILE = arm-linux-
endif

ifeq ($(PLATFORM),INF_TWINPASS)
# Linux 2.6
#LINUX_SRC = /lib/modules/$(shell uname -r)/build
# Linux 2.4 Change to your local setting
LINUX_SRC = /project/stable/twinpass/release/2.0.1/source/kernel/opensource/linux-2.4.31/
CROSS_COMPILE = mips-linux-
endif

ifeq ($(PLATFORM),INF_DANUBE)
LINUX_SRC = /Danube/3.0.5/source/kernel/opensource/linux-2.4.31/
CROSS_COMPILE = /opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin/mips-linux-
endif

export RT2860_DIR RT2860_MODE LINUX_SRC CROSS_COMPILE PLATFORM RELEASE UCOS_SRC

all: build_tools $(TARGET)


build_tools:
	make -C tools
	$(RT2860_DIR)/tools/bin2h

UCOS:
	make -C os/ucos/ MODE=$(RT2860_MODE)
	echo $(RT2860_MODE)


LINUX:
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4 os/linux/Makefile
	make -C os/linux/
#ifeq ($(RT2860_MODE),AP)
#	cp -f $(RT2860_DIR)/os/linux/rt2860ap.o /tftpboot
#else	
#	cp -f $(RT2860_DIR)/os/linux/rt2860sta.o /tftpboot
#endif	
else
	cp -f os/linux/Makefile.6 os/linux/Makefile
	make V=1  -C  $(LINUX_SRC) SUBDIRS=$(PWD)/os/linux modules
ifeq ($(RT2860_MODE),AP)
	cp -f $(RT2860_DIR)/os/linux/rt2860ap.ko /tftpboot
else	
	cp -f $(RT2860_DIR)/os/linux/rt2860sta.ko /tftpboot
endif	
endif

release:
ifeq ($(TARGET), LINUX)
	make -C os/linux -f Makefile.release release
ifeq ($(RELEASE), DPA)
	make -C os/linux -f Makefile.DPA release
	rm -rf build
endif
ifeq ($(RELEASE), DPB)
	make -C os/linux -f Makefile.DPB release
	rm -rf build
endif
endif


clean:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4 os/linux/Makefile
else
	cp -f os/linux/Makefile.6 os/linux/Makefile
endif
	make -C os/linux clean
	rm -rf os/linux/Makefile
endif	
ifeq ($(TARGET), UCOS)
	make -C os/ucos clean MODE=$(RT2860_MODE)
endif

