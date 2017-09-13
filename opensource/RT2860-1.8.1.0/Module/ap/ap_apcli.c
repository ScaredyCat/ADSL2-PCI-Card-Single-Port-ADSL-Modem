/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_apcli.c

    Abstract:
    Support AP-Client function.

    Note:
    1. Call RT28xx_ApCli_Init() in init function and
       call RT28xx_ApCli_Remove() in close function

    2. MAC of ApCli-interface is initialized in RT28xx_ApCli_Init()

    3. ApCli index (0) of different rx packet is got in
       APHandleRxDoneInterrupt() by using FromWhichBSSID = pEntry->apidx;
       Or FromWhichBSSID = BSS0;

    4. ApCli index (0) of different tx packet is assigned in
       MBSS_VirtualIF_PacketSend() by using RTMP_SET_PACKET_NET_DEVICE_MBSSID()
    5. ApCli index (0) of different interface is got in APHardTransmit() by using
       RTMP_GET_PACKET_IF()

    6. ApCli index (0) of IOCTL command is put in pAd->OS_Cookie->ioctl_if

    8. The number of ApCli only can be 1

	9. apcli convert engine subroutines, we should just take care data packet.
    Revision History:
    Who             When            What
    --------------  ----------      ----------------------------------------------
    Shiang, Fonchi  02-13-2007      created
*/

#include "rt_config.h"


/* extern function prototype */
#if 0
extern INT rt28xx_ioctl(
	IN	PNET_DEV			net_dev, 
	IN	OUT	struct ifreq	*rq, 
	IN	INT					cmd);
#endif

/* local function prototype */
static INT ApCli_VirtualIF_Open(
	IN	PNET_DEV	dev_p);

static INT ApCli_VirtualIF_Close(
	IN	PNET_DEV	dev_p);

static INT ApCli_VirtualIF_PacketSend(
	IN PNDIS_PACKET		skb_p, 
	IN PNET_DEV			dev_p);

static INT ApCli_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p,
	IN OUT struct ifreq 	*rq_p,
	IN INT cmd);

#if 0 /* yet implement */
static struct net_device_stats *APCLI_VirtualIF_EtherStats_Get(
    IN  PNET_DEV	dev_p);
#endif





/* --------------------------------- Public -------------------------------- */
/*
========================================================================
Routine Description:
    Init AP-Client function.

Arguments:
    ad_p            points to our adapter
    main_dev_p      points to the main BSS network interface

Return Value:
    None

Note:
	1. Only create and initialize virtual network interfaces.
	2. No main network interface here.
========================================================================
*/
VOID RT28xx_ApCli_Init(
	IN PRTMP_ADAPTER 		ad_p,
	IN PNET_DEV				main_dev_p)
{
#define APCLI_MAX_DEV_NUM	32
	PNET_DEV	cur_dev_p;
	PNET_DEV	new_dev_p;
	VIRTUAL_ADAPTER *apcli_ad_p;
	CHAR slot_name[IFNAMSIZ];
	INT apcli_index;
	INT index = 0;


	/* sanity check to avoid redundant virtual interfaces are created */
	if (ad_p->flg_apcli_init != FALSE)
		return;

	ad_p->flg_apcli_init = TRUE;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	/* init */
	for(apcli_index = 0; apcli_index < MAX_APCLI_NUM; apcli_index++)
		ad_p->ApCfg.ApCliTab[apcli_index].dev = NULL;

	/* create virtual network interface */
	for(apcli_index = 0; apcli_index < MAX_APCLI_NUM; apcli_index++)
	{
		/* allocate a new network device */
#if LINUX_VERSION_CODE <= 0x20402 // Red Hat 7.1
		new_dev_p = alloc_netdev(sizeof(VIRTUAL_ADAPTER), "eth%d", ether_setup);
#else
		new_dev_p = alloc_etherdev(sizeof(VIRTUAL_ADAPTER));
#endif // LINUX_VERSION_CODE //

		if (new_dev_p == NULL)
		{
			/* allocation fail, exit */
			DBGPRINT(RT_DEBUG_ERROR,
						("Allocate network device fail (APCLI)...\n"));
			break;
		}


        /* find a available interface name, max 32 ra interfaces */
		for(index = 0; index < APCLI_MAX_DEV_NUM; index++)
		{
#ifdef MULTIPLE_CARD_SUPPORT
			if (ad_p->MC_RowID >= 0)
				sprintf(slot_name, "apcli%02d_%d", ad_p->MC_RowID, index);
			else
#endif // MULTIPLE_CARD_SUPPORT //
			sprintf(slot_name, "apcli%d", index);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			cur_dev_p = dev_get_by_name(slot_name);
#else
			for(cur_dev_p=dev_base; cur_dev_p!=NULL; cur_dev_p=cur_dev_p->next)
			{
				if (strncmp(cur_dev_p->name, slot_name, 6) == 0)
					break;
			}
#endif // LINUX_VERSION_CODE //

			if (cur_dev_p == NULL)
				break; /* fine, the RA name is not used */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			else
			{
				/* every time dev_get_by_name is called, and it has returned a
					valid struct net_device*, dev_put should be called
					afterwards, because otherwise the machine hangs when the
					device is unregistered (since dev->refcnt > 1). */
				dev_put(cur_dev_p);
			}
#endif // LINUX_VERSION_CODE //
		} /* End of for */

		/* assign interface name to the new network interface */
		if (index < APCLI_MAX_DEV_NUM)
		{
#ifdef MULTIPLE_CARD_SUPPORT
			if (ad_p->MC_RowID >= 0)
				sprintf(new_dev_p->name, "apcli%02d_%d", ad_p->MC_RowID, index);
			else
#endif // MULTIPLE_CARD_SUPPORT //
				sprintf(new_dev_p->name, "apcli%d", index);
			DBGPRINT(RT_DEBUG_TRACE, ("Register APCLI IF (apcli%d)\n", index));
		}
		else
		{
			/* error! no any available ra name can be used! */
			DBGPRINT(RT_DEBUG_ERROR,
						("Has %d apcli interfaces (APCLI)...\n", APCLI_MAX_DEV_NUM));
			kfree(new_dev_p);
			break;
		}

 		/* init the new network interface */
		ether_setup(new_dev_p);

		apcli_ad_p = new_dev_p->priv; /* sizeof(priv) = sizeof(VIRTUAL_ADAPTER) */
		apcli_ad_p->VirtualDev = new_dev_p;  /* 4 Bytes */
		apcli_ad_p->RtmpDev    = main_dev_p; /* 4 Bytes */

		/* init MAC address of virtual network interface */
		COPY_MAC_ADDR(ad_p->ApCfg.ApCliTab[apcli_index].CurrentAddress, ad_p->CurrentAddress);
		ad_p->ApCfg.ApCliTab[apcli_index].CurrentAddress[ETH_LENGTH_OF_ADDRESS - 1] =
			(ad_p->ApCfg.ApCliTab[apcli_index].CurrentAddress[ETH_LENGTH_OF_ADDRESS - 1] + ad_p->ApCfg.BssidNum + MAX_MESH_NUM) & 0xFF;
		NdisMoveMemory(&new_dev_p->dev_addr,
						ad_p->ApCfg.ApCliTab[apcli_index].CurrentAddress,
						MAC_ADDR_LEN);

		/* init operation functions */
		new_dev_p->open				= ApCli_VirtualIF_Open;
		new_dev_p->stop				= ApCli_VirtualIF_Close;
		new_dev_p->hard_start_xmit	= (HARD_START_XMIT_FUNC)ApCli_VirtualIF_PacketSend;
		new_dev_p->do_ioctl			= ApCli_VirtualIF_Ioctl;
		/* if you dont implement get_stats, dont assign your function with empty
			body; or kernel will panic */
		//new_dev_p->get_stats		= ApCli_VirtualIF_EtherStats_Get;
		new_dev_p->priv_flags		= INT_APCLI; /* we are virtual interface */

		/* register this device to OS */
		register_netdevice(new_dev_p);

		/* backup our virtual network interface */
        ad_p->ApCfg.ApCliTab[apcli_index].dev = new_dev_p;
        
#ifdef WSC_AP_SUPPORT
        ad_p->ApCfg.ApCliTab[apcli_index].WscControl.pAd = ad_p;        
        ad_p->ApCfg.ApCliTab[apcli_index].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;
        if (ad_p->ApCfg.ApCliTab[apcli_index].WscControl.WscEnrolleePinCode == 0)
            ad_p->ApCfg.ApCliTab[apcli_index].WscControl.WscEnrolleePinCode = WscGeneratePinCode(ad_p, TRUE, 0);
        NdisZeroMemory(ad_p->ApCfg.ApCliTab[apcli_index].WscControl.EntryAddr, MAC_ADDR_LEN);
        WscInit(ad_p, TRUE, &ad_p->ApCfg.ApCliTab[apcli_index].WscControl);
#endif // WSC_AP_SUPPORT //
	} /* End of for */

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
}


/*
========================================================================
Routine Description:
    Close ApCli network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID RT28xx_ApCli_Close(
	IN PRTMP_ADAPTER ad_p)
{
	UINT index;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	for(index = 0; index < MAX_APCLI_NUM; index++)
	{
		if (ad_p->ApCfg.ApCliTab[index].dev)
			dev_close(ad_p->ApCfg.ApCliTab[index].dev);
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
}


/*
========================================================================
Routine Description:
    Remove ApCli-BSS network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID RT28xx_ApCli_Remove(
	IN PRTMP_ADAPTER ad_p)
{
	UINT index;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	for(index = 0; index < MAX_APCLI_NUM; index++)
	{
		if (ad_p->ApCfg.ApCliTab[index].dev)
		{
			unregister_netdev(ad_p->ApCfg.ApCliTab[index].dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			free_netdev(ad_p->ApCfg.ApCliTab[index].dev);
#else
			kfree(ad_p->ApCfg.ApCliTab[index].dev);
#endif // LINUX_VERSION_CODE //

			// Clear it as NULL to prevent latter access error.
			ad_p->flg_apcli_init = FALSE;
			ad_p->ApCfg.ApCliTab[index].dev = NULL;
		}
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
}




/* --------------------------------- Private -------------------------------- */
/*
========================================================================
Routine Description:
    Open a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: open successfully
    otherwise: open fail

Note:
========================================================================
*/
static INT ApCli_VirtualIF_Open(
	IN PNET_DEV		dev_p)
{
	UCHAR ifIndex;
	VIRTUAL_ADAPTER *virtual_ad_p = dev_p->priv;
	RTMP_ADAPTER *ad_p;

	/* sanity check */
	ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
	ad_p = virtual_ad_p->RtmpDev->priv;
	ASSERT(ad_p);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, dev_p->name));
	ASSERT(virtual_ad_p->VirtualDev);

	for (ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		if (ad_p->ApCfg.ApCliTab[ifIndex].dev == dev_p)
		{
			netif_start_queue(virtual_ad_p->VirtualDev);
			ApCliIfUp(ad_p);
		}
	}
	return 0;
} /* End of ApCli_VirtualIF_Open */


/*
========================================================================
Routine Description:
    Close a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: close successfully
    otherwise: close fail

Note:
========================================================================
*/
static INT ApCli_VirtualIF_Close(
	IN	PNET_DEV	dev_p)
{
	UCHAR ifIndex;
	VIRTUAL_ADAPTER	*virtual_ad_p = dev_p->priv;

	RTMP_ADAPTER *ad_p;

	/* sanity check */
	ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
	ad_p = virtual_ad_p->RtmpDev->priv;
	ASSERT(ad_p);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, dev_p->name));
	ASSERT(virtual_ad_p->VirtualDev);

	for (ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		if (ad_p->ApCfg.ApCliTab[ifIndex].dev == dev_p)
		{
			netif_stop_queue(virtual_ad_p->VirtualDev);

			// send disconnect-req to sta State Machine.
			if (ad_p->ApCfg.ApCliTab[ifIndex].Enable)
			{
				MlmeEnqueueEx(ad_p, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL, ifIndex);
				RT28XX_MLME_HANDLER(ad_p);
				DBGPRINT(RT_DEBUG_TRACE, ("(%s) ApCli interface[%d] startdown.\n", __FUNCTION__, ifIndex));
			}
			break;
		}
	}
	return 0;
} /* End of ApCli_VirtualIF_Close */


/*
========================================================================
Routine Description:
    Send a packet to WLAN.

Arguments:
    skb_p           points to our adapter
    dev_p           which WLAN network interface

Return Value:
    0: transmit successfully
    otherwise: transmit fail

Note:
========================================================================
*/
static INT ApCli_VirtualIF_PacketSend(
	IN PNDIS_PACKET 	skb_p, 
	IN PNET_DEV			dev_p)
{
	VIRTUAL_ADAPTER *virtual_ad_p = dev_p->priv;
	RTMP_ADAPTER *ad_p;
	PAPCLI_STRUCT pApCli;
	INT apcliIndex;


	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	/* sanity check */
	ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
	ad_p = virtual_ad_p->RtmpDev->priv;
	ASSERT(ad_p);

#ifdef RALINK_ATE
	if (ad_p->ate.Mode != ATE_STOP)
	{
		RELEASE_NDIS_PACKET(ad_p, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}
#endif // RALINK_ATE //

	if ((RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) ||
		(RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_RADIO_OFF))          ||
		(RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_RESET_IN_PROGRESS)))
	{
		/* wlan is scanning/disabled/reset */
		RELEASE_NDIS_PACKET(ad_p, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}

	if (!(virtual_ad_p->RtmpDev->flags & IFF_UP))
	{
		/* the interface is down */
		RELEASE_NDIS_PACKET(ad_p, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}


	pApCli = (PAPCLI_STRUCT)&ad_p->ApCfg.ApCliTab;

	for(apcliIndex = 0; apcliIndex < MAX_APCLI_NUM; apcliIndex++)
	{
		if (pApCli[apcliIndex].Valid != TRUE)
			continue;

		/* find the device in our ApCli list */
		if (pApCli[apcliIndex].dev == dev_p)
		{
			/* ya! find it */
			ad_p->RalinkCounters.PendingNdisPacketCount ++;
			RTMP_SET_PACKET_SOURCE(skb_p, PKTSRC_NDIS);
			RTMP_SET_PACKET_MOREDATA(skb_p, FALSE);
			RTMP_SET_PACKET_NET_DEVICE_APCLI(skb_p, apcliIndex);
			RTPKT_TO_OSPKT(skb_p)->dev = virtual_ad_p->RtmpDev;

			if (!(*(RTPKT_TO_OSPKT(skb_p)->data) & 0x01))
			{
				DBGPRINT(RT_DEBUG_INFO,
							("%s(ApCli) - unicast packet to "
							"(apcli%d)\n", __FUNCTION__, apcliIndex));
			}

			/* transmit the packet */
			return rt28xx_packet_xmit(RTPKT_TO_OSPKT(skb_p));
		}
    }


	/* can not find the BSS so discard the packet */
	DBGPRINT(RT_DEBUG_INFO,
				("%s - needn't to send or net_device not "
					"exist.\n", __FUNCTION__));
	RELEASE_NDIS_PACKET(ad_p, skb_p, NDIS_STATUS_FAILURE);
	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
	return 0;
} /* End of ApCli_VirtualIF_PacketSend */


/*
========================================================================
Routine Description:
    IOCTL to WLAN.

Arguments:
    dev_p           which WLAN network interface
    rq_p            command information
    cmd             command ID

Return Value:
    0: IOCTL successfully
    otherwise: IOCTL fail

Note:
    SIOCETHTOOL     8946    New drivers use this ETHTOOL interface to
                            report link failure activity.
========================================================================
*/
static INT ApCli_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT struct ifreq 	*rq_p, 
	IN INT 					cmd)
{
	VIRTUAL_ADAPTER *virtual_ad_p;
	RTMP_ADAPTER *ad_p;

	/* sanity check */
    virtual_ad_p = dev_p->priv;
    ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
    ad_p = virtual_ad_p->RtmpDev->priv;
    ASSERT(ad_p);

	if (!RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_INTERRUPT_IN_USE))
		return -ENETDOWN;
	/* End of if */

	/* do real IOCTL */
	return rt28xx_ioctl(dev_p, rq_p, cmd);
} /* End of ApCli_VirtualIF_Ioctl */


#if 0
/*
========================================================================
Routine Description:
    IOCTL to WLAN.

Arguments:
    dev_p           which WLAN network interface
    rq_p            command information
    cmd             command ID

Return Value:
    0: IOCTL successfully
    otherwise: IOCTL fail

Note:
    1. not implement
    2. can not return NULL; Or kernel will crush.
========================================================================
*/
static struct net_device_stats *ApCli_VirtualIF_EtherStats_Get(
    IN  PNET_DEV	dev_p)
{
//	VIRTUAL_ADAPTER *virtual_ad_p = dev_p->priv;
//	RTMP_ADAPTER *ad_p = virtual_ad_p->RtmpDev->priv;


    DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));
    return NULL;
} /* End of ApCli_VirtualIF_EtherStats_Get */
#endif

SHORT ApCliIfLookUp(
	IN PRTMP_ADAPTER pAd,
	IN VOID *Msg)
{
	SHORT i;
	SHORT ifIndex = -1;
	PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;

	do
	{
		for(i = 0; i < MAX_APCLI_NUM; i++)
		{
			//if(pAd->ApCliTab[i].Enable == FALSE)
			//	continue;

			if(	MAC_ADDR_EQUAL(pAd->ApCfg.ApCliTab[i].CurrentAddress, pFrame->Hdr.Addr1))
			{
				ifIndex = i;
				DBGPRINT(RT_DEBUG_TRACE, ("(%s) ApCliIfIndex = %d\n", __FUNCTION__, ifIndex));
				break;
			}
		}
	} while (FALSE);

	return ifIndex;
}

BOOLEAN isValidApCliIf(
	SHORT ifIndex)
{
	if((ifIndex >= 0) && (ifIndex < MAX_APCLI_NUM))
		return TRUE;
	else
		return FALSE;
}

// The MacTableDeleteApCliEntry is integrated into ap.c::MacTableDeleteEntry
// The MacTableInsertApCliEntry is integrated into cmm_data.c::MacTableInsertEntry
// The ApCliTableLookUp 		is integrated into cmm_data.c::MacTableLookup
#if 0 	
/*
	==========================================================================
	Description:
		Delete all WDS Entry in pAd->MacTab
	==========================================================================
 */
BOOLEAN MacTableDeleteApCliEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr)
{
	USHORT HashIdx;
	MAC_TABLE_ENTRY *pEntry;
	PAPCLI_STRUCT pApCliEntry = NULL;

	if (wcid >= MAX_LEN_OF_MAC_TABLE)
		return FALSE;

	// delete one ApCli entry
	NdisAcquireSpinLock(&pAd->MacTabLock);

	pEntry = &pAd->MacTab.Content[wcid];
	if (pEntry->MatchAPCLITabIdx < MAX_APCLI_NUM)
	{
		UCHAR ApCliTabIdx = pEntry->MatchAPCLITabIdx;

		pApCliEntry = (PAPCLI_STRUCT)&pAd->ApCfg.ApCliTab[ApCliTabIdx];
		if (pApCliEntry->Valid == TRUE) 
		{
			// free resources of BA
			BASessionTearDownALL(pAd, pEntry->Aid);

			pApCliEntry->Valid = FALSE;
		}
	}

	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = &pAd->MacTab.Content[wcid];

	if (pEntry)
	{
		if (MAC_ADDR_EQUAL(pEntry->Addr, pAddr))
		{
			pAd->MacTab.Hash[HashIdx] = pEntry->pNext;

			AsicRemovePairwiseKeyEntry(pAd, (pEntry->MatchAPCLITabIdx + pAd->ApCfg.BssidNum), (UCHAR)wcid);
			NdisZeroMemory(pEntry, sizeof(MAC_TABLE_ENTRY));
			pAd->MacTab.Size --;
			DBGPRINT(RT_DEBUG_TRACE, ("MacTableDeleteApCliEntry - Total= %d\n", pAd->MacTab.Size));
		}
		else
		{
			printk("\n%s: Impossible Wcid = %d !!!!!\n", __FUNCTION__, wcid);
		}
	}

	NdisReleaseSpinLock(&pAd->MacTabLock);

	return TRUE;
}

/*
================================================================
Description : because ApCli and CLI share the same WCID table in ASIC. 
ApCli entry also insert to pAd->MacTab.content[].  Such is marked as ValidAsApCli as TRUE.
Also fills the pairwise key.
Because front MAX_AID_BA entries have direct mapping to BAEntry, which is only used as CLI. So we insert WDS
from index MAX_AID_BA.
================================================================
*/
MAC_TABLE_ENTRY *MacTableInsertApCliEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr)
{
	UCHAR HashIdx;
	int i;
	UINT ApCliIdx = 0;
	PMAC_TABLE_ENTRY pEntry = NULL, pCurrEntry;
	PAPCLI_STRUCT pApCliEntry = NULL;


	// if FULL, return
	if (pAd->MacTab.Size >= MAX_LEN_OF_MAC_TABLE)
		return NULL;

	if((pEntry = ApCliTableLookUp(pAd, pAddr)) != NULL)
		return pEntry;

	// allocate one ApCli entry
	NdisAcquireSpinLock(&pAd->MacTabLock);

	do
	{
		for (i = 0; i < MAX_APCLI_NUM; i++)
		{
			pApCliEntry = (PAPCLI_STRUCT)&pAd->ApCfg.ApCliTab[i];
			if (pApCliEntry->Valid == FALSE)
			{
				pApCliEntry->Valid = TRUE;
				ApCliIdx = i;
				break;
			}
		}

		if (i == MAX_APCLI_NUM)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to allocate ApCliEntry.\n", __FUNCTION__));
			pEntry = NULL;
			break;
		}

		// allocate one MAC entry
		for (i = 1; i< MAX_LEN_OF_MAC_TABLE; i++)   // skip entry#0 so that "entry index == AID" for fast lookup
		{
			// pick up the first available vacancy
			if ((pAd->MacTab.Content[i].ValidAsCLI == FALSE) 
				&& (pAd->MacTab.Content[i].ValidAsWDS == FALSE) 
				&& (pAd->MacTab.Content[i].ValidAsApCli == FALSE))
			{
				pEntry = &pAd->MacTab.Content[i];
				NdisZeroMemory(pEntry, sizeof(MAC_TABLE_ENTRY));
				pEntry->ValidAsWDS = FALSE;
				pEntry->ValidAsCLI = FALSE;
				pEntry->ValidAsApCli = TRUE;
				
				pEntry->isCached = FALSE;

				pEntry->pAd = pAd;
				pEntry->AuthMode = pApCliEntry->AuthMode;
				pEntry->WepStatus = pApCliEntry->WepStatus;
				
				if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
				{
					pEntry->WpaState = AS_NOTUSE;
					pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
				}
				else
				{
					pEntry->WpaState = AS_PTKSTART;
					pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
				}

				pEntry->PairwiseKey.KeyLen = 0;
				pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
				pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
				AsicRemovePairwiseKeyEntry(pAd, (pAd->ApCfg.BssidNum + ApCliIdx), (UCHAR)i);
				
				COPY_MAC_ADDR(pEntry->Addr, pAddr);
				pEntry->Sst = SST_ASSOC;
				pEntry->AuthState = AS_NOT_AUTH;
				pEntry->Aid = (USHORT)i; 
				pEntry->CapabilityInfo = 0;
				pEntry->PsMode = PWR_ACTIVE;
				pEntry->MaxSupportedRate = pAd->CommonCfg.MaxTxRate;
				pEntry->CurrTxRate = pAd->CommonCfg.MaxTxRate;
				pEntry->PsQIdleCount = 0;
				pEntry->NoDataIdleCount = 0;

				InitializeQueueHeader(&pEntry->PsQueue);
				pAd->MacTab.Size ++;
				pApCliEntry->MacTabWCID = (UCHAR)i;
				pEntry->MatchAPCLITabIdx = ApCliIdx;

				// It's critical to do this
				AsicUpdateRxWCIDTable(pAd, pEntry->Aid, pEntry->Addr);

				DBGPRINT(RT_DEBUG_TRACE, ("MacTableInsertApCliEntry - allocate entry #%d, Total= %d\n",i, pAd->MacTab.Size));
				break;
			}
		}
	}while(FALSE);

	// add this MAC entry into HASH table
	if (pEntry)
	{
		HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
		if (pAd->MacTab.Hash[HashIdx] == NULL)
		{
			pAd->MacTab.Hash[HashIdx] = pEntry;
		}
		else
		{
			pCurrEntry = pAd->MacTab.Hash[HashIdx];
			while (pCurrEntry->pNext != NULL)
				pCurrEntry = pCurrEntry->pNext;
			pCurrEntry->pNext = pEntry;
		}
	}

	NdisReleaseSpinLock(&pAd->MacTabLock);

	return pEntry;
}

MAC_TABLE_ENTRY *ApCliTableLookUp(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	USHORT HashIdx;
	ULONG ApCliIndex;
	PMAC_TABLE_ENTRY pCurEntry = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;

	NdisAcquireSpinLock(&pAd->MacTabLock);
	
	do
	{
		HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
		pCurEntry = pAd->MacTab.Hash[HashIdx];

		ApCliIndex = 0xff;
		if ((pCurEntry) && (pCurEntry->ValidAsApCli== TRUE))
		{
			ApCliIndex = pCurEntry->MatchAPCLITabIdx;
		}

		if (ApCliIndex == 0xff)
			break;

		if (pAd->ApCfg.ApCliTab[ApCliIndex].Valid != TRUE)
			break;

		if (MAC_ADDR_EQUAL(pCurEntry->Addr, pAddr))
		{
			pEntry = pCurEntry;
			break;
		}
	} while(FALSE);


	NdisReleaseSpinLock(&pAd->MacTabLock);

	return pEntry;
}
#endif // 0 //

/*! \brief init the management mac frame header
 *  \param p_hdr mac header
 *  \param subtype subtype of the frame
 *  \param p_ds destination address, don't care if it is a broadcast address
 *  \return none
 *  \pre the station has the following information in the pAd->UserCfg
 *   - bssid
 *   - station address
 *  \post
 *  \note this function initializes the following field
 */
VOID ApCliMgtMacHeaderInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN OUT PHEADER_802_11 pHdr80211, 
    IN UCHAR SubType, 
    IN UCHAR ToDs, 
    IN PUCHAR pDA, 
    IN PUCHAR pBssid,
    IN USHORT ifIndex)
{
    NdisZeroMemory(pHdr80211, sizeof(HEADER_802_11));
    pHdr80211->FC.Type = BTYPE_MGMT;
    pHdr80211->FC.SubType = SubType;
    pHdr80211->FC.ToDs = ToDs;
    COPY_MAC_ADDR(pHdr80211->Addr1, pDA);
    COPY_MAC_ADDR(pHdr80211->Addr2, pAd->ApCfg.ApCliTab[ifIndex].CurrentAddress);
    COPY_MAC_ADDR(pHdr80211->Addr3, pBssid);
}

/*
	========================================================================

	Routine Description:
		Verify the support rate for HT phy type

	Arguments:
		pAd 				Pointer to our adapter

	Return Value:
		FALSE if pAd->CommonCfg.SupportedHtPhy doesn't accept the pHtCapability.  (AP Mode)

	IRQL = PASSIVE_LEVEL

	========================================================================
*/
BOOLEAN ApCliCheckHt(
	IN		PRTMP_ADAPTER 		pAd,
	IN		USHORT 				IfIndex,
	IN OUT	HT_CAPABILITY_IE 	*pHtCapability,
	IN OUT	ADD_HT_INFO_IE 		*pAddHtInfo)
{
	PAPCLI_STRUCT pApCliEntry = NULL;
	
	if (IfIndex >= MAX_APCLI_NUM)
		return FALSE;

	pApCliEntry = &pAd->ApCfg.ApCliTab[IfIndex];

	// If use AMSDU, set flag.
	if (pAd->CommonCfg.DesiredHtPhy.AmsduEnable)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_AMSDU_INUSED);
	// Save Peer Capability
	if (pHtCapability->HtCapInfo.ShortGIfor20)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_SGI20_CAPABLE);
	if (pHtCapability->HtCapInfo.ShortGIfor40)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_SGI40_CAPABLE);
	if (pHtCapability->HtCapInfo.TxSTBC)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_TxSTBC_CAPABLE);
	if (pHtCapability->HtCapInfo.RxSTBC)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_RxSTBC_CAPABLE);
	if (pAd->CommonCfg.bRdg && pHtCapability->ExtHtCapInfo.RDGSupport)
		CLIENT_STATUS_SET_FLAG(pApCliEntry, fCLIENT_STATUS_RDG_CAPABLE);
	pApCliEntry->MpduDensity = pHtCapability->HtCapParm.MpduDensity;

	if ((pAd->OpMode == OPMODE_STA))
	{
#if 0 //Dennis TODO		
		if (BaSizeArray[pHtCapability->HtCapParm.MaxRAmpduFactor] > pAd->CommonCfg.BACapability.field.TxBAWinLimit)
			pAd->MacTab.Content[Wcid].BaSizeInUse = (UCHAR)pAd->CommonCfg.BACapability.field.TxBAWinLimit;
		else
			pAd->MacTab.Content[Wcid].BaSizeInUse = BaSizeArray[pHtCapability->HtCapParm.MaxRAmpduFactor];
#endif		
	}
	pAd->MlmeAux.HtCapability.MCSSet[0] = 0xff;
	pAd->MlmeAux.HtCapability.MCSSet[4] = 0x1;
	if (pAd->Antenna.field.TxPath == 2)	// 2: 2Tx   1: 1Tx
	{
		pAd->MlmeAux.HtCapability.MCSSet[1] = 0xff;
	}
	else
	{
		pAd->MlmeAux.HtCapability.MCSSet[1] = 0x00;
	}

	// choose smaller setting
	pAd->MlmeAux.HtCapability.HtCapInfo.ChannelWidth = pAddHtInfo->AddHtInfo.RecomWidth & pAd->CommonCfg.DesiredHtPhy.ChannelWidth;
	pAd->MlmeAux.HtCapability.HtCapInfo.GF =  pHtCapability->HtCapInfo.GF &pAd->CommonCfg.DesiredHtPhy.GF;

	// Send Assoc Req with my HT capability.
	pAd->MlmeAux.HtCapability.HtCapInfo.AMsduSize =  pAd->CommonCfg.DesiredHtPhy.AmsduSize;
	pAd->MlmeAux.HtCapability.HtCapInfo.MimoPs =  pAd->CommonCfg.DesiredHtPhy.MimoPs;
	pAd->MlmeAux.HtCapability.HtCapInfo.ShortGIfor20 =  (pAd->CommonCfg.DesiredHtPhy.ShortGIfor20) & (pHtCapability->HtCapInfo.ShortGIfor20);
	pAd->MlmeAux.HtCapability.HtCapInfo.ShortGIfor40 =  (pAd->CommonCfg.DesiredHtPhy.ShortGIfor40) & (pHtCapability->HtCapInfo.ShortGIfor40);
	pAd->MlmeAux.HtCapability.HtCapInfo.TxSTBC =  (pAd->CommonCfg.DesiredHtPhy.TxSTBC)&(pHtCapability->HtCapInfo.RxSTBC);
	pAd->MlmeAux.HtCapability.HtCapInfo.RxSTBC =  (pAd->CommonCfg.DesiredHtPhy.RxSTBC)&(pHtCapability->HtCapInfo.TxSTBC);
	pAd->MlmeAux.HtCapability.HtCapParm.MaxRAmpduFactor = pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor;
	pAd->MlmeAux.HtCapability.HtCapParm.MpduDensity = pHtCapability->HtCapParm.MpduDensity;
	pAd->MlmeAux.HtCapability.ExtHtCapInfo.PlusHTC = pHtCapability->ExtHtCapInfo.PlusHTC;
	if (pAd->CommonCfg.bRdg)
	{
		pAd->MlmeAux.HtCapability.ExtHtCapInfo.RDGSupport = pHtCapability->ExtHtCapInfo.RDGSupport;
	}
	
	COPY_AP_HTSETTINGS_FROM_BEACON(pAd, pHtCapability);
	return TRUE;
}

/*
    ==========================================================================

	Routine	Description:
		Connected to the BSSID

	Arguments:
		pAd				- Pointer to our adapter
		ApCliIdx		- Which ApCli interface		
	Return Value:		
		FALSE: fail to alloc Mac entry.

	Note:

	==========================================================================
*/
BOOLEAN ApCliLinkUp(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ifIndex)
{
	BOOLEAN result = FALSE;
	PAPCLI_STRUCT pApCliEntry = NULL;
	PMAC_TABLE_ENTRY pMacEntry = NULL;


	do
	{
		if (ifIndex < MAX_APCLI_NUM)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("!!! APCLI LINK UP - IF(apcli%d) AuthMode(%d)=%s, WepStatus(%d)=%s !!!\n", 
										ifIndex, 
										pAd->ApCfg.ApCliTab[ifIndex].AuthMode, GetAuthMode(pAd->ApCfg.ApCliTab[ifIndex].AuthMode),
										pAd->ApCfg.ApCliTab[ifIndex].WepStatus, GetEncryptType(pAd->ApCfg.ApCliTab[ifIndex].WepStatus)));			
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("!!! ERROR : APCLI LINK UP - IF(apcli%d)!!!\n", ifIndex));
			result = FALSE;
			break;
		}

		pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

		// Sanity check: This link had existed. 
		if (pApCliEntry->Valid)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("!!! ERROR : This link had existed - IF(apcli%d)!!!\n", ifIndex));
			result = FALSE;
			break;
		}
	
		// Insert the Remote AP to our MacTable.
		//pMacEntry = MacTableInsertApCliEntry(pAd, (PUCHAR)(pAd->MlmeAux.Bssid));
		pMacEntry = MacTableInsertEntry(pAd, (PUCHAR)(pAd->MlmeAux.Bssid), (ifIndex + MIN_NET_DEVICE_FOR_APCLI), TRUE);
		if (pMacEntry)
		{
			UCHAR Rates[MAX_LEN_OF_SUPPORTED_RATES];
			PUCHAR pRates = Rates;
			UCHAR RatesLen;
			UCHAR MaxSupportedRate = 0;

			pMacEntry->Sst = SST_ASSOC;
			
			pApCliEntry->Valid = TRUE;
			pApCliEntry->MacTabWCID = pMacEntry->Aid;

			COPY_MAC_ADDR(APCLI_ROOT_BSSID_GET(pAd, pApCliEntry->MacTabWCID), pAd->MlmeAux.Bssid);
			pApCliEntry->SsidLen = pAd->MlmeAux.SsidLen;
			NdisMoveMemory(pApCliEntry->Ssid, pAd->MlmeAux.Ssid, pApCliEntry->SsidLen);

			if (pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
				pMacEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
			else
				pMacEntry->PortSecured = WPA_802_1X_PORT_SECURED;

			// Store appropriate RSN_IE for WPA SM negotiation later 
			// If WPAPSK/WPA2SPK mix mode, driver just stores either WPAPSK or WPA2PSK
			// RSNIE. It depends on the AP-Client's authentication mode to store the corresponding RSNIE.   
			if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA) && (pAd->MlmeAux.VarIELen != 0))
			{
				PUCHAR              pVIE;
				UCHAR               len;
				PEID_STRUCT         pEid;

				pVIE = pAd->MlmeAux.VarIEs;
				len	 = pAd->MlmeAux.VarIELen;

				while (len > 0)
				{
					pEid = (PEID_STRUCT) pVIE;	
					// For WPA/WPAPSK
					if ((pEid->Eid == IE_WPA) && (NdisEqualMemory(pEid->Octet, WPA_OUI, 4)) 
						&& (pMacEntry->AuthMode == Ndis802_11AuthModeWPA || pMacEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
					{
						NdisMoveMemory(pMacEntry->RSN_IE, pVIE, (pEid->Len + 2));
						pMacEntry->RSNIE_Len = (pEid->Len + 2);							
						DBGPRINT(RT_DEBUG_TRACE, ("ApCliLinkUp: Store RSN_IE for WPA SM negotiation \n"));
					}
					// For WPA2/WPA2PSK
					else if ((pEid->Eid == IE_RSN) && (NdisEqualMemory(pEid->Octet + 2, RSN_OUI, 3))
						&& (pMacEntry->AuthMode == Ndis802_11AuthModeWPA2 || pMacEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
					{
						NdisMoveMemory(pMacEntry->RSN_IE, pVIE, (pEid->Len + 2));
						pMacEntry->RSNIE_Len = (pEid->Len + 2);	
						DBGPRINT(RT_DEBUG_TRACE, ("ApCliLinkUp: Store RSN_IE for WPA2 SM negotiation \n"));
					}

					pVIE += (pEid->Len + 2);
					len  -= (pEid->Len + 2);
				}							
			}

			if (pMacEntry->RSNIE_Len == 0)
			{			
				DBGPRINT(RT_DEBUG_TRACE, ("ApCliLinkUp: root-AP has no RSN_IE \n"));			
			}
			else
			{
				hex_dump("The RSN_IE of root-AP", pMacEntry->RSN_IE, pMacEntry->RSNIE_Len);
			}		

			SupportRate(pAd->MlmeAux.SupRate, pAd->MlmeAux.SupRateLen, pAd->MlmeAux.ExtRate,
				pAd->MlmeAux.ExtRateLen, &pRates, &RatesLen, &MaxSupportedRate);

			pMacEntry->MaxSupportedRate = min(pAd->CommonCfg.MaxTxRate, MaxSupportedRate);
			pMacEntry->RateLen = RatesLen;
			if (pMacEntry->MaxSupportedRate < RATE_FIRST_OFDM_RATE)
			{
				pMacEntry->MaxHTPhyMode.field.MODE = MODE_CCK;
				pMacEntry->MaxHTPhyMode.field.MCS = pMacEntry->MaxSupportedRate;
				pMacEntry->MinHTPhyMode.field.MODE = MODE_CCK;
				pMacEntry->MinHTPhyMode.field.MCS = pMacEntry->MaxSupportedRate;
				pMacEntry->HTPhyMode.field.MODE = MODE_CCK;
				pMacEntry->HTPhyMode.field.MCS = pMacEntry->MaxSupportedRate;
			}
			else
			{
				pMacEntry->MaxHTPhyMode.field.MODE = MODE_OFDM;
				pMacEntry->MaxHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pMacEntry->MaxSupportedRate];
				pMacEntry->MinHTPhyMode.field.MODE = MODE_OFDM;
				pMacEntry->MinHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pMacEntry->MaxSupportedRate];
				pMacEntry->HTPhyMode.field.MODE = MODE_OFDM;
				pMacEntry->HTPhyMode.field.MCS = OfdmRateToRxwiMCS[pMacEntry->MaxSupportedRate];
			}
			pMacEntry->CapabilityInfo = pAd->MlmeAux.CapabilityInfo;

			// If WEP is enabled, add paiewise and shared key
			if (pApCliEntry->WepStatus == Ndis802_11WEPEnabled)
			{			
				PUCHAR	Key; 			
				UCHAR 	CipherAlg, idx, BssIdx;

				BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + ifIndex;
			
				for (idx=0; idx < SHARE_KEY_NUM; idx++)
    	    	{
					CipherAlg = pApCliEntry->SharedKey[idx].CipherAlg;
    				Key = pApCliEntry->SharedKey[idx].Key;
										
					if (pApCliEntry->SharedKey[idx].KeyLen > 0)
					{
						// Set key material and cipherAlg to Asic
	    				AsicAddSharedKeyEntry(pAd, 
	    									  BssIdx, 
	    									  idx, 
	    									  CipherAlg, 
	    									  Key, 
	    									  NULL, 
	    									  NULL);	

						if (idx == pApCliEntry->DefaultKeyId)
						{						
							// Assign pairwise key info
							RTMPAddWcidAttributeEntry(pAd, 
													 (BssIdx + MIN_NET_DEVICE_FOR_APCLI), 
													 idx, 
													 CipherAlg, 
													 pMacEntry);	
						}
					}	
				}    		   		  		   					
			}

			// If this Entry supports 802.11n, upgrade to HT rate. 
			if (pAd->MlmeAux.HtCapabilityLen != 0)
			{
				UCHAR	j, bitmask; //k,bitmask;
				CHAR    i;
				PHT_CAPABILITY_IE pHtCapability = (PHT_CAPABILITY_IE)&pAd->MlmeAux.HtCapability;

				if ((pAd->MlmeAux.HtCapability.HtCapInfo.GF) && (pAd->CommonCfg.DesiredHtPhy.GF))
				{
					pMacEntry->MaxHTPhyMode.field.MODE = MODE_HTGREENFIELD;
				}
				else
				{	
					pMacEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
					pAd->MacTab.fAnyStationNonGF = TRUE;
					pAd->CommonCfg.AddHTInfo.AddHtInfo2.NonGfPresent = 1;
				}

				if ((pHtCapability->HtCapInfo.ChannelWidth) && (pAd->CommonCfg.DesiredHtPhy.ChannelWidth))
				{
					pMacEntry->MaxHTPhyMode.field.BW= BW_40;
					pMacEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor40)&(pHtCapability->HtCapInfo.ShortGIfor40));
				}
				else
				{	
					pMacEntry->MaxHTPhyMode.field.BW = BW_20;
					pMacEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor20)&(pHtCapability->HtCapInfo.ShortGIfor20));
					pAd->MacTab.fAnyStation20Only = TRUE;
				}

				// find max fixed rate
				for (i=15; i>=0; i--)
				{	
					j = i/8;	
					bitmask = (1<<(i-(j*8)));
					//if ( (pAd->CommonCfg.DesiredHtPhy.MCSSet[j]&bitmask) && (pHtCapability->MCSSet[j]&bitmask))
					if ((pAd->ApCfg.ApCliTab[ifIndex].DesiredHtPhyInfo.MCSSet[j] & bitmask) && (pHtCapability->MCSSet[j] & bitmask))
					{
						pMacEntry->MaxHTPhyMode.field.MCS = i;
						break;
					}
					if (i==0)
						break;
				}

				 
				if (pAd->ApCfg.ApCliTab[ifIndex].DesiredTransmitSetting.field.MCS != MCS_AUTO)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("IF-apcli%d : Desired MCS = %d\n", ifIndex,
						pAd->ApCfg.ApCliTab[ifIndex].DesiredTransmitSetting.field.MCS));

					if (pAd->ApCfg.ApCliTab[ifIndex].DesiredTransmitSetting.field.MCS == 32)
					{
						// Fix MCS as HT Duplicated Mode
						pMacEntry->MaxHTPhyMode.field.BW = 1;
						pMacEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
						pMacEntry->MaxHTPhyMode.field.STBC = 0;
						pMacEntry->MaxHTPhyMode.field.ShortGI = 0;
						pMacEntry->MaxHTPhyMode.field.MCS = 32;
					}
					else if (pMacEntry->MaxHTPhyMode.field.MCS > pAd->ApCfg.ApCliTab[ifIndex].HTPhyMode.field.MCS)
					{
						// STA supports fixed MCS 
						pMacEntry->MaxHTPhyMode.field.MCS = pAd->ApCfg.ApCliTab[ifIndex].HTPhyMode.field.MCS;
					}
				}

				pMacEntry->MaxHTPhyMode.field.STBC = (pHtCapability->HtCapInfo.RxSTBC & (pAd->CommonCfg.DesiredHtPhy.TxSTBC));
				pMacEntry->MpduDensity = pHtCapability->HtCapParm.MpduDensity;
				pMacEntry->MaxRAmpduFactor = pHtCapability->HtCapParm.MaxRAmpduFactor;
				pMacEntry->MmpsMode = (UCHAR)pHtCapability->HtCapInfo.MimoPs;
				pMacEntry->AMsduSize = (UCHAR)pHtCapability->HtCapInfo.AMsduSize;				
				pMacEntry->HTPhyMode.word = pMacEntry->MaxHTPhyMode.word;
				if (pAd->CommonCfg.DesiredHtPhy.AmsduEnable)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_AMSDU_INUSED);
				if (pHtCapability->HtCapInfo.ShortGIfor20)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_SGI20_CAPABLE);
				if (pHtCapability->HtCapInfo.ShortGIfor40)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_SGI40_CAPABLE);
				if (pHtCapability->HtCapInfo.TxSTBC)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_TxSTBC_CAPABLE);
				if (pHtCapability->HtCapInfo.RxSTBC)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_RxSTBC_CAPABLE);
				if (pHtCapability->ExtHtCapInfo.PlusHTC)				
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_HTC_CAPABLE);
				if (pAd->CommonCfg.bRdg && pHtCapability->ExtHtCapInfo.RDGSupport)				
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE);	
				if (pHtCapability->ExtHtCapInfo.MCSFeedback == 0x03)
					CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_MCSFEEDBACK_CAPABLE);		
			}
			else
			{
				pAd->MacTab.fAnyStationIsLegacy = TRUE;
				DBGPRINT(RT_DEBUG_TRACE, ("ApCliLinkUp - MaxSupRate=%d Mbps\n",
								  RateIdToMbps[pMacEntry->MaxSupportedRate]));
			}				

			pMacEntry->HTPhyMode.word = pMacEntry->MaxHTPhyMode.word;
			NdisMoveMemory(&pMacEntry->HTCapability, &pAd->MlmeAux.HtCapability, sizeof(HT_CAPABILITY_IE));
			pMacEntry->CurrTxRate = pMacEntry->MaxSupportedRate;
			
			if (pAd->ApCfg.ApCliTab[ifIndex].bAutoTxRateSwitch == FALSE)
			{
				pMacEntry->bAutoTxRateSwitch = FALSE;
				// If the legacy mode is set, overwrite the transmit setting of this entry.  			
				RTMPUpdateLegacyTxSetting((UCHAR)pAd->ApCfg.ApCliTab[ifIndex].DesiredTransmitSetting.field.FixedTxMode, pMacEntry);	
			}
			else
			{
				pMacEntry->bAutoTxRateSwitch = TRUE;
			}
			
			AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);
			
			// set this entry WMM capable or not
			if (IS_HT_STA(pMacEntry) || (pAd->MlmeAux.APEdcaParm.bValid))
			{
				CLIENT_STATUS_SET_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}
			else
			{
				CLIENT_STATUS_CLEAR_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}

			// set the apcli interface be valid.
			pApCliEntry->Valid = TRUE;
			result = TRUE;

			pAd->ApCfg.ApCliInfRunned++;
			break;
		}
		result = FALSE;

	} while(FALSE);

#ifdef WSC_AP_SUPPORT
    // WSC initial connect to AP, jump to Wsc start action and set the correct parameters    
	if ((result == TRUE) && (pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscConfMode == WSC_ENROLLEE))
	{
		pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscState = WSC_STATE_LINK_UP;
		pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscStatus = WSC_STATE_LINK_UP;
        pAd->ApCfg.ApCliTab[ifIndex].WscControl.EntryApIdx = MIN_NET_DEVICE_FOR_APCLI;
        pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscConfStatus = WSC_SCSTATE_UNCONFIGURED;
        NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].WscControl.EntryAddr, MAC_ADDR_LEN);        
        NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].WscControl.EntryAddr, pAd->MlmeAux.Bssid, MAC_ADDR_LEN);
		WscSendEapolStart(pAd, pMacEntry->Addr, TRUE);
	}
    else
    {
        WscStop(pAd, TRUE, &pAd->ApCfg.ApCliTab[ifIndex].WscControl);
    }
#endif // WSC_AP_SUPPORT //

	return result;
}

/*
    ==========================================================================

	Routine	Description:
		Disconnect current BSSID

	Arguments:
		pAd				- Pointer to our adapter
		ApCliIdx		- Which ApCli interface		
	Return Value:		
		None

	Note:

	==========================================================================
*/
VOID ApCliLinkDown(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ifIndex)
{
	PAPCLI_STRUCT pApCliEntry = NULL;

	if (ifIndex < MAX_APCLI_NUM)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("!!! APCLI LINK DOWN - IF(apcli%d)!!!\n", ifIndex));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("!!! ERROR : APCLI LINK DOWN - IF(apcli%d)!!!\n", ifIndex));
		return;
	}
    	
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if (pApCliEntry->Valid == FALSE)	
		return;

	pAd->ApCfg.ApCliInfRunned--;
	MacTableDeleteEntry(pAd, pApCliEntry->MacTabWCID, APCLI_ROOT_BSSID_GET(pAd, pApCliEntry->MacTabWCID));

	pApCliEntry->Valid = FALSE;	// This link doesn't associated with any remote-AP 
	
}

/* 
    ==========================================================================
    Description:
        APCLI Interface Up.
    ==========================================================================
 */
VOID ApCliIfUp(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR ifIndex;
	PAPCLI_STRUCT pApCliEntry;

	// Reset is in progress, stop immediately
	if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS) ||
		 !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
		return;

	// sanity check whether the interface is initialized.
	if (pAd->flg_apcli_init != TRUE)
		return;

	for(ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
		if (APCLI_IF_UP_CHECK(pAd, ifIndex) 
			&& (pApCliEntry->Enable == TRUE)
			&& (pApCliEntry->Valid == FALSE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("(%s) ApCli interface[%d] startup.\n", __FUNCTION__, ifIndex));
			MlmeEnqueueEx(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_JOIN_REQ, 0, NULL, ifIndex);
		}
	}

	return;
}


/* 
    ==========================================================================
    Description:
        APCLI Interface Down.
    ==========================================================================
 */
VOID ApCliIfDown(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR ifIndex;
	PAPCLI_STRUCT pApCliEntry;

	for(ifIndex = 0; ifIndex < MAX_APCLI_NUM; ifIndex++)
	{
		pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
		if (!(pApCliEntry->Enable))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("(%s) ApCli interface[%d] startdown.\n", __FUNCTION__, ifIndex));
			MlmeEnqueueEx(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL, ifIndex);
		}
	}

	return;
}



/* 
    ==========================================================================
    Description:
        APCLI Interface Monitor.
    ==========================================================================
 */
VOID ApCliIfMonitor(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR index;
	PAPCLI_STRUCT pApCliEntry;	

	// Reset is in progress, stop immediately
	if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS) ||
		 !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
		return;

	// sanity check whether the interface is initialized.
	if (pAd->flg_apcli_init != TRUE)
		return;
	
	for(index = 0; index < MAX_APCLI_NUM; index++)
	{
		pApCliEntry = &pAd->ApCfg.ApCliTab[index];
		if ((pApCliEntry->Valid == TRUE)
			&& (RTMP_TIME_AFTER(pAd->Mlme.Now32 , (pApCliEntry->ApCliRcvBeaconTime + (4 * OS_HZ)))))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("ApCliIfMonitor: IF(apcli%d) - no Beancon is received from root-AP.\n", index));
			DBGPRINT(RT_DEBUG_TRACE, ("ApCliIfMonitor: Reconnect the Root-Ap again.\n"));
			MlmeEnqueueEx(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL, index);
			RT28XX_MLME_HANDLER(pAd);
		}
	}

	return;
}

/*! \brief   To substitute the message type if the message is coming from external
 *  \param  pFrame         The frame received
 *  \param  *Machine       The state machine
 *  \param  *MsgType       the message type for the state machine
 *  \return TRUE if the substitution is successful, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN ApCliMsgTypeSubst(
	IN PRTMP_ADAPTER pAd,
	IN PFRAME_802_11 pFrame, 
	OUT INT *Machine, 
	OUT INT *MsgType)
{
	USHORT Seq;
	UCHAR EAPType; 
	BOOLEAN Return = FALSE;
#ifdef WSC_AP_SUPPORT
	UCHAR EAPCode;
    PMAC_TABLE_ENTRY pEntry;
#endif // WSC_AP_SUPPORT //


	// only PROBE_REQ can be broadcast, all others must be unicast-to-me && is_mybssid; otherwise, 
	// ignore this frame

	// WPA EAPOL PACKET
	if (pFrame->Hdr.FC.Type == BTYPE_DATA)
	{		
#ifdef WSC_AP_SUPPORT    
        //WSC EAPOL PACKET        
        pEntry = MacTableLookup(pAd, pFrame->Hdr.Addr2);
        if (pEntry && (pEntry->ValidAsApCli) && pAd->ApCfg.ApCliTab[pEntry->apidx].WscControl.WscConfMode == WSC_ENROLLEE)
        {
            *Machine = WSC_STATE_MACHINE;
            EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
            EAPCode = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 4);
            Return = WscMsgTypeSubst(EAPType, EAPCode, MsgType);
        }
        if (!Return)
#endif // WSC_AP_SUPPORT //
        {
    		*Machine = AP_WPA_STATE_MACHINE;
    		EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
    		Return = APWpaMsgTypeSubst(EAPType, MsgType);
        }
		return Return;
	}
	else if (pFrame->Hdr.FC.Type == BTYPE_MGMT) 		
	{
		switch (pFrame->Hdr.FC.SubType) 
		{
			case SUBTYPE_ASSOC_RSP:
				*Machine = APCLI_ASSOC_STATE_MACHINE;
				*MsgType = APCLI_MT2_PEER_ASSOC_RSP;
				break;

			case SUBTYPE_DISASSOC:
				*Machine = APCLI_ASSOC_STATE_MACHINE;
				*MsgType = APCLI_MT2_PEER_DISASSOC_REQ;
				break;

			case SUBTYPE_DEAUTH:
				*Machine = APCLI_AUTH_STATE_MACHINE;
				*MsgType = APCLI_MT2_PEER_DEAUTH;
				break;

			case SUBTYPE_AUTH:
				// get the sequence number from payload 24 Mac Header + 2 bytes algorithm
				NdisMoveMemory(&Seq, &pFrame->Octet[2], sizeof(USHORT));
				if (Seq == 2 || Seq == 4)
				{
					*Machine = APCLI_AUTH_STATE_MACHINE;
					*MsgType = APCLI_MT2_PEER_AUTH_EVEN;
				}
				else 
				{
					return FALSE;
				}
				break;

			case SUBTYPE_ACTION:
				*Machine = ACTION_STATE_MACHINE;
				//  Sometimes Sta will return with category bytes with MSB = 1, if they receive catogory out of their support
				if ((pFrame->Octet[0]&0x7F) > MAX_PEER_CATE_MSG) 
				{
					*MsgType = MT2_ACT_INVALID;
				}
				else
				{
					*MsgType = (pFrame->Octet[0]&0x7F);
				}
				break;

			default:
				return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

BOOLEAN preCheckMsgTypeSubset(
	IN PRTMP_ADAPTER  pAd,
	IN PFRAME_802_11 pFrame, 
	OUT INT *Machine, 
	OUT INT *MsgType)
{
	if (pFrame->Hdr.FC.Type == BTYPE_MGMT) 		
	{
		switch (pFrame->Hdr.FC.SubType) 
		{
			// Beacon must be processed be AP Sync state machine.
        	case SUBTYPE_BEACON:
				*Machine = AP_SYNC_STATE_MACHINE;
				*MsgType = APMT2_PEER_BEACON;
            	break;

			// Only Sta have chance to receive Probe-Rsp.
			case SUBTYPE_PROBE_RSP:
				*Machine = APCLI_SYNC_STATE_MACHINE;
				*MsgType = APCLI_MT2_PEER_PROBE_RSP;
				break;

			default:
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
        
    IRQL = DISPATCH_LEVEL

    ==========================================================================
 */
BOOLEAN ApCliPeerAssocRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *pMsg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pStatus, 
    OUT USHORT *pAid, 
    OUT UCHAR SupRate[], 
    OUT UCHAR *pSupRateLen,
    OUT UCHAR ExtRate[], 
    OUT UCHAR *pExtRateLen,
    OUT HT_CAPABILITY_IE *pHtCapability,
    OUT ADD_HT_INFO_IE *pAddHtInfo,	// AP might use this additional ht info IE 
    OUT UCHAR *pHtCapabilityLen,
    OUT UCHAR *pAddHtInfoLen,
    OUT UCHAR *pNewExtChannelOffset,
    OUT PEDCA_PARM pEdcaParm,
    OUT UCHAR *pCkipFlag) 
{
	CHAR          IeType, *Ptr;
	PFRAME_802_11 pFrame = (PFRAME_802_11)pMsg;
	PEID_STRUCT   pEid;
	ULONG         Length = 0;
    
	*pNewExtChannelOffset = 0xff;
	*pHtCapabilityLen = 0;
	*pAddHtInfoLen = 0;
	COPY_MAC_ADDR(pAddr2, pFrame->Hdr.Addr2);
	Ptr = pFrame->Octet;
	Length += LENGTH_802_11;
        
	NdisMoveMemory(pCapabilityInfo, &pFrame->Octet[0], 2);
	Length += 2;
	NdisMoveMemory(pStatus,         &pFrame->Octet[2], 2);
	Length += 2;
	*pCkipFlag = 0;
	*pExtRateLen = 0;
	pEdcaParm->bValid = FALSE;
    
	if (*pStatus != MLME_SUCCESS) 
		return TRUE;
    
	NdisMoveMemory(pAid, &pFrame->Octet[4], 2);
	Length += 2;

	// Aid already swaped byte order in RTMPFrameEndianChange() for big endian platform
	*pAid = (*pAid) & 0x3fff; // AID is low 14-bit
        
	// -- get supported rates from payload and advance the pointer
	IeType = pFrame->Octet[6];
	*pSupRateLen = pFrame->Octet[7];
	if ((IeType != IE_SUPP_RATES) || (*pSupRateLen > MAX_LEN_OF_SUPPORTED_RATES))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocRspSanity fail - wrong SupportedRates IE\n"));
		return FALSE;
	}
	else 
		NdisMoveMemory(SupRate, &pFrame->Octet[8], *pSupRateLen);

	Length = Length + 2 + *pSupRateLen;

	// many AP implement proprietary IEs in non-standard order, we'd better
	// tolerate mis-ordered IEs to get best compatibility
	pEid = (PEID_STRUCT) &pFrame->Octet[8 + (*pSupRateLen)];
            
	// get variable fields from payload and advance the pointer
	while ((Length + 2 + pEid->Len) <= MsgLen)
	{
		switch (pEid->Eid)
		{
			case IE_EXT_SUPP_RATES:
				if (pEid->Len <= MAX_LEN_OF_SUPPORTED_RATES)
				{
					NdisMoveMemory(ExtRate, pEid->Octet, pEid->Len);
					*pExtRateLen = pEid->Len;
				}
				break;

			case IE_HT_CAP:
			case IE_HT_CAP2:
				if (pEid->Len >= SIZE_HT_CAP_IE)  //Note: allow extension.!!
				{
					NdisMoveMemory(pHtCapability, pEid->Octet, SIZE_HT_CAP_IE);
					*pHtCapabilityLen = SIZE_HT_CAP_IE;
				}
				else
				{
					DBGPRINT(RT_DEBUG_WARN, ("PeerAssocRspSanity - wrong IE_HT_CAP. \n"));
				}
				
				break;
			case IE_ADD_HT:
			case IE_ADD_HT2:
				if (pEid->Len >= sizeof(ADD_HT_INFO_IE))				
				{
					// This IE allows extension, but we can ignore extra bytes beyond our knowledge , so only
					// copy first sizeof(ADD_HT_INFO_IE)
					NdisMoveMemory(pAddHtInfo, pEid->Octet, sizeof(ADD_HT_INFO_IE));
					*pAddHtInfoLen = SIZE_ADD_HT_INFO_IE;
				}
				else
				{
					DBGPRINT(RT_DEBUG_WARN, ("PeerAssocRspSanity - wrong IE_ADD_HT. \n"));
				}
				break;
			case IE_NEW_EXT_CHA_OFFSET:
				if (pEid->Len == 1)
				{
					*pNewExtChannelOffset = pEid->Octet[0];
				}
				else
				{
					DBGPRINT(RT_DEBUG_WARN, ("PeerAssocRspSanity - wrong IE_NEW_EXT_CHA_OFFSET. \n"));
				}
				break;
#if 0
			case IE_AIRONET_CKIP:
				// 0. Check Aironet IE length, it must be larger or equal to 28
				//    Cisco's AP VxWork version(will not be supported) used this IE length as 28
				//    Cisco's AP IOS version used this IE length as 30 
				if (pEid->Len < (CKIP_NEGOTIATION_LENGTH - 2))
					break;

				// 1. Copy CKIP flag byte to buffer for process
				*pCkipFlag = *(pEid->Octet + 8);				
				break;

			case IE_AIRONET_IPADDRESS:
				if (pEid->Len != 0x0A)
				break;

				// Get Cisco Aironet IP information
				if (NdisEqualMemory(pEid->Octet, CISCO_OUI, 3) == 1)
					NdisMoveMemory(pAd->StaCfg.AironetIPAddress, pEid->Octet + 4, 4);
				break;
#endif
			// CCX2, WMM use the same IE value
			// case IE_CCX_V2:
			case IE_VENDOR_SPECIFIC:
				// handle WME PARAMTER ELEMENT
				if (NdisEqualMemory(pEid->Octet, WME_PARM_ELEM, 6) && (pEid->Len == 24))
				{
					PUCHAR ptr;
					int i;
        
					// parsing EDCA parameters
					pEdcaParm->bValid          = TRUE;
					pEdcaParm->bQAck           = FALSE; // pEid->Octet[0] & 0x10;
					pEdcaParm->bQueueRequest   = FALSE; // pEid->Octet[0] & 0x20;
					pEdcaParm->bTxopRequest    = FALSE; // pEid->Octet[0] & 0x40;
					//pEdcaParm->bMoreDataAck    = FALSE; // pEid->Octet[0] & 0x80;
					pEdcaParm->EdcaUpdateCount = pEid->Octet[6] & 0x0f;
					pEdcaParm->bAPSDCapable    = (pEid->Octet[6] & 0x80) ? 1 : 0;
					ptr = &pEid->Octet[8];
					for (i=0; i<4; i++)
					{
						UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
						pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
						pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
						pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
						pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
						pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
						ptr += 4; // point to next AC
					}
				}							
#if 0
				// handle CCX IE
				else
				{
					// 0. Check the size and CCX admin control
					if (pAd->StaCfg.CCX2Control.field.Enable == 0)
						break;
					if (pEid->Len != 5)
						break;

					// Turn CCX2 if matched
					if (NdisEqualMemory(pEid->Octet, Ccx2IeInfo, 5) == 1)
						pAd->StaCfg.CCX2Enable = TRUE;
					break;
				}
#endif
				break;

#if 0
				case IE_EDCA_PARAMETER:
					if (pEid->Len == 18)
					{
						PUCHAR ptr;
						int i;
						pEdcaParm->bValid          = TRUE;
						pEdcaParm->bQAck           = pEid->Octet[0] & 0x10;
						pEdcaParm->bQueueRequest   = pEid->Octet[0] & 0x20;
						pEdcaParm->bTxopRequest    = pEid->Octet[0] & 0x40;
//						pEdcaParm->bMoreDataAck    = pEid->Octet[0] & 0x80;
						pEdcaParm->EdcaUpdateCount = pEid->Octet[0] & 0x0f;
						ptr = &pEid->Octet[2];
						for (i=0; i<4; i++)
						{
							UCHAR aci = (*ptr & 0x60) >> 5; // b5~6 is AC INDEX
							pEdcaParm->bACM[aci]  = (((*ptr) & 0x10) == 0x10);   // b5 is ACM
							pEdcaParm->Aifsn[aci] = (*ptr) & 0x0f;               // b0~3 is AIFSN
							pEdcaParm->Cwmin[aci] = *(ptr+1) & 0x0f;             // b0~4 is Cwmin
							pEdcaParm->Cwmax[aci] = *(ptr+1) >> 4;               // b5~8 is Cwmax
							pEdcaParm->Txop[aci]  = *(ptr+2) + 256 * (*(ptr+3)); // in unit of 32-us
							ptr += 4; // point to next AC
						}
					}
					break;
#endif
				default:
					DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocRspSanity - ignore unrecognized EID = %d\n", pEid->Eid));
					break;
		}

		Length = Length + 2 + pEid->Len; 
		pEid = (PEID_STRUCT)((UCHAR*)pEid + 2 + pEid->Len);        
	}

#if 0
	// Force CCX2 enable to TRUE for those AP didn't replay CCX v2 IE, we still force it to be on
	if (pAd->StaCfg.CCX2Control.field.Enable == 1)
		pAd->StaCfg.CCX2Enable = TRUE;
#endif
	return TRUE;
}


MAC_TABLE_ENTRY *ApCliTableLookUpByWcid(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR wcid,
	IN PUCHAR pAddrs)
{
	//USHORT HashIdx;
	ULONG ApCliIndex;
	PMAC_TABLE_ENTRY pCurEntry = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;

	if (wcid <=0 || wcid >= MAX_LEN_OF_MAC_TABLE )
		return NULL;

	NdisAcquireSpinLock(&pAd->MacTabLock);

	do
	{
		pCurEntry = &pAd->MacTab.Content[wcid];

		ApCliIndex = 0xff;
		if ((pCurEntry) && (pCurEntry->ValidAsApCli== TRUE))
		{
			ApCliIndex = pCurEntry->MatchAPCLITabIdx;
		}

		if ((ApCliIndex == 0xff) || (ApCliIndex >= MAX_APCLI_NUM))
			break;

		if (pAd->ApCfg.ApCliTab[ApCliIndex].Valid != TRUE)
			break;

		if (MAC_ADDR_EQUAL(pCurEntry->Addr, pAddrs))
		{
			pEntry = pCurEntry;
			break;
		}
	} while(FALSE);

	NdisReleaseSpinLock(&pAd->MacTabLock);

	return pEntry;
}

/*
	==========================================================================
	Description:
		Check the WDS Entry is valid or not.
	==========================================================================
 */
static inline BOOLEAN ValidApCliEntry(
	IN PRTMP_ADAPTER pAd,
	IN INT apCliIdx)
{
	BOOLEAN result;
	PMAC_TABLE_ENTRY pMacEntry;
	APCLI_STRUCT *pApCliEntry;
	do
	{
		if ((apCliIdx < 0) || (apCliIdx >= MAX_APCLI_NUM))
		{
			result = FALSE;
			break;
		}

		pApCliEntry = (APCLI_STRUCT *)&pAd->ApCfg.ApCliTab[apCliIdx];
		if (pApCliEntry->Valid != TRUE)
		{
			result = FALSE;
			break;
		}

		if ((pApCliEntry->MacTabWCID <= 0) 
			|| (pApCliEntry->MacTabWCID >= MAX_LEN_OF_MAC_TABLE))
		{
			result = FALSE;
			break;
		}
	
		pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID];
		if (pMacEntry->ValidAsApCli != TRUE)
		{
			result = FALSE;
			break;
		}
			
		result = TRUE;
	} while(FALSE);

	return result;
}


BOOLEAN ApCliAllowToSendPacket(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	OUT UCHAR		*pWcid)
{
	UCHAR apCliIdx;
	BOOLEAN	allowed;
		
	//printk("ApCliAllowToSendPacket():Packet to ApCli interface!\n");
	apCliIdx = RTMP_GET_PACKET_NET_DEVICE(pPacket) - MIN_NET_DEVICE_FOR_APCLI;
	if (ValidApCliEntry(pAd, apCliIdx))
	{
		//printk("ApCliAllowToSendPacket(): Set the WCID as %d!\n", pAd->ApCfg.ApCliTab[apCliIdx].MacTabWCID);
		
		*pWcid = pAd->ApCfg.ApCliTab[apCliIdx].MacTabWCID;
		//RTMP_SET_PACKET_WCID(pPacket, pAd->ApCfg.ApCliTab[apCliIdx].MacTabWCID); // to ApClient links.
		
		allowed = TRUE;
	}
	else
	{
		allowed = FALSE;
	}

	return allowed;
	
}


/*
	========================================================================
	
	Routine Description:
		Validate the security configuration against the RSN information 
		element

	Arguments:
		pAdapter	Pointer	to our adapter
		eid_ptr 	Pointer to VIE
		
	Return Value:
		TRUE 	for configuration match 
		FALSE	for otherwise
		
	Note:
		
	========================================================================
*/
BOOLEAN 	ApCliValidateRSNIE(
	IN		PRTMP_ADAPTER	pAd, 
	IN 		PEID_STRUCT    	pEid_ptr,
	IN		USHORT			eid_len,
	IN		USHORT			idx)
{
	PUCHAR              pVIE;
	PUCHAR				pTmp;
	UCHAR         		len;
	PEID_STRUCT         pEid;			
	CIPHER_SUITE		WPA;			// AP announced WPA cipher suite
	CIPHER_SUITE		WPA2;			// AP announced WPA2 cipher suite
	USHORT				Count;
	UCHAR               Sanity;	 
	PAPCLI_STRUCT   	pApCliEntry = NULL;
	PRSN_IE_HEADER_STRUCT			pRsnHeader;
	NDIS_802_11_ENCRYPTION_STATUS	TmpCipher;
	NDIS_802_11_AUTHENTICATION_MODE TmpAuthMode;
	NDIS_802_11_AUTHENTICATION_MODE WPA_AuthMode;
	NDIS_802_11_AUTHENTICATION_MODE WPA_AuthModeAux;
	NDIS_802_11_AUTHENTICATION_MODE WPA2_AuthMode;
	NDIS_802_11_AUTHENTICATION_MODE WPA2_AuthModeAux;

	pVIE = (PUCHAR) pEid_ptr;
	len	 = eid_len;

	if (len > MAX_LEN_OF_RSNIE || len <= MIN_LEN_OF_RSNIE)
		return FALSE;

	if (pAd->ApCfg.ApCliTab[idx].AuthMode < Ndis802_11AuthModeWPA)
		return FALSE;
				
	// Init WPA setting
	WPA.PairCipher    	= Ndis802_11WEPDisabled;
	WPA.PairCipherAux 	= Ndis802_11WEPDisabled;
	WPA.GroupCipher   	= Ndis802_11WEPDisabled;
	WPA.RsnCapability 	= 0;
	WPA.bMixMode      	= FALSE;
	WPA_AuthMode	  	= Ndis802_11AuthModeOpen;
	WPA_AuthModeAux		= Ndis802_11AuthModeOpen;	

	// Init WPA2 setting
	WPA2.PairCipher    	= Ndis802_11WEPDisabled;
	WPA2.PairCipherAux 	= Ndis802_11WEPDisabled;
	WPA2.GroupCipher   	= Ndis802_11WEPDisabled;
	WPA2.RsnCapability 	= 0;
	WPA2.bMixMode      	= FALSE;
	WPA2_AuthMode	  	= Ndis802_11AuthModeOpen;
	WPA2_AuthModeAux	= Ndis802_11AuthModeOpen;

	Sanity = 0;

	// 1. Parse Cipher this received RSNIE
	while (len > 0)
	{		
		pTmp = pVIE;
		pEid = (PEID_STRUCT) pTmp;	

		switch(pEid->Eid)
		{
			case IE_WPA:
				if (NdisEqualMemory(pEid->Octet, WPA_OUI, 4) != 1)
				{
					// if unsupported vendor specific IE
					break;
				}	
				// Skip OUI ,version and multicast suite OUI
				pTmp += 11;

				// Cipher Suite Selectors from Spec P802.11i/D3.2 P26.
	            //  Value      Meaning
	            //  0           None 
	            //  1           WEP-40
	            //  2           Tkip
	            //  3           WRAP
	            //  4           AES
	            //  5           WEP-104
				// Parse group cipher
				switch (*pTmp)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						WPA.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						WPA.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						WPA.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}

				// number of unicast suite
				pTmp += 1;

				// Store unicast cipher count
			    NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
    			Count = cpu2le16(Count);		

				// pointer to unicast cipher
			    pTmp += sizeof(USHORT);	

				// Parsing all unicast cipher suite				
				while (Count > 0)
				{
					// Skip cipher suite OUI
					pTmp += 3;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (*pTmp)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;							
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;						
							break;
						default:
							break;
					}
					if (TmpCipher > WPA.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						WPA.PairCipherAux = WPA.PairCipher;
						WPA.PairCipher    = TmpCipher;
					}
					else
					{
						WPA.PairCipherAux = TmpCipher;
					}
					pTmp++;
					Count--;
				}
			
				// Get AKM suite counts
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
				Count = cpu2le16(Count);		

				pTmp   += sizeof(USHORT);

				// Parse AKM ciphers
				// Parsing all AKM cipher suite				
				while (Count > 0)
				{
			    	// Skip cipher suite OUI
					pTmp   += 3;
					TmpAuthMode = Ndis802_11AuthModeOpen;
					switch (*pTmp)
					{	
						case 1:
							// WPA-enterprise
							TmpAuthMode = Ndis802_11AuthModeWPA;							
							break;
						case 2:
							// WPA-personal
							TmpAuthMode = Ndis802_11AuthModeWPAPSK;									    	
							break;
						default:
							break;
					}
					if (TmpAuthMode > WPA_AuthMode)
					{
						// Move the lower AKM suite to WPA_AuthModeAux
						WPA_AuthModeAux = WPA_AuthMode;
						WPA_AuthMode    = TmpAuthMode;
					}
					else
					{
						WPA_AuthModeAux = TmpAuthMode;
					}
				    pTmp++;
					Count--;										
				}

				// ToDo - Support WPA-None ?

				// Check the Pair & Group, if different, turn on mixed mode flag
				if (WPA.GroupCipher != WPA.PairCipher)
					WPA.bMixMode = TRUE;

				DBGPRINT(RT_DEBUG_TRACE, ("ApCliValidateRSNIE - RSN-WPA1 PairWiseCipher(%s), GroupCipher(%s), AuthMode(%s)\n",
											((WPA.bMixMode) ? (CHAR *)"Mix" : GetEncryptType(WPA.PairCipher)), 
											GetEncryptType(WPA.GroupCipher),
											GetAuthMode(WPA_AuthMode)));

				Sanity |= 0x1;
				break; // End of case IE_WPA //
			case IE_RSN:
				pRsnHeader = (PRSN_IE_HEADER_STRUCT) pTmp;
				
				// 0. Version must be 1
				//  The pRsnHeader->Version exists in native little-endian order, so we may need swap it for BIG_ENDIAN systems.
				if (le2cpu16(pRsnHeader->Version) != 1)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - RSN Version isn't 1(%d) \n", pRsnHeader->Version));
					break;
				}	

				pTmp   += sizeof(RSN_IE_HEADER_STRUCT);

				// 1. Check cipher OUI				
				if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
				{
					// if unsupported vendor specific IE
					break;
				}

				// Skip cipher suite OUI
				pTmp += 3;

				// Parse group cipher
				switch (*pTmp)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						WPA2.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						WPA2.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						WPA2.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}

				// number of unicast suite
				pTmp += 1;

				// Get pairwise cipher counts				
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
				Count = cpu2le16(Count);
				
				pTmp   += sizeof(USHORT);

				// 3. Get pairwise cipher
				// Parsing all unicast cipher suite
				while (Count > 0)
				{
					// Skip OUI
					pTmp += 3;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (*pTmp)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;							
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;							
							break;
						default:
							break;
					}
					if (TmpCipher > WPA2.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						WPA2.PairCipherAux = WPA2.PairCipher;
						WPA2.PairCipher    = TmpCipher;
					}
					else
					{
						WPA2.PairCipherAux = TmpCipher;
					}
					pTmp ++;
					Count--;
				}

				// Get AKM suite counts				
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
				Count = cpu2le16(Count);		

				pTmp   += sizeof(USHORT);

				// Parse AKM ciphers
				// Parsing all AKM cipher suite				
				while (Count > 0)
				{
			    	// Skip cipher suite OUI
					pTmp   += 3;
					TmpAuthMode = Ndis802_11AuthModeOpen;
					switch (*pTmp)
					{	
						case 1:
							// WPA2-enterprise
							TmpAuthMode = Ndis802_11AuthModeWPA2;							
							break;
						case 2:
							// WPA2-personal
							TmpAuthMode = Ndis802_11AuthModeWPA2PSK;									    	
							break;
						default:
							break;
					}
					if (TmpAuthMode > WPA2_AuthMode)
					{
						// Move the lower AKM suite to WPA2_AuthModeAux
						WPA2_AuthModeAux = WPA2_AuthMode;
						WPA2_AuthMode    = TmpAuthMode;
					}
					else
					{
						WPA2_AuthModeAux = TmpAuthMode;
					}
				    pTmp++;
					Count--;										
				}

				// Check the Pair & Group, if different, turn on mixed mode flag
				if (WPA2.GroupCipher != WPA2.PairCipher)
					WPA2.bMixMode = TRUE;

				DBGPRINT(RT_DEBUG_TRACE, ("ApCliValidateRSNIE - RSN-WPA2 PairWiseCipher(%s), GroupCipher(%s), AuthMode(%s)\n",
									(WPA2.bMixMode ? (CHAR *)"Mix" : GetEncryptType(WPA2.PairCipher)), GetEncryptType(WPA2.GroupCipher),
									GetAuthMode(WPA2_AuthMode)));

				Sanity |= 0x2;
				break; // End of case IE_RSN //
			default:
					DBGPRINT(RT_DEBUG_WARN, ("ApCliValidateRSNIE - Unknown pEid->Eid(%d) \n", pEid->Eid));
				break;
		}

		// skip this Eid
		pVIE += (pEid->Len + 2);
		len  -= (pEid->Len + 2);
	
	}

	if (Sanity == 0) 
	{
		DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - unrecognized RSNIE \n"));
		return FALSE;
	}

	// 2. Validate this RSNIE with mine
	pApCliEntry = &pAd->ApCfg.ApCliTab[idx];

	// Recovery user-defined cipher suite
	pApCliEntry->PairCipher  = pApCliEntry->WepStatus;
	pApCliEntry->GroupCipher = pApCliEntry->WepStatus;
	pApCliEntry->bMixCipher  = FALSE;

	Sanity = 0;	
	
	// Check AuthMode and WPA_AuthModeAux for matching, in case AP support dual-AuthMode
	// WPAPSK
	if (WPA_AuthMode == pApCliEntry->AuthMode || WPA_AuthModeAux == pApCliEntry->AuthMode)
	{
		// Check cipher suite, AP must have more secured cipher than station setting
		if (WPA.bMixMode == FALSE)
		{
			if (pApCliEntry->WepStatus != WPA.GroupCipher)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA validate cipher suite error \n"));
				return FALSE;
			}
		}

		// check group cipher
		if (pApCliEntry->WepStatus < WPA.GroupCipher)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA validate group cipher error \n"));
			return FALSE;
		}

		// check pairwise cipher, skip if none matched
		// If profile set to AES, let it pass without question.
		// If profile set to TKIP, we must find one mateched
		if ((pApCliEntry->WepStatus == Ndis802_11Encryption2Enabled) && 
			(pApCliEntry->WepStatus != WPA.PairCipher) && 
			(pApCliEntry->WepStatus != WPA.PairCipherAux))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA validate pairwise cipher error \n"));
			return FALSE;
		}	

		Sanity |= 0x1;
	}
	// WPA2PSK
	else if (WPA2_AuthMode == pApCliEntry->AuthMode || WPA2_AuthModeAux == pApCliEntry->AuthMode)
	{
		// Check cipher suite, AP must have more secured cipher than station setting
		if (WPA2.bMixMode == FALSE)
		{
			if (pApCliEntry->WepStatus != WPA2.GroupCipher)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA2 validate cipher suite error \n"));
				return FALSE;
			}
		}

		// check group cipher
		if (pApCliEntry->WepStatus < WPA2.GroupCipher)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA2 validate group cipher error \n"));
			return FALSE;
		}

		// check pairwise cipher, skip if none matched
		// If profile set to AES, let it pass without question.
		// If profile set to TKIP, we must find one mateched
		if ((pApCliEntry->WepStatus == Ndis802_11Encryption2Enabled) && 
			(pApCliEntry->WepStatus != WPA2.PairCipher) && 
			(pApCliEntry->WepStatus != WPA2.PairCipherAux))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - WPA2 validate pairwise cipher error \n"));
			return FALSE;
		}

		Sanity |= 0x2;
	}

	if (Sanity == 0) 
	{
		DBGPRINT(RT_DEBUG_ERROR, ("ApCliValidateRSNIE - Validate RSIE Failure \n"));
		return FALSE;
	}

	//Re-assign pairwise-cipher and group-cipher. Re-build RSNIE. 
	if ((pApCliEntry->AuthMode == Ndis802_11AuthModeWPA) || (pApCliEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
	{
		pApCliEntry->GroupCipher = WPA.GroupCipher;
			
		if (pApCliEntry->WepStatus == WPA.PairCipher)
			pApCliEntry->PairCipher = WPA.PairCipher;
		else if (WPA.PairCipherAux != Ndis802_11WEPDisabled)
			pApCliEntry->PairCipher = WPA.PairCipherAux;
		else	// There is no PairCipher Aux, downgrade our capability to TKIP
			pApCliEntry->PairCipher = Ndis802_11Encryption2Enabled;			
	}
	else if ((pApCliEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pApCliEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
	{
		pApCliEntry->GroupCipher = WPA2.GroupCipher;
			
		if (pApCliEntry->WepStatus == WPA2.PairCipher)
			pApCliEntry->PairCipher = WPA2.PairCipher;
		else if (WPA2.PairCipherAux != Ndis802_11WEPDisabled)
			pApCliEntry->PairCipher = WPA2.PairCipherAux;
		else	// There is no PairCipher Aux, downgrade our capability to TKIP
			pApCliEntry->PairCipher = Ndis802_11Encryption2Enabled;					
	}

	// Set Mix cipher flag
	if (pApCliEntry->PairCipher != pApCliEntry->GroupCipher)
	{
		pApCliEntry->bMixCipher = TRUE;	

		// re-build RSNIE
		RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (idx + MIN_NET_DEVICE_FOR_APCLI));
	}
	
	return TRUE;	
}


/*
	========================================================================
	
	Routine Description:
		Process Pairwise key Msg-1 of 4-way handshaking and send Msg-2 

	Arguments:
		pAd			Pointer	to our adapter
		Elem		Message body
		
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	ApCliPeerPairMsg1Action(
	IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{
	UCHAR				PTK[80];
	UCHAR               Header802_3[14];
	PEAPOL_PACKET		pMsg1;
	UINT            	MsgLen;	
	EAPOL_PACKET		EAPOLPKT;
	UINT				IfIndex = 0;
	   
	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerPairMsg1Action ----->\n"));

	if ((!pEntry) || (!pEntry->ValidAsApCli))
		return;

    if (Elem->MsgLen < (LENGTH_802_11 + LENGTH_802_1_H + LENGTH_EAPOL_H + sizeof(KEY_DESCRIPTER) - MAX_LEN_OF_RSNIE - 2))
        return;

	IfIndex = pEntry->MatchAPCLITabIdx;
	if (IfIndex >= MAX_APCLI_NUM)
		return;

	// Store the received frame
	pMsg1 = (PEAPOL_PACKET) &Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];
	MsgLen = Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H;
	
	// store the received EAPoL packet
	NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
    NdisMoveMemory(&EAPOLPKT, pMsg1, MsgLen);

	// Sanity Check peer Pairwise message 1 - Replay Counter
	if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, MsgLen, EAPOL_PAIR_MSG_1, pMsg1->KeyDesc.KeyMic, pEntry))
		return;
	
	// Store Replay counter, it will use to verify message 3 and construct message 2
	NdisMoveMemory(pEntry->R_Counter, pMsg1->KeyDesc.ReplayCounter, LEN_KEY_DESC_REPLAY);		

	// Store ANonce
	NdisMoveMemory(pEntry->ANonce, pMsg1->KeyDesc.KeyNonce, LEN_KEY_DESC_NONCE);
		
	// Generate random SNonce
	GenRandom(pAd, pAd->CurrentAddress, pAd->ApCfg.ApCliTab[IfIndex].SNonce);

    // Calculate PTK(ANonce, SNonce)
    WpaCountPTK(pAd,
    			pAd->ApCfg.ApCliTab[IfIndex].PMK,
		     	pEntry->ANonce,
			 	pEntry->Addr, 
			 	pAd->ApCfg.ApCliTab[IfIndex].SNonce, 
			 	pAd->ApCfg.ApCliTab[IfIndex].CurrentAddress, 
			    PTK, 
			    LEN_PTK);
			    
	// Save key to PTK entry
	NdisMoveMemory(pEntry->PTK, PTK, LEN_PTK);
	
	// Update WpaState
	pEntry->WpaState = AS_PTKINIT_NEGOTIATING;

	// Construct EAPoL message - Pairwise Msg 2
	NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
	ConstructEapolMsg(pAd,
					  pEntry->AuthMode,
					  pEntry->WepStatus, 
					  pAd->ApCfg.ApCliTab[IfIndex].GroupCipher,
					  EAPOL_PAIR_MSG_2,  
					  0,
					  pEntry->R_Counter,
					  pAd->ApCfg.ApCliTab[IfIndex].SNonce,
					  NULL,
					  pEntry->PTK,
					  NULL,
					  pAd->ApCfg.ApCliTab[IfIndex].RSN_IE,
					  pAd->ApCfg.ApCliTab[IfIndex].RSNIE_Len,
					  &EAPOLPKT);

	// Make outgoing frame
	MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.ApCliTab[IfIndex].CurrentAddress, EAPOL);	
	
	APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, TRUE);
		
	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerPairMsg1Action: send Msg2 of 4-way \n"));
}	

/*
	========================================================================
	
	Routine Description:
		Process Pairwise key Msg 3 of 4-way handshaking and send Msg 4 

	Arguments:
		pAd	Pointer	to our adapter
		Elem		Message body
		
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	ApCliPeerPairMsg3Action(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{
	PHEADER_802_11		pHeader;
	UCHAR               Header802_3[14];
	EAPOL_PACKET		EAPOLPKT;
	PEAPOL_PACKET		pMsg3;
	UINT            	MsgLen;				
	UCHAR				IfIndex = 0;
	   
	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerPairMsg3Action ----->\n"));
	
	if ((!pEntry) || (!pEntry->ValidAsApCli))
		return;

    if (Elem->MsgLen < (LENGTH_802_11 + LENGTH_802_1_H + LENGTH_EAPOL_H + sizeof(KEY_DESCRIPTER) - MAX_LEN_OF_RSNIE - 2))
		return;

	IfIndex = pEntry->MatchAPCLITabIdx;
	if (IfIndex >= MAX_APCLI_NUM)
	    return;
		
	// Record 802.11 header & the received EAPOL packet Msg3
	pHeader	= (PHEADER_802_11) Elem->Msg;
	pMsg3 = (PEAPOL_PACKET) &Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];
	MsgLen = Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H;

	// store the received EAPoL packet
	NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
    NdisMoveMemory(&EAPOLPKT, pMsg3, MsgLen);
	
	// Sanity Check peer Pairwise message 3 - Replay Counter, MIC, RSNIE
	if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, MsgLen, EAPOL_PAIR_MSG_3, pMsg3->KeyDesc.KeyMic, pEntry))
		return;
	
	// Save Replay counter, it will use construct message 4
	NdisMoveMemory(pEntry->R_Counter, pMsg3->KeyDesc.ReplayCounter, LEN_KEY_DESC_REPLAY);

	// Double check ANonce
	if (!NdisEqualMemory(pEntry->ANonce, pMsg3->KeyDesc.KeyNonce, LEN_KEY_DESC_NONCE))
	{
		return;
	}

	// Construct EAPoL message - Pairwise Msg 4
	NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
	ConstructEapolMsg(pAd,
					  pEntry->AuthMode,
					  pEntry->WepStatus, 
					  pAd->ApCfg.ApCliTab[IfIndex].GroupCipher,
					  EAPOL_PAIR_MSG_4,  
					  0,					// group key index not used in message 4
					  pEntry->R_Counter,
					  NULL,					// Nonce not used in message 4
					  NULL,					// TxRSC not used in message 4
					  pEntry->PTK,
					  NULL,					// GTK not used in message 4
					  NULL,					// RSN IE not used in message 4
					  0,
					  &EAPOLPKT);

	// Update WpaState
	pEntry->WpaState = AS_PTKINITDONE;	 	

	// Update pairwise key								
 	APCliUpdatePairwiseKeyTable(pAd, pMsg3->KeyDesc.KeyRsc, pEntry);

	// open 802.1x port control and privacy filter
	if (pAd->ApCfg.ApCliTab[IfIndex].AuthMode == Ndis802_11AuthModeWPA2PSK || pAd->ApCfg.ApCliTab[IfIndex].AuthMode == Ndis802_11AuthModeWPA2)
	{
		pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
		pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;	
		DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerPairMsg3Action: AuthMode(%s) PairwiseCipher(%s) GroupCipher(%s) \n",
									GetAuthMode(pAd->ApCfg.ApCliTab[IfIndex].AuthMode),
									GetEncryptType(pAd->ApCfg.ApCliTab[IfIndex].PairCipher),
									GetEncryptType(pAd->ApCfg.ApCliTab[IfIndex].GroupCipher)));
	}

	// Init 802.3 header and send out
	MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.ApCliTab[IfIndex].CurrentAddress, EAPOL);	
	APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, TRUE);

	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerPairMsg3Action: send Msg4 of 4-way \n"));
}


/*
	========================================================================
	
	Routine Description:
		Process Group key 2-way handshaking

	Arguments:
		pAd	Pointer	to our adapter
		Elem		Message body
		
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	ApCliPeerGroupMsg1Action(
	IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{
    UCHAR               Header802_3[14];
	EAPOL_PACKET		EAPOLPKT;
	PEAPOL_PACKET		pGroup;
	UINT            	MsgLen;
	UINT				IfIndex = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerGroupMsg1Action ----->\n"));

	if ((!pEntry) || (!pEntry->ValidAsApCli))
        return;

	IfIndex = pEntry->MatchAPCLITabIdx;
	if (IfIndex >= MAX_APCLI_NUM)
		return;
	   
	// Process Group Message 1 frame. skip 802.11 header(24) & LLC_SNAP header(8)
	pGroup = (PEAPOL_PACKET) &Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];
	MsgLen = Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H;

	// store the received EAPoL packet
	NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
    NdisMoveMemory(&EAPOLPKT, pGroup, MsgLen);

	// Sanity Check peer group message 1 - Replay Counter, MIC, RSNIE
	if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, MsgLen, EAPOL_GROUP_MSG_1, pGroup->KeyDesc.KeyMic, pEntry))
		return;

	// Save Replay counter, it will use to construct message 2
	NdisMoveMemory(pEntry->R_Counter, pGroup->KeyDesc.ReplayCounter, LEN_KEY_DESC_REPLAY);	

	// Construct EAPoL message - Group Msg 2
	NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
	ConstructEapolMsg(pAd,
					  pEntry->AuthMode,
					  pEntry->WepStatus, 
					  pAd->ApCfg.ApCliTab[IfIndex].GroupCipher,
					  EAPOL_GROUP_MSG_2,  
					  pAd->ApCfg.ApCliTab[IfIndex].DefaultKeyId,
					  pEntry->R_Counter,
					  NULL,					// Nonce not used
					  NULL,					// TxRSC not used
					  pEntry->PTK,
					  NULL,					// GTK not used
					  NULL,					// RSN IE not used
					  0,
					  &EAPOLPKT);
					
    // open 802.1x port control and privacy filter
	pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
	pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerGroupMsg1Action: AuthMode(%s) PairwiseCipher(%s) GroupCipher(%s) \n",
									GetAuthMode(pAd->ApCfg.ApCliTab[IfIndex].AuthMode),
									GetEncryptType(pAd->ApCfg.ApCliTab[IfIndex].PairCipher),
									GetEncryptType(pAd->ApCfg.ApCliTab[IfIndex].GroupCipher)));
		
	// init header and Fill Packet and send Msg 2 to authenticator	
	MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.ApCliTab[IfIndex].CurrentAddress, EAPOL);	
	APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, FALSE);

	DBGPRINT(RT_DEBUG_TRACE, ("ApCliPeerGroupMsg1Action: sned group message 2\n"));
}	


/*
    ========================================================================
    
    Routine Description:
    Check Sanity RSN IE form AP

    Arguments:
        
    Return Value:

		
    ========================================================================
*/
BOOLEAN ApCliCheckRSNIE(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pData,
	IN  UCHAR           DataLen,
	IN  MAC_TABLE_ENTRY *pEntry,
	OUT	UCHAR			*Offset)
{
	PUCHAR              pVIE;
	UCHAR               len;
	PEID_STRUCT         pEid;
	BOOLEAN				result = FALSE;
		
	pVIE = pData;
	len	 = DataLen;
	*Offset = 0;

	while (len > sizeof(RSNIE2))
	{
		pEid = (PEID_STRUCT) pVIE;	
		// WPA RSN IE
		if ((pEid->Eid == IE_WPA) && (NdisEqualMemory(pEid->Octet, WPA_OUI, 4)))
		{			
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA || pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2)))
			{				
					DBGPRINT(RT_DEBUG_TRACE, ("ApCliCheckRSNIE ==> WPA/WPAPSK RSN IE matched in Msg 3, Length(%d) \n", (pEid->Len + 2)));
					result = TRUE;				
			}		
			
			*Offset += (pEid->Len + 2);			
		}
		// WPA2 RSN IE
		else if ((pEid->Eid == IE_RSN) && (NdisEqualMemory(pEid->Octet + 2, RSN_OUI, 3)))
		{
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2 || pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2)))
			{				
					DBGPRINT(RT_DEBUG_TRACE, ("ApCliCheckRSNIE ==> WPA2/WPA2PSK RSN IE matched in Msg 3, Length(%d) \n", (pEid->Len + 2)));
					result = TRUE;				
			}				

			*Offset += (pEid->Len + 2);
		}		
		else
		{			
			break;
		}

		pVIE += (pEid->Len + 2);
		len  -= (pEid->Len + 2);
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("ApCliCheckRSNIE ==> skip_offset(%d) \n", *Offset));
		
	return result;
	
}



/*
    ========================================================================
    
    Routine Description:
    Parse KEYDATA field.  KEYDATA[] May contain 2 RSN IE and optionally GTK.  
    GTK  is encaptulated in KDE format at  p.83 802.11i D10

    Arguments:
        
    Return Value:

    Note:
        802.11i D10  
        
    ========================================================================
*/
BOOLEAN ApCliParseKeyData(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pKeyData,
	IN  UCHAR           KeyDataLen,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN	UCHAR			IfIdx,
	IN	UCHAR			bPairewise)
{
    PKDE_ENCAP          pKDE = NULL;
    PUCHAR              pMyKeyData = pKeyData;
    UCHAR               KeyDataLength = KeyDataLen;
    UCHAR               GTKLEN;
	UCHAR				DefaultIdx;
	UCHAR				skip_offset;	
	    
	// Verify The RSN IE contained in Pairewise-Msg 3 and skip it
	if (bPairewise)
    {
		// Check RSN IE whether it is WPA2/WPA2PSK   		
		if (!ApCliCheckRSNIE(pAd, pKeyData, KeyDataLen, pEntry, &skip_offset))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ApCliParseKeyData ==> WPA2/WPA2PSK RSN IE mismatched \n"));
			hex_dump("Get KEYDATA :", pKeyData, KeyDataLen);
			return FALSE;			
    	}
    	else
		{
			// skip RSN IE
			pMyKeyData += skip_offset;
			KeyDataLength -= skip_offset;

			//DBGPRINT(RT_DEBUG_TRACE, ("ParseKeyData ==> WPA2/WPA2PSK RSN IE matched in Msg 3, Length(%d) \n", skip_offset));
		}
	}

	DBGPRINT(RT_DEBUG_TRACE,("ApCliParseKeyData ==> KeyDataLength %d without RSN_IE \n", KeyDataLength));

	// Parse EKD format
	if (KeyDataLength >= 8)
    {
        pKDE = (PKDE_ENCAP) pMyKeyData;
        DBGPRINT(RT_DEBUG_INFO, ("pKDE->Type %x \n", pKDE->Type));
        DBGPRINT(RT_DEBUG_INFO, ("pKDE->Len 0x%x \n", pKDE->Len));
        DBGPRINT(RT_DEBUG_INFO, ("pKDE->OUI %x %x %x \n", pKDE->OUI[0],pKDE->OUI[1],pKDE->OUI[2]));
    	DBGPRINT(RT_DEBUG_INFO, ("pKDE->DataType %x \n", pKDE->DataType));
    }
	else
    {
		DBGPRINT(RT_DEBUG_ERROR, ("ERROR: KeyDataLength is too short \n"));
        return FALSE;
    }
    
    
	// Sanity check - shared key index should not be 0
	if (pKDE->GTKEncap.Kid == 0)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key index zero \n"));
        return FALSE;
    }   
    
	// Sanity check - KED length
	if (KeyDataLength < (pKDE->Len + 2))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("ERROR: The len from KDE is too short \n"));
        return FALSE;			
    }

	// Get GTK length - refer to IEEE 802.11i-2004 p.82
	GTKLEN = pKDE->Len -6;

	if (GTKLEN < LEN_AES_KEY)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key length is too short (%d) \n", GTKLEN));
        return FALSE;
	}
	else
		DBGPRINT(RT_DEBUG_TRACE, ("GTK Key with KDE formet got index=%d, len=%d \n", pKDE->GTKEncap.Kid, GTKLEN));

	// Update GTK
	// set key material, TxMic and RxMic for WPAPSK	
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].GTK, pKDE->GTKEncap.GTK, 32);
	pAd->ApCfg.ApCliTab[IfIdx].DefaultKeyId = pKDE->GTKEncap.Kid;
	DefaultIdx = pAd->ApCfg.ApCliTab[IfIdx].DefaultKeyId;

	// Update shared key table
	NdisZeroMemory(&pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx], sizeof(CIPHER_KEY));  
	pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].KeyLen = LEN_TKIP_EK;
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].Key, pKDE->GTKEncap.GTK, LEN_TKIP_EK);
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].RxMic, &pKDE->GTKEncap.GTK[16], LEN_TKIP_RXMICK);
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].TxMic, &pKDE->GTKEncap.GTK[24], LEN_TKIP_TXMICK);

	// Update Shared Key CipherAlg
	pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].CipherAlg = CIPHER_NONE;
	if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption2Enabled)
		pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].CipherAlg = CIPHER_TKIP;
	else if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption3Enabled)
		pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultIdx].CipherAlg = CIPHER_AES;

	return TRUE;
 
}


BOOLEAN  ApCliHandleRxBroadcastFrame(
	IN  PRTMP_ADAPTER   pAd,
	IN	RX_BLK			*pRxBlk,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN	UCHAR			FromWhichBSSID)
{
	PRT28XX_RXD_STRUC	pRxD = &(pRxBlk->RxD);
	PHEADER_802_11		pHeader = pRxBlk->pHeader;
	PRXWI_STRUC			pRxWI = pRxBlk->pRxWI;		
	PAPCLI_STRUCT   	pApCliEntry = NULL;
	
	// It is possible to receive the multicast packet when in AP Client mode
	// Such as a broadcast from remote AP to AP-client, address1 is ffffff, address2 is remote AP's bssid, addr3 is sta4 mac address
																																								
	pApCliEntry	= &pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx];																											
					
	// Filter out Bcast frame which AP relayed for us
	// Multicast packet send from AP1 , received by AP2 and send back to AP1, drop this frame   					
	if (MAC_ADDR_EQUAL(pHeader->Addr3, pApCliEntry->CurrentAddress))
		return FALSE;	// give up this frame

	if (pEntry->PrivacyFilter != Ndis802_11PrivFilterAcceptAll)
		return FALSE;	// give up this frame
					
	DBGPRINT(RT_DEBUG_INFO, ("IF-apcli%d : B/M-cast frame pRxWI->WirelessCliID=%d \n", pEntry->MatchAPCLITabIdx, pRxWI->WirelessCliID));
							
	// Use software to decrypt the encrypted frame.
	// Because this received frame isn't my BSS frame, Asic passed to driver without decrypting it.
	// If receiving an "encrypted" unicast packet(its WEP bit as 1) and doesn't match my BSSID, it 
	// pass to driver with "Decrypted" marked as 0 in RxD.
	if ((pRxD->MyBss == 0) && (pRxD->Decrypted == 0) && (pHeader->FC.Wep == 1)) 
	{											
		if (RTMPSoftDecryptBroadCastData(pAd, pRxBlk, pApCliEntry->GroupCipher, pApCliEntry->SharedKey) == NDIS_STATUS_FAILURE)
        {
				return FALSE;  // give up this frame
		}
	}
							
		
	pRxBlk->pData += LENGTH_802_11;
	pRxBlk->DataSize = (USHORT)pRxWI->MPDUtotalByteCount - LENGTH_802_11;

	// remove the 2 extra QOS CNTL bytes
	if (pHeader->FC.SubType & 0x08)
	{						
		pRxBlk->pData += 2;
		pRxBlk->DataSize -= 2;
	}

	// L2PAD bit on will pad 2 bytes at LLC 
	if (pRxD->L2PAD)
	{
		pRxBlk->pData += 2;	
	}
					
	Indicate_Legacy_Packet(pAd, pRxBlk, FromWhichBSSID);								

	return TRUE;
}


VOID APCliUpdatePairwiseKeyTable(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			*KeyRsc,
	IN  MAC_TABLE_ENTRY *pEntry)
{
	UCHAR	IfIdx;

	IfIdx = pEntry->MatchAPCLITabIdx;

	// Update PTK
	// Prepare pair-wise key information into shared key table
	NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));   
	pEntry->PairwiseKey.KeyLen = LEN_TKIP_EK;
    NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[32], LEN_TKIP_EK);
    NdisMoveMemory(pEntry->PairwiseKey.RxTsc, KeyRsc, 6);
    
    // Prepare Pairwise key - Select RxMic and TxMic based on supplicant
    NdisMoveMemory(pEntry->PairwiseKey.RxMic, &pEntry->PTK[48], LEN_TKIP_RXMICK);
    NdisMoveMemory(pEntry->PairwiseKey.TxMic, &pEntry->PTK[48+LEN_TKIP_RXMICK], LEN_TKIP_TXMICK);
	
	// Prepare Pairwise key - CipherAlg
	pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
	if (pAd->ApCfg.ApCliTab[IfIdx].PairCipher == Ndis802_11Encryption2Enabled)
		pEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
	else if (pAd->ApCfg.ApCliTab[IfIdx].PairCipher == Ndis802_11Encryption3Enabled)
		pEntry->PairwiseKey.CipherAlg = CIPHER_AES;
	
	// Add Pair-wise key to Asic
    AsicAddPairwiseKeyEntry(
    		pAd, 
            pEntry->Addr, 
            (UCHAR)pEntry->Aid, 
            &pEntry->PairwiseKey);

	// update WCID attribute table and IVEIV table for this entry
	RTMPAddWcidAttributeEntry(
			pAd, 
			(pAd->ApCfg.BssidNum + IfIdx + MIN_NET_DEVICE_FOR_APCLI), 
			0, 
			pEntry->PairwiseKey.CipherAlg,
			pEntry);
		
}


BOOLEAN APCliUpdateSharedKeyTable(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pKey,
	IN  UCHAR           KeyLen,
	IN	UCHAR			DefaultKeyIdx,
	IN  MAC_TABLE_ENTRY *pEntry)
{
	UCHAR	IfIdx;
	UCHAR	GTK_len = 0;

	if (!pEntry || pEntry->ValidAsApCli == FALSE)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("APCliUpdateSharedKeyTable : This Entry doesn't exist!!! \n"));		
		return FALSE;
	}

	IfIdx = pEntry->MatchAPCLITabIdx;

	if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption2Enabled && KeyLen >= TKIP_GTK_LENGTH)
	{
		GTK_len = TKIP_GTK_LENGTH;
	}
	else if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption3Enabled && KeyLen >= LEN_AES_KEY)
	{
		GTK_len = LEN_AES_KEY;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("APCliUpdateSharedKeyTable : GTK is invalid (GroupCipher=%d, DataLen=%d) !!! \n", pAd->ApCfg.ApCliTab[IfIdx].GroupCipher, KeyLen));		
		return FALSE;
	}

	// Update GTK
	// set key material, TxMic and RxMic for WPAPSK	
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].GTK, pKey, GTK_len);
	pAd->ApCfg.ApCliTab[IfIdx].DefaultKeyId = DefaultKeyIdx;

	// Update shared key table
	NdisZeroMemory(&pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx], sizeof(CIPHER_KEY));  
	pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].KeyLen = GTK_len;
	NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].Key, pKey, LEN_TKIP_EK);
	if (GTK_len == TKIP_GTK_LENGTH)
	{
		NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].RxMic, pKey + 16, LEN_TKIP_RXMICK);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].TxMic, pKey + 24, LEN_TKIP_TXMICK);
	}

	// Update Shared Key CipherAlg
	pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].CipherAlg = CIPHER_NONE;
	if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption2Enabled)
		pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].CipherAlg = CIPHER_TKIP;
	else if (pAd->ApCfg.ApCliTab[IfIdx].GroupCipher == Ndis802_11Encryption3Enabled)
		pAd->ApCfg.ApCliTab[IfIdx].SharedKey[DefaultKeyIdx].CipherAlg = CIPHER_AES;

	return TRUE;
}

