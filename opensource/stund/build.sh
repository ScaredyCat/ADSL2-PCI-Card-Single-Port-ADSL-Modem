#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="stun-0.96"

echo "----------------------------------------------------------------------"
echo "-----------------------      build STUN client 0.96 ------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
fi

ifx_error_check $? 
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" KERNEL_DIR=${KERNEL_SOURCE_DIR} client libstun.a
ifx_error_check $? 

${IFX_STRIP} client 

#make -C src install INSTDIR=${BUILD_ROOTFS_DIR}usr/sbin/.
cp -f client ${BUILD_ROOTFS_DIR}usr/sbin/stun_client
cp -f ${TOOLCHAIN_DIR}../lib/libstdc++.so.5.0.7 ${BUILD_ROOTFS_DIR}usr/lib/libstdc++.so.5
cp -f ${TOOLCHAIN_DIR}../lib/libgcc_s.so.1 ${BUILD_ROOTFS_DIR}usr/lib/libgcc_s.so.1
ifx_error_check $? 
