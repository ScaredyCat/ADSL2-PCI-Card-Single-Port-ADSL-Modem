# Support ATE function
HAS_ATE=n

# Support 2860 QA ATE function
HAS_2860_QA=n

# Support WSC function
HAS_WSC=n


# Support LLTD function
HAS_LLTD=n

# Support WDS function
HAS_WDS=y

# Support AP-Client function
HAS_APCLI=n

# Support Wpa_Supplicant
HAS_WPA_SUPPLICANT=n

# Support Native WpaSupplicant for Network Maganger
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=n

#Support Net interface block while Tx-Sw queue full
HAS_BLOCK_NET_IF=n

#Support IGMP-Snooping function.
HAS_IGMP_SNOOP_SUPPORT=n

#Support DFS function
HAS_DFS_SUPPORT=n

#Support Carrier-Sense function
HAS_CS_SUPPORT=n

# Support for STA Ethernet Converter
HAS_ETH_CONVERT_SUPPORT=n

# Support user specific transmit rate of Multicast packet.
HAS_MCAST_RATE_SPECIFIC_SUPPORT=n

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 

ifeq ($(RT2860_MODE),AP)
WFLAGS += -DCONFIG_AP_SUPPORT  -DUAPSD_AP_SUPPORT -DMBSS_SUPPORT -DIAPP_SUPPORT -DDBG

ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
ifeq ($(HAS_2860_QA),y)
WFLAGS += -DRALINK_2860_QA
endif
endif


ifeq ($(HAS_WSC),y)
WFLAGS += -DWSC_AP_SUPPORT
endif

ifeq ($(HAS_WDS),y)
WFLAGS += -DWDS_SUPPORT
endif

ifeq ($(HAS_APCLI),y)
WFLAGS += -DAPCLI_SUPPORT -DMAT_SUPPORT
#ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
#WFLAGS += -DETH_CONVERT_SUPPORT
#endif 
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
WFLAGS += -DIGMP_SNOOP_SUPPORT
endif

ifeq ($(HAS_CS_SUPPORT),y)
WFLAGS += -DCARRIER_DETECTION_SUPPORT
endif

ifeq ($(HAS_MCAST_RATE_SPECIFIC_SUPPORT), y)
WFLAGS += -DMCAST_RATE_SPECIFIC
endif

endif #// endif of RT2860_MODE == AP //
#################################################
ifeq ($(RT2860_MODE),STA)
WFLAGS += -DCONFIG_STA_SUPPORT -DDBG 

ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
endif

ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif

ifeq ($(HAS_WSC),y)
WFLAGS += -DWSC_STA_SUPPORT
endif

ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
WFLAGS += -DETH_CONVERT_SUPPORT  -DMAT_SUPPORT
endif

ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
ifeq ($(HAS_2860_QA),y)
WFLAGS += -DRALINK_2860_QA
endif
endif

endif
# endif of ifeq ($(RT2860_MODE),STA)

ifeq ($(PLATFORM),5VT)
#WFLAGS += -DCONFIG_5VT_ENHANCE
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
WFLAGS += -DBLOCK_NET_IF
endif

ifeq ($(HAS_DFS_SUPPORT),y)
WFLAGS += -DDFS_SUPPORT
endif

ifeq ($(HAS_LLTD),y)
WFLAGS += -DLLTD_SUPPORT
endif

ifeq ($(PLATFORM),IXP)
WFLAGS += -DBIG_ENDIAN
endif

ifeq ($(PLATFORM),INF_TWINPASS)
WFLAGS += -DBIG_ENDIAN -DINF_TWINPASS
endif

ifeq ($(PLATFORM),INF_DANUBE)
WFLAGS += -DBIG_ENDIAN -DINF_DANUBE -DCONFIG_5VT_ENHANCE
endif

#kernel build options for 2.4
# move to Makefile outside LINUX_SRC := /opt/star/kernel/linux-2.4.27-star

ifeq ($(PLATFORM),STAR)
CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -I$(RT2860_DIR)/include -Wall -Wstrict-prototypes -Wno-trigraphs -O2 -fno-strict-aliasing -fno-common -Uarm -fno-common -pipe -mapcs-32 -D__LINUX_ARM_ARCH__=4 -march=armv4  -mshort-load-bytes -msoft-float -Uarm -DMODULE -DMODVERSIONS -include $(LINUX_SRC)/include/linux/modversions.h $(WFLAGS)

export CFLAGS
endif

ifeq ($(PLATFORM),SIGMA)
CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -I$(LINUX_SRC)/include/asm/gcc -I$(LINUX_SRC)/include/asm-mips/mach-tango2 -I$(LINUX_SRC)/include/asm-mips/mach-tango2 -DEM86XX_CHIP=EM86XX_CHIPID_TANGO2 -DEM86XX_REVISION=6 -I$(LINUX_SRC)/include/asm-mips/mach-generic -I$(RT2860_DIR)/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -ffreestanding -O2     -fomit-frame-pointer -G 0 -mno-abicalls -fno-pic -pipe  -mabi=32 -march=mips32r2 -Wa,-32 -Wa,-march=mips32r2 -Wa,-mips32r2 -Wa,--trap -DMODULE $(WFLAGS) 

export CFLAGS
endif

ifeq ($(PLATFORM),5VT)
CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -I$(RT2860_DIR)/include -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -O3 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=apcs-gnu -mno-thumb-interwork -D__LINUX_ARM_ARCH__=5 -march=armv5te -mtune=arm926ej-s --param max-inline-insns-single=40000  -Uarm -Wdeclaration-after-statement -Wno-pointer-sign -DMODULE $(WFLAGS) 

export CFLAGS
endif


ifeq ($(PLATFORM),INF_TWINPASS)
CFLAGS := -D__KERNEL__ -DMODULE -I$(LINUX_SRC)/include -I$(RT2860_DIR)/include -Wall -Wstrict-prototypes -Wno-trigraphs -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -G 0 -mno-abicalls -fno-pic -march=4kc -mips32 -Wa,--trap -pipe -mlong-calls $(WFLAGS)
export CFLAGS
endif

ifeq ($(PLATFORM),INF_DANUBE)
CFLAGS := -D__KERNEL__ -DMODULE -I$(LINUX_SRC)/include -I$(RT2860_DIR)/include -Wall -Wstrict-prototypes -Wno-trigraphs -fomit-frame-pointer -fno-strict-aliasing -fno-common -G 0 -mno-abicalls -fno-pic -Os -mtune=4kc -march=4kc -mips32 -Wa,--trap -pipe -mlong-calls $(WFLAGS)
export CFLAGS
endif

ifeq ($(PLATFORM),PC)
    ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	# Linux 2.4
	CFLAGS := -D__KERNEL__ -I$(LINUX_SRC)/include -I$(RT2860_DIR)/include -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=i686 -DMODULE -DMODVERSIONS -include $(LINUX_SRC)/include/linux/modversions.h $(WFLAGS)
	export CFLAGS
    else
	# Linux 2.6
	EXTRA_CFLAGS := $(WFLAGS) -I$(RT2860_DIR)/include
    endif
endif

ifeq ($(PLATFORM),IXP)
        EXTRA_CFLAGS := -v $(WFLAGS) -I$(RT2860_DIR)/include -mbig-endian
endif

