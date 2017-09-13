#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="sysstat"

echo "----------------------------------------------------------------------"
echo "-----------------------      build sysstat  --------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ "$1" = "config_only" ] ;then
	exit 0
fi

if [ $BUILD_CLEAN -eq 1 ]; then
	make  clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

${IFX_STRIP} mpstat 
install -m 755 mpstat ${BUILD_ROOTFS_DIR}usr/sbin/
#install -d ${BUILD_ROOTFS_DIR}usr/sbin/
#cp -f mpstat ${BUILD_ROOTFS_DIR}usr/sbin/
ifx_error_check $? 

