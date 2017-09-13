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
#	cd WSC_UPNP
#	make clean
#	cd ..
	cd Module
	make clean
	cd ..
fi

cd 802.1X
make 
ifx_error_check $? 
cd ..
cd Module
make clean
make 
ifx_error_check $? 
cd ..
#cd WSC_UPNP
#make 
#ifx_error_check $? 
#cd ..

cp -af ./802.1X/rt2860apd ${BUILD_ROOTFS_DIR}usr/sbin/rt2860apd
cp -af ./Module/os/linux/rt2860ap.o ${BUILD_ROOTFS_DIR}usr/lib/rt2860ap.o
ifx_error_check $? 
