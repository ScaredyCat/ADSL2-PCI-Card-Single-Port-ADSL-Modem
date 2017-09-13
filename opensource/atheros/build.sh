#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
. ${TOPDIR}tools/build_tools/config.sh
APPS_NAME="atheros"

export KERNELPATH=${TOPDIR}/source/kernel/opensource/linux-2.4.31/

echo "----------------------------------------------------------------------"
echo "--------------------- build ATHEROS DRIVER----------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
	rm -rf `find ./ -name .tmp_versions`
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} STRIP=${IFX_STRIP} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $?
./cpdriver11n
cd release/wireless_demo 
${IFX_STRIP}    --strip-unneeded * 
chmod 777 ./
chown root:root ./
cd -
# cp -af dfs/ath_dfs.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/ath_pci.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/ath_hal.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/ath_rate_atheros.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_xauth.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_ccmp.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_wep.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_tkip.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_scan_ap.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_scan_sta.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlan_xauth.o ${BUILD_ROOTFS_DIR}usr/
cp -af release/wireless_demo/wlanconfig ${BUILD_ROOTFS_DIR}usr/sbin/
cp -af hostapd/hostapd ${BUILD_ROOTFS_DIR}usr/sbin/
cp -af hostapd.conf ${BUILD_ROOTFS_DIR}ramdisk_copy/etc/

#cp -r release/wireless_demo/ ${BUILD_ROOTFS_DIR}usr/
