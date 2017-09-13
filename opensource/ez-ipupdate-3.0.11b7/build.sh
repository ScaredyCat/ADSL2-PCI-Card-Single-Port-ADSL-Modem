#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="ez-ipupdate"

echo "----------------------------------------------------------------------"
echo "-----------------------    build ez-ipupdate-3.0.11b7 ----------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make disclean
	rm -f config.cache
	rm -f aclocal.m4
	rm -f config.h.in
	rm -f configure
	find . -name Makefile.in | xargs rm -rf
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
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
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} RANLIB=${IFX_RANLIB} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} ./configure --target=${TARGET} --host=${HOST} --build=${BUILD} --prefix=/usr ./configure --target=${TARGET} --host=${HOST} --build=${BUILD} --prefix=/usr
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ; then
    exit $?
fi

make IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

${IFX_STRIP} ez-ipupdate

make install DESTDIR=${BUILD_ROOTFS_DIR}
ifx_error_check $? 
