#include "rt_config.h"

ULONG	RTDebugLevel = RT_DEBUG_ERROR;
//ULONG	RTDebugLevel = RT_DEBUG_TRACE;

BUILD_TIMER_FUNCTION(MlmePeriodicExec);
//BUILD_TIMER_FUNCTION(MlmeRssiReportExec);
BUILD_TIMER_FUNCTION(AsicRxAntEvalTimeout);
BUILD_TIMER_FUNCTION(APSDPeriodicExec);
BUILD_TIMER_FUNCTION(AsicRfTuningExec);

#ifdef CONFIG_AP_SUPPORT
extern VOID APDetectOverlappingExec(
				IN PVOID SystemSpecific1, 
				IN PVOID FunctionContext, 
				IN PVOID SystemSpecific2, 
				IN PVOID SystemSpecific3);

BUILD_TIMER_FUNCTION(APDetectOverlappingExec);

#ifdef DOT11N_DRAFT3
BUILD_TIMER_FUNCTION(Bss2040CoexistTimeOut);
#endif // DOT11N_DRAFT3 //

BUILD_TIMER_FUNCTION(GREKEYPeriodicExec);
BUILD_TIMER_FUNCTION(CMTimerExec);
BUILD_TIMER_FUNCTION(WPARetryExec);
BUILD_TIMER_FUNCTION(EnqueueStartForPSKExec);
BUILD_TIMER_FUNCTION(APScanTimeout);
BUILD_TIMER_FUNCTION(APQuickResponeForRateUpExec);
#ifdef IDS_SUPPORT
BUILD_TIMER_FUNCTION(RTMPIdsPeriodicExec);
#endif // IDS_SUPPORT //
#ifdef WSC_AP_SUPPORT
BUILD_TIMER_FUNCTION(WscEnqueueEapolStart);
#endif // WSC_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


#ifdef WSC_INCLUDED
BUILD_TIMER_FUNCTION(WscEAPOLTimeOutAction);
BUILD_TIMER_FUNCTION(Wsc2MinsTimeOutAction);
BUILD_TIMER_FUNCTION(WscUPnPMsgTimeOutAction);
BUILD_TIMER_FUNCTION(WscUPnPM2DTimeOutAction);
#endif // WSC_INCLUDED //



// for wireless system event message
char const *pWirelessSysEventText[IW_SYS_EVENT_TYPE_NUM] = {    
	// system status event     
    "had associated successfully",							/* IW_ASSOC_EVENT_FLAG */
    "had disassociated",									/* IW_DISASSOC_EVENT_FLAG */
    "had deauthenticated",									/* IW_DEAUTH_EVENT_FLAG */
    "had been aged-out and disassociated",					/* IW_AGEOUT_EVENT_FLAG */
    "occurred CounterMeasures attack",						/* IW_COUNTER_MEASURES_EVENT_FLAG */	
    "occurred replay counter different in Key Handshaking",	/* IW_REPLAY_COUNTER_DIFF_EVENT_FLAG */
    "occurred RSNIE different in Key Handshaking",			/* IW_RSNIE_DIFF_EVENT_FLAG */
    "occurred MIC different in Key Handshaking",			/* IW_MIC_DIFF_EVENT_FLAG */
    "occurred ICV error in RX",								/* IW_ICV_ERROR_EVENT_FLAG */
    "occurred MIC error in RX",								/* IW_MIC_ERROR_EVENT_FLAG */
	"Group Key Handshaking timeout",						/* IW_GROUP_HS_TIMEOUT_EVENT_FLAG */ 
	"Pairwise Key Handshaking timeout",						/* IW_PAIRWISE_HS_TIMEOUT_EVENT_FLAG */ 
	"RSN IE sanity check failure",							/* IW_RSNIE_SANITY_FAIL_EVENT_FLAG */ 
	"set key done in WPA/WPAPSK",							/* IW_SET_KEY_DONE_WPA1_EVENT_FLAG */ 
	"set key done in WPA2/WPA2PSK",                         /* IW_SET_KEY_DONE_WPA2_EVENT_FLAG */ 
	"connects with our wireless client",                    /* IW_STA_LINKUP_EVENT_FLAG */ 
	"disconnects with our wireless client",                 /* IW_STA_LINKDOWN_EVENT_FLAG */
	"scan completed"										/* IW_SCAN_COMPLETED_EVENT_FLAG */
	"scan terminate!! Busy!! Enqueue fail!!"				/* IW_SCAN_ENQUEUE_FAIL_EVENT_FLAG */
	};						

// for wireless IDS_spoof_attack event message
char const *pWirelessSpoofEventText[IW_SPOOF_EVENT_TYPE_NUM] = {   	
    "detected conflict SSID",								/* IW_CONFLICT_SSID_EVENT_FLAG */
    "detected spoofed association response",				/* IW_SPOOF_ASSOC_RESP_EVENT_FLAG */
    "detected spoofed reassociation responses",				/* IW_SPOOF_REASSOC_RESP_EVENT_FLAG */
    "detected spoofed probe response",						/* IW_SPOOF_PROBE_RESP_EVENT_FLAG */
    "detected spoofed beacon",								/* IW_SPOOF_BEACON_EVENT_FLAG */
    "detected spoofed disassociation",						/* IW_SPOOF_DISASSOC_EVENT_FLAG */
    "detected spoofed authentication",						/* IW_SPOOF_AUTH_EVENT_FLAG */
    "detected spoofed deauthentication",					/* IW_SPOOF_DEAUTH_EVENT_FLAG */
    "detected spoofed unknown management frame",			/* IW_SPOOF_UNKNOWN_MGMT_EVENT_FLAG */
	"detected replay attack"								/* IW_REPLAY_ATTACK_EVENT_FLAG */	
	};

// for wireless IDS_flooding_attack event message
char const *pWirelessFloodEventText[IW_FLOOD_EVENT_TYPE_NUM] = {   	
	"detected authentication flooding",						/* IW_FLOOD_AUTH_EVENT_FLAG */
    "detected association request flooding",				/* IW_FLOOD_ASSOC_REQ_EVENT_FLAG */
    "detected reassociation request flooding",				/* IW_FLOOD_REASSOC_REQ_EVENT_FLAG */
    "detected probe request flooding",						/* IW_FLOOD_PROBE_REQ_EVENT_FLAG */
    "detected disassociation flooding",						/* IW_FLOOD_DISASSOC_EVENT_FLAG */
    "detected deauthentication flooding",					/* IW_FLOOD_DEAUTH_EVENT_FLAG */
    "detected 802.1x eap-request flooding"					/* IW_FLOOD_EAP_REQ_EVENT_FLAG */	
	};

#ifdef WSC_INCLUDED
// for WSC wireless event message
char const *pWirelessWscEventText[IW_WSC_EVENT_TYPE_NUM] = {   	
	"PBC Session Overlap",									/* IW_WSC_PBC_SESSION_OVERLAP */
	};
#endif // WSC_INCLUDED //

/* timeout -- ms */
VOID RTMP_SetPeriodicTimer(
	IN	NDIS_MINIPORT_TIMER *pTimer, 
	IN	unsigned long timeout)
{
	timeout = ((timeout*HZ) / 1000);
	pTimer->expires = jiffies + timeout;
	add_timer(pTimer);
}

/* convert NdisMInitializeTimer --> RTMP_OS_Init_Timer */
VOID RTMP_OS_Init_Timer(
	IN	PRTMP_ADAPTER pAd,
	IN	NDIS_MINIPORT_TIMER *pTimer, 
	IN	TIMER_FUNCTION function,
	IN	PVOID data)
{
	init_timer(pTimer);
    pTimer->data = (unsigned long)data;
    pTimer->function = function;		
}


VOID RTMP_OS_Add_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	IN	unsigned long timeout)
{
	if (timer_pending(pTimer))
		return;

	timeout = ((timeout*HZ) / 1000);
	pTimer->expires = jiffies + timeout;
	add_timer(pTimer);
}

VOID RTMP_OS_Mod_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	IN	unsigned long timeout)
{
	timeout = ((timeout*HZ) / 1000);
	mod_timer(pTimer, jiffies + timeout);
}

VOID RTMP_OS_Del_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	OUT	BOOLEAN					*pCancelled)
{
	if (timer_pending(pTimer))
	{	
		*pCancelled = del_timer_sync(pTimer);	
	}
	else
	{
		*pCancelled = TRUE;
	}
	
}

VOID RTMP_OS_Release_Packet(
	IN	PRTMP_ADAPTER pAd,
	IN	PQUEUE_ENTRY  pEntry)
{
	//RTMPFreeNdisPacket(pAd, (struct sk_buff *)pEntry);
}
	
// Unify all delay routine by using udelay
VOID RTMPusecDelay(
	IN	ULONG	usec)
{
	ULONG	i;

	for (i = 0; i < (usec / 50); i++)
		udelay(50);

	if (usec % 50)
		udelay(usec % 50);
}

void RTMP_GetCurrentSystemTime(LARGE_INTEGER *time)
{
	time->u.LowPart = jiffies;
}

// pAd MUST allow to be NULL
NDIS_STATUS os_alloc_mem(
	IN	PRTMP_ADAPTER pAd,
	OUT	PUCHAR *mem,
	IN	ULONG  size)
{	
	*mem = (PUCHAR) kmalloc(size, GFP_ATOMIC);
	if (*mem)
		return (NDIS_STATUS_SUCCESS);
	else
		return (NDIS_STATUS_FAILURE);
}

// pAd MUST allow to be NULL
NDIS_STATUS os_free_mem(
	IN	PRTMP_ADAPTER pAd,
	IN	PUCHAR mem)
{
	
	ASSERT(mem);
	kfree(mem);
	return (NDIS_STATUS_SUCCESS);
}


PNDIS_PACKET RTMP_AllocateFragPacketBuffer(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length)
{
	struct sk_buff *pkt;

	pkt = dev_alloc_skb(Length);

	if (pkt == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("can't allocate frag rx %ld size packet\n",Length));
	}

	if (pkt)
	{
		RTMP_SET_PACKET_SOURCE(OSPKT_TO_RTPKT(pkt), PKTSRC_NDIS);
	}	

	return (PNDIS_PACKET) pkt;	
}


PNDIS_PACKET RTMP_AllocateTxPacketBuffer(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress)
{
	struct sk_buff *pkt;

	pkt = dev_alloc_skb(Length);

	if (pkt == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("can't allocate tx %ld size packet\n",Length));
	}

	if (pkt)
	{
		RTMP_SET_PACKET_SOURCE(OSPKT_TO_RTPKT(pkt), PKTSRC_NDIS);
		*VirtualAddress = (PVOID) pkt->data;	
	}
	else
	{
		*VirtualAddress = (PVOID) NULL;
	}	

	return (PNDIS_PACKET) pkt;	
}


VOID build_tx_packet(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	PUCHAR	pFrame,
	IN	ULONG	FrameLen)
{

	struct sk_buff	*pTxPkt;

	ASSERT(pPacket);
	pTxPkt = RTPKT_TO_OSPKT(pPacket);

	NdisMoveMemory(skb_put(pTxPkt, FrameLen), pFrame, FrameLen);	
}

VOID	RTMPFreeAdapter(
	IN	PRTMP_ADAPTER	pAd)
{
    POS_COOKIE os_cookie;
	int index;	

	os_cookie=(POS_COOKIE)pAd->OS_Cookie;

	kfree(pAd->BeaconBuf);


	NdisFreeSpinLock(&pAd->MgmtRingLock);
	
#ifdef RT2860 
#ifdef WIN_NDIS
	NdisFreeSpinLock(&pAd->TxRingLock);	
	NdisFreeSpinLock(&pAd->LocalTxBufQueueLock);
#endif
	NdisFreeSpinLock(&pAd->RxRingLock);
#endif // RT2860 //

	for (index =0 ; index < NUM_OF_TX_RING; index++)
	{
    	NdisFreeSpinLock(&pAd->TxSwQueueLock[index]);
		NdisFreeSpinLock(&pAd->DeQueueLock[index]);
		pAd->DeQueueRunning[index] = FALSE;
	}
	
	NdisFreeSpinLock(&pAd->irq_lock);

	vfree(pAd); // pci_free_consistent(os_cookie->pci_dev,sizeof(RTMP_ADAPTER),pAd,os_cookie->pAd_pa);
	kfree(os_cookie);
}

BOOLEAN OS_Need_Clone_Packet(void)
{
	return (FALSE);	
}



/*
	========================================================================

	Routine Description:
		clone an input NDIS PACKET to another one. The new internally created NDIS PACKET
		must have only one NDIS BUFFER
		return - byte copied. 0 means can't create NDIS PACKET
		NOTE: internally created NDIS_PACKET should be destroyed by RTMPFreeNdisPacket
		
	Arguments:
		pAd 	Pointer to our adapter
		pInsAMSDUHdr	EWC A-MSDU format has extra 14-bytes header. if TRUE, insert this 14-byte hdr in front of MSDU.
		*pSrcTotalLen			return total packet length. This lenght is calculated with 802.3 format packet.
		
	Return Value:
		NDIS_STATUS_SUCCESS 	
		NDIS_STATUS_FAILURE 	
		
	Note:
	
	========================================================================
*/
NDIS_STATUS RTMPCloneNdisPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	BOOLEAN			pInsAMSDUHdr,
	IN	PNDIS_PACKET	pInPacket,
	OUT PNDIS_PACKET   *ppOutPacket)
{

	struct sk_buff *pkt;

	ASSERT(pInPacket);
	ASSERT(ppOutPacket);

	// 1. Allocate a packet 
	pkt = dev_alloc_skb(2048);
	
	if (pkt == NULL)
	{
		return NDIS_STATUS_FAILURE;
	}

 	skb_put(pkt, GET_OS_PKT_LEN(pInPacket));
	NdisMoveMemory(pkt->data, GET_OS_PKT_DATAPTR(pInPacket), GET_OS_PKT_LEN(pInPacket));  
	*ppOutPacket = OSPKT_TO_RTPKT(pkt);


	RTMP_SET_PACKET_SOURCE(OSPKT_TO_RTPKT(pkt), PKTSRC_NDIS);

	printk("###Clone###\n");

	return NDIS_STATUS_SUCCESS;
}


// the allocated NDIS PACKET must be freed via RTMPFreeNdisPacket()
NDIS_STATUS RTMPAllocateNdisPacket(
	IN	PRTMP_ADAPTER	pAd,
	OUT PNDIS_PACKET   *ppPacket,
	IN	PUCHAR			pHeader,
	IN	UINT			HeaderLen,
	IN	PUCHAR			pData,
	IN	UINT			DataLen)
{
	PNDIS_PACKET	pPacket;
	ASSERT(pData);
	ASSERT(DataLen);

	// 1. Allocate a packet 
	pPacket = (PNDIS_PACKET *) dev_alloc_skb(HeaderLen + DataLen + TXPADDING_SIZE);
	if (pPacket == NULL)
 	{
		*ppPacket = NULL;
#ifdef DEBUG
		printk("RTMPAllocateNdisPacket Fail\n\n");
#endif
		return NDIS_STATUS_FAILURE;
	}

	// 2. clone the frame content
	if (HeaderLen > 0)
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket), pHeader, HeaderLen);
	if (DataLen > 0)
		NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket) + HeaderLen, pData, DataLen);

	// 3. update length of packet
 	skb_put(GET_OS_PKT_TYPE(pPacket), HeaderLen+DataLen);

	RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);
//	printk("%s : pPacket = %p, len = %d\n", __FUNCTION__, pPacket, GET_OS_PKT_LEN(pPacket));
	*ppPacket = pPacket;
	return NDIS_STATUS_SUCCESS;
}

/*
  ========================================================================
  Description:
	This routine frees a miniport internally allocated NDIS_PACKET and its
	corresponding NDIS_BUFFER and allocated memory.
  ========================================================================
*/
VOID RTMPFreeNdisPacket(
	IN PRTMP_ADAPTER pAd,
	IN PNDIS_PACKET  pPacket)
{
	dev_kfree_skb_any(RTPKT_TO_OSPKT(pPacket));
}


// IRQL = DISPATCH_LEVEL
// NOTE: we do have an assumption here, that Byte0 and Byte1 always reasid at the same 
//			 scatter gather buffer
NDIS_STATUS Sniff2BytesFromNdisBuffer(
	IN	PNDIS_BUFFER	pFirstBuffer,
	IN	UCHAR			DesiredOffset,
	OUT PUCHAR			pByte0,
	OUT PUCHAR			pByte1)
{
    *pByte0 = *(PUCHAR)(pFirstBuffer + DesiredOffset);
    *pByte1 = *(PUCHAR)(pFirstBuffer + DesiredOffset + 1);

	return NDIS_STATUS_SUCCESS;
}


void RTMP_QueryPacketInfo(
	IN  PNDIS_PACKET pPacket,
	OUT PACKET_INFO  *pPacketInfo,
	OUT PUCHAR		 *pSrcBufVA,
	OUT	UINT		 *pSrcBufLen)
{
	pPacketInfo->BufferCount = 1;
	pPacketInfo->pFirstBuffer = GET_OS_PKT_DATAPTR(pPacket);
	pPacketInfo->PhysicalBufferCount = 1;
	pPacketInfo->TotalPacketLength = GET_OS_PKT_LEN(pPacket);

	*pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);
	*pSrcBufLen = GET_OS_PKT_LEN(pPacket); 	
}

void RTMP_QueryNextPacketInfo(
	IN  PNDIS_PACKET *ppPacket,
	OUT PACKET_INFO  *pPacketInfo,
	OUT PUCHAR		 *pSrcBufVA,
	OUT	UINT		 *pSrcBufLen)
{
	PNDIS_PACKET pPacket = NULL;

	if (*ppPacket)
		pPacket = GET_OS_PKT_NEXT(*ppPacket);

	if (pPacket)
	{
		pPacketInfo->BufferCount = 1;
		pPacketInfo->pFirstBuffer = GET_OS_PKT_DATAPTR(pPacket);
		pPacketInfo->PhysicalBufferCount = 1;
		pPacketInfo->TotalPacketLength = GET_OS_PKT_LEN(pPacket);

		*pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);
		*pSrcBufLen = GET_OS_PKT_LEN(pPacket); 	
		*ppPacket = GET_OS_PKT_NEXT(pPacket);		
	}
	else
	{
		pPacketInfo->BufferCount = 0;
		pPacketInfo->pFirstBuffer = NULL;
		pPacketInfo->PhysicalBufferCount = 0;
		pPacketInfo->TotalPacketLength = 0;

		*pSrcBufVA = NULL;
		*pSrcBufLen = 0; 	
		*ppPacket = NULL;
	}
}

// not yet support MBSS
PNET_DEV get_netdev_from_bssid(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			FromWhichBSSID)
{
    PNET_DEV dev_p = NULL;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef APCLI_SUPPORT
		if(FromWhichBSSID >= MIN_NET_DEVICE_FOR_APCLI)
		{
			INT ApCliIdx = FromWhichBSSID - MIN_NET_DEVICE_FOR_APCLI;

			dev_p = (ApCliIdx > MAX_APCLI_NUM ? NULL : pAd->ApCfg.ApCliTab[ApCliIdx].dev);
		} 
		else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
		if(FromWhichBSSID >= MIN_NET_DEVICE_FOR_WDS)
		{
			INT WdsIndex = FromWhichBSSID - MIN_NET_DEVICE_FOR_WDS;
			if (WdsIndex > MAX_WDS_ENTRY)
				dev_p = NULL;
			else
				dev_p = pAd->WdsTab.WdsEntry[WdsIndex].dev;
		}
		else
#endif // WDS_SUPPORT //
		{
			if (FromWhichBSSID >= pAd->ApCfg.BssidNum)
	    		{
				DBGPRINT(RT_DEBUG_ERROR,
	    	         ("%s: fatal error ssid > ssid num!\n", __FUNCTION__));
				dev_p = pAd->net_dev; /* return main BSS */
	    		} /* End of if */

	    		if (FromWhichBSSID == BSS0)
				dev_p = pAd->net_dev;
	    		else
	    		{
	    	    		dev_p = pAd->ApCfg.MBSSID[FromWhichBSSID].MSSIDDev;
	    		} /* End of if */
		}
	}
#endif // CONFIG_AP_SUPPORT //


	ASSERT(dev_p);
	return dev_p; /* return one of MBSS */
}
	
PNDIS_PACKET DuplicatePacket(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			FromWhichBSSID)
{
	struct sk_buff	*skb;
	PNDIS_PACKET	pRetPacket = NULL;
	USHORT			DataSize;
	UCHAR			*pData;

	DataSize = (USHORT) GET_OS_PKT_LEN(pPacket);
	pData = (PUCHAR) GET_OS_PKT_DATAPTR(pPacket);	


	skb = skb_clone(RTPKT_TO_OSPKT(pPacket), MEM_ALLOC_FLAG);
	if (skb)
	{
		skb->dev = get_netdev_from_bssid(pAd, FromWhichBSSID);
		pRetPacket = OSPKT_TO_RTPKT(skb);
	}

#if 0
	if ((skb = __dev_alloc_skb(DataSize + 2+32, MEM_ALLOC_FLAG)) != NULL)
	{
		skb_reserve(skb, 2+32);				
		NdisMoveMemory(skb->tail, pData, DataSize);
		skb_put(skb, DataSize);
		skb->dev = get_netdev_from_bssid(pAd, FromWhichBSSID);
		pRetPacket = OSPKT_TO_RTPKT(skb);
	}
#endif

	return pRetPacket;

}

PNDIS_PACKET duplicate_pkt(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			pHeader802_3,
    IN  UINT            HdrLen,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	UCHAR			FromWhichBSSID)
{
	struct sk_buff	*skb;
	PNDIS_PACKET	pPacket = NULL;


	if ((skb = __dev_alloc_skb(HdrLen + DataSize + 2, MEM_ALLOC_FLAG)) != NULL)
	{
		skb_reserve(skb, 2);				
		NdisMoveMemory(skb->tail, pHeader802_3, HdrLen);
		skb_put(skb, HdrLen);
		NdisMoveMemory(skb->tail, pData, DataSize);
		skb_put(skb, DataSize);
		skb->dev = get_netdev_from_bssid(pAd, FromWhichBSSID);
		pPacket = OSPKT_TO_RTPKT(skb);
	}

	return pPacket;
}


#define TKIP_TX_MIC_SIZE		8
PNDIS_PACKET duplicate_pkt_with_TKIP_MIC(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket)
{
	struct sk_buff	*skb, *newskb;
	

	skb = RTPKT_TO_OSPKT(pPacket);
	if (skb_tailroom(skb) < TKIP_TX_MIC_SIZE)
	{
		// alloc a new skb and copy the packet
		newskb = skb_copy_expand(skb, skb_headroom(skb), TKIP_TX_MIC_SIZE, GFP_ATOMIC);
		dev_kfree_skb_any(skb);
		if (newskb == NULL)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Extend Tx.MIC for packet failed!, dropping packet!\n"));
			return NULL;
		}
		skb = newskb;
	}

	return OSPKT_TO_RTPKT(skb);

#if 0	
	if ((data = skb_put(skb, TKIP_TX_MIC_SIZE)) != NULL)
	{	// If we can extend it, well, copy it first.
		NdisMoveMemory(data, pAd->PrivateInfo.Tx.MIC, TKIP_TX_MIC_SIZE);
	} 
	else
	{
		// Otherwise, copy the packet.
		newskb = skb_copy_expand(skb, skb_headroom(skb), TKIP_TX_MIC_SIZE, GFP_ATOMIC);
		dev_kfree_skb_any(skb);
		if (newskb == NULL)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Extend Tx.MIC to packet failed!, dropping packet\n"));
			return NULL;
		}
		skb = newskb;

		NdisMoveMemory(skb->tail, pAd->PrivateInfo.Tx.MIC, TKIP_TX_MIC_SIZE);
		skb_put(skb, TKIP_TX_MIC_SIZE);
	}
	
	return OSPKT_TO_RTPKT(skb);
#endif

}


#ifdef CONFIG_AP_SUPPORT
PNDIS_PACKET duplicate_pkt_with_VLAN(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			pHeader802_3,
    IN  UINT            HdrLen,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	UCHAR			FromWhichBSSID)
{
	struct sk_buff	*skb;
	PNDIS_PACKET	pPacket = NULL;
	UINT16			VLAN_Size;


	if ((skb = __dev_alloc_skb(HdrLen + DataSize + LENGTH_802_1Q + 2, \
								MEM_ALLOC_FLAG)) != NULL)
	{
		skb_reserve(skb, 2);

		/* copy header (maybe +VLAN tag) */
		VLAN_Size = VLAN_8023_Header_Copy(pAd, pHeader802_3, HdrLen,
											skb->tail, FromWhichBSSID);
		skb_put(skb, HdrLen + VLAN_Size);

		/* copy data body */
		NdisMoveMemory(skb->tail, pData, DataSize);
		skb_put(skb, DataSize);
		skb->dev = get_netdev_from_bssid(pAd, FromWhichBSSID);
		pPacket = OSPKT_TO_RTPKT(skb);
	} /* End of if */

	return pPacket;
} /* End of duplicate_pkt_with_VLAN */
#endif // CONFIG_AP_SUPPORT //


PNDIS_PACKET ClonePacket(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PNDIS_PACKET	pPacket,	
	IN	PUCHAR			pData,
	IN	ULONG			DataSize)
{
	struct sk_buff	*pRxPkt;
	struct sk_buff	*pClonedPkt;

	ASSERT(pPacket);
	pRxPkt = RTPKT_TO_OSPKT(pPacket);

	// clone the packet 
	pClonedPkt = skb_clone(pRxPkt, MEM_ALLOC_FLAG);

	if (pClonedPkt)
	{
    	// set the correct dataptr and data len
    	pClonedPkt->dev = pRxPkt->dev;
    	pClonedPkt->data = pData;
    	pClonedPkt->len = DataSize;
    	pClonedPkt->tail = pClonedPkt->data + pClonedPkt->len;
		ASSERT(DataSize < 1530);
	}
	return pClonedPkt;
}

// 
// change OS packet DataPtr and DataLen
// 
void  update_os_packet_info(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RX_BLK			*pRxBlk,
	IN  UCHAR			FromWhichBSSID)
{
	struct sk_buff	*pOSPkt;

	ASSERT(pRxBlk->pRxPacket);
	pOSPkt = RTPKT_TO_OSPKT(pRxBlk->pRxPacket);

	pOSPkt->dev = get_netdev_from_bssid(pAd, FromWhichBSSID); 
	pOSPkt->data = pRxBlk->pData;
	pOSPkt->len = pRxBlk->DataSize;
	pOSPkt->tail = pOSPkt->data + pOSPkt->len;
}


void wlan_802_11_to_802_3_packet(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RX_BLK			*pRxBlk,
	IN	PUCHAR			pHeader802_3,
	IN  UCHAR			FromWhichBSSID)
{
	struct sk_buff	*pOSPkt;

	ASSERT(pRxBlk->pRxPacket);
	ASSERT(pHeader802_3);

	pOSPkt = RTPKT_TO_OSPKT(pRxBlk->pRxPacket);

	pOSPkt->dev = get_netdev_from_bssid(pAd, FromWhichBSSID); 
	pOSPkt->data = pRxBlk->pData;
	pOSPkt->len = pRxBlk->DataSize;
	pOSPkt->tail = pOSPkt->data + pOSPkt->len;

	//
	// copy 802.3 header
	// 
	// 
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		/* maybe insert VLAN tag to the received packet */
		UCHAR VLAN_Size = 0;
		UCHAR *data_p;

		/* VLAN related */
		if ((FromWhichBSSID < pAd->ApCfg.BssidNum) &&
			(pAd->ApCfg.MBSSID[FromWhichBSSID].VLAN_VID != 0))
		{
			VLAN_Size = LENGTH_802_1Q;
		} /* End of if */

		data_p = skb_push(pOSPkt, LENGTH_802_3+VLAN_Size);

		VLAN_8023_Header_Copy(pAd, pHeader802_3, LENGTH_802_3,
								data_p, FromWhichBSSID);
	}
#endif // CONFIG_AP_SUPPORT //

	}



void announce_802_3_packet(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PNDIS_PACKET	pPacket)
{

	struct sk_buff	*pRxPkt;

	ASSERT(pPacket);

	pRxPkt = RTPKT_TO_OSPKT(pPacket);
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MATPktRxNeedConvert(pAd, pRxPkt->dev))
			MATEngineRxHandle(pAd, pPacket, 0);
	}
#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


    /* Push up the protocol stack */
#ifdef IKANOS_VX_1X0
	IKANOS_DataFrameRx(pAd, pRxPkt->dev, pRxPkt, pRxPkt->len);
#else
	pRxPkt->protocol = eth_type_trans(pRxPkt, pRxPkt->dev);

//#ifdef CONFIG_5VT_ENHANCE
//	*(int*)(pRxPkt->cb) = BRIDGE_TAG; 
//#endif
	netif_rx(pRxPkt);
#endif // IKANOS_VX_1X0 //
}


PRTMP_SCATTER_GATHER_LIST
rt_get_sg_list_from_packet(PNDIS_PACKET pPacket, RTMP_SCATTER_GATHER_LIST *sg)
{
	sg->NumberOfElements = 1;
	sg->Elements[0].Address =  GET_OS_PKT_DATAPTR(pPacket);	
	sg->Elements[0].Length = GET_OS_PKT_LEN(pPacket);	
	return (sg);
}

void hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
	unsigned char *pt;
	int x;

	if (RTDebugLevel < RT_DEBUG_TRACE)
		return;
	
	pt = pSrcBufVA;
	printk("%s: %p, len = %d\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
	{
		if (x % 16 == 0) 
			printk("0x%04x : ", x);
		printk("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) printk("\n");
	}
	printk("\n");
}

/*
	========================================================================
	
	Routine Description:
		Send log message through wireless event

		Support standard iw_event with IWEVCUSTOM. It is used below.

		iwreq_data.data.flags is used to store event_flag that is defined by user. 
		iwreq_data.data.length is the length of the event log.

		The format of the event log is composed of the entry's MAC address and
		the desired log message (refer to pWirelessEventText).

			ex: 11:22:33:44:55:66 has associated successfully

		p.s. The requirement of Wireless Extension is v15 or newer. 

	========================================================================
*/
VOID RTMPSendWirelessEvent(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Event_flag,
	IN	PUCHAR 			pAddr,
	IN	UCHAR			BssIdx,
	IN	CHAR			Rssi)
{
#if WIRELESS_EXT >= 15

	union 	iwreq_data      wrqu;
	PUCHAR 	pBuf = NULL, pBufPtr = NULL;
	USHORT	event, type, BufLen;	
	UCHAR	event_table_len = 0;

	type = Event_flag & 0xFF00;	
	event = Event_flag & 0x00FF;

	switch (type)
	{
		case IW_SYS_EVENT_FLAG_START:
			event_table_len = IW_SYS_EVENT_TYPE_NUM;
			break;

		case IW_SPOOF_EVENT_FLAG_START:
			event_table_len = IW_SPOOF_EVENT_TYPE_NUM;
			break;

		case IW_FLOOD_EVENT_FLAG_START:
			event_table_len = IW_FLOOD_EVENT_TYPE_NUM;
			break;
#ifdef WSC_AP_SUPPORT
		case IW_WSC_EVENT_FLAG_START:
			event_table_len = IW_WSC_EVENT_TYPE_NUM;
			break;
#endif // WSC_AP_SUPPORT //
	}
	
	if (event_table_len == 0)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s : The type(%0x02x) is not valid.\n", __FUNCTION__, type));			       		       		
		return;
	}
	
	if (event >= event_table_len)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s : The event(%0x02x) is not valid.\n", __FUNCTION__, event));			       		       		
		return;
	}	
 
	//Allocate memory and copy the msg.
	if((pBuf = kmalloc(IW_CUSTOM_MAX_LEN, GFP_ATOMIC)) != NULL)
	{
		//Prepare the payload 
		memset(pBuf, 0, IW_CUSTOM_MAX_LEN);

		pBufPtr = pBuf;		

		if (pAddr)
			pBufPtr += sprintf(pBufPtr, "(RT2860) STA(%02x:%02x:%02x:%02x:%02x:%02x) ", PRINT_MAC(pAddr));				
		else if (BssIdx < MAX_MBSSID_NUM)
			pBufPtr += sprintf(pBufPtr, "(RT2860) BSS(ra%d) ", BssIdx);
		else
			pBufPtr += sprintf(pBufPtr, "(RT2860) ");

		if (type == IW_SYS_EVENT_FLAG_START)
			pBufPtr += sprintf(pBufPtr, "%s", pWirelessSysEventText[event]);
		else if (type == IW_SPOOF_EVENT_FLAG_START)
			pBufPtr += sprintf(pBufPtr, "%s (RSSI=%d)", pWirelessSpoofEventText[event], Rssi);
		else if (type == IW_FLOOD_EVENT_FLAG_START)
			pBufPtr += sprintf(pBufPtr, "%s", pWirelessFloodEventText[event]);
#ifdef WSC_AP_SUPPORT
		else if (type == IW_WSC_EVENT_FLAG_START)
			pBufPtr += sprintf(pBufPtr, "%s", pWirelessWscEventText[event]);
#endif // WSC_AP_SUPPORT //
		else
			pBufPtr += sprintf(pBufPtr, "%s", "unknown event");
		
		pBufPtr[pBufPtr - pBuf] = '\0';
		BufLen = pBufPtr - pBuf;
		
		memset(&wrqu, 0, sizeof(wrqu));	
	    wrqu.data.flags = Event_flag;
		wrqu.data.length = BufLen;	
		
		//send wireless event
	    wireless_send_event(pAd->net_dev, IWEVCUSTOM, &wrqu, pBuf);
	
		//DBGPRINT(RT_DEBUG_TRACE, ("%s : %s\n", __FUNCTION__, pBuf));	
	
		kfree(pBuf);
	}
	else
		DBGPRINT(RT_DEBUG_ERROR, ("%s : Can't allocate memory for wireless event.\n", __FUNCTION__));			       		       				
#else
	DBGPRINT(RT_DEBUG_ERROR, ("%s : The Wireless Extension MUST be v15 or newer.\n", __FUNCTION__));	
#endif  /* WIRELESS_EXT >= 15 */  
}

#ifdef CONFIG_AP_SUPPORT
VOID SendSingalToDaemon(
					   IN INT              sig,
					   ULONG               pid)
{
	DBGPRINT(RT_DEBUG_TRACE, ("SendSingalToDaemon : Pid=%ld\n", pid));
	if (pid != 0)
	{
		kill_proc(pid, sig, 0);
	}
}
#endif // CONFIG_AP_SUPPORT //



void rtmp_os_thread_init(PUCHAR pThreadName, PVOID pNotify)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	daemonize(pThreadName /*"%s",pAd->net_dev->name*/);

	allow_signal(SIGTERM);
	allow_signal(SIGKILL);
	current->flags |= PF_NOFREEZE;
#else
	unsigned long flags;

	daemonize();
	reparent_to_init();
	strcpy(current->comm, pThreadName);

	siginitsetinv(&current->blocked, sigmask(SIGTERM) | sigmask(SIGKILL));	
	
	/* Allow interception of SIGKILL only
	 * Don't allow other signals to interrupt the transmission */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,22)
	spin_lock_irqsave(&current->sigmask_lock, flags);
	flush_signals(current);
	recalc_sigpending(current);
	spin_unlock_irqrestore(&current->sigmask_lock, flags);
#endif
#endif
	
    /* signal that we've started the thread */
	complete(pNotify);

}

void RTMP_IndicateMediaState(	
	IN	PRTMP_ADAPTER	pAd)
{	
	if (pAd->CommonCfg.bWirelessEvent)	
	{
		if (pAd->IndicateMediaState == NdisMediaStateConnected)
		{
			RTMPSendWirelessEvent(pAd, IW_STA_LINKUP_EVENT_FLAG, pAd->MacTab.Content[BSSID_WCID].Addr, BSS0, 0);
		}
		else
		{							
			RTMPSendWirelessEvent(pAd, IW_STA_LINKDOWN_EVENT_FLAG, pAd->MacTab.Content[BSSID_WCID].Addr, BSS0, 0); 		
		}	
	}
}

