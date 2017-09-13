/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_wds.h

    Abstract:
    Support WDS function.

    Revision History:
    Who       When            What
    ------    ----------      ----------------------------------------------
    Fonchi    02-13-2007      created
*/


#ifndef _AP_WDS_H_
#define _AP_WDS_H_

static inline BOOLEAN WDS_IF_UP_CHECK(
	IN  PRTMP_ADAPTER   pAd, 
	IN  ULONG ifidx)
{
	if (pAd->flg_wds_init != TRUE)
		return FALSE;

	if (ifidx >= MAX_WDS_ENTRY)
		return FALSE;

	if (pAd->WdsTab.WdsEntry[ifidx].dev->flags & IFF_UP)
		return TRUE;

	return FALSE;
}

LONG WdsEntryAlloc(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr);

VOID WdsEntryDel(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr);

MAC_TABLE_ENTRY *MacTableInsertWDSEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr,
	UINT WdsTabIdx);

BOOLEAN MacTableDeleteWDSEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr);

BOOLEAN ValidWdsEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR WdsIndex);

MAC_TABLE_ENTRY *WdsTableLookupByWcid(
    IN  PRTMP_ADAPTER   pAd, 
	IN UCHAR wcid,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount);

MAC_TABLE_ENTRY *WdsTableLookup(
    IN  PRTMP_ADAPTER   pAd, 
    IN  PUCHAR          pAddr,
	IN BOOLEAN bResetIdelCount);

VOID WdsTableMaintenance(
    IN PRTMP_ADAPTER    pAd);

VOID RT2860_WDS_Init(
	IN PRTMP_ADAPTER pAd,
	IN PNET_DEV net_dev);

VOID RT2860_WDS_Close(
	IN PRTMP_ADAPTER pAd);

VOID RT2860_WDS_Remove(
	IN PRTMP_ADAPTER pAd);

VOID AsicUpdateWdsRxWCIDTable(
	IN PRTMP_ADAPTER pAd);

VOID AsicUpdateWdsEncryption(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR wcid);

VOID WdsPeerBeaconProc(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN USHORT CapabilityInfo,
	IN UCHAR MaxSupportedRateIn500Kbps,
	IN UCHAR MaxSupportedRateLen,
	IN BOOLEAN bWmmCapable,
	IN ULONG ClientRalinkIe,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN UCHAR HtCapabilityLen);

VOID APWdsInitialize(
	IN PRTMP_ADAPTER pAd);

INT	Show_WdsTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

VOID rtmp_read_wds_from_file(
			IN  PRTMP_ADAPTER pAd,
			char *tmpbuf,
			char *buffer);

VOID WdsPrepareWepKeyFromMainBss(
	IN  PRTMP_ADAPTER pAd);

#endif // _AP_WDS_H_ //

