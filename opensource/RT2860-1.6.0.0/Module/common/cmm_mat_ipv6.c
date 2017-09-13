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
	cmm_mat_ipv6.c

	Abstract:
		MAT convert engine subroutine for ipv6 base protocols, currently now we 
	just handle IPv6/ICMPv6 packets without Authentication/Encryption headers. 

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Shiang      06/03/07      Init version
*/
#ifdef MAT_SUPPORT

#include "rt_config.h"

//#include <asm/checksum.h>
//#include <net/ip6_checksum.h>


static NDIS_STATUS MATProto_IPv6_Init(VOID);
static NDIS_STATUS MATProto_IPv6_Exit(VOID);
static PUCHAR MATProto_IPv6_Rx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);
static PUCHAR MATProto_IPv6_Tx(PRTMP_ADAPTER pAd, PNDIS_PACKET pSkb, PUCHAR pLayerHdr, UINT infIdx);

#define IPV6_ADDR_LEN 16
#define IPV6_HDR_LEN  40
#define RT_UDP_HDR_LEN	8

// IPv6 address definition
#define IPV6_LINK_LOCAL_ADDR_PREFIX		0xFE8
#define IPV6_SITE_LOCAL_ADDR_PREFIX		0xFEC
#define IPV6_LOCAL_ADDR_PREFIX			0xFE8
#define IPV6_MULTICAST_ADDR_PREFIX		0xFF
#define IPV6_LOOPBACK_ADDR				0x1
#define IPV6_UNSPECIFIED_ADDR			0x0

// defined as sequence in IPv6 header
#define IPV6_NEXT_HEADER_HOP_BY_HOP		0x00	// 0
#define IPV6_NEXT_HEADER_DESTINATION	0x3c	// 60
#define IPV6_NEXT_HEADER_ROUTING		0x2b	// 43
#define IPV6_NEXT_HEADER_FRAGMENT		0x2c	// 44
#define IPV6_NEXT_HEADER_AUTHENTICATION	0x33  	// 51
#define IPV6_NEXT_HEADER_ENCAPSULATION	0x32  	// 50, RFC-2406
#define IPV6_NEXT_HEADER_NONE			0x3b	// 59

#define IPV6_NEXT_HEADER_TCP			0x06
#define IPV6_NEXT_HEADER_UDP			0x11
#define IPV6_NEXT_HEADER_ICMPV6			0x3a

// ICMPv6 msg type definition
#define ICMPV6_MSG_TYPE_ROUTER_SOLICITATION			0x85 // 133
#define ROUTER_SOLICITATION_FIXED_LEN				8

#define ICMPV6_MSG_TYPE_ROUTER_ADVERTISEMENT		0x86 // 134
#define ROUTER_ADVERTISEMENT_FIXED_LEN				16

#define ICMPV6_MSG_TYPE_NEIGHBOR_SOLICITATION		0x87 // 135
#define NEIGHBOR_SOLICITATION_FIXED_LEN				24

#define ICMPV6_MSG_TYPE_NEIGHBOR_ADVERTISEMENT		0x88 // 136
#define NEIGHBOR_ADVERTISEMENT_FIXED_LEN			24

#define ICMPV6_MSG_TYPE_REDIRECT					0x89 // 137
#define REDIRECT_FIXED_LEN							40


typedef struct _IPv6MacMappingEntry
{
	UCHAR ipv6Addr[16];	// In network order
	UCHAR macAddr[MAC_ADDR_LEN];
	UINT  lastTime;
	struct _IPv6MacMappingEntry *pNext;
}IPv6MacMappingEntry, *PIPv6MacMappingEntry;


typedef struct _IPv6MacMappingTable
{
	BOOLEAN			valid;
	IPv6MacMappingEntry *hash[MAT_MAX_HASH_ENTRY_SUPPORT+1]; //0~63 for specific station, 64 for broadcast MacAddress
}IPv6MacMappingTable;

static IPv6MacMappingTable IPv6MacTable=
{
	.valid = FALSE,
}; // Used for IPv6, ICMPv6 protocol

struct _MATProtoEntry MATProtoIPv6Handle =
{
	.init = MATProto_IPv6_Init,
	.tx = MATProto_IPv6_Tx,
	.rx = MATProto_IPv6_Rx,
	.exit = MATProto_IPv6_Exit,
};


/* IPv6 Address related structures */
typedef struct rt_ipv6_addr_
{
	union
	{
		UCHAR	ipv6Addr8[16];
		USHORT	ipv6Addr16[8];
		UINT32	ipv6Addr32[4];
	}addr;
#define ipv6_addr			addr.ipv6Addr8
#define ipv6_addr16			addr.ipv6Addr16
#define ipv6_addr32			addr.ipv6Addr32
}RT_IPV6_ADDR, *PRT_IPV6_ADDR;

	
#define PRINT_IPV6_ADDR(ipv6Addr)	\
	OS_NTOHS((ipv6Addr).ipv6_addr16[0]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[1]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[2]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[3]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[4]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[5]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[6]), \
	OS_NTOHS((ipv6Addr).ipv6_addr16[7])


/*IPv6 Header related structures */
typedef struct PACKED _rt_ipv6_hdr_
{
	UINT32 			ver:4,
					trafficClass:8,
        		   	flowLabel:20;
	USHORT 			payload_len;
	UCHAR  			nextHdr;
	UCHAR  			hopLimit;
	RT_IPV6_ADDR  	srcAddr;
	RT_IPV6_ADDR	dstAddr;
}RT_IPV6_HDR, *PRT_IPV6_HDR;


typedef struct PACKED _rt_ipv6_ext_hdr_
{
	UCHAR	nextProto; // Indicate the protocol type of next extension header.
	UCHAR	extHdrLen; // optional field for msg length of this extension header which didn't include the first "nextProto" field.
	UCHAR	octets[1]; // hook to extend header message body.
}RT_IPV6_EXT_HDR, *PRT_IPV6_EXT_HDR;


/* ICMPv6 related structures */
typedef struct PACKED _rt_ipv6_icmpv6_hdr_
{
	UCHAR	type;
	UCHAR	code;
	USHORT	chksum;
	UCHAR	octets[1]; //hook to extend header message body.
}RT_ICMPV6_HDR, *PRT_ICMPV6_HDR;


typedef struct PACKED _rt_icmp6_option_hdr_
{
	UCHAR type;
	UCHAR len;
	UCHAR octet[1];
}RT_ICMPV6_OPTION_HDR, *PRT_ICMPV6_OPTION_HDR;

typedef enum{
// Defined ICMPv6 Option Types.
	TYPE_SRC_LL_ADDR 	= 1,
	TYPE_TGT_LL_ADDR 	= 2,
	TYPE_PREFIX_INFO 			= 3,
	TYPE_REDIRECTED_HDR			= 4,
	TYPE_MTU					= 5,
}ICMPV6_OPTIONS_TYPE_DEF;


UCHAR IPV6_LOOPBACKADDR[] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
UCHAR IPV6_ZEROADDR[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static inline BOOLEAN needUpdateIPv6MacTB(
	UCHAR 			*pMac,
	RT_IPV6_ADDR 	*pIPv6Addr)
{
	ASSERT(pIPv6Addr);
	
	if (isMcastEtherAddr(pMac) || isZeroEtherAddr(pMac))
		return FALSE;
	
	// IPv6 multicast address
	if (IS_MULTICAST_IPV6_ADDR(*pIPv6Addr))
		return FALSE;
	
	// unspecified address
	if(IS_UNSPECIFIED_IPV6_ADDR(*pIPv6Addr))
		return FALSE;

	// loopback address
	if (IS_LOOPBACK_IPV6_ADDR(*pIPv6Addr)) 
		return FALSE;

/*
	DBGPRINT(RT_DEBUG_INFO, ("%s(): Good IPv6 unicast addr=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
				__FUNCTION__, PRINT_IPV6_ADDR(*pIPv6Addr)));
*/	
	return TRUE;
}


/*
	IPv6 Header Format

     0               1               2               3
     0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Version| Traffic Class |           Flow Label                  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |      Payload Length           |  Next Header  |   Hop Limit   |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    +                                                               +
    |                      Source Address                           |
    +                                                               +
    |                                                               |
    +                                                               +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    +                                                               +
    |                   Destination Address                         |
    +                                                               +
    |                                                               |
    +                                                               +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


ICMPv6 Format:
	|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     Type      |     Code      |           Checksum            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Message Body                            |
	+                                                               +
	|                                                               |
                                    ......
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

NDIS_STATUS  dumpIPv6MacTb(int index)
{
	IPv6MacMappingEntry *pHead;
	int startIdx, endIdx;

	if (!IPv6MacTable.valid)
	{
		printk("%s():IPv6MacTable not init yet, so cannot do dump!\n", __FUNCTION__);
		return FALSE;
	}
	
	
	if(index < 0)
	{	// dump all.
		startIdx = 0;
		endIdx = MAT_MAX_HASH_ENTRY_SUPPORT;
	}
	else
	{	// dump specific hash index.
		startIdx = endIdx = index;
	}

	printk("%s(): Now dump IPv6Mac Table!\n", __FUNCTION__);
	for(; startIdx<= endIdx; startIdx++)
	{
		pHead = IPv6MacTable.hash[startIdx];
	while(pHead)
	{
			printk("IPv6Mac[%d]:\n", startIdx);
			printk("\t:IPv6=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x,Mac=%02x:%02x:%02x:%02x:%02x:%02x, jiffies=0x%x, next=%p\n", 
				PRINT_IPV6_ADDR(*((RT_IPV6_ADDR *)(&pHead->ipv6Addr[0]))), pHead->macAddr[0],pHead->macAddr[1],pHead->macAddr[2],
				pHead->macAddr[3],pHead->macAddr[4],pHead->macAddr[5], pHead->lastTime, pHead->pNext);
		pHead = pHead->pNext;
	}
	}
	printk("----EndOfDump!\n");

	return TRUE;
}



static NDIS_STATUS IPv6MacTableUpdate(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			pMacAddr,
	IN PCHAR			pIPv6Addr)
{
	UINT 				hashIdx;
	IPv6MacMappingEntry	*pEntry = NULL, *pPrev = NULL, *pNewEntry =NULL;

	if (!IPv6MacTable.valid)
		return FALSE;
	
	hashIdx = MAT_IPV6_ADDR_HASH_INDEX(pIPv6Addr);
    pEntry = pPrev = IPv6MacTable.hash[hashIdx];
	while(pEntry)
	{
		// Find a existed IP-MAC Mapping entry
		if (NdisEqualMemory(pIPv6Addr, pEntry->ipv6Addr, IPV6_ADDR_LEN))
    	{
			DBGPRINT(RT_DEBUG_INFO, ("Got Mac(%02x:%02x:%02x:%02x:%02x:%02x) of mapped IPv6(%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x)\n",
					pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2], pEntry->macAddr[3], 
					pEntry->macAddr[4],pEntry->macAddr[5], PRINT_IPV6_ADDR(*((RT_IPV6_ADDR *)&pIPv6Addr)))); 

			// comparison is useless. So we directly copy it into the entry.
			NdisMoveMemory(pEntry->macAddr, pMacAddr, 6);
			pEntry->lastTime = jiffies;
			
	        return TRUE;
		}
        else
        {	// handle the aging-out situation
        	if ((jiffies - pEntry->lastTime) > MAT_TB_ENTRY_AGEOUT_TIME)
        	{
        		// Remove the aged entry
        		if (pEntry == IPv6MacTable.hash[hashIdx])
				{
					IPv6MacTable.hash[hashIdx]= pEntry->pNext;
					pPrev = IPv6MacTable.hash[hashIdx];
        		}
				else 
				{
	        		pPrev->pNext = pEntry->pNext;
				}
				MATDBEntryFree((PUCHAR)pEntry);
				
				pEntry = (pPrev == NULL ? NULL: pPrev->pNext);
#ifdef ETH_CONVERT_SUPPORT
				pAd->EthConvert.nodeCount--;
#endif // ETH_CONVERT_SUPPORT //
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

	// Allocate a new IPv6MacMapping entry and insert into the hash
	pNewEntry = (IPv6MacMappingEntry *)MATDBEntryAlloc(sizeof(IPv6MacMappingEntry));
	if (pNewEntry != NULL)
	{		
		NdisMoveMemory(pNewEntry->ipv6Addr, pIPv6Addr, IPV6_ADDR_LEN);
		NdisMoveMemory(pNewEntry->macAddr, pMacAddr, 6);
		pNewEntry->pNext = NULL;
		pNewEntry->lastTime = jiffies;

		if (IPv6MacTable.hash[hashIdx] == NULL)
		{	// Hash list is empty, directly assign it.
			IPv6MacTable.hash[hashIdx] = pNewEntry;
		} 
		else 
		{
			// Ok, we insert the new entry into the root of hash[hashIdx]
			pNewEntry->pNext = IPv6MacTable.hash[hashIdx];
			IPv6MacTable.hash[hashIdx] = pNewEntry;
		}
		//dumpIPv6MacTb(hashIdx); //for debug
		
#ifdef ETH_CONVERT_SUPPORT
		pAd->EthConvert.nodeCount++;
#endif // ETH_CONVERT_SUPPORT //

		return TRUE;
	}

	DBGPRINT(RT_DEBUG_ERROR, ("IPv6MacTableUpdate():Insertion failed!\n"));
	
	return FALSE;
}


static PUCHAR IPv6MacTableLookUp(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pIPv6Addr)
{
    UINT 				hashIdx;
    IPv6MacMappingEntry	*pEntry = NULL;

    if (!IPv6MacTable.valid)
        return NULL;
	
	// Use hash to find out the location of that entry and get the Mac address.
	hashIdx = MAT_IPV6_ADDR_HASH_INDEX(pIPv6Addr);

//	spin_lock_irqsave(&IPMacTabLock, irqFlag);
    pEntry = IPv6MacTable.hash[hashIdx];
	while(pEntry)
	{
		if (NdisEqualMemory(pEntry->ipv6Addr, pIPv6Addr, IPV6_ADDR_LEN))
        {
			DBGPRINT(RT_DEBUG_ERROR, ("%s(): dstMac=%02x:%02x:%02x:%02x:%02x:%02x for mapped dstIPv6(%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x)\n", 
					__FUNCTION__, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2], pEntry->macAddr[3], 
					pEntry->macAddr[4],pEntry->macAddr[5],PRINT_IPV6_ADDR(*((RT_IPV6_ADDR *)&pIPv6Addr)))); 
			
			//Update the lastTime to prevent the aging before pDA processed!
			pEntry->lastTime = jiffies; 
			
			return pEntry->macAddr;
        }
        else
            pEntry = pEntry->pNext;
	}
	
	// We didn't find any matched Mac address, our policy is treat it as 
	// broadcast packet and send to all.
	return IPv6MacTable.hash[IPV6MAC_TB_HASH_INDEX_OF_BCAST]->macAddr;
	
}


static inline unsigned short int icmpv6_csum(
	RT_IPV6_ADDR *saddr,
	RT_IPV6_ADDR *daddr,
	USHORT		  len,
	UCHAR 		  proto,
	UCHAR 		 *pICMPMsg)
{
	int 	carry;
	UINT32 	ulen;
	UINT32 	uproto;
	int i;
	unsigned int csum = 0;
	unsigned short int chksum;

	if (len % 4)
		return 0;
	
	for( i = 0; i < 4; i++)
	{
		csum += saddr->ipv6_addr32[i];
		carry = (csum < saddr->ipv6_addr32[i]);
		csum += carry;
	}

	for( i = 0; i < 4; i++)
	{
		csum += daddr->ipv6_addr32[i];
		carry = (csum < daddr->ipv6_addr32[i]);
		csum += carry;
	}

	ulen = OS_HTONL((UINT32)len);
	csum += ulen;
	carry = (csum < ulen);
	csum += carry;

	uproto = OS_HTONL((UINT32)proto);
	csum += uproto;
	carry = (csum < uproto);
	csum += carry;
	
	for (i = 0; i < len; i += 4)
	{
		csum += *((UINT32 *)(&pICMPMsg[i]));
		carry = (csum < (*((UINT32 *)(&pICMPMsg[i]))));
		csum += carry;
	}

	while (csum>>16)
		csum = (csum & 0xffff) + (csum >> 16);

	chksum = ~csum;
	
	return chksum;
}



static PUCHAR MATProto_IPv6_Rx(
	IN PRTMP_ADAPTER	pAd, 
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR 			pLayerHdr,
	IN UINT				infIdx)
{

	PUCHAR pMacAddr;
	PUCHAR pDstIPv6Addr;
	
	// Fetch the IPv6 addres from the packet header.
	pDstIPv6Addr = (UCHAR *)(&((RT_IPV6_HDR *)pLayerHdr)->dstAddr);
	DBGPRINT(RT_DEBUG_INFO, ("%s(): Get the dstIP=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n", 
						__FUNCTION__, PRINT_IPV6_ADDR(((RT_IPV6_HDR *)pLayerHdr)->dstAddr)));
	
	pMacAddr = IPv6MacTableLookUp(pAd, pDstIPv6Addr);
	
	return pMacAddr;

}

	
static BOOLEAN IPv6ExtHdrHandle(
	RT_IPV6_EXT_HDR 	*pExtHdr,
	UCHAR 				*pProto,
	UINT32 				*pOffset)
{
	UCHAR nextProto = 0xff;
	UINT32 extLen = 0;
	BOOLEAN status = TRUE;

	//printk("%s(): parsing the Extension Header with Protocol(0x%x):\n", __FUNCTION__, *pProto);
	switch (*pProto)
	{
		case IPV6_NEXT_HEADER_HOP_BY_HOP:
			// IPv6ExtHopByHopHandle();
			nextProto = pExtHdr->nextProto;
			extLen = (pExtHdr->extHdrLen + 1);
			break;
			
		case IPV6_NEXT_HEADER_DESTINATION:
			// IPv6ExtDestHandle();
			nextProto = pExtHdr->nextProto;
			extLen = (pExtHdr->extHdrLen + 1);
			break;
			
		case IPV6_NEXT_HEADER_ROUTING:
			// IPv6ExtRoutingHandle();
			nextProto = pExtHdr->nextProto;
			extLen = (pExtHdr->extHdrLen + 1);
			break;
			
		case IPV6_NEXT_HEADER_FRAGMENT:
			// IPv6ExtFragmentHandle();
			nextProto = pExtHdr->nextProto;
			extLen = 8; // The Fragment header length is fixed to 8 bytes.
			break;
			
		case IPV6_NEXT_HEADER_AUTHENTICATION:
		//   IPV6_NEXT_HEADER_ENCAPSULATION:
			/*
				TODO: Not support. For encryption issue.
			*/
			nextProto = 0xFF;
			status = FALSE;
			break;

		default:
			nextProto = 0xFF;
			status = FALSE;			
			break;
	}

	*pProto = nextProto;
	*pOffset += extLen;
	//printk("%s(): nextProto = 0x%x!, offset=0x%x!\n", __FUNCTION__, nextProto, offset);
	
	return status;
	
}


static PNDIS_PACKET ICMPv6_Handle_Tx(
	IN PRTMP_ADAPTER 	pAd,
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR			pLayerHdr,
	IN UINT 			infIdx,
	IN UINT32			offset)
{
	RT_IPV6_HDR 			*pIPv6Hdr;
	RT_ICMPV6_HDR 			*pICMPv6Hdr;
	RT_ICMPV6_OPTION_HDR 	*pOptHdr;

	USHORT payloadLen;
	UINT32 ICMPOffset = 0, ICMPMsgLen = 0, leftLen;
	
	PNDIS_PACKET newSkb = NULL;
	BOOLEAN needModify = FALSE;
	PUCHAR pSrcMac;

	pIPv6Hdr = (RT_IPV6_HDR *)pLayerHdr;
	payloadLen = OS_NTOHS(pIPv6Hdr->payload_len);
	
	pICMPv6Hdr = (RT_ICMPV6_HDR *)(pLayerHdr + offset);
	ICMPOffset = offset;
	ICMPMsgLen = payloadLen + IPV6_HDR_LEN - ICMPOffset;
	
	DBGPRINT(RT_DEBUG_INFO, ("A ICMPv6 packet! offset=%d! MsgType=0x%x! ICMPMsgLEn=%d!\n", 
							offset, pICMPv6Hdr->type, ICMPMsgLen));

	leftLen = ICMPMsgLen;
	switch (pICMPv6Hdr->type)
	{
		case ICMPV6_MSG_TYPE_ROUTER_SOLICITATION:
			offset += ROUTER_SOLICITATION_FIXED_LEN;
			// for unspecified source address, it should not include the option about link-layer address.
			if (!(IS_UNSPECIFIED_IPV6_ADDR(pIPv6Hdr->srcAddr)))
			{
				while(leftLen > sizeof(RT_ICMPV6_OPTION_HDR))
				{
					pOptHdr = (RT_ICMPV6_OPTION_HDR *)(pLayerHdr + offset);					
					if (pOptHdr->len == 0)
						break;  // discard it, because it's invalid.
					
					if (pOptHdr->type == TYPE_SRC_LL_ADDR)
					{
						//replace the src link-layer address as ours.
						needModify = TRUE;
						offset += 2;	// 2 = "type, len" fields. Here indicate to the place of src mac.
						DBGPRINT(RT_DEBUG_INFO,("SRC_LL_ADDR, modify=TRUE, offset=%d!\n", offset));
						break;
					} else {
						offset += (pOptHdr->len * 8);  // in unit of 8 octets.
						leftLen -= (pOptHdr->len * 8);
					}
				}
			}
			break;

		case ICMPV6_MSG_TYPE_ROUTER_ADVERTISEMENT:
			offset += ROUTER_ADVERTISEMENT_FIXED_LEN;
			// for unspecified source address, it should not include the option about link-layer address.
			if (!(IS_UNSPECIFIED_IPV6_ADDR(pIPv6Hdr->srcAddr)))
			{
				while(leftLen > sizeof(RT_ICMPV6_OPTION_HDR))
				{
					pOptHdr = (RT_ICMPV6_OPTION_HDR *)(pLayerHdr + offset);
					if (pOptHdr->len == 0)
						break;  // discard it, because it's invalid.
						
					if (pOptHdr->type == TYPE_SRC_LL_ADDR)
					{
						//replace the src link-layer address as ours.
						needModify = TRUE;
						offset += 2;	// 2 = "type, len" fields. Here indicate to the place of src mac.
						DBGPRINT(RT_DEBUG_INFO,("SRC_LL_ADDR, modify=TRUE, offset=%d!\n", offset));
						break;
					} else {
						offset += (pOptHdr->len * 8);  // in unit of 8 octets.
						leftLen -= (pOptHdr->len * 8);
					}
				}
			}
			break;
			
		case ICMPV6_MSG_TYPE_NEIGHBOR_SOLICITATION:
			offset += NEIGHBOR_SOLICITATION_FIXED_LEN;
			// for unspecified source address, it should not include the option about link-layer address.
			if (!(IS_UNSPECIFIED_IPV6_ADDR(pIPv6Hdr->srcAddr)))
			{
				while(leftLen > sizeof(RT_ICMPV6_OPTION_HDR))
				{
					pOptHdr = (RT_ICMPV6_OPTION_HDR *)(pLayerHdr + offset);
					if (pOptHdr->len == 0)
						break;  // discard it, because it's invalid.
						
					if (pOptHdr->type == TYPE_SRC_LL_ADDR)
					{
						//replace the src link-layer address as ours.
						needModify = TRUE;
						offset += 2;	// 2 = "type, len" fields. Here indicate to the place of src mac.
						DBGPRINT(RT_DEBUG_INFO,("SRC_LL_ADDR, modify=TRUE, offset=%d!\n", offset));
						break;
					} else {
						offset += (pOptHdr->len * 8);  // in unit of 8 octets.
						leftLen -= (pOptHdr->len * 8);
					}
				}
			}
			break;
			
		case ICMPV6_MSG_TYPE_NEIGHBOR_ADVERTISEMENT:
			offset += NEIGHBOR_ADVERTISEMENT_FIXED_LEN;
			// for unspecified source address, it should not include the option about link-layer address.
			if (!(IS_UNSPECIFIED_IPV6_ADDR(pIPv6Hdr->srcAddr)))
			{
				while(leftLen > sizeof(RT_ICMPV6_OPTION_HDR))
				{
					pOptHdr = (RT_ICMPV6_OPTION_HDR *)(pLayerHdr + offset);
					if (pOptHdr->len == 0)
						break;  // discard it, because it's invalid.
						
					if (pOptHdr->type == TYPE_TGT_LL_ADDR)
					{
						//replace the src link-layer address as ours.
						needModify = TRUE;
						offset += 2;	// 2 = "type, len" fields.
						DBGPRINT(RT_DEBUG_INFO,("TTGT_LL_ADDR, modify=TRUE, offset=%d!\n", offset));
						break;
					}else {
						offset += (pOptHdr->len * 8);  // in unit of 8 octets.
						leftLen -= (pOptHdr->len * 8);
					}
				}
			}
			break;
		case ICMPV6_MSG_TYPE_REDIRECT:
			offset += REDIRECT_FIXED_LEN;
			// for unspecified source address, it should not include the options about link-layer address.
			if (!(IS_UNSPECIFIED_IPV6_ADDR(pIPv6Hdr->srcAddr)))
			{
				while(leftLen > sizeof(RT_ICMPV6_OPTION_HDR))
				{
					pOptHdr = (RT_ICMPV6_OPTION_HDR *)(pLayerHdr + offset);
					if (pOptHdr->len == 0)
						break;  // discard it, because it's invalid.

					if (pOptHdr->type == TYPE_TGT_LL_ADDR)
					{
						// TODO: Need to check if the TGT_LL_ADDR is the inner MAC.
						//replace the src link-layer address as ours.
						needModify = TRUE;
						offset += 2;	// 2 = "type, len" fields.
						DBGPRINT(RT_DEBUG_INFO,("TTGT_LL_ADDR, modify=TRUE, offset=%d!\n", offset));
						break;
					}else {
						offset += (pOptHdr->len * 8);  // in unit of 8 octets.
						leftLen -= (pOptHdr->len * 8);
					}
				}
			}
			break;
			
		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Un-supported ICMPv6 msg type(0x%x)! Ignore it\n", pICMPv6Hdr->type));
			break;
	}
	
	// We need to handle about the solicitation/Advertisement packets.
	if (needModify)
	{	
		if(skb_cloned(RTPKT_TO_OSPKT(pSkb))) 
		{
			newSkb = (PNDIS_PACKET)skb_copy(RTPKT_TO_OSPKT(pSkb), GFP_ATOMIC);
			if(newSkb) {
				if (IS_VLAN_PACKET(RTPKT_TO_OSPKT(newSkb)->data))
					pIPv6Hdr = (RT_IPV6_HDR *)(RTPKT_TO_OSPKT(newSkb)->data + MAT_VLAN_ETH_HDR_LEN); 
				else
					pIPv6Hdr = (RT_IPV6_HDR *)(RTPKT_TO_OSPKT(newSkb)->data + MAT_ETHER_HDR_LEN);
			}
		}

		pICMPv6Hdr = (RT_ICMPV6_HDR *)((PUCHAR)pIPv6Hdr + ICMPOffset);
		pSrcMac = (PUCHAR)((PUCHAR)pIPv6Hdr + offset);
		
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
		NdisMoveMemory(pSrcMac, pAd->ApCfg.ApCliTab[infIdx].CurrentAddress, MAC_ADDR_LEN);
#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

		
		// Now re-calculate the Checksum.
		pICMPv6Hdr->chksum = 0;
		pICMPv6Hdr->chksum = icmpv6_csum(&pIPv6Hdr->srcAddr, &pIPv6Hdr->dstAddr, ICMPMsgLen , 
											IPV6_NEXT_HEADER_ICMPV6, (PUCHAR)pICMPv6Hdr);
	}

	return newSkb;
	
}


static PUCHAR MATProto_IPv6_Tx(
	IN PRTMP_ADAPTER 	pAd,
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR 			pLayerHdr,
	IN UINT 			infIdx)
{
	PUCHAR pSrcMac, pSrcIP;
	BOOLEAN needUpdate;
	UCHAR nextProtocol;
	UINT32 offset;	
	HEADER_802_3 *pEthHdr;
	RT_IPV6_HDR *pIPv6Hdr;
	PNDIS_PACKET newSkb = NULL;
	
	pIPv6Hdr = (RT_IPV6_HDR *)pLayerHdr;
	pEthHdr = (HEADER_802_3 *)(RTPKT_TO_OSPKT(pSkb)->data);
	
	pSrcMac = (UCHAR *)&pEthHdr->SAAddr2;
	pSrcIP = (UCHAR *)&pIPv6Hdr->srcAddr;

	
	DBGPRINT(RT_DEBUG_INFO, ("%s(): Transmit a IPv6 Pkt=>SrcIP=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x!\n", 
				__FUNCTION__, PRINT_IPV6_ADDR(pIPv6Hdr->srcAddr)));
	
	needUpdate = needUpdateIPv6MacTB(pSrcMac, (RT_IPV6_ADDR *)(&pIPv6Hdr->srcAddr));
	if (needUpdate)
		IPv6MacTableUpdate(pAd, pSrcMac, (UCHAR *)(&pIPv6Hdr->srcAddr));


	// We need to traverse the whole IPv6 Header and extend headers to check about the ICMPv6 pacekt.
	
	nextProtocol = pIPv6Hdr->nextHdr;
	offset = IPV6_HDR_LEN;
	//DBGPRINT(RT_DEBUG_INFO, ("NextProtocol=0x%x! payloadLen=%d! offset=%d!\n", nextProtocol, payloadLen, offset));
	while(nextProtocol != IPV6_NEXT_HEADER_ICMPV6 && 
		  nextProtocol != IPV6_NEXT_HEADER_UDP && 
		  nextProtocol != IPV6_NEXT_HEADER_TCP && 
		  nextProtocol != IPV6_NEXT_HEADER_NONE)
	{
		if(IPv6ExtHdrHandle((RT_IPV6_EXT_HDR *)(pLayerHdr + offset), &nextProtocol, &offset) == FALSE)
		{
			DBGPRINT(RT_DEBUG_TRACE,("IPv6ExtHdrHandle failed!\n"));
			break;
		}
	}

	switch (nextProtocol)
	{
		case IPV6_NEXT_HEADER_ICMPV6:
			newSkb = ICMPv6_Handle_Tx(pAd, pSkb, pLayerHdr, infIdx, offset);
			break;
			
		case IPV6_NEXT_HEADER_UDP:
			//newSkb = DHCPv6_Handle_Tx(pAd, pSkb, pLayerHdr, infIdx, offset);
			break;
			
		case IPV6_NEXT_HEADER_TCP:
		case IPV6_NEXT_HEADER_NONE:
		default:
			break;
	}

	return (PUCHAR)newSkb;

}



static NDIS_STATUS IPv6MacTable_RemoveAll(VOID)
{
	IPv6MacMappingEntry *pEntry;
	UINT32				i;

	if (!IPv6MacTable.valid)
		return TRUE;
	
	for (i=0; i<IPV6MAC_TB_HASH_ENTRY_NUM; i++)
	{
		while((pEntry = IPv6MacTable.hash[i]) != NULL)
		{
			IPv6MacTable.hash[i] = pEntry->pNext;
			MATDBEntryFree((PUCHAR)pEntry);
		}
	}

	IPv6MacTable.valid = FALSE;

	return TRUE;
}


static NDIS_STATUS IPv6MacTable_init(VOID)
{
	IPv6MacMappingEntry *pEntry = NULL;
	
	NdisZeroMemory(&IPv6MacTable, sizeof(IPv6MacTable));
	
	//Set the last hash entry (hash[64]) as our default broadcast Mac address
	pEntry = (IPv6MacMappingEntry *)MATDBEntryAlloc(sizeof(IPv6MacMappingEntry));
	if (!pEntry)
		return FALSE;
	
	NdisZeroMemory(&pEntry->ipv6Addr[0], IPV6_ADDR_LEN);
	NdisMoveMemory(pEntry->macAddr, BROADCAST_ADDR, MAC_ADDR_LEN);
	pEntry->pNext = NULL;
	IPv6MacTable.hash[IPV6MAC_TB_HASH_INDEX_OF_BCAST] = pEntry;
	
	IPv6MacTable.valid = TRUE;

	DBGPRINT(RT_DEBUG_TRACE, ("%s(): IPv6MacTable_init success!\n", __FUNCTION__));
	
	return TRUE;
	
}


static NDIS_STATUS MATProto_IPv6_Exit(VOID)
{
	INT status;
		
	status = IPv6MacTable_RemoveAll();

	return status;
}


static NDIS_STATUS MATProto_IPv6_Init(VOID)
{

	BOOLEAN status = FALSE;

	status = (IPv6MacTable.valid==TRUE) ? TRUE : IPv6MacTable_init();

	if (status)
		MATProtoIPv6Handle.pMgtTb =&IPv6MacTable;
	else 
		MATProtoIPv6Handle.pMgtTb = NULL;
	
	return status;
}


#if 0 // Temporary not used, following the RFC, currently we don't need to do any modification about DHCPv6.

/* DHCPv6 related structures */
typedef struct PACKED _rt_dhcpv6_option_hdr_
{
	USHORT 	opcode;
	USHORT 	oplen;
	UCHAR 	options[1];	// hook point for options.
}RT_DHCPV6_OPTION_HDR, *PRT_DHCPV6_OPTION_HDR;


typedef struct PACKED _rt_dhcpv6_hdr_
{
	UCHAR msgType;
	UCHAR trans_id[3];
	UCHAR options[1];	// hook point for options.

}RT_DHCPV6_HDR, *PRT_DHCPV6_HDR;

typedef enum{
// Defined DHCPv6 Message Type.
	DHCPV6_MSG_SOLICIT = 1,
	DHCPV6_MSG_ADVERTISE = 2,
	DHCPV6_MSG_REQUEST = 3,
	DHCPV6_MSG_CONFIRM = 4,
	DHCPV6_MSG_RENEW = 5,
	DHCPV6_MSG_REBIND = 6,
	DHCPV6_MSG_REPLY = 7,
	DHCPV6_MSG_RELEASE = 8,
	DHCPV6_MSG_DECLINE = 9,
	DHCPV6_MSG_RECONFIGURE = 10,
	DHCPV6_MSG_INFO_REQUEST = 11,
	DHCPV6_MSG_RELAY_FORW = 12,
	DHCPV6_MSG_RELAY_REPL = 13,
}DHCPV6_MSG_TYPES;



static inline int ICMPv6OptionHandle(VOID)
{
	return 0;
}


static PNDIS_PACKET DHCPv6_Handle_Tx(
	IN PRTMP_ADAPTER 	pAd,
	IN PNDIS_PACKET		pSkb,
	IN PUCHAR			pLayerHdr,
	IN UINT 			infIdx,
	IN UINT32			offset)
{
	RT_IPV6_HDR 			*pIPv6Hdr;
	RT_DHCPV6_HDR 			*pDHCPv6Hdr;
	PUCHAR udpHdr;
	PNDIS_PACKET newSkb = NULL;
	UINT16 srcPort, dstPort;
	PUCHAR bootpHdr;
	UINT16 bootpFlag;
	UINT16 srcPort, dstPort;

	udpHdr = (PUCHAR)(pLayerHdr + offset);
	srcPort = OS_NTOHS(*((UINT16 *)(udpHdr)));
	dstPort = OS_NTOHS(*((UINT16 *)(udpHdr+2)));

	// Now we just need to handle DHCPv6
	if ((srcPort==546 && dstPort==547) || 
		(srcPort==547 && dstPort==546))
	{
		pDHCPv6Hdr = (RT_DHCPV6_HDR *)(udpHdr + RT_UDP_HDR_LEN);

		printk("DHCPv6 MsgType=0x%x!\n", pDHCPv6Hdr->msgType);
		switch (pDHCPv6Hdr->msgType)
		{
			case DHCPV6_MSG_SOLICIT:
			case DHCPV6_MSG_ADVERTISE:
			case DHCPV6_MSG_REQUEST:
			case DHCPV6_MSG_CONFIRM:
			case DHCPV6_MSG_RENEW:
			case DHCPV6_MSG_REBIND:
			case DHCPV6_MSG_REPLY:
			case DHCPV6_MSG_RELEASE:
			case DHCPV6_MSG_DECLINE:
			case DHCPV6_MSG_RECONFIGURE:
			case DHCPV6_MSG_INFO_REQUEST:
			case DHCPV6_MSG_RELAY_FORW:
			case DHCPV6_MSG_RELAY_REPL:
				break;
		}		
	}
	return newSkb;

}
#endif

VOID getIPv6MacTbInfo(char *pOutBuf)
{
	IPv6MacMappingEntry *pHead;
	int startIdx, endIdx;
    char Ipstr[40] = {0};

	if (!IPv6MacTable.valid)
	{
        DBGPRINT(RT_DEBUG_TRACE, ("%s():IPv6MacTable not init yet!\n", __FUNCTION__));
		return;
	}
	
	
	// dump all.
	startIdx = 0;
	endIdx = MAT_MAX_HASH_ENTRY_SUPPORT;

	sprintf(pOutBuf, "\n");
    sprintf(pOutBuf+strlen(pOutBuf), "%-40s%-20s\n", "IP", "MAC");
	for(; startIdx< endIdx; startIdx++)
	{
		pHead = IPv6MacTable.hash[startIdx];

        while(pHead)
    	{
    	    if (strlen(pOutBuf) > (IW_PRIV_SIZE_MASK - 30))
                break;
            NdisZeroMemory(Ipstr, 40);
            sprintf(Ipstr, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", PRINT_IPV6_ADDR(*((RT_IPV6_ADDR *)(&pHead->ipv6Addr[0]))));
			sprintf(pOutBuf+strlen(pOutBuf), "%-40s%02x:%02x:%02x:%02x:%02x:%02x\n",
				Ipstr, pHead->macAddr[0],pHead->macAddr[1],pHead->macAddr[2],
				pHead->macAddr[3],pHead->macAddr[4],pHead->macAddr[5]);
    		pHead = pHead->pNext;
    	}
	}
}

#endif // MAT_SUPPORT //

