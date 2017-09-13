#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
parse_args $@
APPS_NAME="wireless_tools"

echo "----------------------------------------------------------------------"
echo "-----------------------      build brctl      ------------------------"
echo "----------------------------------------------------------------------"

if [ "$1" = "config_only" ] ;then
	exit 0
fi

if [ $BUILD_CLEAN -eq 1 -o $BUILD_CONFIGURE -eq 1 ] ;then
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

make IFX_CFLAGS="${IFX_CFLAGS}"
ifx_error_check $? 

install -d ${BUILD_ROOTFS_DIR}usr/sbin/

