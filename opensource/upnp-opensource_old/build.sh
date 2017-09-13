#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="upnp-opensource"

echo "----------------------------------------------------------------------"
echo "-----------------------      build UPNP      -------------------------"
echo "----------------------------------------------------------------------"

cd libupnp-1.2.1/upnp/

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	make clean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	make clean
	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi
make clean
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all TARGET=mips-linux CLIENT=0
ifx_error_check $? 

cp -vf ./bin/mips-linux/* ${TOOLCHAIN_DIR}/lib

cp -vf ./bin/mips-linux/* ${BUILD_ROOTFS_DIR}/lib

ifx_error_check $? 

cd -

cd IGD/

parse_args $@

#if [ $BUILD_CLEAN -eq 1 ]; then
#	rm -rf .config_ok
	make clean
#	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
#fi

#if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
#	make clean
#	ifx_error_check $? 
#	echo -n > .config_ok
#fi

#if [ "$1" = "config_only" ] ;then
#	exit 0
#fi


make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all TARGET=mips-linux CLIENT=0
# make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"
ifx_error_check $? 
mkdir ${RAMDISK_DIR}etc/linuxigd
#cp -vf etc/gatedesc.xml ${BUILD_ROOTFS_DIR}etc/linuxigd
cp -vf ./etc/*  ${RAMDISK_DIR}etc/linuxigd
#cp -vf etc/gateconnSCPD.xml  ${BUILD_ROOTFS_DIR}etc/linuxigd
#cp -vf etc/gateicfgSCPD.xml ${BUILD_ROOTFS_DIR}etc/linuxigd
#cp -vf etc/ligd.gif ${BUILD_ROOTFS_DIR}etc/linuxigd
cp -vf upnpd ${BUILD_ROOTFS_DIR}usr/sbin
ifx_error_check $? 

cd -
