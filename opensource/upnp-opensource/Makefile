#include ../../Makefile.inc

#TARGET=$(TARGET_DIR)/

CROSS_COMPILE_PREFIX=mips-linux-

CROSS_TOOL_HEAD=mips-linux

SBIN_DIR=usr/sbin
ETC_DIR=etc
UPNP_DIR=/etc/linuxigd
VAR_TMP_DIR=var/tmp
export CROSS_COMPILE_PREFIX

#all: build install
all: build

test:
	$(MAKE) -C ./linux-igd

build:
	@if [ ! -f "./igd-client/libixml.a" ] || [ ! -f "./igd-client/libthreadutil.a" ] || [ ! -f "./igd-client/libupnp.a" ]; then \
		$(MAKE) -C ./libupnp-1.2.1 clean; \
		$(MAKE) -C ./libupnp-1.2.1/upnp  WEB=0 CLIENT=1 STATIC=1 TARGET=$(CROSS_TOOL_HEAD); \
		cp ./libupnp-1.2.1/ixml/src/libixml.a ./igd-client; \
		cp ./libupnp-1.2.1/threadutil/src/libthreadutil.a ./igd-client; \
		cp ./libupnp-1.2.1/upnp/bin/libupnp.a ./igd-client; \
  fi
	$(MAKE) -C ./igd-client TARGET=$(CROSS_TOOL_HEAD)
	@if [ ! -f "./linux-igd/libixml.a" ] || [ ! -f "./linux-igd/libthreadutil.a" ] || [ ! -f "./linux-igd/libupnp.a" ]; then \
		$(MAKE) -C ./libupnp-1.2.1 clean; \
		$(MAKE) -C ./libupnp-1.2.1/upnp  WEB=0 CLIENT=0 STATIC=1 TARGET=$(CROSS_TOOL_HEAD); \
		cp ./libupnp-1.2.1/ixml/src/libixml.a ./linux-igd; \
		cp ./libupnp-1.2.1/threadutil/src/libthreadutil.a ./linux-igd; \
		cp ./libupnp-1.2.1/upnp/bin/libupnp.a ./linux-igd; \
	fi 
	ln -sf ../libupnp-1.2.1/upnp/inc/ ./linux-igd/upnp
	$(MAKE) -C ./linux-igd
	$(CROSS_COMPILE_PREFIX)strip igd-client/upnp_igd_ctrlpt
	$(CROSS_COMPILE_PREFIX)strip linux-igd/upnpd
	
install:
	$(CROSS_COMPILE_PREFIX)strip igd-client/upnp_igd_ctrlpt
	install -d $(TARGET)$(SBIN_DIR)
	install -d $(TARGET)$(ETC_DIR)
	install -m 0755 igd-client/upnp_igd_ctrlpt $(TARGET)$(SBIN_DIR)
	$(CROSS_COMPILE_PREFIX)strip linux-igd/upnpd
	install -d $(TARGET)$(SBIN_DIR)
	install -d $(TARGET)$(ETC_DIR)
	install -d $(TARGET)$(UPNP_DIR)/linux-igd
	install -m 0755 linux-igd/upnpd $(TARGET)$(SBIN_DIR)
	install -m 0664 linux-igd/etc_linux-igd/gate* $(TARGET)$(UPNP_DIR)/linux-igd
	install -m 0664 linux-igd/etc_linux-igd/*.gif $(TARGET)$(UPNP_DIR)/linux-igd
	
clean:
	$(MAKE) -C ./libupnp-1.2.1 clean
	$(MAKE) -C ./igd-client clean
	@if [ -f "./igd-client/upnp_igd_ctrlpt" ]; then rm ./igd-client/upnp_igd_ctrlpt; fi
	$(MAKE) -C ./linux-igd clean
	@if [ -f "./linux-igd/upnpd" ]; then rm ./linux-igd/upnpd; fi
	@rm -rf ./linux-igd/upnp/inc

distclean: clean
	
dist: install

