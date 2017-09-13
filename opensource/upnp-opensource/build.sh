#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="upnp-opensource"

echo "----------------------------------------------------------------------"
echo "-----------------------      build UPNP      -------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi
make clean
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all TARGET=mips-linux
ifx_error_check $? 


mkdir ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd
cp -aRf linux-igd/etc_linux-igd/gateconnSCPD.xml  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/gateconnSCPD.xml
cp -aRf linux-igd/etc_linux-igd/gatedesc.skl  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/gatedesc.skl
cp -aRf linux-igd/etc_linux-igd/gateicfgSCPD.xml  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/gateicfgSCPD.xml
cp -aRf linux-igd/etc_linux-igd/gateinfoSCPD.xml  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/gateinfoSCPD.xml
cp -aRf linux-igd/etc_linux-igd/gatelayer3forwardingSCPD.xml  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/gatelayer3forwardingSCPD.xml
cp -aRf linux-igd/etc_linux-igd/ligd.gif  ${IFX_ROOTFS_DIR}flashdisk/ramdisk_copy/etc/linux-igd/ligd.gif
cp -aRf linux-igd/upnpd ${BUILD_ROOTFS_DIR}usr/sbin
#cp -aRf igd-client/upnp_igd_ctrlpt ${BUILD_ROOTFS_DIR}usr/sbin
ifx_error_check $? 
