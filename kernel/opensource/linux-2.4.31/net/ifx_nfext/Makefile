#
# Makefile for the netlink driver.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

#CONFIG_FULL_PACKAGE=y

ifeq ($(CONFIG_FULL_PACKAGE),y)
export-objs = ifx_nfext_core.o

obj-y =
obj-m =

obj-$(CONFIG_NET_IFX_EXTENSION) += ifx_nfext_core.o
obj-$(CONFIG_PPP_IFX_IDLETIME_EXTENSION) += ifx_nfext_ppp.o
obj-$(CONFIG_IFX_NFEXT_SWITCH_PHYPORT) += ifx_nfext_sw_phyport.o
obj-$(CONFIG_IFX_NFEXT_VBRIDGE) += ifx_nfext_vbridge.o

include $(TOPDIR)/Rules.make

else

obj-y =
obj-m =

# 165001:henryhsu:20050818:Modify for release build fail.
obj-$(CONFIG_NET_IFX_EXTENSION) += ifx_nfext_core.o
obj-$(CONFIG_PPP_IFX_IDLETIME_EXTENSION) += ifx_nfext_ppp.o
obj-$(CONFIG_IFX_NFEXT_SWITCH_PHYPORT) += ifx_nfext_sw_phyport.o
obj-$(CONFIG_IFX_NFEXT_VBRIDGE) += ifx_nfext_vbridge.o
#export-objs = ifx_nfext_core.o
# 165001



include $(TOPDIR)/Rules.make

ifx_nfext_core.o : ifx_nfext_core.oo
	cp -f ifx_nfext_core.oo ifx_nfext_core.o

ifx_nfext_ppp.o : ifx_nfext_ppp.oo
	cp -f ifx_nfext_ppp.oo ifx_nfext_ppp.o

ifx_nfext_sw_phyport.o : ifx_nfext_sw_phyport.oo
	cp -f ifx_nfext_sw_phyport.oo ifx_nfext_sw_phyport.o

ifx_nfext_vbridge.o : ifx_nfext_vbridge.oo
	cp -f ifx_nfext_vbridge.oo ifx_nfext_vbridge.o


# 000008:jelly:6/30:For new create_customer_CDs.sh
#obj-$(CONFIG_NET_IFX_EXTENSION) += ifx_nfext_core.o
#obj-$(CONFIG_PPP_IFX_IDLETIME_EXTENSION) += ifx_nfext_ppp.o
#obj-$(CONFIG_IFX_NFEXT_DANUBE_SWITCH_PHYPORT) += ifx_nfext_sw_phyport.o
#obj-$(CONFIG_IFX_NFEXT_VBRIDGE) += ifx_nfext_vbridge.o
#export-objs = ifx_nfext_core.o
# 000008:end

endif

