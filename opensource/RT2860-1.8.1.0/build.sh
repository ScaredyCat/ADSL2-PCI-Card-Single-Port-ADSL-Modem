#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
APPS_NAME="RT2860"

echo "----------------------------------------------------------------------"
echo "-----------------------       build RT2860            ----------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
  cd 802.1X
	make clean
	cd ..
	cd WSC_UPNP
	make clean
	cd ..
	cd Module
	make clean
	cd ..
	
fi

cd 802.1X
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $? 
cd ..
cd Module
#make 
make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $?
cd ..
cd WSC_UPNP/libupnp-1.3.1
./configure --host=${TARGET} CC=${IFX_CC} CFLAGS="${IFX_CFLAGS}" LDFLAGS="${IFX_LDFLAGS}"
make 
cd ..
make
ifx_error_check $? 
cd ..

cp -af 802.1X/rt2860apd ${BUILD_ROOTFS_DIR}usr/sbin/rt2860apd
cp -af Module/os/linux/rt2860ap.o ${BUILD_ROOTFS_DIR}usr/lib/rt2860ap.o
cp -af WSC_UPNP/wscd ${BUILD_ROOTFS_DIR}bin/wscd
cp -af WSC_UPNP/xml ${BUILD_ROOTFS_DIR}etc/xml
ifx_error_check $? 
