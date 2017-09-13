#!/bin/sh
# 512151:jelly lin:2005/12/15:add new feature "firmware upgrade"
  
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="tftp"

echo "----------------------------------------------------------------------"
echo "-----------------------      build tftp       ------------------------"
echo "----------------------------------------------------------------------"

#CONFIG_FULL_PACKAGE=y
if [ "$CONFIG_FULL_PACKAGE" == "y" ]; then

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make distclean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} ./configure --target=mips-linux --host=mips-linux --build=i386-pc-linux-gnu --prefix=/usr CFLAGS="${IFX_CFLAGS} -DIFX_TFTP_UPGRADE" LDFLAGS="${IFX_LDFLAGS} -lIFXAPIs" 
	ifx_error_check $? 
	echo -n >.config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

#make IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
make all
ifx_error_check $? 

${IFX_STRIP} tftpd/tftpd
install -d ${BUILD_ROOTFS_DIR}usr/sbin/
cp -f tftpd/tftpd ${BUILD_ROOTFS_DIR}usr/sbin/
ifx_error_check $? 

else

  install -d ${BUILD_ROOTFS_DIR}usr/sbin/
  cp -f tftpd $BUILD_ROOTFS_DIR/usr/sbin/

fi
