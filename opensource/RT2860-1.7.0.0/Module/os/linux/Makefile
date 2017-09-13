include $(RT28xx_DIR)/os/linux/config.mk 

ifeq ($(RT28xx_MODE),AP)
MOD_NAME = rt$(CHIPSET)ap
DAT_PATH = /etc/Wireless/RT$(CHIPSET)AP
DAT_FILE_NAME = RT$(CHIPSET)AP.dat
else
ifeq ($(RT28xx_MODE), STA)
MOD_NAME = rt$(CHIPSET)sta
DAT_PATH = /etc/Wireless/RT$(CHIPSET)STA
DAT_FILE_NAME = RT$(CHIPSET)STA.dat
endif
endif

OBJ := $(MOD_NAME).o

RT28XX_AP_OBJ := \
	$(RT28xx_DIR)/common/md5.o\
	$(RT28xx_DIR)/common/mlme.o\
	$(RT28xx_DIR)/common/rtmp_wep.o\
	$(RT28xx_DIR)/common/action.o\
	$(RT28xx_DIR)/common/ba_action.o\
	$(RT28xx_DIR)/common/cmm_data.o\
	$(RT28xx_DIR)/common/rtmp_init.o\
	$(RT28xx_DIR)/common/rtmp_tkip.o\
	$(RT28xx_DIR)/common/cmm_sync.o\
	$(RT28xx_DIR)/common/eeprom.o\
	$(RT28xx_DIR)/common/cmm_sanity.o\
	$(RT28xx_DIR)/common/cmm_info.o\
	$(RT28xx_DIR)/common/cmm_wpa.o\
	$(RT28xx_DIR)/common/dfs.o\
	$(RT28xx_DIR)/ap/ap.o\
	$(RT28xx_DIR)/ap/ap_assoc.o\
	$(RT28xx_DIR)/ap/ap_auth.o\
	$(RT28xx_DIR)/ap/ap_authrsp.o\
	$(RT28xx_DIR)/ap/ap_connect.o\
	$(RT28xx_DIR)/ap/ap_mlme.o\
	$(RT28xx_DIR)/ap/ap_sanity.o\
	$(RT28xx_DIR)/ap/ap_sync.o\
	$(RT28xx_DIR)/ap/ap_wpa.o\
	$(RT28xx_DIR)/ap/ap_data.o\
	$(RT28xx_DIR)/ap/ap_uapsd.o\
	$(RT28xx_DIR)/ap/ap_mbss.o\
	$(RT28xx_DIR)/os/linux/rt_linux.o\
	$(RT28xx_DIR)/os/linux/rt_profile.o\
	$(RT28xx_DIR)/os/linux/rt_main_dev.o\
	$(RT28xx_DIR)/os/linux/ap_ioctl.o\
	$(RT28xx_DIR)/ap/ap_autoChSel.o


ifeq ($(CHIPSET),2860)
RT28XX_AP_OBJ += \
	$(RT28xx_DIR)/common/2860_rtmp_init.o\
	$(RT28xx_DIR)/os/linux/2860_main_dev.o\
	$(RT28xx_DIR)/common/cmm_data_2860.o
endif

ifeq ($(CHIPSET),2870)
RT28XX_AP_OBJ += \
	$(RT28xx_DIR)/os/linux/2870_main_dev.o\
	$(RT28xx_DIR)/common/2870_rtmp_init.o\
	$(RT28xx_DIR)/common/rtusb_io.o\
	$(RT28xx_DIR)/common/rtusb_bulk.o\
	$(RT28xx_DIR)/common/rtusb_data.o\
	$(RT28xx_DIR)/common/cmm_data_2870.o
endif

ifeq ($(HAS_ATE),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/os/linux/rt_ate.o
endif

ifeq ($(HAS_WSC),y)
RT28XX_AP_OBJ += \
	$(RT28xx_DIR)/common/wsc.o\
	$(RT28xx_DIR)/common/wsc_tlv.o\
	$(RT28xx_DIR)/common/sha2.o\
	$(RT28xx_DIR)/common/hmac.o\
	$(RT28xx_DIR)/common/dh_key.o\
	$(RT28xx_DIR)/common/evp_enc.o	
endif

ifeq ($(HAS_WDS),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/ap/ap_wds.o
endif


ifeq ($(HAS_APCLI),y)
RT28XX_AP_OBJ += \
	$(RT28xx_DIR)/ap/ap_apcli.o \
	$(RT28xx_DIR)/ap/apcli_mlme.o \
	$(RT28xx_DIR)/ap/apcli_ctrl.o \
	$(RT28xx_DIR)/ap/apcli_sync.o \
	$(RT28xx_DIR)/ap/apcli_auth.o \
	$(RT28xx_DIR)/ap/apcli_assoc.o \
	$(RT28xx_DIR)/common/cmm_mat.o \
	$(RT28xx_DIR)/common/cmm_mat_iparp.o \
	$(RT28xx_DIR)/common/cmm_mat_pppoe.o \
	$(RT28xx_DIR)/common/cmm_mat_ipv6.o
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/common/netif_block.o
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/common/igmp_snoop.o
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/ap/ap_dls.o
endif

ifeq ($(HAS_IDS_SUPPORT),y)
RT28XX_AP_OBJ += $(RT28xx_DIR)/ap/ap_ids.o
endif

RT28XX_STA_OBJ := \
	$(RT28xx_DIR)/common/md5.o\
	$(RT28xx_DIR)/common/mlme.o\
	$(RT28xx_DIR)/common/rtmp_wep.o\
	$(RT28xx_DIR)/common/action.o\
	$(RT28xx_DIR)/common/ba_action.o\
	$(RT28xx_DIR)/common/cmm_data.o\
	$(RT28xx_DIR)/common/rtmp_init.o\
	$(RT28xx_DIR)/common/rtmp_tkip.o\
	$(RT28xx_DIR)/common/cmm_sync.o\
	$(RT28xx_DIR)/common/eeprom.o\
	$(RT28xx_DIR)/common/cmm_sanity.o\
	$(RT28xx_DIR)/common/cmm_info.o\
	$(RT28xx_DIR)/common/cmm_wpa.o\
	$(RT28xx_DIR)/common/dfs.o\
	$(RT28xx_DIR)/sta/assoc.o\
	$(RT28xx_DIR)/sta/aironet.o\
	$(RT28xx_DIR)/sta/auth.o\
	$(RT28xx_DIR)/sta/auth_rsp.o\
	$(RT28xx_DIR)/sta/sync.o\
	$(RT28xx_DIR)/sta/sanity.o\
	$(RT28xx_DIR)/sta/rtmp_data.o\
	$(RT28xx_DIR)/sta/connect.o\
	$(RT28xx_DIR)/sta/wpa.o\
	$(RT28xx_DIR)/os/linux/rt_linux.o\
    $(RT28xx_DIR)/os/linux/rt_profile.o\
	$(RT28xx_DIR)/os/linux/rt_main_dev.o\
	$(RT28xx_DIR)/os/linux/sta_ioctl.o

ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
RT28XX_STA_OBJ += \
	$(RT28xx_DIR)/common/cmm_mat.o \
	$(RT28xx_DIR)/common/cmm_mat_iparp.o \
	$(RT28xx_DIR)/common/cmm_mat_pppoe.o \
	$(RT28xx_DIR)/common/cmm_mat_ipv6.o
endif

ifeq ($(HAS_WSC),y)
RT28XX_STA_OBJ += \
	$(RT28xx_DIR)/common/wsc.o\
	$(RT28xx_DIR)/common/wsc_tlv.o\
	$(RT28xx_DIR)/common/sha2.o\
	$(RT28xx_DIR)/common/hmac.o\
	$(RT28xx_DIR)/common/dh_key.o\
	$(RT28xx_DIR)/common/evp_enc.o	
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
RT28XX_STA_OBJ += $(RT28xx_DIR)/common/netif_block.o
endif

ifeq ($(CHIPSET),2860)
RT28XX_STA_OBJ += \
	$(RT28xx_DIR)/common/2860_rtmp_init.o\
	$(RT28xx_DIR)/os/linux/2860_main_dev.o\
	$(RT28xx_DIR)/common/cmm_data_2860.o
endif

ifeq ($(CHIPSET),2870)
RT28XX_STA_OBJ += \
	$(RT28xx_DIR)/os/linux/2870_main_dev.o\
	$(RT28xx_DIR)/common/2870_rtmp_init.o\
	$(RT28xx_DIR)/common/rtusb_io.o\
	$(RT28xx_DIR)/common/rtusb_bulk.o\
	$(RT28xx_DIR)/common/rtusb_data.o\
	$(RT28xx_DIR)/common/cmm_data_2870.o
endif

ifeq ($(HAS_ATE),y)
RT28XX_STA_OBJ += $(RT28xx_DIR)/os/linux/rt_ate.o
endif

all:$(OBJ)

rt$(CHIPSET)sta.o: $(RT28XX_STA_OBJ)
	$(LD) -r $^ -o $@

rt$(CHIPSET)ap.o: $(RT28XX_AP_OBJ)
	$(LD) -r $^ -o $@

release:
	echo "MAKE Linux Station Code Release"

clean:
	rm -f $(RT28xx_DIR)/common/*.o
	rm -f $(RT28xx_DIR)/common/.*.{cmd,flags,d}
	rm -f $(RT28xx_DIR)/os/linux/*.{o,ko,mod.{o,c}}
	rm -f $(RT28xx_DIR)/os/linux/.*.{cmd,flags,d}
	rm -fr $(RT28xx_DIR)/os/linux/.tmp_versions
ifeq ($(RT28xx_MODE),AP)
	rm -f $(RT28xx_DIR)/ap/*.o
	rm -f $(RT28xx_DIR)/ap/.*.{cmd,flags,d}
else
ifeq ($(RT28xx_MODE),STA)
	rm -f $(RT28xx_DIR)/sta/*.o
	rm -f $(RT28xx_DIR)/sta/.*.{cmd,flags,d}
endif
endif

install:
	rm -rf $(DAT_PATH)
	$(shell [ ! -f /etc/Wireless ] && mkdir /etc/Wireless)
	mkdir $(DAT_PATH)
	cp $(RT28xx_DIR)/$(DAT_FILE_NAME) $(DAT_PATH)/.
	install -d $(LINUX_SRC_MODULE)
	install -m 644 -c $(addsuffix .o,$(MOD_NAME)) $(LINUX_SRC_MODULE)
	/sbin/depmod -a ${shell uname -r}

uninstall:
#	rm -rf $(DAT_PATH)
	rm -rf $(addprefix $(LINUX_SRC_MODULE),$(addsuffix .o,$(MOD_NAME)))
	/sbin/depmod -a ${shell uname -r}
