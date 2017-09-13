#!/bin/sh
TOPDIR=`pwd`/../../../../
. ${TOPDIR}tools/build_tools/Path.sh
. ${TOPDIR}tools/build_tools/config.sh
APPS_NAME="busybox"

					

create_busybox_config()
{
	cp -f ifx_busybox_config .config
	chmod u+w .config
#608241:hsur add openswan code 	
	if [ "${IFX_CONFIG_OPENSWAN}" == 1 ]; then    
	    cp -f ifx_busybox_config_ipsec .config
    	    chmod u+w .config	    
	fi

	# append options to config file
	if [ $IFX_CONFIG_WIRELESS -eq 1 ]; then
		echo "CONFIG_IWCONFIG=y" >> .config
	else
		echo "# CONFIG_IWCONFIG is not set" >> .config
	fi
	if [ "$IFX_CONFIG_INETD" = "1" ]; then
		echo "CONFIG_INETD=y" >> .config
		echo "# CONFIG_FEATURE_INETD_SUPPORT_BILTIN_ECHO is not set" >> .config
		echo "# CONFIG_FEATURE_INETD_SUPPORT_BILTIN_DISCARD is not set" >> .config
		echo "# CONFIG_FEATURE_INETD_SUPPORT_BILTIN_TIME is not set" >> .config
		echo "# CONFIG_FEATURE_INETD_SUPPORT_BILTIN_DAYTIME is not set" >> .config
		echo "# CONFIG_FEATURE_INETD_SUPPORT_BILTIN_CHARGEN is not set" >> .config
	else
		echo "# CONFIG_INETD is not set" >> .config
	fi
	if [ "$IFX_CONFIG_TELNET_SERVER" = "1" ]; then
		echo "CONFIG_TELNETD=y" >> .config
		echo "CONFIG_FEATURE_TELNETD_INACTIVE_TIMEOUT=y" >> .config
		if [ "$IFX_CONFIG_INETD" = "1" ]; then
			echo "CONFIG_FEATURE_TELNETD_INETD=y" >> .config
		else
			echo "# CONFIG_FEATURE_TELNETD_INETD is not set" >> .config
		fi
	else
		echo "# CONFIG_TELNETD is not set" >> .config
	fi

	if [ "$IFX_CONFIG_DHCP_SERVER" = "1" ]; then
		echo "CONFIG_UDHCPD=y" >> .config
	else
		echo "# CONFIG_UDHCPD is not set" >> .config
	fi

	if [ "$IFX_CONFIG_DHCP_CLIENT" = "1" ]; then
		echo "CONFIG_UDHCPC=y" >> .config
	else
		echo "# CONFIG_UDHCPC is not set" >> .config
	fi

	if [ "$IFX_CONFIG_SYSTEM_LOG" = "1" ]; then
		echo "CONFIG_SYSLOGD=y" >> .config
		echo "CONFIG_FEATURE_ROTATE_LOGFILE=y" >> .config
		echo "# CONFIG_FEATURE_REMOTE_LOG is not set" >> .config
		echo "CONFIG_FEATURE_IPC_SYSLOG=y" >> .config
		echo "CONFIG_FEATURE_IPC_SYSLOG_BUFFER_SIZE=16" >> .config
		echo "CONFIG_LOGREAD=y" >> .config
		echo "CONFIG_FEATURE_LOGREAD_REDUCED_LOCKING=y" >> .config
		echo "CONFIG_KLOGD=y" >> .config
		echo "CONFIG_LOGGER=y" >> .config
		echo "CONFIG_FEATURE_UDHCP_SYSLOG=y" >> .config

	else
		echo "# CONFIG_SYSLOGD is not set" >> .config
		echo "# CONFIG_LOGGER is not set" >> .config
		echo "# CONFIG_FEATURE_UDHCP_SYSLOG is not set" >> .config
	fi
}

#--------------- main  function ----------------------------

echo "----------------------------------------------------------------------"
echo "-----------------------      build busybox    ------------------------"
echo "----------------------------------------------------------------------"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
	rm -f libiw.a
	make distclean
	rm -f .config
	rm -f .config_ok 
	[ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ "$1" = "config_only" -a ! -f .config_ok -o $BUILD_CONFIGURE -eq 1 ]; then
	ln -sf ../wireless_tools.27/libiw.a .
	create_busybox_config
	make oldconfig 
	ifx_error_check $?
	echo -n > .config_ok
fi

if [ "$1" = "config_only" ] ; then
	exit 0
fi

make AR=${IFX_AR} AS=${IFX_AS} LD=${IFX_LD} NM=${IFX_NM} CC=${IFX_CC} BUILDCC=${IFX_HOSTCC} GCC=${IFX_CC} CXX=${IFX_CXX} CPP=${IFX_CPP} RANLIB=${IFX_RANLIB} IFX_CFLAGS="${IFX_CFLAGS}" IFX_LDFLAGS="${IFX_LDFLAGS}" all
ifx_error_check $? 

make PREFIX=${BUILD_ROOTFS_DIR} install 
ifx_error_check $? 
