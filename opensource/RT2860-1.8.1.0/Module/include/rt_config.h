/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rt_config.h

	Abstract:
	Central header file to maintain all include files for all NDIS
	miniport driver routines.

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Paul Lin    08-01-2002    created

*/
#ifndef	__RT_CONFIG_H__
#define	__RT_CONFIG_H__

#ifdef UCOS
#include "includes.h"
#include <stdio.h>
#include 	"rt_ucos.h"
#endif 

#ifdef LINUX
#include	"rt_linux.h"
#endif
#include    "rtmp_type.h"
#include    "rtmp_def.h"
#include    "rt28xx.h"

#ifdef RT2860
#include	"rt2860.h"
#endif // RT2860 //


#include    "oid.h"
#include    "mlme.h"
#include    "wpa.h"
#include    "md5.h"
#include    "rtmp.h"
#include	"ap.h"
#include	"dfs.h"
#include	"chlist.h"
#ifdef MLME_EX
#include	"mlme_ex_def.h"
#include	"mlme_ex.h"
#endif // MLME_EX //

#undef AP_WSC_INCLUDED
#undef STA_WSC_INCLUDED
#undef WSC_INCLUDED
#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
#include    "ap_uapsd.h"
#endif // UAPSD_AP_SUPPORT //

#ifdef MBSS_SUPPORT
#include    "ap_mbss.h"
#endif // MBSS_SUPPORT //

#ifdef WDS_SUPPORT
#include    "ap_wds.h"
#endif // WDS_SUPPORT //

#ifdef APCLI_SUPPORT
#include	"ap_apcli.h"
#endif // APCLI_SUPPORT //

#ifdef WSC_AP_SUPPORT
#define AP_WSC_INCLUDED
#endif // WSC_AP_SUPPORT //

#include 	"ap_ids.h"
#endif // CONFIG_AP_SUPPORT //

#ifdef MAT_SUPPORT
#include "mat.h"
#endif // MAT_SUPPORT //

#ifdef LEAP_SUPPORT
#include    "leap.h"
#endif // LEAP_SUPPORT //


#ifdef BLOCK_NET_IF
#include "netif_block.h"
#endif // BLOCK_NET_IF //

#ifdef IGMP_SNOOP_SUPPORT
#include "igmp_snoop.h"
#endif // IGMP_SNOOP_SUPPORT //

#ifdef RALINK_ATE
#include "rt_ate.h"
#endif // RALINK_ATE //

#ifdef MESH_SUPPORT
#include "mesh.h"
#endif // MESH_SUPPORT //

#if defined(AP_WSC_INCLUDED) || defined(STA_WSC_INCLUDED)
#define WSC_INCLUDED
#endif


#ifdef CONFIG_AP_SUPPORT
#ifdef WDS_SUPPORT
#define RALINK_PASSPHRASE	"Ralink"
#endif // WDS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //



#if 0
//
//	Miniport defined header files
//
#include    "oid.h"
#include	"mlme.h"
#include	"md5.h"
#include	"wpa.h"
#include	"aironet.h"
#include	"action.h"
#include    "ap_wpa.h"
#include	"rtmp.h"
#include    "ap.h"
#include    "rtmp_ckipmic.h"
#include    "md4.h"
#include    "leap.h"
#endif

#ifdef WSC_INCLUDED
// WSC security code
#include	"sha2.h"
#include	"hmac.h"
#include	"dh_key.h"
#include	"evp_enc.h"

#include    "wsc.h"
#include    "wsc_tlv.h"
#endif // WSC_INCLUDE_HEADER //

#ifdef IKANOS_VX_1X0
#include	"vr_ikans.h"
#endif // IKANOS_VX_1X0 //

#endif	// __RT_CONFIG_H__

