include $(RT2860_DIR)/os/linux/config.mk 

ifeq ($(RT2860_MODE),AP)
OBJ = rt2860ap.o

RT2860_OBJ = $(RT2860_DIR)/common/md5.o\
	$(RT2860_DIR)/common/mlme.o\
	$(RT2860_DIR)/common/rtmp_wep.o\
	$(RT2860_DIR)/common/action.o\
	$(RT2860_DIR)/common/ba_action.o\
	$(RT2860_DIR)/common/cmm_data.o\
	$(RT2860_DIR)/common/rtmp_init.o\
	$(RT2860_DIR)/common/rtmp_tkip.o\
	$(RT2860_DIR)/common/cmm_sync.o\
	$(RT2860_DIR)/common/eeprom.o\
	$(RT2860_DIR)/common/cmm_sanity.o\
	$(RT2860_DIR)/common/cmm_info.o\
	$(RT2860_DIR)/common/cmm_wpa.o\
	$(RT2860_DIR)/common/dfs.o\
	$(RT2860_DIR)/ap/ap.o\
	$(RT2860_DIR)/ap/ap_assoc.o\
	$(RT2860_DIR)/ap/ap_auth.o\
	$(RT2860_DIR)/ap/ap_authrsp.o\
	$(RT2860_DIR)/ap/ap_connect.o\
	$(RT2860_DIR)/ap/ap_mlme.o\
	$(RT2860_DIR)/ap/ap_sanity.o\
	$(RT2860_DIR)/ap/ap_sync.o\
	$(RT2860_DIR)/ap/ap_wpa.o\
	$(RT2860_DIR)/ap/ap_data.o\
	$(RT2860_DIR)/ap/ap_uapsd.o\
	$(RT2860_DIR)/ap/ap_mbss.o\
	$(RT2860_DIR)/os/linux/rt_linux.o\
        $(RT2860_DIR)/os/linux/rt_profile.o\
	$(RT2860_DIR)/os/linux/rt_main_dev.o\
	$(RT2860_DIR)/os/linux/ap_ioctl.o\
	$(RT2860_DIR)/ap/ap_autoChSel.o
	
ifeq ($(HAS_ATE),y)
RT2860_OBJ += $(RT2860_DIR)/os/linux/rt_ate.o
endif

ifeq ($(HAS_WSC),y)
RT2860_OBJ += $(RT2860_DIR)/common/wsc.o\
	$(RT2860_DIR)/common/wsc_tlv.o\
	$(RT2860_DIR)/common/sha2.o\
	$(RT2860_DIR)/common/hmac.o\
	$(RT2860_DIR)/common/dh_key.o\
	$(RT2860_DIR)/common/evp_enc.o	
endif

ifeq ($(HAS_WDS),y)
RT2860_OBJ += $(RT2860_DIR)/ap/ap_wds.o
endif


ifeq ($(HAS_APCLI),y)
RT2860_OBJ += \
	$(RT2860_DIR)/ap/ap_apcli.o \
	$(RT2860_DIR)/ap/apcli_mlme.o \
	$(RT2860_DIR)/ap/apcli_ctrl.o \
	$(RT2860_DIR)/ap/apcli_sync.o \
	$(RT2860_DIR)/ap/apcli_auth.o \
	$(RT2860_DIR)/ap/apcli_assoc.o \
	$(RT2860_DIR)/common/cmm_mat.o \
	$(RT2860_DIR)/common/cmm_mat_iparp.o \
	$(RT2860_DIR)/common/cmm_mat_pppoe.o \
	$(RT2860_DIR)/common/cmm_mat_ipv6.o
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
RT2860_OBJ += $(RT2860_DIR)/common/netif_block.o
endif

ifeq ($(HAS_IGMP_SNOOP_SUPPORT),y)
RT2860_OBJ += $(RT2860_DIR)/common/igmp_snoop.o
endif

all:$(OBJ)

rt2860ap.o: $(RT2860_OBJ)
          
	$(LD) -r $^ -o $@
	
clean:
	rm -f $(RT2860_DIR)/ap/*.o
	rm -f $(RT2860_DIR)/common/*.o
	rm -f $(RT2860_DIR)/os/linux/*.o

endif #// endif of ifeq ($(RT2860_MODE), AP) //

ifeq ($(RT2860_MODE),STA)
OBJ = rt2860sta.o
all:$(OBJ)

RT2860_OBJ = \
	$(RT2860_DIR)/common/md5.o\
	$(RT2860_DIR)/common/mlme.o\
	$(RT2860_DIR)/common/rtmp_wep.o\
	$(RT2860_DIR)/common/action.o\
	$(RT2860_DIR)/common/ba_action.o\
	$(RT2860_DIR)/common/cmm_data.o\
	$(RT2860_DIR)/common/rtmp_init.o\
	$(RT2860_DIR)/common/rtmp_tkip.o\
	$(RT2860_DIR)/common/cmm_sync.o\
	$(RT2860_DIR)/common/eeprom.o\
	$(RT2860_DIR)/common/cmm_sanity.o\
	$(RT2860_DIR)/common/cmm_info.o\
	$(RT2860_DIR)/common/cmm_wpa.o\
	$(RT2860_DIR)/common/dfs.o\
	$(RT2860_DIR)/sta/assoc.o\
	$(RT2860_DIR)/sta/aironet.o\
	$(RT2860_DIR)/sta/auth.o\
	$(RT2860_DIR)/sta/auth_rsp.o\
	$(RT2860_DIR)/sta/sync.o\
	$(RT2860_DIR)/sta/sanity.o\
	$(RT2860_DIR)/sta/rtmp_data.o\
	$(RT2860_DIR)/sta/connect.o\
	$(RT2860_DIR)/sta/wpa.o\
	$(RT2860_DIR)/os/linux/rt_linux.o\
	$(RT2860_DIR)/os/linux/rt_profile.o\
	$(RT2860_DIR)/os/linux/rt_main_dev.o\
	$(RT2860_DIR)/os/linux/sta_ioctl.o

ifeq ($(HAS_ATE),y)
RT2860_OBJ += $(RT2860_DIR)/os/linux/rt_ate.o
endif

ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
RT2860_OBJ += \
	$(RT2860_DIR)/common/cmm_mat.o \
	$(RT2860_DIR)/common/cmm_mat_iparp.o \
	$(RT2860_DIR)/common/cmm_mat_pppoe.o \
	$(RT2860_DIR)/common/cmm_mat_ipv6.o
endif

ifeq ($(HAS_WSC),y)
RT2860_OBJ += $(RT2860_DIR)/common/wsc.o\
	$(RT2860_DIR)/common/wsc_tlv.o\
	$(RT2860_DIR)/common/sha2.o\
	$(RT2860_DIR)/common/hmac.o\
	$(RT2860_DIR)/common/dh_key.o\
	$(RT2860_DIR)/common/evp_enc.o	
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
RT2860_OBJ += $(RT2860_DIR)/common/netif_block.o
endif

all:$(OBJ)

rt2860sta.o: $(RT2860_OBJ)
	$(LD) -r $^ -o $@

release:
	echo "MAKE Linux Station Code Release"

clean:
	rm -f $(RT2860_DIR)/sta/*.o
	rm -f $(RT2860_DIR)/common/*.o
	rm -f $(RT2860_DIR)/os/linux/*.o

endif #// endif of ifeq ($(RT2860_MODE),STA) //

