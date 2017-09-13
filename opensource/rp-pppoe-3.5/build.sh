#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="rp-pppoe"
                                                                                                                               

display_info "----------------------------------------------------------------------"
display_info "-----------------------      build rp-pppoe   ------------------------"
display_info "----------------------------------------------------------------------"

cd src/

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	make clean
	./ifx-configure 
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" pppoe-relay
ifx_error_check $? 

${IFX_STRIP} --strip-unneeded pppoe-relay

cd -

install -d ${BUILD_ROOTFS_DIR}usr/sbin/
cp -af src/pppoe-relay ${BUILD_ROOTFS_DIR}usr/sbin/.
ifx_error_check $? 
