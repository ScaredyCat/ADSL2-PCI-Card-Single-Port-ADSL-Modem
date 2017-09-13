#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="brctl"

echo "----------------------------------------------------------------------"
echo "-----------------------      build brctl      ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make distclean
	find . -name Makefile | xargs rm -rf
	rm -rf install_sh missing mkinstalldirs autom4te.cache config.log
	rm -f config.cache
	rm -f aclocal.m4
	rm -f config.h.in
	rm -f configure
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	aclocal
	autoheader
	autoconf
	automake --foreign --add-missing
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} RANLIB=${IFX_RANLIB} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} TARGET=${TARGET} HOST=${HOST} BUILD=${BUILD} ./configure --target=${TARGET} --host=${HOST} --build=${BUILD} --prefix=/usr --with-linux=${KERNEL_SOURCE_DIR}include
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 

${IFX_STRIP} ${USER_OPENSOURCE_DIR}/bridge-utils/brctl/brctl
ifx_error_check $? 

install -d ${BUILD_ROOTFS_DIR}usr/sbin/
cp -f ${USER_OPENSOURCE_DIR}bridge-utils/brctl/brctl ${BUILD_ROOTFS_DIR}usr/sbin/.
ifx_error_check $? 
