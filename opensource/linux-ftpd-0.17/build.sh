#!/bin/sh
# 512151:jelly lin:2005/12/15:add new feature "firmware upgrade", check IFX_FTP_UPGRADE 
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="ftpd"

echo "----------------------------------------------------------------------"
echo "-----------------------    build ftpd         --------------------"
echo "----------------------------------------------------------------------"

#CONFIG_FULL_PACKAGE=y
if [ "$CONFIG_FULL_PACKAGE" == "y" ]; then

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	IFX_CFLAGS="${IFX_CFLAGS} -DIFX_SMALL_FOOTPRINT -DIFX_FTP_UPGRADE"
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" ./configure --with-c-compiler=mips-linux-gcc --disable-shadow --disable-libinet6 --disable-ipv6
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" make all
ifx_error_check $? 

AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" INSTALLROOT=${BUILD_ROOTFS_DIR} make install
ifx_error_check $? 

else

  install -d $BUILD_ROOTFS_DIR/usr/sbin/
  cp -f ftpd $BUILD_ROOTFS_DIR/usr/sbin/
   
fi
