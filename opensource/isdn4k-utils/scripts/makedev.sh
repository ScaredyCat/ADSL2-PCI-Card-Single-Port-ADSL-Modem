#!/bin/sh
#
# $Id: makedev.sh,v 1.5 2000/02/24 13:29:26 paul Exp $
#
# This script creates all ISDN devices under /dev .
# Many/most distributions don't include these devices.

if [ "`id | grep uid=0`" = "" ]; then
	echo "In order to create device inodes, you must run this script as root."
	exit 1
fi
echo -e "Creating device inodes ... \c"

if [ $# = 1 ] ; then
	DEV=$1/dev
else
	DEV=/dev
fi

MAJ=45
MIN=0
rm -f $DEV/isdnctrl* $DEV/ippp*
while [ $MIN -lt 64 ] ; do
	mknod -m 600 $DEV/isdn$MIN c $MAJ $MIN
	mknod -m 660 $DEV/isdnctrl$MIN c $MAJ `expr $MIN + 64`
	mknod -m 600 $DEV/ippp$MIN c $MAJ `expr $MIN + 128`
	MIN=`expr $MIN + 1`
done
if   grep '^pppusers:' /etc/group > /dev/null
then	# RH 5 standard
	chgrp pppusers $DEV/isdnctrl* $DEV/ippp*
elif grep '^dialout:' /etc/group > /dev/null
then	# Debian/SuSE standard
	chgrp dialout $DEV/isdnctrl* $DEV/ippp*
fi
rm -f $DEV/isdninfo
mknod -m 444 $DEV/isdninfo c $MAJ 255
ln -sf isdnctrl0 $DEV/isdnctrl

MAJ=43
MIN=0
rm -f $DEV/ttyI*
while [ $MIN -lt 64 ] ; do
	mknod -m 666 $DEV/ttyI$MIN c $MAJ $MIN
	MIN=`expr $MIN + 1`
done

MAJ=44
MIN=0
rm -f $DEV/cui*
while [ $MIN -lt 64 ] ; do
	mknod -m 666 $DEV/cui$MIN c $MAJ $MIN
	MIN=`expr $MIN + 1`
done

if grep '^dialout:' /etc/group > /dev/null
then	# Debian/SuSE standard
	chgrp dialout $DEV/ttyI* $DEV/cui*
fi

MAJ=68
MIN=0
rm -f $DEV/capi20*
mknod -m 666 $DEV/capi20 c $MAJ 0
mknod -m 666 $DEV/capi20.00 c $MAJ 1
mknod -m 666 $DEV/capi20.01 c $MAJ 2
mknod -m 666 $DEV/capi20.02 c $MAJ 3
mknod -m 666 $DEV/capi20.03 c $MAJ 4
mknod -m 666 $DEV/capi20.04 c $MAJ 5
mknod -m 666 $DEV/capi20.05 c $MAJ 6
mknod -m 666 $DEV/capi20.06 c $MAJ 7
mknod -m 666 $DEV/capi20.07 c $MAJ 8
mknod -m 666 $DEV/capi20.08 c $MAJ 9
mknod -m 666 $DEV/capi20.09 c $MAJ 10
mknod -m 666 $DEV/capi20.10 c $MAJ 11
mknod -m 666 $DEV/capi20.11 c $MAJ 12
mknod -m 666 $DEV/capi20.12 c $MAJ 13
mknod -m 666 $DEV/capi20.13 c $MAJ 14
mknod -m 666 $DEV/capi20.14 c $MAJ 15
mknod -m 666 $DEV/capi20.15 c $MAJ 16
mknod -m 666 $DEV/capi20.16 c $MAJ 17
mknod -m 666 $DEV/capi20.17 c $MAJ 18
mknod -m 666 $DEV/capi20.18 c $MAJ 19
mknod -m 666 $DEV/capi20.19 c $MAJ 20

if grep '^dialout:' /etc/group > /dev/null
then	# Debian/SuSE standard
	chgrp dialout $DEV/capi20 $DEV/capi20.??
fi

echo "done."
