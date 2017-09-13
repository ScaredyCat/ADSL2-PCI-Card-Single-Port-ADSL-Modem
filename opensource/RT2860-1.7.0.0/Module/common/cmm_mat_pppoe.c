/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2007, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cmm_mat_pppoe.c

	Abstract:
	    MAT convert engine subroutine for PPPoE protocol.Due to the difference 
	of characteristic of PPPoE discovery stage and session stage, we seperate 
	that as two parts and used different stretegy to handle it.

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Shiang      02/26/07      Init version
*/

#ifdef MAT_SUPPORT

#include "rt_config.h"

static NDIS_STATUS MATProto_PPPoEDis_Init(VOID);
static NDIS_STATUS MATProto_PPPoEDis_Exit(VOID);
static PUCHAR MATProto_PPPoEDis_Rx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);
static PUCHAR MATProto_PPPoEDis_Tx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);

static NDIS_STATUS MATProto_PPPoESes_Init(VOID);
static NDIS_STATUS MATProto_PPPoESes_Exit(VOID);
static PUCHAR MATProto_PPPoESes_Rx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);
static PUCHAR MATProto_PPPoESes_Tx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);


/*
                  1                 2               3             4
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  VER  | TYPE  |      CODE     |          SESSION_ID           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            LENGTH             |           payload             ~
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	VER = 0x1, TYPE =0x1
	
PPPoE Discovery Stage(Ethernet protocol type = 0x8863):
	PADI:
		DESTINATION_ADDR = 0xffffffff
		CODE = 0x09, SESSION_ID = 0x0000
		LENGTH = payload length

	PADO:
		DESTINATION_ADDR = Unicast Ethernet address of sender
		CODE = 0x07, SESSION_ID = 0x0000
		LENGTH = payload length
		NEcessary TAGS: AC-NAME(0x0102), Sevice-Name(0x0101), and other service names.

		Note: if the PPPoE server cannot serve the PADI it MUST NOT respond with a PADO

	
	PADR:
		DESTINATION_ADDR = unicast Ethernet address 
		CODE = 0x19, SESSION_ID = 0x0000
		LENGTH = payload length
		Necessary TAGS: Service-Name(0x0101)
		Optional TAGS: ....

	PADS:
		If success:
			DESTINATION_ADDR = unicast Ethernet address 
			CODE = 0x65, SESSION_ID = unique value for this pppoe session.(16 bits)
			LENGHT - payload length
			Necessary TAGS: Service-Name(0x0101)

		if failed:
			SESSION_ID = 0x0000
			Necessary TAGS: Service-Name-Error(0x0201).

	PADT:
		DESTINATION_ADDR = unicast Ethernet address
		CODE = 0xa7, SESSION_ID = previous assigned 16 bits session ID.
		Necessary TAGS: NO.

PPPoE Session Stage(Ethernet protocol type = 0x8864):
	PPP data:
		DESTINATION_ADDR = unicast Ethernet address
		CODE = 0x00, 
	LCP:
		DESTINATION_ADDR = unicast Ethernet address
		CODE = 0x00, 

*/

#define PPPOE_CODE_PADI			0x09
#define PPPOE_CODE_PADO			0x07
#define PPPOE_CODE_PADR			0x19
#define PPPOE_CODE_PADS			0x65
#define PPPOE_CODE_PADT			0xa7
#define PPPOE_TAG_ID_HOST_UNIQ	0x0103
#define PPPOE_TAG_ID_AC_COOKIE	0x0104

//#define APCLI_MAX_HOST_UNIQ_LEN 16
#define PPPoE_SES_ENTRY_AGEOUT_TIME 3000

/* Data structure used for PPPoE discovery stage */
#define PPPOE_DIS_UID_LEN		6
typedef struct _UidMacMappingEntry
{
	UCHAR isServer;
	UCHAR uIDAddByUs;				 // If the host-uniq or AC-cookie is add by our driver, set it as 1, else set as 0.
	UCHAR uIDStr[PPPOE_DIS_UID_LEN]; // String used for identify who sent this pppoe packet in discovery stage.
	UCHAR macAddr[MAC_ADDR_LEN];	 // Mac address associated to this uid string.
	UINT lastTime;
	struct _UidMacMappingEntry *pNext;	//Pointer to next entry in link-list of Uid hash table.
}UidMacMappingEntry, *PUidMacMappingEntry;

typedef struct _UidMacMappingTable
{
	BOOLEAN valid;
	UidMacMappingEntry *uidHash[MAT_MAX_HASH_ENTRY_SUPPORT];
}UidMacMappingTable;

// "Host-Uniq <-> Mac Address" Mapping table used for PPPoE Discovery stage
static UidMacMappingTable UidMacTable=
{
	.valid = FALSE,
};

/* Data struct used for PPPoE session stage */
typedef struct _SesMacMappingEntry
{
	UINT16	sessionID;	// In network order
	UCHAR	outMacAddr[MAC_ADDR_LEN];
	UCHAR	inMacAddr[MAC_ADDR_LEN];
	UINT 	lastTime;
	struct	_SesMacMappingEntry *pNext;	
}SesMacMappingEntry, *PSesMacMappingEntry;

typedef struct _SesMacMappingTable
{
	BOOLEAN valid;
	SesMacMappingEntry *sesHash[MAT_MAX_HASH_ENTRY_SUPPORT];
}SesMacMappingTable;

static SesMacMappingTable SesMacTable =
{
	.valid = FALSE,
};

// Declaration of protocol handler for PPPoE Discovery stage
struct _MATProtoEntry MATProtoPPPoEDisHandle =
{
	.init = MATProto_PPPoEDis_Init,
	.tx = MATProto_PPPoEDis_Tx,
	.rx = MATProto_PPPoEDis_Rx,
	.exit = MATProto_PPPoEDis_Exit,
};

// Declaration of protocol handler for PPPoE Session stage
struct _MATProtoEntry MATProtoPPPoESesHandle =
{
	.init = MATProto_PPPoESes_Init,
	.tx = MATProto_PPPoESes_Tx,
	.rx = MATProto_PPPoESes_Rx,
	.exit =MATProto_PPPoESes_Exit,
};

NDIS_STATUS dumpSesMacTb(int hashIdx)
{
	SesMacMappingEntry *pHead;
	int startIdx, endIdx;

	if (!SesMacTable.valid)
	{
		printk("SesMacTable not init yet, so cannot do dump!\n");
		return FALSE;
	}
	
	if(hashIdx < 0)
	{	// dump all.
		startIdx = 0;
		endIdx = MAT_MAX_HASH_ENTRY_SUPPORT - 1;
	}
	else
	{	// dump specific hash index.
		startIdx = endIdx = hashIdx;
	}

	printk("%s(): Now dump SesMac Table!\n", __FUNCTION__);
	for (; startIdx<= endIdx; startIdx++)
	{
		pHead = SesMacTable.sesHash[startIdx];
	while(pHead)
	{
			printk("SesMac[%d]:\n", startIdx);
			printk("\tsesID=%d,inMac=%02x:%02x:%02x:%02x:%02x:%02x,outMac=%02x:%02x:%02x:%02x:%02x:%02x,jiffies=0x%x, pNext=%p\n",
		pHead->sessionID, pHead->inMacAddr[0], pHead->inMacAddr[1], pHead->inMacAddr[2], pHead->inMacAddr[3], pHead->inMacAddr[4],
		pHead->inMacAddr[5], pHead->inMacAddr[0], pHead->inMacAddr[1], pHead->inMacAddr[2], pHead->inMacAddr[3], pHead->inMacAddr[4],
		pHead->inMacAddr[5],pHead->lastTime, pHead->pNext);
		pHead = pHead->pNext;
	}
	}
	printk("----EndOfDump!\n");

	return TRUE;

}


NDIS_STATUS dumpUidMacTb(int hashIdx)
{
	UidMacMappingEntry *pHead;
	int i;
	int startIdx, endIdx;
	
	if (!UidMacTable.valid)
	{
		printk("UidMacTable not init yet, so cannot do dump!\n");
		return FALSE;
	}
	
	if(hashIdx < 0)
	{	// dump all.
		startIdx = 0;
		endIdx = MAT_MAX_HASH_ENTRY_SUPPORT-1;
	}
	else
	{	// dump specific hash index.
		startIdx = endIdx = hashIdx;
	}

	printk("%s(): Now dump UidMac Table!\n", __FUNCTION__);
	for (; startIdx<= endIdx; startIdx++)
	{
		pHead = UidMacTable.uidHash[startIdx];
	while(pHead)
	{
			printk("UidMac[%d]:\n", startIdx);
			printk("\tisSrv=%d, uIDAddbyUs=%d, Mac=%02x:%02x:%02x:%02x:%02x:%02x, jiffies=0x%x, pNext=%p\n", 
				pHead->isServer, pHead->uIDAddByUs, pHead->macAddr[0],pHead->macAddr[1],pHead->macAddr[2],
				pHead->macAddr[3],pHead->macAddr[4],pHead->macAddr[5], pHead->lastTime, pHead->pNext);
			printk("\tuIDStr=");
		for(i=0; i< PPPOE_DIS_UID_LEN; i++)
			printk("%02x", pHead->uIDStr[i]);
		printk("\n");
		pHead = pHead->pNext;
	}
	}
	printk("----EndOfDump!\n");
	
	return TRUE;
}

static NDIS_STATUS UidMacTable_RemoveAll(VOID)
{
	UidMacMappingEntry *pEntry;
	INT             i;

	if (!UidMacTable.valid)
		return TRUE;

	for (i=0; i<IPMAC_TB_HASH_ENTRY_NUM; i++)
	{
		while((pEntry = UidMacTable.uidHash[i]) != NULL)
		{
			UidMacTable.uidHash[i] = pEntry->pNext;
			MATDBEntryFree((PUCHAR)pEntry);
		}
	}

	UidMacTable.valid = FALSE;

	return TRUE;
}


static NDIS_STATUS SesMacTable_RemoveAll(VOID)
{
	SesMacMappingEntry *pEntry;
	INT             i;

	if (!SesMacTable.valid)
		return TRUE;

	for (i=0; i<IPMAC_TB_HASH_ENTRY_NUM; i++)
	{
		while((pEntry = SesMacTable.sesHash[i]) != NULL)
		{
			SesMacTable.sesHash[i] = pEntry->pNext;
			MATDBEntryFree((PUCHAR)pEntry);
		}
	}

	SesMacTable.valid = FALSE;
	
	return TRUE;

}


static PUidMacMappingEntry UidMacTableUpdate(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			pInMac,
	IN PUCHAR			pOutMac,
	IN PUCHAR			pTagInfo,
	IN UINT16			tagLen,
	IN UINT16			isServer)
{
	UINT 				hashIdx, i=0, uIDAddByUs = 0;
	UidMacMappingEntry	*pEntry = NULL, *pPrev = NULL, *pNewEntry =NULL;
	UCHAR 				hashVal = 0;
	PUCHAR				pUIDStr= NULL;

	if (!UidMacTable.valid)
		return NULL;

	if (pTagInfo && tagLen >0)
	{
		pUIDStr = pTagInfo;
		uIDAddByUs = 0;
		tagLen = (tagLen > PPPOE_DIS_UID_LEN ? PPPOE_DIS_UID_LEN : tagLen);
	}
	else
	{
		// We assume the station just have one role,i.e., just a PPPoE server or just a PPPoE client.
		// For a packet send by server, we use the destination MAC as our uIDStr
		// For a packet send by client, we use the source MAC as our uIDStr.
		pUIDStr = isServer ? pOutMac: pInMac;
		tagLen = MAC_ADDR_LEN;
		uIDAddByUs = 1;
	}

	for (i=0; i<tagLen; i++)
		hashVal ^= (pUIDStr[i] & 0xff);
	hashIdx = hashVal % MAT_MAX_HASH_ENTRY_SUPPORT;
	
	//First, check if the hashIdx exists
	if (hashIdx < MAT_MAX_HASH_ENTRY_SUPPORT)
	{
		pEntry = pPrev = UidMacTable.uidHash[hashIdx];
		while(pEntry)
		{
			// Find the existed UidMac Mapping entry
			if (NdisEqualMemory(pUIDStr, pEntry->uIDStr, tagLen) && IS_EQUAL_MAC(pEntry->macAddr, pInMac))
    		{
#if 0
				printk("%s(): Got the Mac(%02x:%02x:%02x:%02x:%02x:%02x) with tagInfo:\n",
						__FUNCTION__, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
						pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5]); 
				for(i=0; i< tagLen; i++)
					printk("%02x ", pUIDStr[i] & 0xff);
				printk("\n");
#endif
				// Update info of this entry
				pEntry->isServer = isServer;
				pEntry->uIDAddByUs = uIDAddByUs;
				pEntry->lastTime = jiffies;
				return pEntry;
			}
			else
        	{	// handle the age-out situation
	        	if ((jiffies - pEntry->lastTime) > MAT_TB_ENTRY_AGEOUT_TIME)
				{
					// Remove the aged entry from the uidHash
					if (pEntry == UidMacTable.uidHash[hashIdx])
					{
						UidMacTable.uidHash[hashIdx]= pEntry->pNext;
						pPrev = UidMacTable.uidHash[hashIdx];
	        		}
					else 
					{	
						pPrev->pNext = pEntry->pNext;
					}

					//After remove this entry from macHash list and uidHash list, now free it!
					MATDBEntryFree((PUCHAR)pEntry);

#ifdef ETH_CONVERT_SUPPORT
					pAd->EthConvert.nodeCount--;
#endif // ETH_CONVERT_SUPPORT //
					pEntry = (pPrev == NULL ? NULL: pPrev->pNext);
	        		} 
				else
				{
					pPrev = pEntry;
					pEntry = pEntry->pNext;
				}
			}
		}
	}

#ifdef ETH_CONVERT_SUPPORT
	if (pAd->EthConvert.nodeCount >= ETH_CONVERT_NODE_MAX)
		return FALSE;
#endif // ETH_CONVERT_SUPPORT //

	// Allocate a new UidMacMapping entry and insert into the double-hash
	pNewEntry = (UidMacMappingEntry *)MATDBEntryAlloc(sizeof(UidMacMappingEntry));
	if (pNewEntry)
	{	
		NdisZeroMemory(pNewEntry, sizeof(UidMacMappingEntry));
		
		pNewEntry->isServer = isServer;
		pNewEntry->uIDAddByUs = uIDAddByUs;
		NdisMoveMemory(pNewEntry->macAddr, pInMac, MAC_ADDR_LEN);
		NdisMoveMemory(pNewEntry->uIDStr, pUIDStr, tagLen);
		pNewEntry->pNext = NULL;
		pNewEntry->lastTime = jiffies;
		
		// Update mac-side hash link list
		if (UidMacTable.uidHash[hashIdx] == NULL)
		{	// Hash list is empty, directly assign it.
			UidMacTable.uidHash[hashIdx] = pNewEntry;
		}
		else 
		{
			// Ok, we insert the new entry into the root of uidHash[hashIdx]
			pNewEntry->pNext = UidMacTable.uidHash[hashIdx];
			UidMacTable.uidHash[hashIdx] = pNewEntry;
		}
		//dumpUidMacTb(hashIdx); //for debug
		
#ifdef ETH_CONVERT_SUPPORT
		pAd->EthConvert.nodeCount++;
#endif // ETH_CONVERT_SUPPORT //

		return pNewEntry;
	}
	
	return NULL;
}


static PUidMacMappingEntry UidMacTableLookUp(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			pTagInfo,
	IN UINT16			tagLen)
{
    UINT 				hashIdx;
	UINT16				len;
	UCHAR				hashValue = 0;
    UidMacMappingEntry	*pEntry = NULL;

	if (!UidMacTable.valid)
		return NULL;

	// Use hash to find out the location of that entry and get the Mac address.
	len = tagLen;
	while(len)
		hashValue ^= pTagInfo[--len];
	hashIdx = hashValue % MAT_MAX_HASH_ENTRY_SUPPORT;

	pEntry = UidMacTable.uidHash[hashIdx];
	while(pEntry)
	{
		if (NdisEqualMemory(pEntry->uIDStr, pTagInfo, tagLen))
		{
/*			printk("%s(): dstMac=%02x:%02x:%02x:%02x:%02x:%02x for mapped dstIP(%d.%d.%d.%d)\n", 
					__FUNCTION__, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
					pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5],
					(ipAddr>>24) & 0xff, (ipAddr>>16) & 0xff, (ipAddr>>8) & 0xff, ipAddr & 0xff); 
*/			
			//Update the lastTime to prevent the aging before pDA processed!
			pEntry->lastTime = jiffies; 
			
			return pEntry;
        }
        else
			pEntry = pEntry->pNext;
	}
	
	// We didn't find any matched Mac address.
	return NULL;
	
}


static PUCHAR getInMacByOutMacFromSesMacTb(
	IN PUCHAR outMac,
	IN UINT16 sesID)
{
	UINT16 				hashIdx;
	SesMacMappingEntry *pEntry = NULL;

	if (!SesMacTable.valid)
		return NULL;
	
	// Use hash to find out the location of that entry and get the Mac address.
	hashIdx = sesID % MAT_MAX_HASH_ENTRY_SUPPORT;

	pEntry = SesMacTable.sesHash[hashIdx];
	while(pEntry)
	{
		if ((pEntry->sessionID == sesID) &&  IS_EQUAL_MAC(pEntry->outMacAddr, outMac))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): find it! dstMac=%02x:%02x:%02x:%02x:%02x:%02x\n", 
				__FUNCTION__, pEntry->inMacAddr[0],pEntry->inMacAddr[1],pEntry->inMacAddr[2],
				pEntry->inMacAddr[3],pEntry->inMacAddr[4],pEntry->inMacAddr[5]));

			//Update the lastTime to prevent the aging before pDA processed!
			pEntry->lastTime = jiffies; 

			return pEntry->inMacAddr;
		} 
		else
		{
			pEntry = pEntry->pNext;
		}
	}

	// We didn't find any matched Mac address, just return and didn't do any modification
	return NULL;
}
		

/* This function used to maintain the pppoe convert table which incoming node 
	is a pppoe client and want to connect to use inner pppoe server.
*/
static NDIS_STATUS SesMacTableUpdate(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR inMacAddr,
	IN UINT16 sesID,
	IN PUCHAR outMacAddr)
{
	UINT16 hashIdx;
	SesMacMappingEntry *pEntry, *pPrev, *pNewEntry;

	if (!SesMacTable.valid)
		return FALSE;
	
	hashIdx = sesID % MAT_MAX_HASH_ENTRY_SUPPORT;

/*
	printk("%s():sesID=0x%04x,inMac=%02x%02x:%02x:%02x:%02x:%02x, outMac=%02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, sesID,
			inMacAddr[0],inMacAddr[1],inMacAddr[2],inMacAddr[3],inMacAddr[4],inMacAddr[5],
			outMacAddr[0],outMacAddr[1],outMacAddr[2],outMacAddr[3],outMacAddr[4],outMacAddr[5]);
*/

	pEntry = pPrev = SesMacTable.sesHash[hashIdx];
	while(pEntry)
	{
		// Find a existed IP-MAC Mapping entry
		if ((sesID == pEntry->sessionID) && 
			 IS_EQUAL_MAC(pEntry->inMacAddr, inMacAddr) && 
			 IS_EQUAL_MAC(pEntry->outMacAddr, outMacAddr))
    	{
			// compare is useless. So we directly copy it into the entry.
			pEntry->lastTime = jiffies;
			
	        return TRUE;
		}
        else
        {	// handle the age-out situation
			if ((jiffies - pEntry->lastTime) > PPPoE_SES_ENTRY_AGEOUT_TIME)
        	{
        		// Remove the aged entry
				if (pEntry == SesMacTable.sesHash[hashIdx])
				{
					SesMacTable.sesHash[hashIdx]= pEntry->pNext;
					pPrev = SesMacTable.sesHash[hashIdx];
        		}
				else 
				{	
	        		pPrev->pNext = pEntry->pNext;
				}
				MATDBEntryFree((PUCHAR)pEntry);

#ifdef ETH_CONVERT_SUPPORT
				pAd->EthConvert.nodeCount--;
#endif // ETH_CONVERT_SUPPORT //

				pEntry = (pPrev == NULL ? NULL: pPrev->pNext);
        	} 
			else
			{
				pPrev = pEntry;
	            pEntry = pEntry->pNext;
        	}
        }
	}
#ifdef ETH_CONVERT_SUPPORT
	if (pAd->EthConvert.nodeCount >= ETH_CONVERT_NODE_MAX)
		return FALSE;
#endif // ETH_CONVERT_SUPPORT //

	// Allocate a new IPMacMapping entry and insert into the hash
	pNewEntry = (SesMacMappingEntry *)MATDBEntryAlloc(sizeof(SesMacMappingEntry));
	if (pNewEntry != NULL)
	{	
		pNewEntry->sessionID= sesID;
		NdisMoveMemory(pNewEntry->inMacAddr, inMacAddr, MAC_ADDR_LEN);
		NdisMoveMemory(pNewEntry->outMacAddr, outMacAddr, MAC_ADDR_LEN);
		pNewEntry->pNext = NULL;
		pNewEntry->lastTime = jiffies;

		if (SesMacTable.sesHash[hashIdx] == NULL)
		{	// Hash list is empty, directly assign it.
			SesMacTable.sesHash[hashIdx] = pNewEntry;
		} 
		else 
		{
			// Ok, we insert the new entry into the root of hash[hashIdx]
			pNewEntry->pNext = SesMacTable.sesHash[hashIdx];
			SesMacTable.sesHash[hashIdx] = pNewEntry;
		}
			
		//dumpSesMacTb(hashIdx);
#ifdef ETH_CONVERT_SUPPORT
		pAd->EthConvert.nodeCount++;
#endif // ETH_CONVERT_SUPPORT //

		return TRUE;
	}
	
	return FALSE;
}


/* PPPoE discovery stage Rx handler.
	When Rx, check if the PPPoE tag "Host-uniq" exists or not.
	If exists, we check our database and convert the dstMac to correct one.
 */
static PUCHAR MATProto_PPPoEDis_Rx(
	IN PRTMP_ADAPTER	pAd,
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR			pLayerHdr,
	IN UINT				infIdx)
{
	PUCHAR pData, pSrvMac = NULL, pCliMac= NULL, pOutMac=NULL, pInMac = NULL, pTagContent = NULL, pPayloadLen;
	UINT16 payloadLen, leftLen;
	UINT16 tagID, tagLen =0;
	UINT16 needUpdateSesTb= 0, sesID=0, isPADT = 0;
	UINT16 findTag=0;
	PUidMacMappingEntry pEntry = NULL; 

	pData = pLayerHdr;
	if (*(pData) != 0x11)
		return NULL;
	
	// Check the Code type.
	pData++;
	switch(*pData)
	{
		case PPPOE_CODE_PADO:
			//It's a packet send by a PPPoE server which behind of our device.
			findTag = PPPOE_TAG_ID_HOST_UNIQ;
			break;
		case PPPOE_CODE_PADS:
			needUpdateSesTb = 1;
			findTag = PPPOE_TAG_ID_HOST_UNIQ;
			pCliMac = (PUCHAR)(RTPKT_TO_OSPKT(pSkb)->data);
			pSrvMac = (PUCHAR)(RTPKT_TO_OSPKT(pSkb)->data + 6);
			break;
		case PPPOE_CODE_PADR:
			//It's a packet send by a PPPoE client which in front of our device.
			findTag = PPPOE_TAG_ID_AC_COOKIE;
			break;
		case PPPOE_CODE_PADI:
			//Do nothing! Just forward this packet to upper layer directly.
			return NULL;
		case PPPOE_CODE_PADT:
			isPADT = 1;
			pOutMac= (PUCHAR)(RTPKT_TO_OSPKT(pSkb)->data + 6);
			break;
		default:
		return NULL;
	}

	// Ignore the Code field(length=1)
	pData ++;
	if (needUpdateSesTb || isPADT)
		sesID = OS_NTOHS(*((UINT16 *)pData));

	if (isPADT)
	{
		pInMac = getInMacByOutMacFromSesMacTb(pOutMac, sesID);
		return pInMac;
	}
	// Ignore the session ID field.(length = 2)
	pData += 2;

	// Get the payload length, ignore the payload length field.(length = 2)
	payloadLen = OS_NTOHS(*((UINT16 *)pData));
	pPayloadLen = pData;
	pData += 2; 

#if 0
	printk("%s():needUpdateSesTb=%d, payloadLen=%d, findTag=0x%04x\n", __FUNCTION__, needUpdateSesTb, payloadLen, findTag);
	printk("%s():Dump pppoe payload==>\n", __FUNCTION__);	
	dumpPkt(pData, payloadLen);
#endif

	// First parsing the PPPoE paylod to find out the required tag(e.g., x0103 or 0x0104)
	leftLen = payloadLen;
	while (leftLen)
	{
		tagID = OS_NTOHS(*((UINT16 *)pData));
		tagLen = OS_NTOHS(*((UINT16 *)(pData+2)));
		DBGPRINT(RT_DEBUG_INFO, ("%s():tagID=0x%4x, tagLen= 0x%4x\n", __FUNCTION__, tagID, tagLen));
		if (tagID== findTag && tagLen>0)
		{
			DBGPRINT(RT_DEBUG_INFO, ("%s():Find a PPPoE packet which have required tag(0x%04x)! tagLen=%d\n", __FUNCTION__, findTag, tagLen));
			//shift to the tag value field.
			pTagContent = pData + 4; 
			tagLen = tagLen > PPPOE_DIS_UID_LEN ? PPPOE_DIS_UID_LEN : tagLen;
			break;
		} 
		else
		{
			pData += (tagLen + 4);
			leftLen -= (tagLen + 4);		
		}
	}
	
	DBGPRINT(RT_DEBUG_INFO, ("%s():After parsing, the pTagConent=0x%p, tagLen=%d!\n", __FUNCTION__, pTagContent, tagLen));

	// Now update our pppoe discovery table "UidMacTable"
	if (pTagContent)
	{
		pEntry  = UidMacTableLookUp(pAd, pTagContent, tagLen);
		DBGPRINT(RT_DEBUG_INFO, ("%s():After UidMacTableLookUp(), the pEntry=%p, pTagContent=%p\n", __FUNCTION__, pEntry, pTagContent));

		// Remove the AC-Cookie or host-uniq if we ever add the field for this session.
		if (pEntry)
		{
			if (pEntry->uIDAddByUs)
			{
				PUCHAR tagHead, nextTagHead;
				UINT removedTagLen, tailLen;

				removedTagLen = 4 + tagLen;  	//The total length tag ID/info we want to remove.
				tagHead = pTagContent - 4;	//The start address of the tag we want to remove in sk bufffer 
				tailLen = RTPKT_TO_OSPKT(pSkb)->len - (pTagContent - RTPKT_TO_OSPKT(pSkb)->data) - removedTagLen; //Total left bytes we want to move.
				if (tailLen)
				{
					nextTagHead = pTagContent + tagLen;	//The start address of next tag ID/info in sk buffer.
					memmove(tagHead, nextTagHead, tailLen);
				}
				RTPKT_TO_OSPKT(pSkb)->tail -= removedTagLen;
				RTPKT_TO_OSPKT(pSkb)->len -= removedTagLen;
				*((UINT16 *)pPayloadLen) = OS_HTONS(payloadLen - removedTagLen);
			}

			if (needUpdateSesTb) {
				DBGPRINT(RT_DEBUG_INFO, ("%s(): This's a PADS packet, so we need to create the SesMacTable!\n", __FUNCTION__));
				SesMacTableUpdate(pAd, pEntry->macAddr,sesID, pSrvMac);
			}
			
			return pEntry->macAddr;
		}	
	}
	
	return NULL;
}



/* PPPoE discovery stage Tx handler.
	If the pakcet is PADI/PADR, check if the PPPoE tag "Host-uniq" exists or not. 
		If exists, we just record it in our table, else we insert the Mac address 
		 of Sender as well as the host-uniq, then forward to the destination. It's
		 a one(MAC)-to-one(Host-uniq) mapping in our table.
	If the packet is PADO/PADS, check if the PPPoE tag "AC-Cookie" exists or not.
		If exists, we just record it in our table, else we insert the Mac address
		of Sender as well as the AC-Cookie, then forward to the destination. It may
		one(MAC)-to-many(AC-Cookie) mapping in our table.

    Host-uniq TAG ID= 0x0103
    AC-Cookie TAG ID= 0x0104
 */
static PUCHAR MATProto_PPPoEDis_Tx(
	IN PRTMP_ADAPTER 	pAd,
	IN PNDIS_PACKET 	pSkb,
	IN PUCHAR 			pLayerHdr,
	IN UINT				infIdx)
{
	PUCHAR pData, pTagContent = NULL, pPayloadLen, pPPPPoETail;
	PUCHAR pSrcMac, pDstMac;
	UINT16 payloadLen, leftLen, offset;
	UINT16 tagID, tagLen =0;
	UINT16 isServer = 0, needUpdateSesTb= 0, sesID = 0;
	UINT16 findTag=0;
	PUidMacMappingEntry pEntry = NULL; 
	PUCHAR pPktHdr;
	PNDIS_PACKET pModSkb = NULL;

	pPktHdr = RTPKT_TO_OSPKT(pSkb)->data;
	pDstMac = pPktHdr;
	pSrcMac = (pPktHdr + 6);
	pData = pLayerHdr;


	// Check the pppoe version and Type. It should be 0x11
	if (*(pData) != 0x11)
		return NULL;

	// Check the Code type.
	pData++;
	switch(*pData)
	{
		// Send by pppoe client
		case PPPOE_CODE_PADI:
		case PPPOE_CODE_PADR:
			findTag = PPPOE_TAG_ID_HOST_UNIQ;
			break;
		// Send by pppoe server
		case PPPOE_CODE_PADO:
		case PPPOE_CODE_PADS:
			isServer = 1;
			findTag = PPPOE_TAG_ID_AC_COOKIE;
			if (*pData == PPPOE_CODE_PADS)  // For PADS, we need record the session ID.
				needUpdateSesTb = 1;
			break;
		// Both server and client can send this packet
		case PPPOE_CODE_PADT:
			/* TODO:
				currently we didn't handle PADT packet. We just leave the 
				session entry and make it age-out automatically. Maybe we
				can remove the entry when we receive this packet.
			 */
			return NULL;
		default:
			return NULL;
	}
	
	// 
	// Ignore the Code field(length=1) and if it's a PADS packet, we
	// should hold the session ID and for latter to update our table.
	//
	pData ++;
	if (needUpdateSesTb)
		sesID = OS_NTOHS(*((UINT16 *)pData));

	// Ignore the session ID field.(length = 2)
	pData += 2;

	// Get the payload length, and  shift the payload length field(length = 2) to next field.
	payloadLen = OS_NTOHS(*((UINT16 *)pData));
	pPayloadLen = pData;
	offset = pPayloadLen - RTPKT_TO_OSPKT(pSkb)->data;
	pData += 2; 

#if 0
	printk("%s():isServer=%d, payloadLen=%d, findTag=0x%04x\n", __FUNCTION__, isServer, payloadLen, findTag);
	printk("%s():Dump pppoe payload==>\n", __FUNCTION__);	
	dumpPkt(pData, payloadLen);
#endif

	// First parsing the PPPoE paylod to find out the required tag(e.g., x0103 or 0x0104)
	leftLen = payloadLen;
	while (leftLen)
	{
		tagID = OS_NTOHS(*((UINT16 *)pData));
		tagLen = OS_NTOHS(*((UINT16 *)(pData+2)));
		DBGPRINT(RT_DEBUG_INFO, ("%s():tagID=0x%04x, tagLen= 0x%04x\n", __FUNCTION__, tagID, tagLen));
		if (tagID== findTag && tagLen>0)
		{
			DBGPRINT(RT_DEBUG_INFO, ("%s(): Find a PPPoE packet which have required tag(0x%04x)! tagLen=%d\n", __FUNCTION__, findTag, tagLen));
			// Move the pointer to the tag value field. 4 = 2(TAG ID) + 2(TAG_LEN)
			pTagContent = pData + 4; 
//			tagLen = tagLen > PPPOE_DIS_UID_LEN ? PPPOE_DIS_UID_LEN : tagLen;
			break;
		} 
		else
		{
			pData += (tagLen + 4);
			leftLen -= (tagLen + 4);		
		}
	}
	
	DBGPRINT(RT_DEBUG_INFO, ("%s(): After parsing the pppoe payload, the pTagConent=0x%p, tagLen=%d!\n", __FUNCTION__, pTagContent, tagLen));

	// Now update our pppoe discovery table "UidMacTable"
	pEntry  = UidMacTableUpdate(pAd, pSrcMac, pDstMac, pTagContent, tagLen, isServer);
	DBGPRINT(RT_DEBUG_INFO, ("%s(): after UidMacTableUpdate(), the pEntry=%p, pTagContent=%p\n", __FUNCTION__, pEntry, pTagContent));
	if (pEntry && (pTagContent == NULL))
	{
		PUCHAR tailHead;

		if(skb_cloned(RTPKT_TO_OSPKT(pSkb)))
			pModSkb = (PNDIS_PACKET)skb_copy(RTPKT_TO_OSPKT(pSkb), MEM_ALLOC_FLAG);
		else
			pModSkb = (PNDIS_PACKET)RTPKT_TO_OSPKT(pSkb);

		if(!pModSkb)
			return NULL;
		
		tailHead = skb_put(RTPKT_TO_OSPKT(pModSkb), (PPPOE_DIS_UID_LEN + 4));
		if (tailHead)
		{
			pPayloadLen = RTPKT_TO_OSPKT(pModSkb)->data + offset;
			pPPPPoETail = pPayloadLen + payloadLen;
			if(tailHead > pPPPPoETail)
				tailHead = pPPPPoETail;
				
			if (pEntry->isServer)
			{	//Append the AC-Cookie tag info in the tail of the pppoe packet.
				tailHead[0] = 0x01;
				tailHead[1] = 0x04;
				tailHead[2] = 0x00;
				tailHead[3] = PPPOE_DIS_UID_LEN;
				tailHead += 4;
				NdisMoveMemory(tailHead, pEntry->uIDStr, PPPOE_DIS_UID_LEN);
			} 
			else 
			{	//Append the host-uniq tag info in the tail of the pppoe packet.
				tailHead[0] = 0x01;
				tailHead[1] = 0x03;
				tailHead[2] = 0x00;
				tailHead[3] = PPPOE_DIS_UID_LEN;
				tailHead += 4;
				NdisMoveMemory(tailHead, pEntry->uIDStr, PPPOE_DIS_UID_LEN);
			}
			*(UINT16 *)pPayloadLen = OS_HTONS(payloadLen + 4 + PPPOE_DIS_UID_LEN);
		}
	}

	if (needUpdateSesTb)
		SesMacTableUpdate(pAd, pSrcMac, sesID, pDstMac);
	
	return (PUCHAR)pModSkb;
}

/* PPPoE discovery stage init function */
static NDIS_STATUS MATProto_PPPoEDis_Init(VOID)
{
	NdisZeroMemory(&UidMacTable, sizeof(UidMacTable));
	UidMacTable.valid = TRUE;

	NdisZeroMemory(&SesMacTable, sizeof(SesMacTable));
	SesMacTable.valid = TRUE;
	
	return TRUE;
}


/* PPPoE discovery stage exit function */
static NDIS_STATUS MATProto_PPPoEDis_Exit(VOID)
{
	UidMacTable_RemoveAll();
	SesMacTable_RemoveAll();
	
	return TRUE;
}


/* PPPoE Session stage Rx handler
	When we receive a ppp pakcet, first check if the srcMac is a PPPoE server or not.
		if it's a server, check the session ID of specific PPPoEServeryEntry and find out the
			correct dstMac Address.
		if it's not a server, check the session ID and find out the cor
		
 */
static PUCHAR MATProto_PPPoESes_Rx(
	IN PRTMP_ADAPTER	pAd,
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR			pLayerHdr,
	IN UINT				infIdx)
{
	PUCHAR srcMac, dstMac = NULL, pData;	
	UINT16 sesID;
	
	srcMac = (RTPKT_TO_OSPKT(pSkb)->data + 6);
	pData = pLayerHdr;

	//skip the first two bytes.(version/Type/Code)
	pData += 2;

	//get the session ID
	sesID = OS_NTOHS( *((UINT16 *)pData));

	// Try to find the dstMac from SesMacHash table.
	dstMac = getInMacByOutMacFromSesMacTb(srcMac, sesID);

	return dstMac;
}

// PPPoE Session stage Tx handler
static PUCHAR MATProto_PPPoESes_Tx(
	IN PRTMP_ADAPTER 	pAd,
	IN PNDIS_PACKET 	pSkb,
	IN PUCHAR 			pLayerHdr,
	IN UINT				infIdx)
{

	/*
		For transmit packet, we do nothing.
	 */
	return NULL;
}


/* PPPoE session stage init function */
static NDIS_STATUS MATProto_PPPoESes_Init(VOID)
{	
	return TRUE;
}

/* PPPoE session stage exit function */
static NDIS_STATUS MATProto_PPPoESes_Exit(VOID)
{

	return TRUE;
}

#endif //endif of "APCLI_SUPPORT"
