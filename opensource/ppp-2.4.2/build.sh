#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="ppp-2.4.2"

echo "----------------------------------------------------------------------"
echo "-----------------------      build ppp-2.4.2  ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make dist-clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	. ./configure
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

IFX_CFLAGS="${IFX_CFLAGS} -DIFX_SMALL_FOOTPRINT"
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" IFX_2MB_PPP_PKG=1 install DESTDIR=${BUILD_ROOTFS_DIR}
ifx_error_check $? 

#make install DESTDIR=${BUILD_ROOTFS_DIR}
   VERSION=2.4.2
   install -d $BUILD_ROOTFS_DIR/usr/sbin/
   #install -d  $BUILD_ROOTFS_DIR/usr/lib/
   install -d $BUILD_ROOTFS_DIR/usr/lib/pppd/$VERSION
   cp -f ./pppd/pppd $BUILD_ROOTFS_DIR/usr/sbin/
   #cp -f pppd $BUILD_ROOTFS_DIR/usr/lib/
   cp -f ./pppd/plugins/pppoatm/pppoatm.so $BUILD_ROOTFS_DIR/usr/lib/pppd/$VERSION
   cp -f ./pppd/plugins/rp-pppoe/rp-pppoe.so $BUILD_ROOTFS_DIR/usr/lib/pppd/$VERSION
ifx_error_check $? 

