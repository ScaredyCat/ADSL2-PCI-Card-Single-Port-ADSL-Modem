#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
. ${TOPDIR}tools/build_tools/config.sh
APPS_NAME="snmp-5.1"

echo "----------------------------------------------------------------------"
echo "----------------------- Build net snmp-5.1     ------------------------"
echo "----------------------------------------------------------------------"

#CONFIG_FULL_PACKAGE=y
if [ "$CONFIG_FULL_PACKAGE" == "y" ]; then

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -rf .config_ok
	rm -f ${BUILD_ROOTFS_DIR}usr/lib/libnetsnmp*
	rm -f ${BUILD_ROOTFS_DIR}usr/sbin/snmp*
	make distclean
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

#export IFX_CFLAGS TARGET HOST BUILD IFX_LDFLAGS
	# Added by Subbi for ATM-MIB support in User space */
	if [ "$IFX_CONFIG_SNMP_ATM_MIB" = "1" ]; then
		IFX_CONFIG_MIB_MODULES="${IFX_CONFIG_MIB_MODULES} atmMIB"
		IFX_CFLAGS="${IFX_CFLAGS} -DIFX_CONFIG_SNMP_ATM_MIB" 
	fi

	# Added by Subbi for ADSL-MIB support in User space */
	if [ "$IFX_CONFIG_SNMP_ADSL_MIB" = "1" ]; then
		IFX_CONFIG_MIB_MODULES="${IFX_CONFIG_MIB_MODULES} adslMIB"
		IFX_CFLAGS="${IFX_CFLAGS} -DIFX_CONFIG_SNMP_ADSL_MIB" 
	fi
	
	#Added by Subbi for SNMP Transports in SNMP
	if [ "$IFX_SNMP_TRANSPORT_ATMPVC_DOMAIN" = "1" ]; then
		if [ "$IFX_SNMP_TRANSPORT_EOC_DOMAIN" = "1" ]; then
			IFX_CONFIG_SNMP_TRANSPORTS="${IFX_CONFIG_SNMP_TRANSPORTS} AAL5PVC EOC"
		else  
			IFX_CONFIG_SNMP_TRANSPORTS="${IFX_CONFIG_SNMP_TRANSPORTS} AAL5PVC"
		fi
	fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	make distclean
	if [ "$BUILD_2MB_PACKAGE" = "1" ]; then
		echo "Configuring for 2MB pkg"
		cp -f configure_2MB_pkg current_configuration
		cp -f configure_2MB configure
		cp -f Makefile.rules.2MB_pkg Makefile.rules
	elif [ "$IFX_CONFIG_SNMPv1" = "1" ]; then
		echo "Configuring for SNMPv1"
		cp -f configure_snmpv1_pkg current_configuration
		cp -f configure_full configure
		cp -f Makefile.rules.full Makefile.rules
	elif [ "$IFX_CONFIG_SNMPv3" = "1" ]; then
		echo "Configuring for SNMPv3"
		cp -f configure_snmpv3_pkg current_configuration
		cp -f configure_full configure
		cp -f Makefile.rules.full Makefile.rules 
		IFX_CFLAGS="${IFX_CFLAGS} -DIFX_CONFIG_SNMPv3" 
	fi

	chmod a+x current_configuration
	AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC}  RANLIB=${IFX_RANLIB} BUILDCC=${IFX_HOSTCC} CXX=${IFX_CXX} OBJCOPY=${IFX_OBJCOPY} OBJDUMP=${IFX_OBJDUMP} IFX_CFLAGS={IFX_CFLAGS} IFX_LDFLAGS={IFX_LDFLAGS} TARGET=${TARGET} HOST=${HOST} BUILD=${BUILD} BUILD_ROOTFS_DIR=${BUILD_ROOTFS_DIR} IFX_CONFIG_SNMP_TRANSPORTS=${IFX_CONFIG_SNMP_TRANSPORTS} IFX_CONFIG_MIB_MODULES=${IFX_CONFIG_MIB_MODULES} ./current_configuration

	ifx_error_check $? 
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ;then
	exit 0
fi

if [ "$BUILD_2MB_PACKAGE" = "1" ]; then
	IFX_CFLAGS="${IFX_CFLAGS} -DHAVE_MINIMAL_HELPERS -DHAVE_MINIMAL_LIBSUPPORT -DHAVE_OPTIMIZED_CODE"
else 
	IFX_CFLAGS="${IFX_CFLAGS}"
fi

#chandrav, for ilmid
if [ "$IFX_CONFIG_TR037" = "1" ]; then
        IFX_CFLAGS="${IFX_CFLAGS} -DIFX_CONFIG_TR037"
fi

make IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $? 

${IFX_STRIP} -R.note -R.comment ${BUILD_ROOTFS_DIR}usr/sbin/snmp*

if [ "$BUILD_2MB_PACKAGE" = "1" ]; then
	install -d $BUILD_ROOTFS_DIR/usr/sbin/
	cp -f snmpd $BUILD_ROOTFS_DIR/usr/sbin/
else
	make -C agent install  IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}"  
	ifx_error_check $? 

	make -C snmplib install IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" 
	ifx_error_check $?

	rm -f ${BUILD_ROOTFS_DIR}usr/lib/libnetsnmp*.a
	rm -f ${BUILD_ROOTFS_DIR}usr/lib/libnetsnmp*.la
	${IFX_STRIP} --strip-unneeded ${BUILD_ROOTFS_DIR}usr/lib/libnetsnmp*
fi

#610052:hsur add code for create cd image
else
echo "Copying snmpd"
cp -f ./snmpd  $BUILD_ROOTFS_DIR/usr/sbin/
cp -f ./libnetsnmpagent.so.5.1.0 $BUILD_ROOTFS_DIR/usr/lib/libnetsnmpagent.so.5
cp -f ./libnetsnmpmibs.so.5.1.0 $BUILD_ROOTFS_DIR/usr/lib/libnetsnmpmibs.so.5
cp -f ./libnetsnmphelpers.so.5.1.0 $BUILD_ROOTFS_DIR/usr/lib/libnetsnmphelpers.so.5
cp -f ./libnetsnmp.so.5.1.0 $BUILD_ROOTFS_DIR/usr/lib/libnetsnmp.so.5

fi
