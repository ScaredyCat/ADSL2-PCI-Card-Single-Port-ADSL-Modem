#
# Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer,
#    without modification.
# 2. Redistributions in binary form must reproduce at minimum a disclaimer
#    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
#    redistribution must be conditioned upon including a substantially
#    similar Disclaimer requirement for further binary redistribution.
# 3. Neither the names of the above-listed copyright holders nor the names
#    of any contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# Alternatively, this software may be distributed under the terms of the
# GNU General Public License ("GPL") version 2 as published by the Free
# Software Foundation.
#
# NO WARRANTY
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGES.
#
# $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/madwifi/Makefile.inc#5 $
#

ifeq ($(obj),)
obj=	.
endif

# other locales sometimes cause trouble
export LC_ALL = POSIX

#
OS=		linux
#
# TARGET defines the target platform ISA per GNU rules.
# It must match one of the target platforms supported by
# the HAL.  To set the set of target platforms look at
# hal/linux/*.inc.  You can set this in the environment
# to override the default setting.
#
ifeq ($(TARGET),)
TARGET=		i386-elf
endif

# BUS defines the bus type to which the wireless devices is attached.
# Currently, the valid BUS types are PCI and AHB.  If BUS is not
# defined, then, we assume the bus type is PCI

ifndef BUS
BUS=		PCI
endif

# If we are AHB bus type, then we need the kernel path for the target
# kernel sources.

ifeq ($(strip ${BUS}),AHB)
TARGET=		mipsisa32-be-elf

ifndef KERNELPATH
$(error KERNELPATH must be defined for bus type AHB.)
endif

ifndef TOOLPATH
$(error TOOLPATH must be defined for bus type AHB.)
endif

KERNELCONF=	${KERNELPATH}/.config

ifndef KERNELRELEASE
VERSION := $(strip $(shell head -n 1 ${KERNELPATH}/Makefile | cut -f 2 -d'='))
PATCHLEVEL := $(strip $(shell head -n 2 ${KERNELPATH}/Makefile | tail -1 | cut -f 2 -d'='))
SUBVERSION := $(strip $(shell head -n 3 ${KERNELPATH}/Makefile | tail -1 | cut -f 2 -d'='))
KERNELRELEASE := $(strip ${VERSION}.${PATCHLEVEL}.${SUBVERSION}${EXTRAVERSION})
endif

ifndef MODULEPATH
MODULEPATH :=   ${KERNELPATH}/arch/mips/ar531x/ROOTDISK/rootdir/lib/modules/${KERNELRELEASE}/net
endif

endif

#
NM=		nm
AWK=		awk

# Path to the Linux kernel, include files, etc.
#
# KERNELRELEASE is the target kernel's version.  If KERNELRELEASE
# is not set in the environment then it is taken from the running
# system.
#
# KERNELPATH is the path to the target kernel's build/source area.
# This is used to obtain the kernel configuration and include files.
# If KERNELPATH is not set in the environment then it is derived
# from KERNELRELEASE.
#
ifeq ($(KERNELRELEASE),)
KERNELRELEASE =	$(shell uname -r)
endif

ifneq ($(findstring 2.6,$(KERNELRELEASE)),)
export-objs	:=
list-multi	:=
KMODSUF		:= ko
else
KMODSUF		:= o
endif

ifeq ($(KERNELPATH),)
KERNELPATH=	/lib/modules/${KERNELRELEASE}/build
endif

ifeq ($(DESTDIR),)
DESTDIR=
endif

ifeq ($(MODULEPATH),)
MODULEPATH=	/lib/modules/${KERNELRELEASE}/net
endif

#
# Path to the HAL source code.
#
ifeq ($(HAL),)
HAL=	${obj}/${DEPTH}/hal
endif
#
# Path to the HAL build area.
#
ATH_HAL=${DEPTH}/ath_hal
#
# Path to the 802.11 include files.
#
WLAN=	${DEPTH}/net80211
#
# Path to the Atheros device driver.
#
ATH=	${DEPTH}/ath
#
# Path to the rate control algorithm.
#
ifeq ($(ATH_RATE),)
ATH_RATE=	ath_rate/onoe
endif

# Packet logging
ATH_PKTLOG= ${DEPTH}/ath_pktlog

ifeq ($(AP71_DEBUG),1)
COPTS+=	-Werror
else
COPTS+=	-Werror -O2 
endif
INCS=	-include ${obj}/${DEPTH}/include/compat.h -I${obj}/${DEPTH}/include

ifeq ($(strip ${BUS}),AHB)
INCS+= -isystem ${TOOLPATH}/include
endif

include ${HAL}/public/${TARGET}.inc
include ${obj}/${DEPTH}/BuildCaps.inc
