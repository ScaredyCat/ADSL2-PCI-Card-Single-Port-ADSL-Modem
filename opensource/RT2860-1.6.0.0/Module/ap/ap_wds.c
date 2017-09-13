
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
    ap_wds.c

    Abstract:
    Support WDS function.

    Revision History:
    Who       When            What
    ------    ----------      ----------------------------------------------
    Fonchi    02-13-2007      created
*/

#include "rt_config.h"


#define VAILD_KEY_INDEX( _X ) ((((_X) >= 0) && ((_X) < 4)) ? (TRUE) : (FALSE))


static INT WdsVirtualIFSendPackets(IN PNDIS_PACKET pSkb, IN PNET_DEV dev);
static INT WdsVirtualIF_open(IN PNET_DEV dev);
static INT WdsVirtualIF_close(IN PNET_DEV dev);
static INT WdsVirtualIF_ioctl(IN PNET_DEV net_dev, IN OUT struct ifreq *rq, IN INT cmd);

extern INT rt2860_ioctl(PNET_DEV net_dev, struct ifreq *rq, int cmd);

extern INT RTMPGetKeyParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer);

static INT WdsVirtualIFSendPackets(
	IN PNDIS_PACKET pSkb, 
	IN PNET_DEV dev)
{
	UCHAR i;
	VIRTUAL_ADAPTER *pVirtualAd = dev->priv;
	RTMP_ADAPTER *pAd = (PRTMP_ADAPTER) pVirtualAd->RtmpDev->priv;
	PNDIS_PACKET pPacket = (PNDIS_PACKET) pSkb;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;
	}
#endif // RALINK_ATE //

	if ((RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) ||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))          ||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)))
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;
	}
	
	if (!(pVirtualAd->RtmpDev->flags & IFF_UP))
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;
	}

	for (i = 0; i < MAX_WDS_ENTRY; i++)
	{
		if (ValidWdsEntry(pAd, i) && (pAd->WdsTab.WdsEntry[i].dev == dev))
		{
			pAd->RalinkCounters.PendingNdisPacketCount ++;
			RTMP_SET_PACKET_SOURCE(pSkb, PKTSRC_NDIS);
			RTMP_SET_PACKET_MOREDATA(pSkb, FALSE);
			RTMP_SET_PACKET_NET_DEVICE_WDS(pSkb, i);
			RTPKT_TO_OSPKT(pSkb)->dev = pVirtualAd->RtmpDev;
			
			DBGPRINT(RT_DEBUG_INFO, ("VirtualIFSendPackets(WDS) - packet to (%d)\n", i));

			return rt2860_packet_xmit(pSkb);
		}
	}

	DBGPRINT(RT_DEBUG_INFO, ("VirtualIFSendPackets - needn't to send or net_device not exist.\n"));
	RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
	return 0;
}

static INT WdsVirtualIF_open(
	IN	PNET_DEV dev)
{
	VIRTUAL_ADAPTER	*pVirtualAd = dev->priv;
#ifdef RTL865X_SOC
	RTMP_ADAPTER	*pAd = pVirtualAd->RtmpDev->priv;
	INT				index;
	unsigned int 	linkid;
#endif

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> VirtualIF_open\n", dev->name));

	netif_start_queue(pVirtualAd->VirtualDev);
	
	return 0;
}

static INT WdsVirtualIF_close(
	IN PNET_DEV dev)
{
	VIRTUAL_ADAPTER	*pVirtualAd = dev->priv;

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> VirtualIF_close\n", dev->name));

	netif_stop_queue(pVirtualAd->VirtualDev);
	
	return 0;
}

static INT WdsVirtualIF_ioctl(
	IN PNET_DEV net_dev, 
	IN OUT struct ifreq *rq, 
	IN INT cmd)
{
	VIRTUAL_ADAPTER *pVirtualAd = net_dev->priv;
	RTMP_ADAPTER *pAd = pVirtualAd->RtmpDev->priv;
	// struct iwreq *wrq = (struct iwreq *) rq;
	
	if(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("VirtualIF_ioctl(%s)::Network is down!\n", net_dev->name));
		return -ENETDOWN;
	}

	return rt2860_ioctl(net_dev, rq, cmd);
}

/*
	==========================================================================
	Description:
		Check the WDS Entry is valid or not.
	==========================================================================
 */
BOOLEAN ValidWdsEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR WdsIndex)
{
	BOOLEAN result;
	PMAC_TABLE_ENTRY pMacEntry;

	do
	{
		if (WdsIndex >= MAX_WDS_ENTRY)
		{
			result = FALSE;
			break;
		}

		if (pAd->WdsTab.WdsEntry[WdsIndex].Valid != TRUE)
		{
			result = FALSE;
			break;
		}

		if ((pAd->WdsTab.WdsEntry[WdsIndex].MacTabMatchWCID==0)
			|| (pAd->WdsTab.WdsEntry[WdsIndex].MacTabMatchWCID >= MAX_LEN_OF_MAC_TABLE))
		{
			result = FALSE;
			break;
		}

		pMacEntry = &pAd->MacTab.Content[pAd->WdsTab.WdsEntry[WdsIndex].MacTabMatchWCID];
		if (pMacEntry->ValidAsWDS != TRUE)
		{
			result = FALSE;
			break;
		}
			
		result = TRUE;
	} while(FALSE);

	return result;
}

LONG WdsEntryAlloc(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	INT i;
	LONG WdsTabIdx = -1;

	NdisAcquireSpinLock(&pAd->WdsTabLock);
	for (i = 0; i < MAX_WDS_ENTRY; i++)
	{
		if ((pAd->WdsTab.Mode >= WDS_LAZY_MODE) && !WDS_IF_UP_CHECK(pAd, i))
			continue;

		if (pAd->WdsTab.WdsEntry[i].Valid == FALSE)
		{
			pAd->WdsTab.WdsEntry[i].Valid = TRUE;
			pAd->WdsTab.Size ++;
			COPY_MAC_ADDR(pAd->WdsTab.WdsEntry[i].PeerWdsAddr, pAddr);
			WdsTabIdx = i;
			break;
		}
		else if (MAC_ADDR_EQUAL(pAd->WdsTab.WdsEntry[i].PeerWdsAddr, pAddr))
		{
			WdsTabIdx = i;
			break;
		}
	}

	if (i == MAX_WDS_ENTRY)
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to allocate WdsEntry.\n", __FUNCTION__));

	NdisReleaseSpinLock(&pAd->WdsTabLock);

	return WdsTabIdx;
}

VOID WdsEntryDel(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	INT i;

	// delete one WDS entry
	NdisAcquireSpinLock(&pAd->WdsTabLock);

	for (i = 0; i < MAX_WDS_ENTRY; i++)
	{
		if (MAC_ADDR_EQUAL(pAddr, pAd->WdsTab.WdsEntry[i].PeerWdsAddr)
			&& (pAd->WdsTab.WdsEntry[i].Valid == TRUE))
		{
			pAd->WdsTab.WdsEntry[i].Valid = FALSE;
			NdisZeroMemory(pAd->WdsTab.WdsEntry[i].PeerWdsAddr, MAC_ADDR_LEN);
			pAd->WdsTab.Size--;
			break;
		}
	}

	NdisReleaseSpinLock(&pAd->WdsTabLock);
}

/*
	==========================================================================
	Description:
		Delete all WDS Entry in pAd->MacTab
	==========================================================================
 */
BOOLEAN MacTableDeleteWDSEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr)
{
	if (wcid >= MAX_LEN_OF_MAC_TABLE)
		return FALSE;


	MacTableDeleteEntry(pAd, wcid, pAddr);

	return TRUE;
}

/*
================================================================
Description : because WDS and CLI share the same WCID table in ASIC. 
WDS entry also insert to pAd->MacTab.content[].  Such is marked as ValidAsWDS = TRUE.
Also fills the pairwise key.
Because front MAX_AID_BA entries have direct mapping to BAEntry, which is only used as CLI. So we insert WDS
from index MAX_AID_BA.
================================================================
*/
MAC_TABLE_ENTRY *MacTableInsertWDSEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr,
	UINT WdsTabIdx)
{
	PMAC_TABLE_ENTRY pEntry = NULL;
	HTTRANSMIT_SETTING HTPhyMode;

	// if FULL, return
	if (pAd->MacTab.Size >= MAX_LEN_OF_MAC_TABLE)
		return NULL;

	if((pEntry = WdsTableLookup(pAd, pAddr, TRUE)) != NULL)
		return pEntry;

	// allocate one WDS entry
	do
	{
		// allocate one MAC entry
		pEntry = MacTableInsertEntry(pAd, pAddr, (WdsTabIdx + MIN_NET_DEVICE_FOR_WDS), TRUE);
		if (pEntry)
		{
			pEntry->PortSecured = WPA_802_1X_PORT_SECURED;

			// specific Max Tx Rate for Wds link.
			NdisZeroMemory(&HTPhyMode, sizeof(HTTRANSMIT_SETTING));
			switch (pAd->WdsTab.WdsEntry[WdsTabIdx].PhyMode)
			{
				case 0xff: // user doesn't specific a Mode for WDS link.
				case MODE_OFDM: // specific OFDM mode.
					HTPhyMode.field.MODE = MODE_OFDM;
					HTPhyMode.field.MCS = 7;
					pEntry->RateLen = 8;
					break;

				case MODE_CCK:
					HTPhyMode.field.MODE = MODE_CCK;
					HTPhyMode.field.MCS = 3;
					pEntry->RateLen = 4;
					break;

				case MODE_HTMIX:
					HTPhyMode.field.MCS = 7;
					HTPhyMode.field.ShortGI = pAd->CommonCfg.HTPhyMode.field.ShortGI;
					HTPhyMode.field.BW = pAd->CommonCfg.HTPhyMode.field.BW;
					HTPhyMode.field.STBC = pAd->CommonCfg.HTPhyMode.field.STBC;
					HTPhyMode.field.MODE = MODE_HTMIX;
					pEntry->RateLen = 12;
					break;

				case MODE_HTGREENFIELD:
					HTPhyMode.field.MCS = 7;
					HTPhyMode.field.ShortGI = pAd->CommonCfg.HTPhyMode.field.ShortGI;
					HTPhyMode.field.BW = pAd->CommonCfg.HTPhyMode.field.BW;
					HTPhyMode.field.STBC = pAd->CommonCfg.HTPhyMode.field.STBC;
					HTPhyMode.field.MODE = MODE_HTGREENFIELD;
					pEntry->RateLen = 12;
					break;

				default:
					break;
			}

			pEntry->MaxHTPhyMode.word = HTPhyMode.word;
			pEntry->MinHTPhyMode.word = pAd->CommonCfg.MinHTPhyMode.word;
			pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

			if (pAd->WdsTab.WdsEntry[WdsTabIdx].PhyMode >= MODE_HTMIX)
			{
				if (pAd->CommonCfg.RegTransmitSetting.field.MCS != MCS_AUTO)
				{
					printk("@@@ pAd->CommonCfg.RegTransmitSetting.field.MCS = %d\n",
						pAd->CommonCfg.RegTransmitSetting.field.MCS);
					if (pAd->CommonCfg.RegTransmitSetting.field.MCS == 32)
					{
						// Fix MCS as HT Duplicated Mode
						pEntry->MaxHTPhyMode.field.BW = 1;
						pEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
						pEntry->MaxHTPhyMode.field.STBC = 0;
						pEntry->MaxHTPhyMode.field.ShortGI = 0;
						pEntry->MaxHTPhyMode.field.MCS = 32;
					}
					else if (pEntry->MaxHTPhyMode.field.MCS > pAd->CommonCfg.HTPhyMode.field.MCS)
					{
						// STA supports fixed MCS 
						pEntry->MaxHTPhyMode.field.MCS = pAd->CommonCfg.HTPhyMode.field.MCS;
					}
				}

				pEntry->HTCapability.MCSSet[0] = pAd->CommonCfg.HtCapability.MCSSet[0];
				pEntry->HTCapability.MCSSet[1] = pAd->CommonCfg.HtCapability.MCSSet[1];
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}

			if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
				pEntry->HTPhyMode.field.MCS = pAd->CommonCfg.RegTransmitSetting.field.MCS;

			pAd->WdsTab.WdsEntry[WdsTabIdx].MacTabMatchWCID = (UCHAR)pEntry->Aid;
			pEntry->MatchWDSTabIdx = WdsTabIdx;

			//AsicAddKeyEntry(pAd, pEntry->Aid, BSS0, 0, &pAd->WdsTab.Wpa_key, TRUE, TRUE);
			AsicUpdateWdsEncryption(pAd, pEntry->Aid);

			DBGPRINT(RT_DEBUG_TRACE, ("MacTableInsertWDSEntry - allocate entry #%d, Total= %d\n",WdsTabIdx, pAd->MacTab.Size));
			break;
		}
	}while(FALSE);

	return pEntry;
}

MAC_TABLE_ENTRY *WdsTableLookupByWcid(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR wcid,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount)
{
	//USHORT HashIdx;
	ULONG WdsIndex;
	PMAC_TABLE_ENTRY pCurEntry = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;

	if (wcid <=0 || wcid >= MAX_LEN_OF_MAC_TABLE )
		return NULL;

	NdisAcquireSpinLock(&pAd->WdsTabLock);
	NdisAcquireSpinLock(&pAd->MacTabLock);

	do
	{
		//HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
		//pCurEntry = pAd->MacTab.Hash[wcid];
		pCurEntry = &pAd->MacTab.Content[wcid];

		WdsIndex = 0xff;
		if ((pCurEntry) && (pCurEntry->ValidAsWDS == TRUE))
		{
			WdsIndex = pCurEntry->MatchWDSTabIdx;
		}

		if (WdsIndex == 0xff)
			break;

		if (pAd->WdsTab.WdsEntry[WdsIndex].Valid != TRUE)
			break;

		if (MAC_ADDR_EQUAL(pCurEntry->Addr, pAddr))
		{
			if(bResetIdelCount)
				pCurEntry->NoDataIdleCount = 0;
			pEntry = pCurEntry;
			break;
		}
	} while(FALSE);

	NdisReleaseSpinLock(&pAd->MacTabLock);
	NdisReleaseSpinLock(&pAd->WdsTabLock);

	return pEntry;
}

MAC_TABLE_ENTRY *WdsTableLookup(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount)
{
	USHORT HashIdx;
	PMAC_TABLE_ENTRY pEntry = NULL;

	NdisAcquireSpinLock(&pAd->WdsTabLock);
	NdisAcquireSpinLock(&pAd->MacTabLock);

	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = pAd->MacTab.Hash[HashIdx];

	while (pEntry)
	{
		if ((pEntry->ValidAsWDS == TRUE) && MAC_ADDR_EQUAL(pEntry->Addr, pAddr))
		{
			if(bResetIdelCount)
				pEntry->NoDataIdleCount = 0;
			break;
		}
		else
			pEntry = pEntry->pNext;
	}

	NdisReleaseSpinLock(&pAd->MacTabLock);
	NdisReleaseSpinLock(&pAd->WdsTabLock);

	return pEntry;
}

/*
	==========================================================================
	Description:
		This routine is called by APMlmePeriodicExec() every second to check if
		1. any associated client in PSM. If yes, then TX MCAST/BCAST should be
		   out in DTIM only
		2. any client being idle for too long and should be aged-out from MAC table
		3. garbage collect PSQ
	==========================================================================
*/
VOID WdsTableMaintenance(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR idx;

	if (pAd->WdsTab.Mode != WDS_LAZY_MODE)
		return;

	for (idx = 0; idx < pAd->WdsTab.Size; idx++)
	{
		UCHAR wcid = pAd->WdsTab.WdsEntry[idx].MacTabMatchWCID;
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[wcid];

		if(pEntry->ValidAsWDS != TRUE)
			continue;

		NdisAcquireSpinLock(&pAd->WdsTabLock);
		NdisAcquireSpinLock(&pAd->MacTabLock);
		pEntry->NoDataIdleCount += 5;
		NdisReleaseSpinLock(&pAd->MacTabLock);
		NdisReleaseSpinLock(&pAd->WdsTabLock);

		// delete those MAC entry that has been idle for a long time
		if (pEntry->NoDataIdleCount >= MAC_TABLE_AGEOUT_TIME)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("ageout %02x:%02x:%02x:%02x:%02x:%02x from WDS #%d after %d-sec silence\n",
					pEntry->Addr[0],pEntry->Addr[1],pEntry->Addr[2],pEntry->Addr[3],
					pEntry->Addr[4],pEntry->Addr[5], idx, MAC_TABLE_AGEOUT_TIME));
			WdsEntryDel(pAd, pEntry->Addr);
			MacTableDeleteWDSEntry(pAd, pEntry->Aid, pEntry->Addr);
		}
	}

}

// Register WDS interface
VOID RT2860_WDS_Init(
	IN PRTMP_ADAPTER pAd,
	IN PNET_DEV net_dev)
{
	INT index;
	PNET_DEV dev;
	VIRTUAL_ADAPTER *pWdsAd;
	INT i = 0;
	CHAR slot_name[IFNAMSIZ];
	PNET_DEV device;

	/* sanity check to avoid redundant virtual interfaces are created */
	if (pAd->flg_wds_init != FALSE)
		return;
	pAd->flg_wds_init = TRUE;

	for(index = 0; index < MAX_WDS_ENTRY; index++)
	{
#if LINUX_VERSION_CODE <= 0x20402		// Red Hat 7.1
		dev = alloc_netdev(sizeof(VIRTUAL_ADAPTER), "eth%d", ether_setup);
#else
		dev = alloc_etherdev(sizeof(VIRTUAL_ADAPTER));
#endif
		{// find available 
			for (i = 0; i < 32; i++)
			{
				sprintf(slot_name, "wds%d", i);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				device = dev_get_by_name(slot_name);
#else
				for (device = dev_base; device != NULL; device = device->next)
				{
					if (strncmp(device->name, slot_name, 6) == 0)
					{
						break;
					}
				}
#endif
				if(device == NULL)
					break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				else
					dev_put(device);
#endif
			}

			if(i != 32)
			{
				PMAC_TABLE_ENTRY pWdsEntry;
				sprintf(dev->name, "wds%d", i);
				
				pWdsEntry = &pAd->MacTab.Content[pAd->WdsTab.WdsEntry[index].MacTabMatchWCID];
				DBGPRINT(RT_DEBUG_TRACE, ("Register WDS(virtual) interface(wds0%d)-%02X:%02X:%02X:%02X:%02X:%02X\n",
					i, pWdsEntry->Addr[0], pWdsEntry->Addr[1], pWdsEntry->Addr[2], pWdsEntry->Addr[3], pWdsEntry->Addr[4], pWdsEntry->Addr[5]));
			}
			else
			{
				DBGPRINT(RT_DEBUG_ERROR, ("Ethernet interface number overflow...\n"));
				break;
			}
		}
		
		ether_setup(dev);
		pWdsAd = dev->priv;

		pWdsAd->VirtualDev  = dev;
		pWdsAd->RtmpDev     = net_dev;

		NdisMoveMemory(&dev->dev_addr, &net_dev->dev_addr, MAC_ADDR_LEN);

		dev->hard_start_xmit    = (HARD_START_XMIT_FUNC)WdsVirtualIFSendPackets;
		dev->stop               = WdsVirtualIF_close;
		dev->open               = WdsVirtualIF_open;
		dev->do_ioctl           = WdsVirtualIF_ioctl;
		dev->priv_flags			= INT_WDS;

		pAd->WdsTab.WdsEntry[index].PhyMode = 0xff;

		// Register this device
		register_netdevice(dev);
		pAd->WdsTab.WdsEntry[index].dev = dev;
	}
}

VOID RT2860_WDS_Close(
	IN PRTMP_ADAPTER pAd)
{
	UINT index;

	for(index = 0; index < MAX_WDS_ENTRY; index++)
	{
		// free resources of BA
		BASessionTearDownALL(pAd, index);

		if (pAd->WdsTab.WdsEntry[index].dev)
		{
			dev_close(pAd->WdsTab.WdsEntry[index].dev);
		}
	}
	return;
}


VOID RT2860_WDS_Remove(
	IN PRTMP_ADAPTER pAd)
{
	UINT index;

	for(index = 0; index < MAX_WDS_ENTRY; index++)
	{
		if (pAd->WdsTab.WdsEntry[index].dev)
	    {
			unregister_netdev(pAd->WdsTab.WdsEntry[index].dev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			free_netdev(pAd->WdsTab.WdsEntry[index].dev);
#else
			kfree(pAd->WdsTab.WdsEntry[index].dev);
#endif
		}
	}
}

VOID AsicUpdateWdsRxWCIDTable(
	IN PRTMP_ADAPTER pAd)
{
	UINT index;

	for(index = 0; index < MAX_WDS_ENTRY; index++)
	{
		if (pAd->WdsTab.WdsEntry[index].Valid != TRUE)
			continue;

		MacTableInsertWDSEntry(pAd, pAd->WdsTab.WdsEntry[index].PeerWdsAddr, index);
	}

	return;
}

VOID AsicUpdateWdsEncryption(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR wcid)
{
	UINT WdsIdex;
	PMAC_TABLE_ENTRY pEntry = NULL;

	do
	{
		if (wcid >= MAX_LEN_OF_MAC_TABLE)
			break;

		pEntry = &pAd->MacTab.Content[wcid];
		if (pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].Valid != TRUE)
			break;

		if (pEntry->ValidAsWDS != TRUE)
			break;

		WdsIdex = pEntry->MatchWDSTabIdx;
				
		if (((pAd->WdsTab.WdsEntry[WdsIdex].WepStatus == Ndis802_11Encryption1Enabled) || 
			   (pAd->WdsTab.WdsEntry[WdsIdex].WepStatus == Ndis802_11Encryption2Enabled) ||
			   (pAd->WdsTab.WdsEntry[WdsIdex].WepStatus == Ndis802_11Encryption3Enabled))
				&& (pAd->WdsTab.WdsEntry[WdsIdex].WdsKey.KeyLen > 0))
		{
			
			INT DefaultKeyId = 0;

			if (pAd->WdsTab.WdsEntry[WdsIdex].WepStatus == Ndis802_11Encryption1Enabled)
				DefaultKeyId = pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].KeyIdx;

			if (!VAILD_KEY_INDEX(DefaultKeyId))
				break;

			AsicAddPairwiseKeyEntry(
				pAd,
				pEntry->Addr,
				pEntry->Aid,
				&pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WdsKey);

			// update WCID attribute table and IVEIV table for this entry
			RTMPAddWcidAttributeEntry(
				pAd, 
				MAIN_MBSSID + MIN_NET_DEVICE_FOR_WDS,
				DefaultKeyId, 
				pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WdsKey.CipherAlg,
				pEntry);
		}
	} while (FALSE);

	return;
}

VOID WdsPeerBeaconProc(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN USHORT CapabilityInfo,
	IN UCHAR MaxSupportedRateIn500Kbps,
	IN UCHAR MaxSupportedRateLen,
	IN BOOLEAN bWmmCapable,
	IN ULONG ClientRalinkIe,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN UCHAR HtCapabilityLen)
{
	UCHAR MaxSupportedRate = RATE_11;

	switch (MaxSupportedRateIn500Kbps)
	{
		case 108: MaxSupportedRate = RATE_54;   break;
		case 96:  MaxSupportedRate = RATE_48;   break;
		case 72:  MaxSupportedRate = RATE_36;   break;
		case 48:  MaxSupportedRate = RATE_24;   break;
		case 36:  MaxSupportedRate = RATE_18;   break;
		case 24:  MaxSupportedRate = RATE_12;   break;
		case 18:  MaxSupportedRate = RATE_9;    break;
		case 12:  MaxSupportedRate = RATE_6;    break;
		case 22:  MaxSupportedRate = RATE_11;   break;
		case 11:  MaxSupportedRate = RATE_5_5;  break;
		case 4:   MaxSupportedRate = RATE_2;    break;
		case 2:   MaxSupportedRate = RATE_1;    break;
		default:  MaxSupportedRate = RATE_11;   break;
	}

	if (pEntry && pEntry->ValidAsWDS)
	{
		pEntry->MaxSupportedRate = min(pAd->CommonCfg.MaxTxRate, MaxSupportedRate);
		pEntry->RateLen = MaxSupportedRateLen;
		if (pEntry->MaxSupportedRate < RATE_FIRST_OFDM_RATE)
		{
			pEntry->MaxHTPhyMode.field.MODE = MODE_CCK;
			pEntry->MaxHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
			pEntry->MinHTPhyMode.field.MODE = MODE_CCK;
			pEntry->MinHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
		}
		else
		{
			pEntry->MaxHTPhyMode.field.MODE = MODE_OFDM;
			pEntry->MaxHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
			pEntry->MinHTPhyMode.field.MODE = MODE_OFDM;
			pEntry->MinHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
		}
		pEntry->CapabilityInfo = CapabilityInfo;

		if (ClientRalinkIe & 0x00000004)
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
		else
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
			
		if (pAd->CommonCfg.bAggregationCapable)
		{
			if ((pAd->CommonCfg.bPiggyBackCapable) && (ClientRalinkIe & 0x00000003) == 3)
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
				//RTMPSetPiggyBack(pAd, TRUE);
				DBGPRINT(RT_DEBUG_INFO, ("ASSOC -PiggyBack= 1\n"));
			}
			else if (ClientRalinkIe & 0x00000001)
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
			}
			else
			{
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
			}
		}
		else
		{
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
			if ((pAd->CommonCfg.bPiggyBackCapable) && (ClientRalinkIe & 0x00000002) == 2)
			{				
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
				//RTMPSetPiggyBack(pAd, TRUE);
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC -PiggyBack2= 1\n"));
			}
			else
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
		}

		// If this Entry supports 802.11n, upgrade to HT rate.
		if ((HtCapabilityLen != 0) 
			&& (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
		{
			UCHAR	j, bitmask;//k,bitmask;
			CHAR    i;

			if ((pHtCapability->HtCapInfo.GF) && (pAd->CommonCfg.DesiredHtPhy.GF))
			{
				pEntry->MaxHTPhyMode.field.MODE = MODE_HTGREENFIELD;
			}
			else
			{	
				pEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
				pAd->MacTab.fAnyStationNonGF = TRUE;
				pAd->CommonCfg.AddHTInfo.AddHtInfo2.NonGfPresent = 1;
			}

			if ((pHtCapability->HtCapInfo.ChannelWidth) && (pAd->CommonCfg.DesiredHtPhy.ChannelWidth))
			{
				pEntry->MaxHTPhyMode.field.BW= BW_40;
				pEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor40)&(pHtCapability->HtCapInfo.ShortGIfor40));
			}
			else
			{	
				pEntry->MaxHTPhyMode.field.BW = BW_20;
				pEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor20)&(pHtCapability->HtCapInfo.ShortGIfor20));
				pAd->MacTab.fAnyStation20Only = TRUE;
			}

			// find max fixed rate
			for (i=15; i>=0; i--)
			{	
				j = i/8;	
				bitmask = (1<<(i-(j*8)));
				if ( (pAd->CommonCfg.DesiredHtPhy.MCSSet[j]&bitmask) && (pHtCapability->MCSSet[j]&bitmask))
				{
					pEntry->MaxHTPhyMode.field.MCS = i;
					break;
				}
				if (i==0)
					break;
			}

			if ((pEntry->MaxHTPhyMode.field.MCS > pAd->CommonCfg.HTPhyMode.field.MCS) && (pAd->CommonCfg.HTPhyMode.field.MCS != MCS_AUTO))
				pEntry->MaxHTPhyMode.field.MCS = pAd->CommonCfg.HTPhyMode.field.MCS;
			pEntry->MaxHTPhyMode.field.STBC = (pHtCapability->HtCapInfo.RxSTBC & (pAd->CommonCfg.DesiredHtPhy.TxSTBC));
			pEntry->MpduDensity = pHtCapability->HtCapParm.MpduDensity;
			pEntry->MaxRAmpduFactor = pHtCapability->HtCapParm.MaxRAmpduFactor;
			pEntry->MmpsMode = (UCHAR)pHtCapability->HtCapInfo.MimoPs;
			pEntry->AMsduSize = (UCHAR)pHtCapability->HtCapInfo.AMsduSize;
			if (pAd->CommonCfg.DesiredHtPhy.AmsduEnable)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED);
			if (pHtCapability->HtCapInfo.ShortGIfor20)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI20_CAPABLE);
			if (pHtCapability->HtCapInfo.ShortGIfor40)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI40_CAPABLE);
			if (pHtCapability->HtCapInfo.TxSTBC)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_TxSTBC_CAPABLE);
			if (pHtCapability->HtCapInfo.RxSTBC)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RxSTBC_CAPABLE);
			if (pHtCapability->ExtHtCapInfo.PlusHTC)				
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_HTC_CAPABLE);
			if (pAd->CommonCfg.bRdg && pHtCapability->ExtHtCapInfo.RDGSupport)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RDG_CAPABLE);	
			if (pHtCapability->ExtHtCapInfo.MCSFeedback == 0x03)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_MCSFEEDBACK_CAPABLE);

			NdisMoveMemory(&pEntry->HTCapability, pHtCapability, sizeof(HT_CAPABILITY_IE));
		}
		else
		{
			pAd->MacTab.fAnyStationIsLegacy = TRUE;
		}

		if (bWmmCapable || (pEntry->MaxHTPhyMode.field.MODE >= MODE_HTMIX))
		{
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
		}
		else
		{
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
		}

		pEntry->HTPhyMode.field.MODE = pEntry->MaxHTPhyMode.field.MODE;
		pEntry->HTPhyMode.field.STBC = pEntry->MaxHTPhyMode.field.STBC;
		pEntry->HTPhyMode.field.ShortGI = pEntry->MaxHTPhyMode.field.ShortGI;
		pEntry->HTPhyMode.field.BW = pEntry->MaxHTPhyMode.field.BW;
	}
}

VOID APWdsInitialize(
	IN PRTMP_ADAPTER pAd)
{
	INT i;

	pAd->WdsTab.Mode = WDS_DISABLE_MODE;
	pAd->WdsTab.Size = 0;
	for (i = 0; i < MAX_WDS_ENTRY; i++)
	{
		pAd->WdsTab.WdsEntry[i].PhyMode = 0xff;
		pAd->WdsTab.WdsEntry[i].Valid = FALSE;
		pAd->WdsTab.WdsEntry[i].MacTabMatchWCID = 0;
		pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11EncryptionDisabled;
		pAd->WdsTab.WdsEntry[i].KeyIdx = 0;
		NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));
	}
	return;	
}

INT	Show_WdsTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT i, QueIdx=0;

	for(i = 0; i < MAX_WDS_ENTRY; i++)
	{					
		printk("IF/WDS%d-%02x:%02x:%02x:%02x:%02x:%02x(%s) ,%s, KeyId=%d\n", i, 
		PRINT_MAC(pAd->WdsTab.WdsEntry[i].PeerWdsAddr), 
			pAd->WdsTab.WdsEntry[i].Valid == 1 ? "Valid" : "Invalid",
			GetEncryptType(pAd->WdsTab.WdsEntry[i].WepStatus), 
			pAd->WdsTab.WdsEntry[i].KeyIdx);

		if (pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen > 0)
			hex_dump("Wds Key", pAd->WdsTab.WdsEntry[i].WdsKey.Key, pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen);
	}

	printk("[Tx]: SwFreeIdx=%ld, CpuIdx=%ld, DmaIdx=%ld\n",
		pAd->TxRing[QueIdx].TxSwFreeIdx, 
		pAd->TxRing[QueIdx].TxCpuIdx,
		pAd->TxRing[QueIdx].TxDmaIdx);
		
	printk("[Rx]:  SwRedIdx=%ld, CpuIdx=%ld, DmaIdx=%ld\n", 
		pAd->RxRing.RxSwReadIdx,
		pAd->RxRing.RxCpuIdx,
		pAd->RxRing.RxDmaIdx);
	
	printk("\n%-19s%-4s%-4s%-4s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s\n",
		"MAC", "IDX", "AID", "PSM", "RSSI0", "RSSI1", "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry->ValidAsWDS)
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d", (int)pEntry->MatchWDSTabIdx);
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-7d", pEntry->RssiSample.AvgRssi0);
			printk("%-7d", pEntry->RssiSample.AvgRssi1);
			printk("%-7d", pEntry->RssiSample.AvgRssi2);
			printk("%-10s", GetPhyMode(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", GetBW(pEntry->HTPhyMode.field.BW));
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d\n", pEntry->HTPhyMode.field.STBC);
		}
	} 

	return TRUE;
}

VOID rtmp_read_wds_from_file(
			IN  PRTMP_ADAPTER pAd,
			char *tmpbuf,
			char *buffer)
{
	PUCHAR		macptr;
	INT			i=0, j;
	char		tok_str[16];
	BOOLEAN		bUsePrevFormat = FALSE;
	UCHAR		macAddress[MAC_ADDR_LEN];
	UCHAR	    keyMaterial[40];	
	UCHAR		KeyLen, CipherAlg = CIPHER_NONE, KeyIdx;
		
	//WdsPhyMode
	if (RTMPGetKeyParameter("WdsPhyMode", tmpbuf, MAX_PARAM_BUFFER_SIZE, buffer))
	{	
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_WDS_ENTRY); macptr = rstrtok(NULL,";"), i++) 
		{
	        if ((strncmp(macptr, "CCK", 3) == 0) || (strncmp(macptr, "cck", 3) == 0))
	            pAd->WdsTab.WdsEntry[i].PhyMode = MODE_CCK;
	        else if ((strncmp(macptr, "OFDM", 4) == 0) || (strncmp(macptr, "ofdm", 4) == 0))
	            pAd->WdsTab.WdsEntry[i].PhyMode = MODE_OFDM;
	        else if ((strncmp(macptr, "HTMIX", 5) == 0) || (strncmp(macptr, "htmix", 5) == 0))
	            pAd->WdsTab.WdsEntry[i].PhyMode = MODE_HTMIX;
	        else if ((strncmp(macptr, "GREENFIELD", 10) == 0) || (strncmp(macptr, "greenfield", 10) == 0))
	            pAd->WdsTab.WdsEntry[i].PhyMode = MODE_HTGREENFIELD;
	        else
	            pAd->WdsTab.WdsEntry[i].PhyMode = 0xff;
		
	        DBGPRINT(RT_DEBUG_TRACE, ("If/wds%d - WdsPhyMode=%d\n", i, pAd->WdsTab.WdsEntry[i].PhyMode));	    
		}
	}
	
	//WdsList
	if (RTMPGetKeyParameter("WdsList", tmpbuf, MAX_PARAM_BUFFER_SIZE, buffer))
	{
		if (pAd->WdsTab.Mode != WDS_LAZY_MODE)
		{
			for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_WDS_ENTRY); macptr = rstrtok(NULL,";"), i++) 
			{				
				if(strlen(macptr) != 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
					continue; 
				if(strcmp(macptr,"00:00:00:00:00:00") == 0)
					continue; 
				if(i >= MAX_WDS_ENTRY)
					break; 

				for (j=0; j<ETH_LENGTH_OF_ADDRESS; j++)
				{
					AtoH(macptr, &macAddress[j], 2);
					macptr=macptr+3;
				}	

				WdsEntryAlloc(pAd, macAddress);				
			}
		}
	}
	//WdsEncrypType
	if (RTMPGetKeyParameter("WdsEncrypType", tmpbuf, 128, buffer))
	{				
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_WDS_ENTRY); macptr = rstrtok(NULL,";"), i++)
	    {
	        if ((strncmp(macptr, "NONE", 4) == 0) || (strncmp(macptr, "none", 4) == 0))
	            pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11WEPDisabled;
	        else if ((strncmp(macptr, "WEP", 3) == 0) || (strncmp(macptr, "wep", 3) == 0))
	            pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11WEPEnabled;
	        else if ((strncmp(macptr, "TKIP", 4) == 0) || (strncmp(macptr, "tkip", 4) == 0))
	            pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11Encryption2Enabled;
	        else if ((strncmp(macptr, "AES", 3) == 0) || (strncmp(macptr, "aes", 3) == 0))
	            pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11Encryption3Enabled;
	        else
	            pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11WEPDisabled;

	        DBGPRINT(RT_DEBUG_TRACE, ("WdsEncrypType[%d]=%d(%s)\n", i, pAd->WdsTab.WdsEntry[i].WepStatus, GetEncryptType(pAd->WdsTab.WdsEntry[i].WepStatus)));
	    }
		
		// Previous WDS only supports single encryption type.
		// For backward compatible, other wds link encryption type shall be the same with the first.
		if (i == 1)
		{
			for (j = 1; j < MAX_WDS_ENTRY; j++)
			{
				pAd->WdsTab.WdsEntry[j].WepStatus = pAd->WdsTab.WdsEntry[0].WepStatus;	
				DBGPRINT(RT_DEBUG_TRACE, ("@WdsEncrypType[%d]=%d(%s)\n", j, pAd->WdsTab.WdsEntry[i].WepStatus, GetEncryptType(pAd->WdsTab.WdsEntry[i].WepStatus)));	
			}
		}
			
	}
	// WdsKey
	// This is a previous parameter and it only stores WPA key material, not WEP key
	if (RTMPGetKeyParameter("WdsKey", tmpbuf, 255, buffer))
	{			
		for (i = 0; i < MAX_WDS_ENTRY; i++)
			NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));

		if (strlen(tmpbuf) > 0)
			bUsePrevFormat = TRUE;

		// check if the wds-0 link key material is valid 
		if (((pAd->WdsTab.WdsEntry[0].WepStatus == Ndis802_11Encryption2Enabled)
				|| (pAd->WdsTab.WdsEntry[0].WepStatus == Ndis802_11Encryption3Enabled))
			&& (strlen(tmpbuf) >= 8) && (strlen(tmpbuf) <= 64))
		{					
			PasswordHash((char *)tmpbuf, RALINK_PASSPHRASE, sizeof(RALINK_PASSPHRASE), keyMaterial);

			if (pAd->WdsTab.WdsEntry[0].WepStatus == Ndis802_11Encryption3Enabled)
				pAd->WdsTab.WdsEntry[0].WdsKey.CipherAlg = CIPHER_AES;
			else
				pAd->WdsTab.WdsEntry[0].WdsKey.CipherAlg = CIPHER_TKIP;
			
			NdisMoveMemory(&pAd->WdsTab.WdsEntry[0].WdsKey.Key, keyMaterial, 16);
			pAd->WdsTab.WdsEntry[0].WdsKey.KeyLen = 16;
			NdisMoveMemory(&pAd->WdsTab.WdsEntry[0].WdsKey.RxMic, keyMaterial+16, 8);
			NdisMoveMemory(&pAd->WdsTab.WdsEntry[0].WdsKey.TxMic, keyMaterial+16, 8);
		}

		// Previous WDS only supports single key-material.
		// For backward compatible, other wds link key-material shall be the same with the first.
		if (pAd->WdsTab.WdsEntry[0].WdsKey.KeyLen == 16)
		{
			for (j = 1; j < MAX_WDS_ENTRY; j++)
			{
				NdisMoveMemory(&pAd->WdsTab.WdsEntry[j].WdsKey, &pAd->WdsTab.WdsEntry[0].WdsKey, sizeof(CIPHER_KEY));								
			}
		}
	
	}

	// The parameters can provide different key information for each WDS-Link
	// no matter WEP or WPA
	if (!bUsePrevFormat)
	{
		for (i = 0; i < MAX_WDS_ENTRY; i++)
		{
			sprintf(tok_str, "Wds%dKey", i);
			
			// WdsXKey (X=0~MAX_WDS_ENTRY-1)
			if (RTMPGetKeyParameter(tok_str, tmpbuf, 128, buffer))
			{			
				if (pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption1Enabled)
				{
					// Ascii type
					if (strlen(tmpbuf) == 5 || strlen(tmpbuf) == 13)
					{		
						KeyLen = strlen(tmpbuf);
						pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen = KeyLen;
						NdisMoveMemory(pAd->WdsTab.WdsEntry[i].WdsKey.Key, tmpbuf, KeyLen);
						if (KeyLen == 5)
							CipherAlg = CIPHER_WEP64;
						else
							CipherAlg = CIPHER_WEP128;	

						pAd->WdsTab.WdsEntry[i].WdsKey.CipherAlg = CipherAlg;
						DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d Key=%s ,type=Ascii, CipherAlg(%s)\n", i, tmpbuf, (CipherAlg == CIPHER_WEP64 ? "wep64" : "wep128")));
					}
					// Hex type
					else if (strlen(tmpbuf) == 10 || strlen(tmpbuf) == 26)
					{		
						KeyLen = strlen(tmpbuf);
						pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen = KeyLen / 2;
						AtoH(tmpbuf, pAd->WdsTab.WdsEntry[i].WdsKey.Key, KeyLen / 2);						
						if (KeyLen == 10)
							CipherAlg = CIPHER_WEP64;
						else
							CipherAlg = CIPHER_WEP128;	

						pAd->WdsTab.WdsEntry[i].WdsKey.CipherAlg = CipherAlg;
						DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d Key=%s ,type=Hex, CipherAlg(%s)\n", i, tmpbuf, (CipherAlg == CIPHER_WEP64 ? "wep64" : "wep128")));
					}
					// Invalid type
					else
					{
						pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11EncryptionDisabled;
						NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));
						DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d has invalid key for WEP, reset encryption to OPEN\n", i));
					}
				}
				else if ((pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption2Enabled)
					|| (pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption3Enabled))					
				{
					if ((strlen(tmpbuf) >= 8) && (strlen(tmpbuf) <= 64))
					{
						PasswordHash((char *)tmpbuf, RALINK_PASSPHRASE, sizeof(RALINK_PASSPHRASE), keyMaterial);

						if (pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption3Enabled)
							pAd->WdsTab.WdsEntry[i].WdsKey.CipherAlg = CIPHER_AES;
						else
							pAd->WdsTab.WdsEntry[i].WdsKey.CipherAlg = CIPHER_TKIP;
						
						NdisMoveMemory(&pAd->WdsTab.WdsEntry[i].WdsKey.Key, keyMaterial, 16);
						pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen = 16;
						NdisMoveMemory(&pAd->WdsTab.WdsEntry[i].WdsKey.RxMic, keyMaterial+16, 8);
						NdisMoveMemory(&pAd->WdsTab.WdsEntry[i].WdsKey.TxMic, keyMaterial+16, 8);
						DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d Key=%s, CipherAlg(%s)\n", i, tmpbuf, (CipherAlg == CIPHER_AES ? "AES" : "TKIP")));
					}
					else
					{
						DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d has invalid key for WPA, reset encryption to OPEN\n", i));
						pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11EncryptionDisabled;
						NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));
					}

				}
				else
				{									
					pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11EncryptionDisabled;
					NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));
				}								
			}
		}
	}

	// WdsDefaultKeyID
	if(RTMPGetKeyParameter("WdsDefaultKeyID", tmpbuf, 10, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_WDS_ENTRY); macptr = rstrtok(NULL,";"), i++)
		{
			KeyIdx = simple_strtol(macptr, 0, 10);
			if((KeyIdx >= 1 ) && (KeyIdx <= 4))
				pAd->WdsTab.WdsEntry[i].KeyIdx = (UCHAR) (KeyIdx - 1);
			else
				pAd->WdsTab.WdsEntry[i].KeyIdx = 0;

			if ((pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption2Enabled)
					|| (pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption3Enabled))
				pAd->WdsTab.WdsEntry[i].KeyIdx = 0;	

			DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d - WdsDefaultKeyID(0~3)=%d\n", i, pAd->WdsTab.WdsEntry[i].KeyIdx));	
		}				
	}
	
	//WdsEnable
	if(RTMPGetKeyParameter("WdsEnable", tmpbuf, 10, buffer))
	{						
		RT_802_11_WDS_ENTRY *pWdsEntry;
		switch(simple_strtol(tmpbuf, 0, 10))
		{
		    case 1: // Restricted mode
				pAd->WdsTab.Mode = WDS_RESTRICT_MODE;
				break;
			case 2: // Bridge mode, DisAllow association(stop Beacon generation and Probe Req.
				pAd->WdsTab.Mode = WDS_BRIDGE_MODE;
				break;
		    case 3: // Repeater mode
				pAd->WdsTab.Mode = WDS_REPEATER_MODE;
				break;
			case 4: // Lazy mode, Auto learn wds entry by same SSID, channel, security policy
				for(i = 0; i < MAX_WDS_ENTRY; i++)
				{
					pWdsEntry = &pAd->WdsTab.WdsEntry[i];
					if (pWdsEntry->Valid)
						WdsEntryDel(pAd, pWdsEntry->PeerWdsAddr);
				
					// When Lazy mode is enabled, the all wds-link shall share the same encryption type and key material
					if (i > 0)
					{
						pAd->WdsTab.WdsEntry[i].WepStatus = pAd->WdsTab.WdsEntry[0].WepStatus;
						pAd->WdsTab.WdsEntry[i].KeyIdx = pAd->WdsTab.WdsEntry[0].KeyIdx;
						NdisMoveMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, &pAd->WdsTab.WdsEntry[0].WdsKey, sizeof(CIPHER_KEY));
					}
				}
				pAd->WdsTab.Mode = WDS_LAZY_MODE;
				break;
		    case 0: // Disable mode
		    default:
				APWdsInitialize(pAd);
			    pAd->WdsTab.Mode = WDS_DISABLE_MODE;
			   	break;
		}

		DBGPRINT(RT_DEBUG_TRACE, ("WDS-Enable mode=%d\n", pAd->WdsTab.Mode));

#if 0
		for(i = 0; i < MAX_WDS_ENTRY; i++)
		{					
			DBGPRINT(RT_DEBUG_TRACE, ("IF/wds%d-%02x:%02x:%02x:%02x:%02x:%02x(%s) and %s\n", i, 
				PRINT_MAC(pAd->WdsTab.WdsEntry[i].PeerWdsAddr), 
				pAd->WdsTab.WdsEntry[i].Valid == 1 ? "Valid" : "Invalid",
				GetEncryptType(pAd->WdsTab.WdsEntry[i].WepStatus)));

			if (pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen > 0)
				hex_dump("Wds Key", pAd->WdsTab.WdsEntry[i].WdsKey.Key, pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen);
		}
#endif // DBG //
	}
	
}

VOID WdsPrepareWepKeyFromMainBss(
	IN  PRTMP_ADAPTER pAd)
{
	INT	i;
	
	// Prepare WEP key for each wds-link if necessary	
	for (i = 0; i < MAX_WDS_ENTRY; i++)
	{	
		// For WDS backward compatible, refer to the WEP key of Main BSS in WEP mode  
		if (pAd->WdsTab.WdsEntry[i].WepStatus == Ndis802_11Encryption1Enabled &&
			pAd->WdsTab.WdsEntry[i].WdsKey.KeyLen == 0)
		{
			UCHAR	main_bss_keyid = pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId;
		
			if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption1Enabled && 
				(pAd->SharedKey[MAIN_MBSSID][main_bss_keyid].KeyLen == 5 ||
				 pAd->SharedKey[MAIN_MBSSID][main_bss_keyid].KeyLen == 13))	
			{
				DBGPRINT(RT_DEBUG_TRACE, ("Duplicate IF/WDS%d wep key from main_bssid \n", (UCHAR)i));
				pAd->WdsTab.WdsEntry[i].KeyIdx = main_bss_keyid;
				NdisMoveMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, &pAd->SharedKey[MAIN_MBSSID][main_bss_keyid], sizeof(CIPHER_KEY));
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("No available wep key for IF/WDS%d, reset its encryption as OPEN \n", (UCHAR)i));
				pAd->WdsTab.WdsEntry[i].WepStatus = Ndis802_11EncryptionDisabled;
				NdisZeroMemory(&pAd->WdsTab.WdsEntry[i].WdsKey, sizeof(CIPHER_KEY));
			}
		}
	}

}

