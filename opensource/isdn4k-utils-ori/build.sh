#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="isdn4k-utils"

echo "----------------------------------------------------------------------"
echo "-----------------------       build isdn4k-utils      ----------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make clean
fi

make ar=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} ranlib=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

${IFX_STRIP} ./isdnctrl/isdnctrl ./hisax/hisaxctrl ./divertctrl/divertctrl

cp -af ./isdnctrl/isdnctrl ${BUILD_ROOTFS_DIR}usr/sbin/isdnctrl
cp -af ./hisax/hisaxctrl ${BUILD_ROOTFS_DIR}usr/sbin/hisaxctrl
cp -af ./divertctrl/divertctrl ${BUILD_ROOTFS_DIR}usr/sbin/divertctrl
#ifx_error_check $? 
