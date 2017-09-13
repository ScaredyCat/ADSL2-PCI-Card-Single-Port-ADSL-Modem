#!/bin/sh
#000005:tc.chen 2005/06/17 fix hostname does not work fine issue.
TOPDIR=`pwd`/../../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="sip 1.6.9"

echo "----------------------------------------------------------------------"
echo "-----------------------      build sip-1.6.9  ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ "$1" = "config_only" ] ;then
	exit 0
fi

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi


make  AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $? 

#chmod 777 ../sample/sipua/sipUa

#${IFX_STRIP} ./Linux/lib/libcclsip.a

#cp -af ../sample/sipua/sipUa ${BUILD_ROOTFS_DIR}usr/sbin/sipUa
cp -af ./Linux/lib/libcclsip.a ${IFX_APIS_DIR}lib/libcclsip.a
ifx_error_check $? 
