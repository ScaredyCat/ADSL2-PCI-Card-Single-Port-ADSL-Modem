#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
. ${TOPDIR}tools/build_tools/config.sh
APPS_NAME="linux-atm-2.4.1"

echo "----------------------------------------------------------------------"
echo "----------------------- build linux-atm-2.4.1 ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make distclean
	rm -f missing mkinstalldirs ylwrap depcomp install_sh
	rm -f config.cache
	rm -f aclocal.m4
	rm -f config.h.in
	rm -f configure
	rm -f ltmain.sh
	rm -rf autom4te.cache/
	find . -name Makefile.in | xargs rm -rf
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	if [ -f /usr/share/libtool/ltmain.sh ]; then
		cp /usr/share/libtool/ltmain.sh .
	else
		cp ltmain.sh.org ltmain.sh
	fi
	# 608071:fchang.removed
	# aclocal
	# 608071:fchang.added
	aclocal-1.6

	autoheader
	autoconf
	# 608071:fchang.removed
	# automake --foreign --add-missing
	# 608071:fchang.added
	automake-1.6 --foreign --add-missing

	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} RANLIB=${IFX_RANLIB} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} IFX_LDFLAGS='${IFX_LDFLAGS} -nostdlib' IFX_CFLAGS=${IFX_CFLAGS} TARGET=${TARGET} BUILD=${BUILD} HOST=${HOST} ./configure --target=${TARGET} --host=${HOST} --build=${BUILD} --prefix=/usr --enable-ifx_opt_for_small_size
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make -C src/lib/ install DESTDIR=${BUILD_ROOTFS_DIR} libdir=/lib IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

if [ $IFX_CONFIG_CLIP -a $IFX_CONFIG_CLIP = "1" ]; then
	make -C src/arpd  DESTDIR=${BUILD_ROOTFS_DIR} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
	ifx_error_check $? 

	make -C src/maint atmaddr IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
	ifx_error_check $? 

	${IFX_STRIP} src/arpd/.libs/atmarpd
	ifx_error_check $? 

	cp -f src/arpd/.libs/atmarpd ${BUILD_ROOTFS_DIR}usr/sbin/
	ifx_error_check $? 

	${IFX_STRIP} src/arpd/.libs/atmarp
	ifx_error_check $? 

	cp -f src/arpd/.libs/atmarp ${BUILD_ROOTFS_DIR}usr/sbin/
	ifx_error_check $? 

	${IFX_STRIP} src/maint/.libs/atmaddr
	ifx_error_check $? 

	cp -f src/maint/.libs/atmaddr ${BUILD_ROOTFS_DIR}usr/sbin/
	ifx_error_check $? 
fi

