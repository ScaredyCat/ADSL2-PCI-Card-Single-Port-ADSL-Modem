/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2007, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    cmm_mat.c

    Abstract:
	    Support Mac Address Translation function.

    Note:
		MAC Address Translation(MAT) engine subroutines, we should just take care 
	 packet to bridge.
		
    Revision History:
    Who             When            What
    --------------  ----------      ----------------------------------------------
    Shiang  		02-26-2007      Init version
*/

#include "rt_config.h"


extern MATProtoEntry MATProtoIPHandle;
extern MATProtoEntry MATProtoARPHandle;
extern MATProtoEntry MATProtoPPPoEDisHandle;
extern MATProtoEntry MATProtoPPPoESesHandle;
extern MATProtoEntry MATProtoIPv6Handle;

extern UCHAR SNAP_802_1H[];
extern UCHAR SNAP_BRIDGE_TUNNEL[];

static MATProtoTable MATProtoTb[]=
{
	{ETH_P_IP, 			&MATProtoIPHandle},			// IP handler
	{ETH_P_ARP, 		&MATProtoARPHandle},		// ARP handler
	{ETH_P_PPP_DISC,	&MATProtoPPPoEDisHandle}, 	// PPPoE discovery stage handler
	{ETH_P_PPP_SES,		&MATProtoPPPoESesHandle},	// PPPoE session stage handler
	{ETH_P_IPV6,		&MATProtoIPv6Handle},		// IPv6 handler
//	{0x0,    			NULL},					
};

#define MAX_MAT_SUPPORT_PROTO_NUM (sizeof(MATProtoTb)/sizeof(MATProtoTable))

#define MAX_MAT_NODE_ENTRY_NUM	127	// We support maxima 127 node entry for our system
#define MAT_NODE_ENTRY_SIZE	40 //28	// bytes   //change to 40 for IPv6Mac Table

typedef struct _MATNodeEntry
{
	UCHAR data[MAT_NODE_ENTRY_SIZE];
	struct _MATNodeEntry *next;
}MATNodeEntry, *PMATNodeEntry;

static int MATEngineInited = 0;
static int MATEngineExited = 1;
NDIS_SPIN_LOCK MATDBLock;

/* --------------------------------- Public Function-------------------------------- */
#ifdef KMALLOC_BATCH
static MATNodeEntry *MATNodeEntryPoll = NULL;
#endif

NDIS_STATUS MATDBEntryFree(PUCHAR NodeEntry)
{
#ifdef KMALLOC_BATCH
	MATNodeEntry *pPtr;

	pPtr = (MATNodeEntry *)NodeEntry;
	NdisZeroMemory(pPtr, sizeof(MATNodeEntry));
	if (MATNodeEntryPoll->next)
	{
		pPtr->next = MATNodeEntryPoll->next;
		MATNodeEntryPoll->next = pPtr;
	} else {
		MATNodeEntryPoll->next = pPtr;
	}
#else
	os_free_mem(NULL, NodeEntry);
#endif

	return TRUE;

}

PUCHAR MATDBEntryAlloc(UINT32 size)
{
#ifdef KMALLOC_BATCH
	struct _MATNodeEntry *pPtr = NULL;
	
	if (MATNodeEntryPoll->next)
	{
		pPtr = MATNodeEntryPoll->next;
		MATNodeEntryPoll->next = pPtr->next;
	}
	
#else
	UCHAR *pPtr = NULL;

	os_alloc_mem(NULL, (PUCHAR *)&pPtr, size);
	//pPtr = kmalloc(size, MEM_ALLOC_FLAG);

#endif

	return (PUCHAR)pPtr;
}


VOID dumpPkt(PUCHAR pHeader, int len)
{
	int i;
	char *tmp;

	tmp = pHeader;

	printk("--StartDump\n");
	for(i=0;i<len; i++)
	{
		if ( (i%16==0) && (i!=0))
			printk("\n");
		printk("%02x ", tmp[i]& 0xff);
	}
	printk("\n--EndDump\n");

	return;
}


/*
	========================================================================
	Routine	Description:
		For each out-going packet, check the upper layer protocol type if need
		to handled by our APCLI convert engine. If yes, call corresponding handler 
		to handle it.
		
	Arguments:
		pAd		=>Pointer to our adapter
		pPkt 	=>pointer to the 802.11 header of outgoing packet 
		ifIdx   =>Interface Index want to dispatch to.

	Return Value:
		Success	=>
			TRUE
			Mapped mac address if found, else return specific default mac address 
			depends on the upper layer protocol type.
		Error	=>
			FALSE.

	Note:
		1.the pPktHdr must be a 802.3 packet.
		2.Maybe we need a TxD arguments?
		3.We check every packet here including group mac address becasue we need to
		  handle DHCP packet.
	========================================================================
 */
PUCHAR MATEngineTxHandle(
	IN PRTMP_ADAPTER	pAd,
	IN PNDIS_PACKET	    pPkt,
	IN UINT				ifIdx)
{
	PUCHAR 		pLayerHdr = NULL, pPktHdr = NULL;
	UINT16		protoType;
	INT			i;
	struct _MATProtoEntry 	*pHandle = NULL;
	PUCHAR  retSkb = NULL;
	BOOLEAN bVLANPkt = FALSE;

	pPktHdr = GET_OS_PKT_DATAPTR(pPkt);
	if (!pPktHdr)
		return NULL;
	
	// Get the upper layer protocol type of this 802.3 pkt.
	protoType = OS_NTOHS(*((UINT16 *)(pPktHdr + 12)));

	// handle 802.1q enabled packet. Skip the VLAN tag field to get the protocol type.
	if (protoType == 0x8100)
	{
		protoType = OS_NTOHS(*((UINT16 *)(pPktHdr + 12 + 4)));
		bVLANPkt = TRUE;
	}

	DBGPRINT(RT_DEBUG_INFO,("%s(): protoType=0x%04x\n", __FUNCTION__, protoType));
	
	// For differnet protocol, dispatch to specific handler
	for (i=0; i < MAX_MAT_SUPPORT_PROTO_NUM; i++)
	{
		if (protoType == MATProtoTb[i].protoCode)
		{
			pHandle = MATProtoTb[i].pHandle;	// the pHandle must not be null!
			pLayerHdr = bVLANPkt ? (pPktHdr + MAT_VLAN_ETH_HDR_LEN) : (pPktHdr + MAT_ETHER_HDR_LEN);
			if (pHandle->tx!=NULL)
				retSkb = pHandle->tx(pAd, RTPKT_TO_OSPKT(pPkt), pLayerHdr, ifIdx);

			return retSkb;
		}
	}
	return retSkb;
}


/*
	========================================================================
	Routine	Description:
		Depends on the Received packet, check the upper layer protocol type
		and search for specific mapping table to find out the real destination 
		MAC address.
		
	Arguments:
		pAd		=>Pointer to our adapter
		pPkt	=>pointer to the 802.11 header of receviced packet 
		infIdx	=>Interface Index want to dispatch to.

	Return Value:
		Success	=>
			Mapped mac address if found, else return specific default mac address 
			depends on the upper layer protocol type.
		Error	=>
			NULL

	Note:
	========================================================================
 */
PUCHAR MATEngineRxHandle(
	IN PRTMP_ADAPTER	pAd,
	IN PNDIS_PACKET		pPkt,
	IN UINT				infIdx)
{
	PUCHAR				pMacAddr = NULL;
	PUCHAR 		pLayerHdr = NULL, pPktHdr = NULL;
	UINT16		protoType;
	INT			i =0;
	struct _MATProtoEntry 	*pHandle = NULL;

	pPktHdr = GET_OS_PKT_DATAPTR(pPkt);
	if (!pPktHdr)
		return NULL;

	// If it's a multicast/broadcast packet, we do nothing.
	if (IS_GROUP_MAC(pPktHdr))
		return NULL;

#if 0 // Move this checking to stand-alone function call and it should be called in previous of this function.
	// Check if the packet will be send to apcli interface.
	while(i<MAX_APCLI_NUM)
	{
		//BSSID match the ApCliBssid ?(from a valid AP)
		if ((pAd->ApCfg.ApCliTab[i].Valid == TRUE) 
			&& (net_dev == pAd->ApCfg.ApCliTab[i].dev))
			break;
		i++;
	}

	if(i == MAX_APCLI_NUM)
		return NULL;
#endif

	// Get the upper layer protocol type of this 802.3 pkt and dispatch to specific handler
	protoType = OS_NTOHS(*((UINT16 *)(pPktHdr + 12)));
	DBGPRINT(RT_DEBUG_INFO, ("%s(): protoType=0x%04x\n", __FUNCTION__, protoType));
	for (i=0; i<MAX_MAT_SUPPORT_PROTO_NUM; i++)
	{
		if (protoType == MATProtoTb[i].protoCode)
		{
			pHandle = MATProtoTb[i].pHandle;	// the pHandle must not be null!
			pLayerHdr = (pPktHdr + MAT_ETHER_HDR_LEN);
//			RTMP_SEM_LOCK(&MATDBLock);
			if(pHandle->rx!=NULL)
				pMacAddr = pHandle->rx(pAd, RTPKT_TO_OSPKT(pPkt), pLayerHdr, infIdx);
//			RTMP_SEM_UNLOCK(&MATDBLock);
			break;
		}
	}

	if (pMacAddr)
		NdisMoveMemory(pPktHdr, pMacAddr, MAC_ADDR_LEN);

	return NULL;

}


BOOLEAN MATPktRxNeedConvert(
		IN PRTMP_ADAPTER	pAd, 
		IN PNET_DEV			net_dev)
{
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	int i = 0;
	
	// Check if the packet will be send to apcli interface.
	while(i<MAX_APCLI_NUM)
	{
		//BSSID match the ApCliBssid ?(from a valid AP)
		if ((pAd->ApCfg.ApCliTab[i].Valid == TRUE) 
			&& (net_dev == pAd->ApCfg.ApCliTab[i].dev))
			return TRUE;
		i++;
	}
#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


	return FALSE;
	
}


NDIS_STATUS MATEngineExit(VOID)
{
	struct _MATProtoEntry 	*pHandle = NULL;
	int i;

	if(MATEngineExited)
		return TRUE;
	
	// For each registered protocol, we call it's exit handler.
	for (i=0; i<MAX_MAT_SUPPORT_PROTO_NUM; i++)
	{
		if (MATProtoTb[i].pHandle != NULL)
		{
			pHandle = MATProtoTb[i].pHandle;
			if (pHandle->exit!=NULL)
				pHandle->exit();
		}
	}

#ifdef KMALLOC_BATCH
	// Free the memory used to store node entries.
	if (MATNodeEntryPoll) 
		os_free_mem(NULL, MATNodeEntryPoll);
#endif

	MATEngineExited = 1;
	
	return TRUE;
	
}


NDIS_STATUS MATEngineInit(VOID)
{
	MATProtoEntry 	*pHandle = NULL;
	int i;
#ifdef KMALLOC_BATCH
	int status;
#endif

	if(MATEngineInited)
		return TRUE;
	
#ifdef KMALLOC_BATCH
	// Allocate memory for node entry, we totally allocate 127 entries and link list them together.
	status = os_alloc_mem(NULL, (PUCHAR *)&MATNodeEntryPoll, sizeof(MATNodeEntry) * MAX_MAT_NODE_ENTRY_NUM);
	if ((status == NDIS_STATUS_SUCCESS) && (MATNodeEntryPoll != NULL))
	{
		MATNodeEntry *pPtr=NULL;

		NdisZeroMemory(MATNodeEntryPoll, sizeof(MATNodeEntry) * MAX_MAT_NODE_ENTRY_NUM);
		pPtr = MATNodeEntryPoll;
		for (i = 0; i < (MAX_MAT_NODE_ENTRY_NUM -1); i++)
		{
			pPtr->next = (MATNodeEntry *)(pPtr+1);
			pPtr = pPtr->next;
		}
		pPtr->next = NULL;
	} else {
		return FALSE;
	}
#endif

	// For each specific protocol, call it's init function.
	for (i = 0; i < MAX_MAT_SUPPORT_PROTO_NUM; i++)
	{
		if (MATProtoTb[i].pHandle != NULL)
		{
			pHandle = MATProtoTb[i].pHandle;
			if (pHandle->init != NULL)
				pHandle->init();
		}
	}

	NdisAllocateSpinLock(&MATDBLock);

	MATEngineInited = 1;
	return TRUE;
}

