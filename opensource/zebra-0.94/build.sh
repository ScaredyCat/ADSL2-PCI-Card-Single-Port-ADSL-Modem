#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh

echo "topdir is $TOPDIR"

APPS_NAME="zebra-0.94"

echo "----------------------------------------------------------------------"
echo "-----------------------      build zebra-0.94 ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	make distclean
	find . -name Makefile.in | xargs rm -rf
	rm -f config.cache
	rm -f aclocal.m4
	rm -f config.h.in
	rm -f configure
	rm -rf .config_ok
	#165001:henryhsu:20050809:fix for build clean fail
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
	#165001
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	find . -name Makefile.in | xargs rm -rf
	aclocal
	autoheader
	autoconf
	automake --foreign --add-missing
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS} " IFX_LDFLAGS="${IFX_LDFLAGS}" TARGET=${TARGET} BUILD=${BUILD} HOST=${HOST} ./configure --target=${TARGET} --host=${HOST} --build=${BUILD} --prefix=/usr --disable-vtysh --disable-ipv6 --enable-ripd_standalone --disable-debug_ripd_standalone --disable-bgpd --disable-ripngd --disable-ospfd --disable-ospf6d --disable-bgp-announce --disable-bgpd --disable-ripngd --disable-ospfd --disable-ospf6d --disable-bgp-announce --disable-zebra IFX_CFLAGS="${IFX_CFLAGS} " IFX_LDFLAGS="${IFX_LDFLAGS}"
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make IFX_CFLAGS="${IFX_CFLAGS} -DVTY_REMOVE -DRIPD_MEMORY_FIX" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $? 

#make install DESTDIR=${BUILD_ROOTFS_DIR}

install -d ${BUILD_ROOTFS_DIR}usr/sbin/

cp -f ripd/ripd ${BUILD_ROOTFS_DIR}usr/sbin/
ifx_error_check $? 
