#!/bin/sh
#
# Patch mtd into kernel
#
# usage:patch [-j] kernelpath
# 	kernelpath must be given
#	-j includes filesystems (jffs, jffs2)
#	
# Works for Kernels >= 2.4.11 full functional
# Works for Kernels >= 2.4 and <= 2.4.10 partly (JFFS2 support is missing)
# For 2.2 Kernels it's actually disabled, as I have none to test it.
#
# You can use it for pristine kernels and for already patches kernels too.
#
# Detects Kernelversion and applies neccecary modifications
# For Kernelversions < 2.4.20 ZLIB-Patch is applied, if 
# filesystem option is set and ZLIB-Patch is not already there 
#
# Maybe some sed/awk experts would make it better, but I'm not
# one of them. Feel free to make it better
# 
# Thomas (tglx@linutronix.de)
#
# $Id: patchin.sh,v 1.19 2004/05/05 20:42:55 gleixner Exp $
#
# 05-05-2004 tglx Include include/mtd
# 12-06-2003 dwmw2 Leave out JFFS1, do Makefile.common only if it exists.
# 27-05-2003 dwmw2 Link Makefile to Makefile.common since we moved them around
# 02-10-2003 tglx replaced grep -m by head -n 1, as older grep versions don't support -m	
# 03-08-2003 tglx -c option for copying files to kernel tree instead of linking
#		  moved file selection to variables

# Preset variables
FILESYSTEMS="no"
VERSION=0
PATCHLEVEL=0
SUBLEVEL=0
ZLIBPATCH="no"
CONFIG="Config.in"
LNCP="ln -sf"
METHOD="Link"

# MTD - files and directories
MTD_DIRS="drivers/mtd drivers/mtd/chips drivers/mtd/devices drivers/mtd/maps drivers/mtd/nand include/linux/mtd include/mtd"
MTD_FILES="*.[ch] Makefile Rules.make"

# JFFS2 files and directories
FS_DIRS="fs/jffs2"
FS_FILES="*.[ch] Makefile Rules.make"
# kernel version < 2.4.20 needs zlib headers
FS_INC_BEL2420="jffs*.h workqueue.h z*.h rb*.h suspend.h"
# kernel version < 2.5.x
FS_INC_BEL25="jffs2*.h workqueue.h rb*.h suspend.h"
# kernelversion >= 2.5
FS_INC_25="jffs2*.h"
FS_INC_DIR="include/linux"

# shared ZLIB patch
ZLIB_DIRS="lib/zlib_deflate lib/zlib_inflate"
ZLIB_FILES="*.[ch] Makefile"


# Display usage of this script
usage () {
	echo "usage:  $0 [-c] [-j] kernelpath"
	echo "   -c  -- copy files to kernel tree instead of building links"
	echo "   -j  -- include jffs2 filesystem" 
	exit 1
}

# Function to patch kernel source
patchit () {
for DIR in $PATCH_DIRS 
do
	echo $DIR
	mkdir -p $DIR
	cd $TOPDIR/$DIR
	FILES=`ls $PATCH_FILES 2>/dev/null`
	for FILE in $FILES 
	do
		# If there's a Makefile.common it goes in place of Makefile
		if [ "$FILE" = "Makefile" -a -r $TOPDIR/$DIR/Makefile.common ]; then
		    SRCFILE=Makefile.common
		else
		    SRCFILE=$FILE
		fi
		rm -f $LINUXDIR/$DIR/$FILE 2>/dev/null
		$LNCP $TOPDIR/$DIR/$SRCFILE $LINUXDIR/$DIR/$FILE
	done
	cd $LINUXDIR
done	
}


# Start of script

# Get commandline options
while getopts cj opt
do
    case "$opt" in
      j)  FILESYSTEMS=yes;;
      c)  LNCP="cp -f"; METHOD="Copy";;
      \?)
	  usage;
    esac
done
shift `expr $OPTIND - 1`
LINUXDIR=$1

if [ -z $LINUXDIR ]; then
    usage;
fi

# Check if kerneldir contains a Makefile
if [ ! -f $LINUXDIR/Makefile ] 
then 
	echo "Directory $LINUXDIR does not exist or is not a kernel source directory";
	exit 1;
fi

# Get kernel version
VERSION=`grep -s VERSION <$LINUXDIR/Makefile | head -n 1 | sed s/'VERSION = '//`
PATCHLEVEL=`grep -s PATCHLEVEL <$LINUXDIR/Makefile | head -n 1 | sed s/'PATCHLEVEL = '//`
SUBLEVEL=`grep -s SUBLEVEL <$LINUXDIR/Makefile | head -n 1 | sed s/'SUBLEVEL = '//`

# Can we handle this ?
if test $VERSION -ne 2 -o $PATCHLEVEL -lt 4
then 
	echo "Cannot patch kernel version $VERSION.$PATCHLEVEL.$SUBLEVEL";
	exit 1;
fi

# Use Kconfig instead of Config.in for Kernels >= 2.5
if test $PATCHLEVEL -gt 4
then
	CONFIG="Kconfig";
fi
MTD_FILES="$MTD_FILES $CONFIG"

# Have we to use ZLIB PATCH ? 
if [ "$FILESYSTEMS" = "yes" ]
then
	PATCHDONE=`grep -s zlib_deflate $LINUXDIR/lib/Makefile | head -n 1`
	if test $PATCHLEVEL -eq 4 -a $SUBLEVEL -lt 20 
	then
		if [ "$PATCHDONE" = "" ] 
		then
			ZLIBPATCH=yes;
		fi
	fi
fi

# Check which header files we need depending on kernel version
HDIR="include/linux"
if test $PATCHLEVEL -eq 4 
then	
	# 2.4 below 2.4.20 zlib headers are neccecary
	if test $SUBLEVEL -lt 20
	then
		JFFS2_H=$FS_INC_BEL2420
	else
		JFFS2_H=$FS_INC_BEL25
	fi
else
	#	>= 2.5
	JFFS2_H=$FS_INC_25
fi

echo Patching $LINUXDIR 
echo Include Filesytems: $FILESYSTEMS
echo Zlib-Patch needed: $ZLIBPATCH
echo Method: $METHOD
read -p "Can we start now ? [y/N]" ANSWER
echo ""

if [ "$ANSWER" != "y" ]
then
	echo Patching Kernel cancelled
	exit 1;
fi

# Here we go
cd `dirname $0`
THISDIR=`pwd`
TOPDIR=`dirname $THISDIR`

cd $LINUXDIR

# make directories, if necessary
# remove existing files/links and link/copy the new ones
echo "Patching MTD"
PATCH_DIRS=$MTD_DIRS
PATCH_FILES=$MTD_FILES
patchit;

# check, if we have to include JFFS(2)
if [ "$FILESYSTEMS" = "yes" ]
then
	echo "Patching JFFS(2)"
	
	PATCH_DIRS=$FS_DIRS
	PATCH_FILES=$FS_FILES
	patchit;

	PATCH_DIRS=$FS_INC_DIR
	PATCH_FILES=$JFFS2_H
	patchit;

	# this is the ugly part	
	PATCHDONE=`grep -s jffs2 fs/Makefile | head -n 1`
	if [ "$PATCHDONE" = "" ]
	then
		echo "Add JFFS2 to Makefile and Config.in manually. JFFS2 is included as of 2.4.12"	
	else
		if test $PATCHLEVEL -lt 5
		then
			JFFS=`grep -n JFFS fs/Config.in | head -n 1 | sed s/:.*//`
			CRAMFS=`grep -n CRAMFS fs/Config.in | head -n 1 | sed s/:.*//`
			let JFFS=JFFS-1
			let CRAMFS=CRAMFS-1
			sed "$JFFS"q fs/Config.in >Config.tmp
			cat $TOPDIR/fs/Config.in >>Config.tmp
			sed 1,"$CRAMFS"d fs/Config.in >>Config.tmp
			mv -f Config.tmp fs/Config.in
			
			if [ -f include/linux/crc32.h ] 
			then
				# check, if it is already defined there
				CRC32=`grep -s 'crc32(' include/linux/crc32.h | head -n 1`
				if [ "$CRC32" = "" ]
				then
					# patch in header from fs/jffs2
					LASTLINE=`grep -n '#endif' include/linux/crc32.h | head -n 1 | sed s/:.*//`
					let LASTLINE=LASTLINE-1
					sed "$LASTLINE"q include/linux/crc32.h >Crc32.tmp
					cat fs/jffs2/crc32.h >>Crc32.tmp
					echo "#endif" >>Crc32.tmp
					mv -f Crc32.tmp include/linux/crc32.h
				fi
			else
				rm -f include/linux/crc32.h
				$LNCP $TOPDIR/fs/jffs2/crc32.h include/linux
			fi
		fi
	fi
fi

if [ "$ZLIBPATCH" = "yes" ]
then
	echo "Patching ZLIB"
	
	PATCH_DIRS=$ZLIB_DIRS
	PATCH_FILES=$ZLIB_FILES
	patchit;

	patch -p1 -i $TOPDIR/lib/patch-Makefile
fi

echo "Patching done"

if test $PATCHLEVEL -lt 5
then 
	# FIXME: SED/AWK experts should know how to do it automagic
	echo "Please update Documentation/Configure.help from $TOPDIR/Documentation/Configure.help"
fi

