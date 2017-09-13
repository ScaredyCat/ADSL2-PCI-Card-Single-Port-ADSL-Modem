

#include "rt_config.h"
#include "igmp_snoop.h"


NDIS_SPIN_LOCK MulticastFilterTabLock;
static MEMBER_ENTRY freeMemberPool[FREE_MEMBER_POOL_SIZE];
static LIST_HEADER freeEntryList;

static inline void initFreeEntryList(
	IN PLIST_HEADER pList)
{
	int i;

	for (i = 0; i < FREE_MEMBER_POOL_SIZE; i++)
		insertTailList(pList, (PLIST_ENTRY)&freeMemberPool[i]);
	return;
}


static VOID IGMPTableDisplay(
	IN PRTMP_ADAPTER pAd);

static BOOLEAN isIgmpMacAddr(
	IN PUCHAR pMacAddr);

static VOID InsertIgmpMember(
	IN PLIST_HEADER pList,
	IN USHORT Aid);

static VOID DeleteIgmpMember(
	IN PLIST_HEADER pList,
	IN USHORT Aid);

static VOID DeleteIgmpMemberList(
	IN PLIST_HEADER pList);

extern INT RTMPGetKeyParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer);

/*
    ==========================================================================
    Description:
        This routine init the entire IGMP table.
    ==========================================================================
 */
VOID MulticastFilterTableInit(
	IN PMULTICAST_FILTER_TABLE *ppMulticastFilterTable)
{
	// Initialize MAC table and allocate spin lock
	*ppMulticastFilterTable = kmalloc(sizeof(MULTICAST_FILTER_TABLE), MEM_ALLOC_FLAG);
	if (*ppMulticastFilterTable == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s unable to alloc memory for Multicase filter table, size=%d\n",
			__FUNCTION__, sizeof(MULTICAST_FILTER_TABLE)));
		return;
	}

	NdisZeroMemory(*ppMulticastFilterTable, sizeof(MULTICAST_FILTER_TABLE));
	initList(&freeEntryList);
	initFreeEntryList(&freeEntryList);
	NdisAllocateSpinLock(&MulticastFilterTabLock);
	return;
}

/*
    ==========================================================================
    Description:
        This routine reset the entire IGMP table.
    ==========================================================================
 */
VOID MultiCastFilterTableReset(
	IN PMULTICAST_FILTER_TABLE *ppMulticastFilterTable)
{
	if(*ppMulticastFilterTable == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Multicase filter table is not ready.\n", __FUNCTION__));
		return;
	}

	RTMP_SEM_LOCK(&MulticastFilterTabLock);

	kfree(*ppMulticastFilterTable);
	*ppMulticastFilterTable = NULL;

	RTMP_SEM_UNLOCK(&MulticastFilterTabLock);
}

/*
    ==========================================================================
    Description:
        Display all entrys in IGMP table
    ==========================================================================
 */
static VOID IGMPTableDisplay(
	IN PRTMP_ADAPTER pAd)
{
	int i;
	MULTICAST_FILTER_TABLE_ENTRY *pEntry = NULL;
	PMULTICAST_FILTER_TABLE pMulticastFilterTable = pAd->pMulticastFilterTable;

	if (pMulticastFilterTable == NULL)
	{
		printk("%s Multicase filter table is not ready.\n", __FUNCTION__);
		return;
	}

	// if FULL, return
	if (pMulticastFilterTable->Size == 0)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Table empty.\n"));
		return;
	}

	// allocate one MAC entry
	RTMP_SEM_LOCK(&MulticastFilterTabLock);

	for (i = 0; i< MAX_LEN_OF_MULTICAST_FILTER_TABLE; i++)
	{
		// pick up the first available vacancy
		if (pMulticastFilterTable->Content[i].Valid == TRUE)
		{
			PMEMBER_ENTRY pMemberEntry = NULL;
			pEntry = &pMulticastFilterTable->Content[i];

			printk("IF(%s) entry #%d, type=%s, GrpId=(%02x:%02x:%02x:%02x:%02x:%02x) memberCnt=%d\n",
				pEntry->net_dev->name, i, (pEntry->type==0 ? "static":"dynamic"),
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5],
				IgmpMemberCnt(&pEntry->MemberList));

			pMemberEntry = (PMEMBER_ENTRY)pEntry->MemberList.pHead;
			while (pMemberEntry)
			{
				PMAC_TABLE_ENTRY pMacEntry = &pAd->MacTab.Content[pMemberEntry->Aid];
				printk("member mac=(%02x:%02x:%02x:%02x:%02x:%02x)\n",
					pMacEntry->Addr[0], pMacEntry->Addr[1], pMacEntry->Addr[2],
					pMacEntry->Addr[3], pMacEntry->Addr[4], pMacEntry->Addr[5]);

				pMemberEntry = pMemberEntry->pNext;
			}
		}
	}

	RTMP_SEM_UNLOCK(&MulticastFilterTabLock);

	return;
}

/*
    ==========================================================================
    Description:
        Add and new entry into MAC table
    ==========================================================================
 */
BOOLEAN MulticastFilterTableInsertEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pGrpId,
	IN PUCHAR pMemberAddr,
	IN PNET_DEV dev,
	IN MulticastFilterEntryType type)
{
	UCHAR HashIdx;
	int i;
	MULTICAST_FILTER_TABLE_ENTRY *pEntry = NULL, *pCurrEntry, *pPrevEntry;
	PMEMBER_ENTRY pMemberEntry;
	PMULTICAST_FILTER_TABLE pMulticastFilterTable = pAd->pMulticastFilterTable;
	USHORT Aid = MCAST_WCID;
	SST	Sst = SST_ASSOC;
	UCHAR PsMode = PWR_ACTIVE, Rate;
	PMAC_TABLE_ENTRY pMacEntry;


	if (pMulticastFilterTable == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Multicase filter table is not ready.\n", __FUNCTION__));
		return FALSE;
	}

	// if FULL, return
	if (pMulticastFilterTable->Size >= MAX_LEN_OF_MULTICAST_FILTER_TABLE)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Multicase filter table full. max-entries = %d\n",
			__FUNCTION__, MAX_LEN_OF_MULTICAST_FILTER_TABLE));
		return FALSE;
	}

	// check the rule is in table already or not.
	if ((pEntry = MulticastFilterTableLookup(pMulticastFilterTable, pGrpId, dev)))
	{
		// doesn't indicate member mac address.
		if(pMemberAddr == NULL)
		{
			return FALSE;
		}

		pMemberEntry = (PMEMBER_ENTRY)pEntry->MemberList.pHead;
		pMacEntry = APSsPsInquiry(pAd, pMemberAddr, &Sst, &Aid, &PsMode, &Rate);
		while (pMemberEntry)
		{
			if (pMacEntry && (Aid == pMemberEntry->Aid))
			{
				DBGPRINT(RT_DEBUG_ERROR, ("%s: already in Members list.\n", __FUNCTION__));
				return FALSE;
			}

			pMemberEntry = pMemberEntry->pNext;
		}
	}

	RTMP_SEM_LOCK(&MulticastFilterTabLock);

	do
	{
		// the multicast entry already exist but doesn't include the member yet.
		if (pEntry != NULL && Aid != MCAST_WCID)
		{
			InsertIgmpMember(&pEntry->MemberList, Aid);
			break;
		}

		// allocate one MAC entry
		for (i = 0; i < MAX_LEN_OF_MULTICAST_FILTER_TABLE; i++)
		{
			// pick up the first available vacancy
			pEntry = &pMulticastFilterTable->Content[i];
			if ((pEntry->Valid == TRUE) && (pEntry->type == MCAT_FILTER_DYNAMIC)
				&& ((jiffies - pEntry->lastTime) > IGMPMAC_TB_ENTRY_AGEOUT_TIME))
			{
				PMULTICAST_FILTER_TABLE_ENTRY pHashEntry;

				HashIdx = MULTICAST_ADDR_HASH_INDEX(pEntry->Addr);
				pHashEntry = pMulticastFilterTable->Hash[HashIdx];

				if ((pEntry->net_dev == pHashEntry->net_dev)
					&& MAC_ADDR_EQUAL(pEntry->Addr, pHashEntry->Addr))
				{
					pMulticastFilterTable->Hash[HashIdx] = pHashEntry->pNext;
					pMulticastFilterTable->Size --;
					DBGPRINT(RT_DEBUG_TRACE, ("MCastFilterTableDeleteEntry 1 - Total= %d\n", pMulticastFilterTable->Size));
				} else
				{
					while (pHashEntry->pNext)
					{
						pPrevEntry = pHashEntry;
						pHashEntry = pHashEntry->pNext;
						if ((pEntry->net_dev == pHashEntry->net_dev)
							&& MAC_ADDR_EQUAL(pEntry->Addr, pHashEntry->Addr))
						{
							pPrevEntry->pNext = pHashEntry->pNext;
							pMulticastFilterTable->Size --;
							DBGPRINT(RT_DEBUG_TRACE, ("MCastFilterTableDeleteEntry 2 - Total= %d\n", pMulticastFilterTable->Size));
							break;
						}
					}
				}
				pEntry->Valid = FALSE;
				DeleteIgmpMemberList(&pEntry->MemberList);
			}

			if (pEntry->Valid == FALSE)
			{
				NdisZeroMemory(pEntry, sizeof(MULTICAST_FILTER_TABLE_ENTRY));
				pEntry->Valid = TRUE;

				COPY_MAC_ADDR(pEntry->Addr, pGrpId);
				pEntry->net_dev = dev;
				pEntry->lastTime = jiffies;
				pEntry->type = type;
				initList(&pEntry->MemberList);
				if (pMemberAddr != NULL)
					pMacEntry = APSsPsInquiry(pAd, pMemberAddr, &Sst, &Aid, &PsMode, &Rate);
				else
					pMacEntry = NULL;

				if (pMacEntry)
					InsertIgmpMember(&pEntry->MemberList, Aid);

				pMulticastFilterTable->Size ++;

				DBGPRINT(RT_DEBUG_TRACE, ("MulticastFilterTableInsertEntry -IF(%s) allocate entry #%d, Total= %d\n", dev->name, i, pMulticastFilterTable->Size));
				break;
			}
		}

		// add this MAC entry into HASH table
		if (pEntry)
		{
			HashIdx = MULTICAST_ADDR_HASH_INDEX(pGrpId);
			if (pMulticastFilterTable->Hash[HashIdx] == NULL)
			{
				pMulticastFilterTable->Hash[HashIdx] = pEntry;
			} else
			{
				pCurrEntry = pMulticastFilterTable->Hash[HashIdx];
				while (pCurrEntry->pNext != NULL)
					pCurrEntry = pCurrEntry->pNext;
				pCurrEntry->pNext = pEntry;
			}
		}
	}while(FALSE);

	RTMP_SEM_UNLOCK(&MulticastFilterTabLock);

	return TRUE;
}

/*
    ==========================================================================
    Description:
        Delete a specified client from MAC table
    ==========================================================================
 */
BOOLEAN MulticastFilterTableDeleteEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pGrpId,
	IN PUCHAR pMemberAddr,
	IN PNET_DEV dev)
{
	USHORT HashIdx;
	MULTICAST_FILTER_TABLE_ENTRY *pEntry, *pPrevEntry;
	PMULTICAST_FILTER_TABLE pMulticastFilterTable = pAd->pMulticastFilterTable;
	USHORT Aid = MCAST_WCID;
	SST	Sst = SST_ASSOC;
	UCHAR PsMode = PWR_ACTIVE, Rate;

	if (pMulticastFilterTable == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Multicase filter table is not ready.\n", __FUNCTION__));
		return FALSE;
	}

	RTMP_SEM_LOCK(&MulticastFilterTabLock);

	do
	{
		HashIdx = MULTICAST_ADDR_HASH_INDEX(pGrpId);
		pEntry = pMulticastFilterTable->Hash[HashIdx];

		// check the rule is in table already or not.
		if (pEntry && (pMemberAddr != NULL))
		{
			APSsPsInquiry(pAd, pMemberAddr, &Sst, &Aid, &PsMode, &Rate);
			DeleteIgmpMember(&pEntry->MemberList, Aid);
			if (IgmpMemberCnt(&pEntry->MemberList) > 0)
				break;
		}

		if (pEntry)
		{
			if ((dev == pEntry->net_dev)
				&& MAC_ADDR_EQUAL(pEntry->Addr, pGrpId))
			{
				pMulticastFilterTable->Hash[HashIdx] = pEntry->pNext;
				DeleteIgmpMemberList(&pEntry->MemberList);
				NdisZeroMemory(pEntry, sizeof(MULTICAST_FILTER_TABLE_ENTRY));
				pMulticastFilterTable->Size --;
				DBGPRINT(RT_DEBUG_TRACE, ("MCastFilterTableDeleteEntry 1 - Total= %d\n", pMulticastFilterTable->Size));
			} else
			{
				while (pEntry->pNext)
				{
					pPrevEntry = pEntry;
					pEntry = pEntry->pNext;
					if ((dev == pEntry->net_dev)
						&& MAC_ADDR_EQUAL(pEntry->Addr, pGrpId))
					{
						pPrevEntry->pNext = pEntry->pNext;
						DeleteIgmpMemberList(&pEntry->MemberList);
						NdisZeroMemory(pEntry, sizeof(MULTICAST_FILTER_TABLE_ENTRY));
						pMulticastFilterTable->Size --;
						DBGPRINT(RT_DEBUG_TRACE, ("MCastFilterTableDeleteEntry 2 - Total= %d\n", pMulticastFilterTable->Size));
						break;
					}
				}
			}
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s: the Group doesn't exist.\n", __FUNCTION__));
		}
	} while(FALSE);

	RTMP_SEM_UNLOCK(&MulticastFilterTabLock);
    
	return TRUE;
}

/*
    ==========================================================================
    Description:
        Look up the MAC address in the IGMP table. Return NULL if not found.
    Return:
        pEntry - pointer to the MAC entry; NULL is not found
    ==========================================================================
*/
PMULTICAST_FILTER_TABLE_ENTRY MulticastFilterTableLookup(
	IN PMULTICAST_FILTER_TABLE pMulticastFilterTable,
	IN PUCHAR pAddr,
	IN PNET_DEV dev)
{
	ULONG HashIdx;
	PMULTICAST_FILTER_TABLE_ENTRY pEntry = NULL, pPrev = NULL;

	if (pMulticastFilterTable == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s Multicase filter table is not ready.\n", __FUNCTION__));
		return NULL;
	}

	HashIdx = MULTICAST_ADDR_HASH_INDEX(pAddr);
	pEntry = pPrev = pMulticastFilterTable->Hash[HashIdx];

	while (pEntry && pEntry->Valid)
	{
		if ((pEntry->net_dev ==  dev)
			&& MAC_ADDR_EQUAL(pEntry->Addr, pAddr))
		{
			pEntry->lastTime = jiffies;
			break;
		}
		else
		{
			if ((pEntry->Valid == TRUE) && (pEntry->type == MCAT_FILTER_DYNAMIC)
				&& RTMP_TIME_AFTER(jiffies, pEntry->lastTime+IGMPMAC_TB_ENTRY_AGEOUT_TIME))
			{
				// Remove the aged entry
				if (pEntry == pMulticastFilterTable->Hash[HashIdx])
				{
					pMulticastFilterTable->Hash[HashIdx] = pEntry->pNext;
					pPrev = pMulticastFilterTable->Hash[HashIdx];
				}
				else 
				{	
					pPrev->pNext = pEntry->pNext;
				}

				NdisZeroMemory(pEntry, sizeof(MULTICAST_FILTER_TABLE_ENTRY));
				pMulticastFilterTable->Size --;
				pEntry = (pPrev == NULL ? NULL: pPrev->pNext);
				DBGPRINT(RT_DEBUG_TRACE, ("MCastFilterTableDeleteEntry 2 - Total= %d\n", pMulticastFilterTable->Size));
			}
			pEntry = NULL;
		}
	}

	return pEntry;
}

VOID IGMPSnooping(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pDstMacAddr,
	IN PUCHAR pSrcMacAddr,
	IN PUCHAR pIpHeader,
	IN PNET_DEV pDev)
{
	INT i;
	INT IpHeaderLen;
	UCHAR GroupType;
	UINT16 numOfGroup;
	UCHAR IgmpVerType;
	PUCHAR pIgmpHeader;
	PUCHAR pGroup;
	UCHAR AuxDataLen;
	UINT16 numOfSources;
	PUCHAR pGroupIpAddr;
	UCHAR GroupMacAddr[6];
	PUCHAR pGroupMacAddr = (PUCHAR)&GroupMacAddr;

	if(isIgmpPkt(pDstMacAddr, pIpHeader))
	{
		IpHeaderLen = (*(pIpHeader + 2) & 0x0f) * 4;
		pIgmpHeader = pIpHeader + 2 + IpHeaderLen;
		IgmpVerType = (UCHAR)(*(pIgmpHeader));

		DBGPRINT(RT_DEBUG_TRACE, ("IGMP type=%0x\n", IgmpVerType));

		switch(IgmpVerType)
		{
		case IGMP_V1_MEMBERSHIP_REPORT: // IGMP version 1 membership report.
		case IGMP_V2_MEMBERSHIP_REPORT: // IGMP version 2 membership report.
			pGroupIpAddr = (PUCHAR)(pIgmpHeader + 4);
			ConvertMulticastIP2MAC(pGroupIpAddr, (PUCHAR *)&pGroupMacAddr);
			DBGPRINT(RT_DEBUG_TRACE, ("IGMP Group=%02x:%02x:%02x:%02x:%02x:%02x\n",
				GroupMacAddr[0], GroupMacAddr[1], GroupMacAddr[2], GroupMacAddr[3], GroupMacAddr[4], GroupMacAddr[5]));
			MulticastFilterTableInsertEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev, MCAT_FILTER_DYNAMIC);
			break;

		case IGMP_LEAVE_GROUP: // IGMP version 1 and version 2 leave group.
			pGroupIpAddr = (PUCHAR)(pIgmpHeader + 4);
			ConvertMulticastIP2MAC(pGroupIpAddr, (PUCHAR *)&pGroupMacAddr);
			DBGPRINT(RT_DEBUG_TRACE, ("IGMP Group=%02x:%02x:%02x:%02x:%02x:%02x\n",
				GroupMacAddr[0], GroupMacAddr[1], GroupMacAddr[2], GroupMacAddr[3], GroupMacAddr[4], GroupMacAddr[5]));
			MulticastFilterTableDeleteEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev);
			break;

		case IGMP_V3_MEMBERSHIP_REPORT: // IGMP version 3 membership report.
			numOfGroup = ntohs(*((UINT16 *)(pIgmpHeader + 6)));
			pGroup = (PUCHAR)(pIgmpHeader + 8);
			for (i=0; i < numOfGroup; i++)
			{
				GroupType = (UCHAR)(*pGroup);
				AuxDataLen = (UCHAR)(*(pGroup + 1));
				numOfSources = ntohs(*((UINT16 *)(pGroup + 2)));
				pGroupIpAddr = (PUCHAR)(pGroup + 4);
				DBGPRINT(RT_DEBUG_TRACE, ("IGMPv3 Type=%d, ADL=%d, numOfSource=%d\n", GroupType, AuxDataLen, numOfSources));
				ConvertMulticastIP2MAC(pGroupIpAddr, (PUCHAR *)&pGroupMacAddr);
				DBGPRINT(RT_DEBUG_TRACE, ("IGMP Group=%02x:%02x:%02x:%02x:%02x:%02x\n",
					GroupMacAddr[0], GroupMacAddr[1], GroupMacAddr[2], GroupMacAddr[3], GroupMacAddr[4], GroupMacAddr[5]));

				do
				{
					if((GroupType == MODE_IS_EXCLUDE) || (GroupType == CHANGE_TO_EXCLUDE_MODE) || (GroupType == ALLOW_NEW_SOURCES))
					{
						MulticastFilterTableInsertEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev, MCAT_FILTER_DYNAMIC);
						break;
					}

					if((GroupType == MODE_IS_INCLUDE) || (GroupType == BLOCK_OLD_SOURCES))
					{
						MulticastFilterTableDeleteEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev);
						break;
					}

					if((GroupType == CHANGE_TO_INCLUDE_MODE))
					{
						if(numOfSources == 0)
							MulticastFilterTableDeleteEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev);
						else
							MulticastFilterTableInsertEntry(pAd, GroupMacAddr, pSrcMacAddr, pDev, MCAT_FILTER_DYNAMIC);
						break;
					}
				} while(FALSE);
				pGroup += (8 + (numOfSources * 4) + AuxDataLen);
			}
			break;

		default:
			DBGPRINT(RT_DEBUG_TRACE, ("unknow IGMP Type=%d\n", IgmpVerType));
			break;
		}
	}

	return;
}

#if 0 // Doesn't be used.
BOOLEAN MulticastFilterApply(
	IN PMULTICAST_FILTER_TABLE pMulticastFilterTable,
	IN PUCHAR pDstMacAddr,
	IN PNET_DEV pDev)
{
	if(isIgmpMacAddr(pDstMacAddr))
		return FALSE;

	return MulticastFilterTableLookup(pMulticastFilterTable, pDstMacAddr, pDev) ? FALSE : TRUE;
}
#endif

VOID ConvertMulticastIP2MAC(
	IN PUCHAR pIpAddr,
	IN PUCHAR *ppMacAddr)
{
	if(pIpAddr == NULL)
		return;

	if(ppMacAddr == NULL || *ppMacAddr == NULL)
		return;

	memset(*ppMacAddr, 0, ETH_LENGTH_OF_ADDRESS);
	*(*ppMacAddr) = 0x01;
	*(*ppMacAddr + 1) = 0x00;
	*(*ppMacAddr + 2) = 0x5e;
	*(*ppMacAddr + 3) = pIpAddr[1] & 0x7f;
	*(*ppMacAddr + 4) = pIpAddr[2];
	*(*ppMacAddr + 5) = pIpAddr[3];
	return;
}

static BOOLEAN isIgmpMacAddr(
	IN PUCHAR pMacAddr)
{
	if((pMacAddr[0] == 0x01)
		&& (pMacAddr[1] == 0x00)
		&& (pMacAddr[2] == 0x5e))
		return TRUE;
	return FALSE;
}

BOOLEAN isIgmpPkt(
	IN PUCHAR pDstMacAddr,
	IN PUCHAR pIpHeader)
{
	UINT16 IpProtocol = ntohs(*((UINT16 *)(pIpHeader)));
	UCHAR IgmpProtocol;

	if(!isIgmpMacAddr(pDstMacAddr))
		return FALSE;

	if(IpProtocol == ETH_P_IP)
	{
		IgmpProtocol = (UCHAR)*(pIpHeader + 11);
		if(IgmpProtocol == IGMP_PROTOCOL_DESCRIPTOR)
				return TRUE;
	}

	return FALSE;
}

static VOID InsertIgmpMember(
	IN PLIST_HEADER pList,
	IN USHORT Aid)
{
	PMEMBER_ENTRY pMemberEntry;

	if(pList == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: membert list doesn't exist.\n", __FUNCTION__));
		return;
	}

	if((Aid == MCAST_WCID) && (Aid >= MAX_LEN_OF_MAC_TABLE))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: invalid member.\n", __FUNCTION__));
		return;
	}

	if((pMemberEntry = (PMEMBER_ENTRY)removeHeadList(&freeEntryList)) != NULL)
	{
		NdisZeroMemory(pMemberEntry, sizeof(MEMBER_ENTRY));
		pMemberEntry->Aid = Aid;
		insertTailList(pList, (PLIST_ENTRY)pMemberEntry);

		DBGPRINT(RT_DEBUG_TRACE, ("%s Aid=%d\n",
			__FUNCTION__, pMemberEntry->Aid));
	}
	return;
}

static VOID DeleteIgmpMember(
	IN PLIST_HEADER pList,
	IN USHORT Aid)
{
	PMEMBER_ENTRY pCurEntry;

	if((pList == NULL) || (pList->pHead == NULL))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: membert list doesn't exist.\n", __FUNCTION__));
		return;
	}

	if((Aid == MCAST_WCID) && (Aid >= MAX_LEN_OF_MAC_TABLE))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: invalid member.\n", __FUNCTION__));
		return;
	}

	pCurEntry = (PMEMBER_ENTRY)pList->pHead;
	while (pCurEntry)
	{
		if(Aid == pCurEntry->Aid)
		{
			delEntryList(pList, (PLIST_ENTRY)pCurEntry);
			insertTailList(&freeEntryList, (PLIST_ENTRY)pCurEntry);
			break;
		}
		pCurEntry = pCurEntry->pNext;
	}

	return;
}

static VOID DeleteIgmpMemberList(
	IN PLIST_HEADER pList)
{
	PMEMBER_ENTRY pCurEntry, pPrvEntry;

	if((pList == NULL) || (pList->pHead == NULL))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: membert list doesn't exist.\n", __FUNCTION__));
		return;
	}

	pPrvEntry = pCurEntry = (PMEMBER_ENTRY)pList->pHead;
	while (pCurEntry)
	{
		delEntryList(pList, (PLIST_ENTRY)pCurEntry);
		pPrvEntry = pCurEntry;
		pCurEntry = pCurEntry->pNext;
		insertTailList(&freeEntryList, (PLIST_ENTRY)pPrvEntry);
	}

	initList(pList);
	return;
}


UCHAR IgmpMemberCnt(
	IN PLIST_HEADER pList)
{
	if(pList == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: membert list doesn't exist.\n", __FUNCTION__));
		return 0;
	}

	return getListSize(pList);
}

VOID IgmpGroupDelMembers(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pMemberAddr,
	IN PNET_DEV pDev)
{
	INT i;
	MULTICAST_FILTER_TABLE_ENTRY *pEntry = NULL;
	PMULTICAST_FILTER_TABLE pMulticastFilterTable = pAd->pMulticastFilterTable;
	
	for (i = 0; i < MAX_LEN_OF_MULTICAST_FILTER_TABLE; i++)
	{
		// pick up the first available vacancy
		pEntry = &pMulticastFilterTable->Content[i];
		if (pEntry->Valid == TRUE)
			MulticastFilterTableDeleteEntry(pAd, pEntry->Addr, pMemberAddr, pDev);
	}
}

INT Set_IgmpSn_Enable_Proc(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR arg)
{
	UINT Enable;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	PNET_DEV pDev;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	ifIndex = pObj->ioctl_if;

	pDev = (ifIndex == MAIN_MBSSID) ? (pAd->net_dev) : (pAd->ApCfg.MBSSID[ifIndex].MSSIDDev);
	Enable = simple_strtol(arg, 0, 10);

	pAd->ApCfg.MBSSID[ifIndex].IgmpSnoopEnable = (BOOLEAN)(Enable == 0 ? 0 : 1);
	DBGPRINT(RT_DEBUG_TRACE, ("%s::(%s) %s\n", __FUNCTION__, pDev->name, Enable == TRUE ? "Enable IGMP Snooping":"Disable IGMP Snooping"));

	return TRUE;
}

INT Set_IgmpSn_AddEntry_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	BOOLEAN bGroupId = 1;
	PUCHAR value;
	PUCHAR thisChar;
	UCHAR IpAddr[4];
	UCHAR Addr[ETH_LENGTH_OF_ADDRESS];
	UCHAR GroupId[ETH_LENGTH_OF_ADDRESS];
	PUCHAR *pAddr = (PUCHAR *)&Addr;
	PNET_DEV pDev;
	POS_COOKIE pObj;
	UCHAR ifIndex;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	ifIndex = pObj->ioctl_if;

	pDev = (ifIndex == MAIN_MBSSID) ? (pAd->net_dev) : (pAd->ApCfg.MBSSID[ifIndex].MSSIDDev);

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		// refuse the Member if it's not a MAC address.
		if((bGroupId == 0) && (strlen(thisChar) != 17))
			continue;

		if(strlen(thisChar) == 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
		{
			for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
			{
				if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
					return FALSE;  //Invalid

				AtoH(value, &Addr[i++], 2);
			}

			if(i != 6)
				return FALSE;  //Invalid
		}
		else
		{
			for (i=0, value = rstrtok(thisChar,"."); value; value = rstrtok(NULL,".")) 
			{
				if((strlen(value) > 0) && (strlen(value) <= 3)) 
				{
					int ii;
					for(ii=0; ii<strlen(value); ii++)
						if (!isxdigit(*(value + ii)))
							return FALSE;
				}
				else
					return FALSE;  //Invalid

				IpAddr[i] = (UCHAR)simple_strtol(value, NULL, 10);
				i++;
			}

			if(i != 4)
				return FALSE;  //Invalid

			ConvertMulticastIP2MAC(IpAddr, (PUCHAR *)&pAddr);
		}

		if(bGroupId == 1)
			COPY_MAC_ADDR(GroupId, Addr);

		// Group-Id must be a MCAST address.
		if((bGroupId == 1) && IS_MULTICAST_MAC_ADDR(Addr))
			MulticastFilterTableInsertEntry(pAd, GroupId, NULL, pDev, MCAT_FILTER_STATIC);
		// Group-Member must be a UCAST address.
		else if ((bGroupId == 0) && !IS_MULTICAST_MAC_ADDR(Addr))
			MulticastFilterTableInsertEntry(pAd, GroupId, Addr, pDev, MCAT_FILTER_STATIC);
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s (%2X:%2X:%2X:%2X:%2X:%2X) is not a acceptable address.\n",
				__FUNCTION__, Addr[0], Addr[1], Addr[2], Addr[3], Addr[4], Addr[5]));
			return FALSE;
		}

		bGroupId = 0;
		DBGPRINT(RT_DEBUG_TRACE, ("%s (%2X:%2X:%2X:%2X:%2X:%2X)\n",
			__FUNCTION__, Addr[0], Addr[1], Addr[2], Addr[3], Addr[4], Addr[5]));

	}

	return TRUE;
}

INT Set_IgmpSn_DelEntry_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i, memberCnt = 0;
	BOOLEAN bGroupId = 1;
	PUCHAR value;
	PUCHAR thisChar;
	UCHAR IpAddr[4];
	UCHAR Addr[ETH_LENGTH_OF_ADDRESS];
	UCHAR GroupId[ETH_LENGTH_OF_ADDRESS];
	PUCHAR *pAddr = (PUCHAR *)&Addr;
	PNET_DEV pDev;
	POS_COOKIE pObj;
	UCHAR ifIndex;

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	ifIndex = pObj->ioctl_if;

	pDev = (ifIndex == MAIN_MBSSID) ? (pAd->net_dev) : (pAd->ApCfg.MBSSID[ifIndex].MSSIDDev);

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		// refuse the Member if it's not a MAC address.
		if((bGroupId == 0) && (strlen(thisChar) != 17))
			continue;

		if(strlen(thisChar) == 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
		{
			for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":")) 
			{
				if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
					return FALSE;  //Invalid

				AtoH(value, &Addr[i++], 2);
			}

			if(i != 6)
				return FALSE;  //Invalid
		}
		else
		{
			for (i=0, value = rstrtok(thisChar,"."); value; value = rstrtok(NULL,".")) 
			{
				if((strlen(value) > 0) && (strlen(value) <= 3)) 
				{
					int ii;
					for(ii=0; ii<strlen(value); ii++)
						if (!isxdigit(*(value + ii)))
							return FALSE;
				}
				else
					return FALSE;  //Invalid

				IpAddr[i] = (UCHAR)simple_strtol(value, NULL, 10);
				i++;
			}

			if(i != 4)
				return FALSE;  //Invalid

			ConvertMulticastIP2MAC(IpAddr, (PUCHAR *)&pAddr);
		}

		if(bGroupId == 1)
			COPY_MAC_ADDR(GroupId, Addr);
		else
			memberCnt++;

		if (memberCnt > 0 )
			MulticastFilterTableDeleteEntry(pAd, (PUCHAR)GroupId, Addr, pDev);

		bGroupId = 0;
	}

	if(memberCnt == 0)
		MulticastFilterTableDeleteEntry(pAd, (PUCHAR)GroupId, NULL, pDev);

	DBGPRINT(RT_DEBUG_TRACE, ("%s (%2X:%2X:%2X:%2X:%2X:%2X)\n",
		__FUNCTION__, Addr[0], Addr[1], Addr[2], Addr[3], Addr[4], Addr[5]));

	return TRUE;
}

INT Set_IgmpSn_TabDisplay_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	IGMPTableDisplay(pAd);
	return TRUE;
}

void rtmp_read_igmp_snoop_from_file(
	IN  PRTMP_ADAPTER pAd,
	char *tmpbuf,
	char *buffer)
{
	PUCHAR		macptr;						
	INT			i=0;

	//IgmpSnEnable
	if(RTMPGetKeyParameter("IgmpSnEnable", tmpbuf, 128, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < pAd->ApCfg.BssidNum); macptr = rstrtok(NULL,";"), i++)
		{
			if ((strncmp(macptr, "0", 1) == 0))
				pAd->ApCfg.MBSSID[i].IgmpSnoopEnable = FALSE;
			else if ((strncmp(macptr, "1", 1) == 0))
				pAd->ApCfg.MBSSID[i].IgmpSnoopEnable = TRUE;
	        else
				pAd->ApCfg.MBSSID[i].IgmpSnoopEnable = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("MBSSID[%d].Enable=%d\n", i, pAd->ApCfg.MBSSID[i].IgmpSnoopEnable));
	    }
	}
}

NDIS_STATUS IgmpPktInfoQuery(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pSrcBufVA,
	IN PNDIS_PACKET pPacket,
	IN UCHAR apidx,
	OUT BOOLEAN *pInIgmpGroup,
	OUT PMULTICAST_FILTER_TABLE_ENTRY *ppGroupEntry)
{
	if(IS_MULTICAST_MAC_ADDR(pSrcBufVA))
	{
		PUCHAR pIpHeader = pSrcBufVA + 12;

		if (isIgmpPkt(pSrcBufVA, pIpHeader))
		{
			*ppGroupEntry = NULL;
		}
		else if ((*ppGroupEntry = MulticastFilterTableLookup(pAd->pMulticastFilterTable, pSrcBufVA,
									pAd->ApCfg.MBSSID[apidx].MSSIDDev)) == NULL)
		{
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			return NDIS_STATUS_FAILURE;
		}
		*pInIgmpGroup = TRUE;
	}
	else if (IS_BROADCAST_MAC_ADDR(pSrcBufVA))
	{
		PUCHAR pDstIpAddr = pSrcBufVA + 30; // point to Destination of Ip address of IP header.
		UCHAR GroupMacAddr[6];
		PUCHAR pGroupMacAddr = (PUCHAR)&GroupMacAddr;

		ConvertMulticastIP2MAC(pDstIpAddr, (PUCHAR *)&pGroupMacAddr);
		if ((*ppGroupEntry = MulticastFilterTableLookup(pAd->pMulticastFilterTable, pGroupMacAddr,
								pAd->ApCfg.MBSSID[apidx].MSSIDDev)) != NULL)
		{
			*pInIgmpGroup = TRUE;
		}
	}
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS IgmpPktClone(
	IN PRTMP_ADAPTER pAd,
	IN PNDIS_PACKET pPacket,
	IN UCHAR QueIdx,
	IN PMULTICAST_FILTER_TABLE_ENTRY pGroupEntry)
{
	PNDIS_PACKET pSkbClone = NULL;
	PMEMBER_ENTRY pMemberEntry = (PMEMBER_ENTRY)pGroupEntry->MemberList.pHead;
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	SST	Sst = SST_ASSOC;
	UCHAR PsMode = PWR_ACTIVE;
	unsigned long IrqFlags;


	// check all members of the IGMP group.
	while(pMemberEntry != NULL)
	{
		pSkbClone = skb_clone(RTPKT_TO_OSPKT(pPacket), MEM_ALLOC_FLAG);
		if(pSkbClone)
		{
			pMacEntry = &pAd->MacTab.Content[pMemberEntry->Aid];
			Sst = pMacEntry->Sst;
			PsMode = pMacEntry->PsMode;
			if (pMacEntry && (Sst == SST_ASSOC) && (PsMode != PWR_SAVE))
			{
				RTMP_SET_PACKET_WCID(pSkbClone, (UCHAR)pMemberEntry->Aid);
				// Pkt type must set to PKTSRC_NDIS.
				// It cause of the deason that APHardTransmit()
				// doesn't handle PKTSRC_DRIVER pkt type in version 1.3.0.0.
				RTMP_SET_PACKET_SOURCE(pSkbClone, PKTSRC_NDIS);
			}
			else
			{
				RELEASE_NDIS_PACKET(pAd, pSkbClone, NDIS_STATUS_FAILURE);
				pMemberEntry = pMemberEntry->pNext;
				continue;
			}

			// insert the pkt to TxSwQueue.
			if (pAd->TxSwQueue[QueIdx].Number >= MAX_PACKETS_IN_QUEUE)
			{
#ifdef BLOCK_NET_IF
				StopNetIfQueue(pAd, QueIdx, pSkbClone);
#endif // BLOCK_NET_IF //
				RELEASE_NDIS_PACKET(pAd, pSkbClone, NDIS_STATUS_FAILURE);
				return NDIS_STATUS_FAILURE;
			}
			else
			{
				RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
				InsertTailQueue(&pAd->TxSwQueue[QueIdx], PACKET_TO_QUEUE_ENTRY(pSkbClone));
				RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
			}
		}
		pMemberEntry = pMemberEntry->pNext;
	}
	return NDIS_STATUS_SUCCESS;
}

