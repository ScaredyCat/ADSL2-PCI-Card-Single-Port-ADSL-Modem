#!/bin/sh
#000005:tc.chen 2005/06/17 fix hostname does not work fine issue.
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="dnrd-2.19"

echo "----------------------------------------------------------------------"
echo "-----------------------      build dnrd-2.19  ------------------------"
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
	find . -name Makefile.in | xargs rm -rf
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	aclocal
	autoheader
	autoconf
	automake --foreign --add-missing
	# 000005:tc.chen AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC}  RANLIB=${IFX_RANLIB} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} IFX_CFLAGS={IFX_CFLAGS} IFX_LDFLAGS={IFX_LDFLAGS}  ./configure --host=${HOST} --build=${BUILD} --target=${TARGET} --disable-master
	#000005:tc.chen start : remove disable-master option
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC}  RANLIB=${IFX_RANLIB} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} IFX_CFLAGS={IFX_CFLAGS} IFX_LDFLAGS={IFX_LDFLAGS}  ./configure --host=${HOST} --build=${BUILD} --target=${TARGET} 
	#000005:tc.chen end

	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

make
ifx_error_check $? 

${IFX_STRIP} src/dnrd

#make -C src install INSTDIR=${BUILD_ROOTFS_DIR}usr/sbin/.
cp -f src/dnrd ${BUILD_ROOTFS_DIR}usr/sbin/
ifx_error_check $? 
