#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="no-ip"

echo "----------------------------------------------------------------------"
echo "-----------------------       build noip-2.1.3        ----------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
fi

if [ "$1" = "config_only" ] ; then
    exit $?
fi
make clean
make CC=${IFX_CC} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

${IFX_STRIP} noip2
cp -af ./noip2 ${BUILD_ROOTFS_DIR}usr/sbin/noip2
chmod 700 ${BUILD_ROOTFS_DIR}usr/sbin/noip2
chown root:root ${BUILD_ROOTFS_DIR}usr/sbin/noip2
ifx_error_check $? 
