#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="OpenAgent2.0"

echo "----------------------------------------------------------------------"
echo "-----------------------     OpenAgent2.0      ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" ] ; then
    exit $?
fi

make clean
make
ifx_error_check $? 
cp -af src/agent ${BUILD_ROOTFS_DIR}usr/sbin/agent
cd src/CLI/
make clean
make test
ifx_error_check $? 
cp -af tr069 ${BUILD_ROOTFS_DIR}usr/sbin/tr069
cd -
cd conf/Dllso_example/
make clean
make
ifx_error_check $? 
cp -af libdev.so ${BUILD_ROOTFS_DIR}usr/lib/libdev.so
cd - 
mkdir -p ${BUILD_ROOTFS_DIR}ramdisk_copy/etc/conf
cp -af conf/param.xml ${BUILD_ROOTFS_DIR}ramdisk_copy/etc/conf/param.xml
cd -