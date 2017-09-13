#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
. ${TOPDIR}tools/build_tools/config.sh
APPS_NAME="iproute2"

echo "----------------------------------------------------------------------"
echo "-----------------------    build iproute2         --------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" KERNEL_INCLUDE=${KERNEL_SOURCE_DIR}/include 
ifx_error_check $? 

${IFX_STRIP} tc/tc
install -d ${BUILD_ROOTFS_DIR}usr/sbin/
cp -f tc/tc ${BUILD_ROOTFS_DIR}usr/sbin/

#608241:hsur add openswan code
if [ "${IFX_CONFIG_OPENSWAN}" == 1 ]; then	
   ${IFX_STRIP} ip/ip	
   cp ip/ip  ${BUILD_ROOTFS_DIR}usr/sbin/ip
fi       	
ifx_error_check $? 

