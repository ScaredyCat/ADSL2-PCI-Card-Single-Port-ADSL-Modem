/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mesh_link_mng.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2007-06-25      For mesh (802.11s) support.
*/

#include "rt_config.h"
#include "mlme_ex.h"
#include "mesh_def.h"
#include "mesh_sanity.h"


static VOID MeshTORTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

static VOID MeshTOCTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

static VOID MeshTOHTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

VOID PeerLinkEstablished(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

VOID PeerLinkClosed(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

static VOID MlmePasopAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID MlmeActopAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsAcptWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID MlmeCnclActionWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpenAcptWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOR1WhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpenAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnRjctWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnRjctWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOR2WhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID MlmeCnclWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnAcptWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsAcptWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnRjctWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnRjctWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID MlmeCnclWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOCWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnAcptWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOR1WhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnRjctWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnRjctWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOR2WhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID MlmeCnclWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnAcptWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsAcptWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnRjctWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnRjctWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID CnclWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID TOHWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerOpnActpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerCfnactpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);

static VOID PeerClsActpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx);


static VOID EnquePeerLinkOpen(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx);

static VOID EnquePeerLinkConfirm(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx, 
	IN UINT16 Aid,
	IN UINT16 StatusCode);

static VOID EnquePeerLinkClose(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx, 
	IN UINT16 ReasonCode);

static VOID EnquePeerLinkCloseEx(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pPeerMac,
	IN UINT32 LocalLinkId,
	IN UINT32 PeerLinkId,
	IN UINT16 ReasonCode);

static BOOLEAN PeerLinkOpnRcvd(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

static BOOLEAN PeerLinkCfnRcvd(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

static USHORT MeshBuildOpen(
	IN PRTMP_ADAPTER pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN UCHAR MaxSupportedRateIn500Kbps,
	IN UCHAR SupRateLen,
	IN BOOLEAN bWmmCapable,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN UCHAR HtCapabilityLen);

DECLARE_TIMER_FUNCTION(MeshTORTimeout);
DECLARE_TIMER_FUNCTION(MeshTOCTimeout);
DECLARE_TIMER_FUNCTION(MeshTOHTimeout);

BUILD_TIMER_FUNCTION(MeshTORTimeout);
BUILD_TIMER_FUNCTION(MeshTOCTimeout);
BUILD_TIMER_FUNCTION(MeshTOHTimeout);

/*
    ==========================================================================
    Description:
        check the Link is runing or not.
    Parameters:
        Standard timer parameters
    ==========================================================================
 */
BOOLEAN PeerLinkMngRuning(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	BOOLEAN Valid;

	if ((pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_IDLE)
		|| (pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_LISTEN)
		|| (pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_ESTAB))
		Valid = FALSE;
	else
		Valid = TRUE;

	return Valid;
}

BOOLEAN PeerLinkValidCheck(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	BOOLEAN Valid;

	if(!VALID_MESH_LINK_ID(Idx))
		Valid = FALSE;
	else if (pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_ESTAB)
		Valid = TRUE;
	else
		Valid = FALSE;

	return Valid;
}

VOID PeerLinkEstablished(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	PMESH_NEIGHBOR_ENTRY pNeighbor = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link Index (%d).\n", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("!!! Mesh link-%d is established !!!\n", Idx));			

	// Check if the current state is established. 
	if (pAd->MeshTab.MeshLink[Idx].CurrentState != MESH_LINK_MNG_ESTAB)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("!!! ERROR : This link(%d) isn't established (state=%d)!!!\n", Idx, pAd->MeshTab.MeshLink[Idx].CurrentState));
		return;
	}

	pNeighbor = NeighborSearch(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr);
	if (pNeighbor)
	{
		pNeighbor->State = LINK_AVAILABLE;
		pNeighbor->MeshLinkIdx = Idx;
	}

	// Check if this link exists in MAC_TABLE_ENTRY
	if((pEntry = MeshTableLookup(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, TRUE)) == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("!!! ERROR : This link(%d) doesn't exist in MAC_TABLE_ENTRY. (%02x:%02x:%02x:%02x:%02x:%02x)!!!\n", 
					(UCHAR)Idx, PRINT_MAC(pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr)));
		return;
	}
	
	if (pEntry)
	{
		// In WEP or WPANone mode, set pairwise-key to ASIC for per-Mesh-Entry 
		if (pEntry->AuthMode < Ndis802_11AuthModeWPA || pEntry->AuthMode == Ndis802_11AuthModeWPANone)
		{						
			UCHAR	BssIdx = BSS0;

			pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)	
			BssIdx = pAd->ApCfg.BssidNum + MIN_NET_DEVICE_FOR_MESH;
#endif // CONFIG_AP_SUPPORT //

			if ((pEntry->WepStatus == Ndis802_11WEPEnabled) || 
				 (pEntry->AuthMode == Ndis802_11AuthModeWPANone))
			{
				UCHAR	Keyidx = 0;

				if (pEntry->AuthMode == Ndis802_11WEPEnabled)
					Keyidx = pAd->MeshTab.DefaultKeyId;
			
				pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
				NdisMoveMemory(&pEntry->PairwiseKey, &pAd->MeshTab.SharedKey, sizeof(CIPHER_KEY));

				// Add Pair-wise key to Asic
            	AsicAddPairwiseKeyEntry(pAd,
										pEntry->Addr, 
										(UCHAR)pEntry->Aid,
										&pEntry->PairwiseKey);															

				RTMPAddWcidAttributeEntry(pAd, 
										  BssIdx,
										  Keyidx, 
										  pEntry->PairwiseKey.CipherAlg, 
										  pEntry);
				
			}
			
			// Once a mesh link is established, turn on this flag. 
			pAd->MeshTab.bInitialMsaDone = TRUE;

		}
			
	
	}

	return;
}

VOID PeerLinkClosed(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	PMESH_NEIGHBOR_ENTRY pNeighbor = NULL;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link Index (%d).\n", __FUNCTION__, Idx));
		return;
	}

	pNeighbor = NeighborSearch(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr);
	if (pNeighbor)
	{
		pNeighbor->State = NEIGHBOR_MP;
		pNeighbor->MeshLinkIdx = 0;
	}

	if (pAd->MeshTab.LinkSize == 0)
	{
		pAd->MeshTab.bInitialMsaDone = FALSE;
	}

	MultipathListDelete(pAd, Idx);

	return;
}

PMESH_LINK_ENTRY MeshLinkLookUp(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	INT i;
	PMESH_LINK_ENTRY pLinkEntry = NULL;

	RTMP_SEM_LOCK(&pAd->MeshTabLock);

	for (i = 0; i < MAX_MESH_LINKS; i++)
	{
		if ((pAd->MeshTab.MeshLink[i].Entry.Valid == TRUE)
			&& MAC_ADDR_EQUAL(pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr, pAddr))
		{
			pLinkEntry = &pAd->MeshTab.MeshLink[i].Entry;
			break;
		}
	}

	RTMP_SEM_UNLOCK(&pAd->MeshTabLock);

	return pLinkEntry;
}

ULONG MeshLinkAlloc(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN UCHAR LinkType)
{
	UINT i;
	ULONG MeshLinkIdx = BSS_NOT_FOUND;
	ULONG Now;

/*
	if (!MESH_ON(pAd))
	{
		return -1;
	}
*/

#ifdef WDS_SUPPORT
	if (WdsTableLookup(pAd, pAddr, FALSE) != NULL)
	{
		DBGPRINT(RT_DEBUG_INFO, ("%s() Peer WDS entry.\n", __FUNCTION__));
		return MeshLinkIdx;
	}
#endif // WDS_SUPPORT //

	// allocate one Mesh entry
	RTMP_SEM_LOCK(&pAd->MeshTabLock);

	do
	{
		for (i = 0; i < MAX_MESH_LINKS; i++)
		{
			if ((pAd->MeshTab.MeshLink[i].Entry.Valid == TRUE)
				&& MAC_ADDR_EQUAL(pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr, pAddr))
				break;
		}

		if (i < MAX_MESH_LINKS)
		{
			MeshLinkIdx = i;
			break;
		}

		for (i = 0; i < MAX_MESH_LINKS; i++)
		{
			if (pAd->MeshTab.MeshLink[i].Entry.Valid == FALSE)
			{
				int ii;
				NdisGetSystemUpTime(&Now);
				pAd->MeshTab.MeshLink[i].Entry.LastBeaconTime = Now;
				pAd->MeshTab.MeshLink[i].Entry.Valid = TRUE;
				pAd->MeshTab.MeshLink[i].Entry.LocalLinkId = RandomLinkId(pAd);
				pAd->MeshTab.MeshLink[i].Entry.Metrics = 10;
				pAd->MeshTab.MeshLink[i].Entry.LinkType = LinkType;
				COPY_MAC_ADDR(pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr, pAddr);
				pAd->MeshTab.LinkSize ++;
				MeshLinkIdx = i;
				pAd->MeshTab.MeshCapability.field.AcceptPeerLinks = MeshAcceptPeerLink(pAd);
				for (ii = 0; ii < MULTIPATH_HASH_TAB_SIZE; ii++)
					initList(&pAd->MeshTab.MeshLink[i].Entry.MultiPathHash[ii]);

				DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId(%d)=%x\n", __FUNCTION__, i, pAd->MeshTab.MeshLink[i].Entry.LocalLinkId));
				break;
			}
		}

		if (i == MAX_MESH_LINKS)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to allocate Mesh-Link.\n", __FUNCTION__));
			MeshLinkIdx = BSS_NOT_FOUND;
			break;
		}
	} while(FALSE);

#if 0 // this patch need more verification.
	// no necessary to create a mesh link for reachable MP.
	// if the MP is reachable through other MP then it's no necessary to create a link for it.
	if (i > (MAX_MESH_LINKS/2))
	{
		LONG RouteId = PathRouteIDSearch(pAd, pAddr);
		if ((RouteId >= 0) && (RouteId < BMCAST_ROUTE_ID))
		{ // the Candidate MP is reachable through other MP. It's no necessary to create a link for it.
			MeshLinkIdx = BSS_NOT_FOUND;
		}
	}
#endif

	// Upon reception of a peer link open frame from a candidate peer MP 
	// that contains an MSAIE, the local MP shall determine if it is the Selector MP.
	if (VALID_MESH_LINK_ID(MeshLinkIdx))
		ValidateLocalMPAsSelector(pAd, MeshLinkIdx);

	RTMP_SEM_UNLOCK(&pAd->MeshTabLock);

	return MeshLinkIdx;
}

VOID MeshLinkDelete(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN UINT MeshLinkIdx)
{
	// delete one Mesh entry
	RTMP_SEM_LOCK(&pAd->MeshTabLock);

	if (MeshLinkIdx < MAX_MESH_LINKS)
	{
		if (pAd->MeshTab.MeshLink[MeshLinkIdx].Entry.Valid == TRUE) 
		{
			pAd->MeshTab.MeshLink[MeshLinkIdx].Entry.Valid = FALSE;
			NdisZeroMemory(&pAd->MeshTab.MeshLink[MeshLinkIdx].Entry, sizeof(MESH_LINK_ENTRY));
			pAd->MeshTab.LinkSize --;
			pAd->MeshTab.MeshCapability.field.AcceptPeerLinks = MeshAcceptPeerLink(pAd);
		}
	}

	RTMP_SEM_UNLOCK(&pAd->MeshTabLock);

	return;
}

/*
    ==========================================================================
    Description:
        The mesh link management state machine, 
    Parameters:
        Sm - pointer to the state machine
    Note:
        the state machine looks like the following
    ==========================================================================
 */
VOID MeshLinkMngStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE_EX *Sm,
	OUT STATE_MACHINE_FUNC_EX Trans[])
{
	UCHAR i;

	StateMachineInitEx(Sm, (STATE_MACHINE_FUNC_EX*)Trans, (ULONG)MESH_LINK_MNG_MAX_STATES,
		(ULONG)MESH_LINK_MNG_MAX_EVENTS, (STATE_MACHINE_FUNC_EX)DropEx, MESH_LINK_MNG_IDLE, MESH_LINK_MNG_IDLE);

	// IDLE state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_IDLE, MESH_LINK_MNG_PASOPN, (STATE_MACHINE_FUNC_EX)MlmePasopAction);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_IDLE, MESH_LINK_MNG_ACTOPN, (STATE_MACHINE_FUNC_EX)MlmeActopAction);

	// LISTEN state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_LISTEN, MESH_LINK_MNG_CNCL, (STATE_MACHINE_FUNC_EX)MlmeCnclActionWhenListen);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_LISTEN, MESH_LINK_MNG_ACTOPN, (STATE_MACHINE_FUNC_EX)MlmeActopAction);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_LISTEN, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsAcptWhenListen);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_LISTEN, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpenAcptWhenListen);

	// OPN_SNT state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_TOR1, (STATE_MACHINE_FUNC_EX)TOR1WhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_CFN_ACPT, (STATE_MACHINE_FUNC_EX)PeerCfnAcptWhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpenAcptWhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsAcptWhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_OPEN_RJCT, (STATE_MACHINE_FUNC_EX)PeerOpnRjctWhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_CFN_RJCT, (STATE_MACHINE_FUNC_EX)PeerCfnRjctWhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_TOR2, (STATE_MACHINE_FUNC_EX)TOR2WhenOpnSnt);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_SNT, MESH_LINK_MNG_CNCL, (STATE_MACHINE_FUNC_EX)MlmeCnclWhenOpnSnt);

	// CFN_RCVD state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_CFN_ACPT, (STATE_MACHINE_FUNC_EX)DropEx);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpnAcptWhenCfnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsAcptWhenCfnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_OPEN_RJCT, (STATE_MACHINE_FUNC_EX)PeerOpnRjctWhenCfnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_CFN_RJCT, (STATE_MACHINE_FUNC_EX)PeerCfnRjctWhenCfnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_CNCL, (STATE_MACHINE_FUNC_EX)MlmeCnclWhenCfnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_CFN_RCVD, MESH_LINK_MNG_TOC, (STATE_MACHINE_FUNC_EX)TOCWhenCfnRcvd);

	// OPN_RCVD state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_TOR1, (STATE_MACHINE_FUNC_EX)TOR1WhenOpnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpnAcptWhenOpnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_CFN_ACPT, (STATE_MACHINE_FUNC_EX)PeerCfnAcptWhenOpnRcvd);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsAcptWhenOpnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_OPEN_RJCT, (STATE_MACHINE_FUNC_EX)PeerOpnRjctWhenOpnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_CFN_RJCT, (STATE_MACHINE_FUNC_EX)PeerCfnRjctWhenOpnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_TOR2, (STATE_MACHINE_FUNC_EX)TOR2WhenOpnRcvd);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_OPN_RCVD, MESH_LINK_MNG_CNCL, (STATE_MACHINE_FUNC_EX)MlmeCnclWhenOpnRcvd);

	// ESTAB state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_ESTAB, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpnAcptWhenEstab);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_ESTAB, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsAcptWhenEstab);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_ESTAB, MESH_LINK_MNG_OPEN_RJCT, (STATE_MACHINE_FUNC_EX)PeerOpnRjctWhenEstab);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_ESTAB, MESH_LINK_MNG_CFN_RJCT, (STATE_MACHINE_FUNC_EX)PeerCfnRjctWhenEstab);
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_ESTAB, MESH_LINK_MNG_CNCL, (STATE_MACHINE_FUNC_EX)CnclWhenEstab);

	// HOLDING state
	StateMachineSetActionEx(Sm, MESH_LINK_MNG_HOLDING, MESH_LINK_MNG_TOH, (STATE_MACHINE_FUNC_EX)TOHWhenHolding);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_HOLDING, MESH_LINK_MNG_OPEN_ACPT, (STATE_MACHINE_FUNC_EX)PeerOpnActpWhenHolding);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_HOLDING, MESH_LINK_MNG_CFN_ACPT, (STATE_MACHINE_FUNC_EX)PeerCfnactpWhenHolding);
 	StateMachineSetActionEx(Sm, MESH_LINK_MNG_HOLDING, MESH_LINK_MNG_CLS_ACPT, (STATE_MACHINE_FUNC_EX)PeerClsActpWhenHolding);

	for (i = 0; i < MAX_MESH_LINKS; i++)
	{
		/* init all Mesh link state. */
		pAd->MeshTab.MeshLink[i].CurrentState = MESH_LINK_MNG_LISTEN;

	/* init all timer such as Timer-R, Timer-C and Timer-H relative to mesh Link-Mng. */
		RTMPInitTimer(pAd, &pAd->MeshTab.MeshLink[i].TOR, GET_TIMER_FUNCTION(MeshTORTimeout), pAd, FALSE);
		RTMPInitTimer(pAd, &pAd->MeshTab.MeshLink[i].TOC, GET_TIMER_FUNCTION(MeshTOCTimeout), pAd, FALSE);
		RTMPInitTimer(pAd, &pAd->MeshTab.MeshLink[i].TOH, GET_TIMER_FUNCTION(MeshTOHTimeout), pAd, FALSE);
	}

	return;
}

/*
    ==========================================================================
    Description:
        Peer-Link-Discovery timeout procedure.
    Parameters:
        Standard timer parameters
    ==========================================================================
 */
static VOID MeshTORTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	UCHAR idx;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PRALINK_TIMER_STRUCT pTimer = (PRALINK_TIMER_STRUCT) SystemSpecific3;

	for (idx = 0; idx < MAX_MESH_LINKS; idx++)
	{
		if (&pAd->MeshTab.MeshLink[idx].TOR == pTimer)
			break;
	}

	if (idx == MAX_MESH_LINKS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - unknow timer.\n", __FUNCTION__));
		return;
	}

	if (pAd->MeshTab.MeshLink[idx].Entry.OpenRetyCnt < MAX_OPEN_RETRY)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - enqueue MESH_LINK_MNG_TOR1 to MESH_LINK_MNG_STATE_MACHINE State-Machine. idx(%d)\n", __FUNCTION__, idx));
		MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_TOR1, 0, NULL, idx);
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - enqueue MESH_LINK_MNG_TOR2 to MESH_LINK_MNG_STATE_MACHINE State-Machine.idx(%d)\n", __FUNCTION__, idx));
		MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_TOR2, 0, NULL, idx);
	}
		
	MeshMlmeHandler(pAd);

	return;
}

/*
    ==========================================================================
    Description:
        Mesh-Channel-Switch timeout procedure.
    Parameters:
        Standard timer parameters
    ==========================================================================
 */
static VOID MeshTOCTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	UCHAR idx;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PRALINK_TIMER_STRUCT pTimer = (PRALINK_TIMER_STRUCT) SystemSpecific3;

	for (idx = 0; idx < MAX_MESH_LINKS; idx++)
	{
		if (&pAd->MeshTab.MeshLink[idx].TOC == pTimer)
			break;
	}

	if (idx == MAX_MESH_LINKS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - unknow timer.\n", __FUNCTION__));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s - enqueue MESH_LINK_MNG_TOC to MESH_LINK_MNG_STATE_MACHINE State-Machine. idx(%d)\n", __FUNCTION__, idx));

	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_TOC, 0, NULL, idx);
    MeshMlmeHandler(pAd);

	return;
}

/*
    ==========================================================================
    Description:
        Mesh-Channel-Switch timeout procedure.
    Parameters:
        Standard timer parameters
    ==========================================================================
 */
static VOID MeshTOHTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	UCHAR idx;
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;
	PRALINK_TIMER_STRUCT pTimer = (PRALINK_TIMER_STRUCT) SystemSpecific3;

	for (idx = 0; idx < MAX_MESH_LINKS; idx++)
	{
		if (&pAd->MeshTab.MeshLink[idx].TOH == pTimer)
			break;
	}

	if (idx == MAX_MESH_LINKS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - unknow timer.\n", __FUNCTION__));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s - enqueue MESH_LINK_MNG_TOH to MESH_LINK_MNG_STATE_MACHINE State-Machine. idx(%d)\n", __FUNCTION__, idx));

	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_TOH, 0, NULL, idx);
    MeshMlmeHandler(pAd);

	return;
}

static VOID MlmePasopAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// transit state to MESH_LINK_MNG_LISTEN state.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get PASOPN Event at Idle (Idx=%d).\n", __FUNCTION__, Idx));

	*pCurrState = MESH_LINK_MNG_LISTEN;

	return;
}

static VOID MlmeActopAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Send Peer Link Open frame to Candidate MP.
	// Set Time TOR.
	// transit state to MESH_LINK_MNG_OPN_SNT state.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get ACTOPN Event at Idle (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkOpen(pAd, Idx);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOR, TOR_TIME);
	*pCurrState = MESH_LINK_MNG_OPN_SNT;

	return;
}

static VOID PeerClsAcptWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// transit state to MESH_LINK_MNG_IDLE.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CLSAcpt event at Listen (Idx=%d).\n", __FUNCTION__, Idx));

	*pCurrState = MESH_LINK_MNG_LISTEN;

	return;
}

static VOID MlmeCnclActionWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// transit state to MESH_LINK_MNG_IDLE state.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get MlmeCNCL enent at Listen (Idx=%d).\n", __FUNCTION__, Idx));

	*pCurrState = MESH_LINK_MNG_LISTEN;

	return;
}

static VOID PeerOpenAcptWhenListen(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Send Peer Link Open frame to Candidate MP.
	// Send Peer Link Confirm frame to Candidate MP.
	// Set Time TOR.
	// transit state to MESH_LINK_MNG_OPN_RCVD state.

	PMAC_TABLE_ENTRY pMacEntry = NULL;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNAcpt at Listen (Idx=%d).\n", __FUNCTION__, Idx));

	pAd->MeshTab.MeshLink[Idx].Entry.PeerLinkId = pInfo->PeerLinkId;

	pMacEntry = MacTableInsertMeshEntry(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, Idx);
	if (MeshBuildOpen(pAd, pMacEntry, pInfo->MaxSupportedRate,pInfo->SupRateLen, pInfo->bWmmCapable,
						&pInfo->HTCapability, pInfo->HTCapability_Len) != MLME_SUCCESS)
	{
		MacTableDeleteEntry(pAd, pMacEntry->Aid, pMacEntry->Addr);
		pMacEntry = NULL;
	}

	if (pMacEntry)
	{
		EnquePeerLinkOpen(pAd, Idx);
		EnquePeerLinkConfirm(pAd, Idx, pMacEntry->Aid, MLME_SUCCESS);
		RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOR, TOR_TIME);
		*pCurrState = MESH_LINK_MNG_OPN_RCVD;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to allocate MacEntry here.\n", __FUNCTION__));
		EnquePeerLinkConfirm(pAd, Idx, 0, MLME_FAIL_NO_RESOURCE);
		RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
		*pCurrState = MESH_LINK_MNG_HOLDING;
	}

	return;
}

static VOID TOR1WhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Send Peer Link Open frame to Candidate MP.
	// Set Time TOR.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOR1 event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkOpen(pAd, Idx);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOR, TOR_TIME);

	return;
}

static VOID PeerCfnAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel TOR
	// set TOC
	// transit state to MESH_LINK_MNG_CFN_RCVD.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNAcpt event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOC, TOC_TIME);
	*pCurrState = MESH_LINK_MNG_CFN_RCVD;

	return;
}

static VOID PeerOpenAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Confirm to Candidate MP.
	// transit state to MESH_LINK_MNG_OPN_RCVD

	PMAC_TABLE_ENTRY pMacEntry = NULL;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNAcpt event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	pAd->MeshTab.MeshLink[Idx].Entry.PeerLinkId = pInfo->PeerLinkId;
	pMacEntry = MacTableInsertMeshEntry(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, Idx);
	if (MeshBuildOpen(pAd, pMacEntry, pInfo->MaxSupportedRate,pInfo->SupRateLen, pInfo->bWmmCapable,
						&pInfo->HTCapability, pInfo->HTCapability_Len) != MLME_SUCCESS)
	{
		MacTableDeleteEntry(pAd, pMacEntry->Aid, pMacEntry->Addr);
		pMacEntry = NULL;
	}

	if (pMacEntry)
	{
		EnquePeerLinkConfirm(pAd, Idx, pMacEntry->Aid, MLME_SUCCESS);
		*pCurrState = MESH_LINK_MNG_OPN_RCVD;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to a MacEntry here.\n", __FUNCTION__));
		EnquePeerLinkConfirm(pAd, Idx, 0, MLME_FAIL_NO_RESOURCE);
		RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
		*pCurrState = MESH_LINK_MNG_HOLDING;
	}

	return;
}

static VOID PeerClsAcptWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// Cancel Timer TOR.
	// Set Timer TOH.
	// Transit state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get ClsAcpt event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_CLOSE_RCVD);
	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnRjctWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNRjct event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerCfnRjctWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_CONFIRM_MSG_STRUCT pInfo = (PMESH_LINK_CONFIRM_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNRjct event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID TOR2WhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOR2 event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_MAX_RETRIES);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID MlmeCnclWhenOpnSnt(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get Cncl event at OpnSnt (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnAcptWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOC
	// send Peer Link Confirm to Candidate MP.
	// Transite state to MESH_LINK_MNG_ESTAB.

	BOOLEAN Cancelled;
	PMAC_TABLE_ENTRY pMacEntry = NULL;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNAcpt event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOC, &Cancelled);
	pAd->MeshTab.MeshLink[Idx].Entry.PeerLinkId = pInfo->PeerLinkId;
	pMacEntry = MacTableInsertMeshEntry(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, Idx);
	if (MeshBuildOpen(pAd, pMacEntry, pInfo->MaxSupportedRate,pInfo->SupRateLen, pInfo->bWmmCapable,
						&pInfo->HTCapability, pInfo->HTCapability_Len) != MLME_SUCCESS)
	{
		MacTableDeleteEntry(pAd, pMacEntry->Aid, pMacEntry->Addr);
		pMacEntry = NULL;
	}

	if (pMacEntry)
	{
		EnquePeerLinkConfirm(pAd, Idx, pMacEntry->Aid, MLME_SUCCESS);
		*pCurrState = MESH_LINK_MNG_ESTAB;
		PeerLinkEstablished(pAd, Idx);
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to a MacEntry here.\n", __FUNCTION__));
		EnquePeerLinkConfirm(pAd, Idx, 0, MLME_FAIL_NO_RESOURCE);
		RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
		*pCurrState = MESH_LINK_MNG_HOLDING;
	}

	return;
}

static VOID PeerClsAcptWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOC
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CLSAcpt event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOC, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, MESH_CLOSE_RCVD);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnRjctWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOC
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNRjct event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOC, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerCfnRjctWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOC
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_CONFIRM_MSG_STRUCT pInfo = (PMESH_LINK_CONFIRM_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNRjct event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOC, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID MlmeCnclWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOC
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get Cncl event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOC, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID TOCWhenCfnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOC event at CfnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_CONFIRM_TIMEOUT);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID TOR1WhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Send Peer Link Open frame to Candidate MP.
	// Set Time TOR.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOR1 event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkOpen(pAd, Idx);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOR, TOR_TIME);

}

static VOID PeerOpnAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Confirm to Candidate MP.

	PMAC_TABLE_ENTRY pMacEntry = NULL;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNAcpt event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	pMacEntry = MeshTableLookup(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, TRUE);

	if (pMacEntry)
		EnquePeerLinkConfirm(pAd, Idx, pMacEntry->Aid, MLME_SUCCESS);
	else
		printk("%s: MacEntry doesn't exist.\n", __FUNCTION__);

	return;
}

static VOID PeerCfnAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel TOR
	// transit state to MESH_LINK_MNG_ESTAB.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNAcpt event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	*pCurrState = MESH_LINK_MNG_ESTAB;
	PeerLinkEstablished(pAd, Idx);

	return;
}

static VOID PeerClsAcptWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// Cancel Timer TOR.
	// Set Timer TOH.
	// Transit state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get ClsAcpt event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_CLOSE_RCVD);
	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnRjctWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNRjct event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerCfnRjctWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_CONFIRM_MSG_STRUCT pInfo = (PMESH_LINK_CONFIRM_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNRjct event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID TOR2WhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOR2 event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_MAX_RETRIES);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID MlmeCnclWhenOpnRcvd(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get Cncl event at OpnRcvd (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnAcptWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Confirm to Candidate MP.

	PMAC_TABLE_ENTRY pMacEntry = NULL;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNAcpt event at Estab (Idx=%d).\n", __FUNCTION__, Idx));

	pMacEntry = MeshTableLookup(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, TRUE);

	if (pMacEntry)
		EnquePeerLinkConfirm(pAd, Idx, pMacEntry->Aid, MLME_SUCCESS);
	else
		DBGPRINT(RT_DEBUG_ERROR, ("%s: MacEntry doesn't exist.\n", __FUNCTION__));

	return;
}

static VOID PeerClsAcptWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// Cancel Timer TOR.
	// Set Timer TOH.
	// Transit state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get ClsAcpt event at Estab (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_CLOSE_RCVD);
	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerOpnRjctWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_OPEN_MSG_STRUCT pInfo = (PMESH_LINK_OPEN_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNRjct event at Estab (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID PeerCfnRjctWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// Cancel Timer TOR
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	BOOLEAN Cancelled;
	PMESH_LINK_CONFIRM_MSG_STRUCT pInfo = (PMESH_LINK_CONFIRM_MSG_STRUCT)(Elem->Msg);

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNRjct event at Estab (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOR, &Cancelled);
	EnquePeerLinkClose(pAd, Idx, pInfo->ReasonCode);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}

static VOID CnclWhenEstab(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	// send Peer Link Close to Candidate MP.
	// set Timer TOH.
	// Transite state to MESH_LINK_MNG_HOLDING.

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get Cncl event at Estab (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);
	RTMPSetTimer(&pAd->MeshTab.MeshLink[Idx].TOH, TOH_TIME);
	*pCurrState = MESH_LINK_MNG_HOLDING;

	return;
}
	
static VOID TOHWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get TOH event at Holding (Idx=%d).\n", __FUNCTION__, Idx));

	PeerLinkClosed(pAd, Idx);

	if(MeshTableLookup(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, TRUE))
		MacTableDeleteMeshEntry(pAd, pAd->MeshTab.MeshLink[Idx].Entry.MacTabMatchWCID,
			pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr);

	if ((pAd->MeshTab.MeshLink[Idx].Entry.LinkType == MESH_LINK_DYNAMIC)
		&& (pAd->MeshTab.MeshLink[Idx].Entry.Valid))
		MeshLinkDelete(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, Idx);

	*pCurrState = MESH_LINK_MNG_LISTEN;

	return;
}

static VOID PeerOpnActpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get OPNActp at Holding (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);

	return;	
}

static VOID PeerCfnactpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CFNActp at Holding (Idx=%d).\n", __FUNCTION__, Idx));

	EnquePeerLinkClose(pAd, Idx, MESH_LINK_CANCELLED);

	return;
}

static VOID PeerClsActpWhenHolding(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem,
	OUT PULONG pCurrState,
	IN USHORT Idx)
{
	BOOLEAN Cancelled;

	if (!VALID_MESH_LINK_ID(Idx))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Link-Idx=%d", __FUNCTION__, Idx));
		return;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s: Get CLSActp at Holding (Idx=%d).\n", __FUNCTION__, Idx));

	RTMPCancelTimer(&pAd->MeshTab.MeshLink[Idx].TOH, &Cancelled);

	PeerLinkClosed(pAd, Idx);

	if(MeshTableLookup(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, TRUE))
		MacTableDeleteMeshEntry(pAd, pAd->MeshTab.MeshLink[Idx].Entry.MacTabMatchWCID,
			pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr);

	if ((pAd->MeshTab.MeshLink[Idx].Entry.LinkType == MESH_LINK_DYNAMIC)
		&& (pAd->MeshTab.MeshLink[Idx].Entry.Valid))
		MeshLinkDelete(pAd, pAd->MeshTab.MeshLink[Idx].Entry.PeerMacAddr, Idx);

	*pCurrState = MESH_LINK_MNG_LISTEN;

	return;
}


static VOID EnquePeerLinkOpen(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx)
{
	HEADER_802_11 MeshHdr;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen;
	MESH_FLAG MeshFlag;
	UINT16 CapabilityInfoForAssocResp;
	UINT32 MeshSeq = INC_MESH_SEQ(pAd->MeshTab.MeshSeq);

	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pOutBuffer);  //Get an unused nonpaged memory
	if(NStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s() allocate memory failed \n", __FUNCTION__));
		return;
	}

	MeshHeaderInit(pAd, &MeshHdr,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr,		// addr1
		pAd->MeshTab.CurrentAddress,							// addr2
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr);		// addr3
	NdisMoveMemory(pOutBuffer, (PCHAR)&MeshHdr, sizeof(HEADER_802_11));
	FrameLen = sizeof(HEADER_802_11);

	// Mesh Header
	MeshFlag.word = 0;
	MeshFlag.field.AE = 1;	// Peer-Link manager frame never carry 6 addresses.
	InsertMeshHeader(pAd, (pOutBuffer + FrameLen), &FrameLen, MeshFlag.word,
		pAd->MeshTab.TTL, MeshSeq, pAd->MeshTab.CurrentAddress, NULL, NULL);

	// Action field
	InsertMeshActField(pAd, (pOutBuffer + FrameLen), &FrameLen, CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_OPEN);

	// Capability
	CapabilityInfoForAssocResp = GET_CAPABILITY_INFO(pAd) & ~(0x0003);
	InsertCapabilityInfo(pAd, (pOutBuffer + FrameLen), &FrameLen, CapabilityInfoForAssocResp);

	// Supported rates
	InsertSupRateIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// Extend rate
	InsertExtRateIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// RSN IE
	if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
		InsertRSNIE(pAd, pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr, (pOutBuffer + FrameLen), &FrameLen);

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		InsertHtCapIE(pAd, (pOutBuffer + FrameLen), &FrameLen, &pAd->CommonCfg.HtCapability);
		InsertAddHtInfoIE(pAd, (pOutBuffer + FrameLen), &FrameLen, &pAd->CommonCfg.AddHTInfo);
	}

	// Mesh Id IE
	InsertMeshIdIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// Mesh Configuration IE
	InsertMeshConfigurationIE(pAd, (pOutBuffer + FrameLen), &FrameLen,TRUE);
	
	// Mesh Peer Link Management IE
	InsertMeshPeerLinkMngIE(pAd, (pOutBuffer + FrameLen), &FrameLen, SUBTYPE_PEER_LINK_OPEN,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId, 0, 0);

	// Insert MSCIE
	InsertMSCIE(pAd, (pOutBuffer+FrameLen), &FrameLen);

	// Insert MSAIE
	InsertMSAIE(pAd, LinkIdx, SUBTYPE_PEER_LINK_OPEN, (pOutBuffer+FrameLen), &FrameLen);

	MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);

	pAd->MeshTab.MeshLink[LinkIdx].Entry.OpenRetyCnt++;

	DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId(%d)=%x\n",
		__FUNCTION__, LinkIdx, pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId));

	return;
}

static VOID EnquePeerLinkConfirm(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx, 
	IN UINT16 Aid,
	IN UINT16 StatusCode)
{
	HEADER_802_11 MeshHdr;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen;
	MESH_FLAG MeshFlag;
	UINT16 CapabilityInfoForAssocResp;
	UINT32 MeshSeq = INC_MESH_SEQ(pAd->MeshTab.MeshSeq);

	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pOutBuffer);  //Get an unused nonpaged memory
	if(NStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s() allocate memory failed \n", __FUNCTION__));
		return;
	}

	MeshHeaderInit(pAd, &MeshHdr,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr,		// addr1
		pAd->MeshTab.CurrentAddress,							// addr2
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr);		// addr3
	NdisMoveMemory(pOutBuffer, (PCHAR)&MeshHdr, sizeof(HEADER_802_11));
	FrameLen = sizeof(HEADER_802_11);

	// Mesh Header
	MeshFlag.word = 0;
	MeshFlag.field.AE = 1;	// Peer-Link manager frame never carry 6 addresses.
	InsertMeshHeader(pAd, (pOutBuffer + FrameLen), &FrameLen, MeshFlag.word,
		pAd->MeshTab.TTL, MeshSeq, pAd->MeshTab.CurrentAddress, NULL, NULL);

	// Action field
	InsertMeshActField(pAd, (pOutBuffer + FrameLen), &FrameLen, CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_CONFIRM);

	// Capability
	CapabilityInfoForAssocResp = GET_CAPABILITY_INFO(pAd) & ~(0x0003);
	InsertCapabilityInfo(pAd, (pOutBuffer + FrameLen), &FrameLen, CapabilityInfoForAssocResp);

	// Status code
	InsertStatusCode(pAd, (pOutBuffer + FrameLen), &FrameLen, StatusCode);

	InsertAID(pAd, (pOutBuffer + FrameLen), &FrameLen, Aid);

	// Supported rates
	InsertSupRateIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// Extend rate
	InsertExtRateIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// RSN IE
	if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
		InsertRSNIE(pAd, pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr, (pOutBuffer + FrameLen), &FrameLen);

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[pAd->MeshTab.MeshLink[LinkIdx].Entry.MacTabMatchWCID];
		if (pEntry->ValidAsMesh == TRUE)
		{
			InsertHtCapIE(pAd, (pOutBuffer + FrameLen), &FrameLen, &pAd->CommonCfg.HtCapability);
			InsertAddHtInfoIE(pAd, (pOutBuffer + FrameLen), &FrameLen, &pAd->CommonCfg.AddHTInfo);
		}
	}

	// Mesh Id IE
	InsertMeshIdIE(pAd, (pOutBuffer + FrameLen), &FrameLen);

	// Mesh Configuration IE
	InsertMeshConfigurationIE(pAd, (pOutBuffer + FrameLen), &FrameLen,TRUE);
	
	// Mesh Peer Link Management IE
	InsertMeshPeerLinkMngIE(pAd, (pOutBuffer + FrameLen), &FrameLen, SUBTYPE_PEER_LINK_CONFIRM,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId, pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerLinkId, 0);

	// Insert MSCIE
	InsertMSCIE(pAd, (pOutBuffer+FrameLen), &FrameLen);

	// Insert MSAIE
	InsertMSAIE(pAd, LinkIdx, SUBTYPE_PEER_LINK_CONFIRM, (pOutBuffer+FrameLen), &FrameLen);

	MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId(%d)=%x, PeerLinkId=%x\n",
		__FUNCTION__, LinkIdx, pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerLinkId));

	return;
}

static VOID EnquePeerLinkClose(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx, 
	IN UINT16 ReasonCode)
{
	EnquePeerLinkCloseEx(pAd, pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerMacAddr,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerLinkId,
		ReasonCode);

	pAd->MeshTab.MeshLink[LinkIdx].Entry.OpenRetyCnt = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId(%d)=%x, PeerLinkId=%x, Reason=%d\n",
		__FUNCTION__, LinkIdx, pAd->MeshTab.MeshLink[LinkIdx].Entry.LocalLinkId,
		pAd->MeshTab.MeshLink[LinkIdx].Entry.PeerLinkId, ReasonCode));

	return;
}

static VOID EnquePeerLinkCloseEx(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pPeerMac,
	IN UINT32 LocalLinkId,
	IN UINT32 PeerLinkId,
	IN UINT16 ReasonCode)
{
	HEADER_802_11 MeshHdr;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen;
	MESH_FLAG MeshFlag;
	UINT32 MeshSeq = INC_MESH_SEQ(pAd->MeshTab.MeshSeq);

	NStatus = MlmeAllocateMemory(pAd, (PVOID)&pOutBuffer);  //Get an unused nonpaged memory
	if(NStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s() allocate memory failed \n", __FUNCTION__));
		return;
	}

	MeshHeaderInit(pAd, &MeshHdr,
		pPeerMac,		// addr1
		pAd->MeshTab.CurrentAddress,							// addr2
		pPeerMac);		// addr3
	NdisMoveMemory(pOutBuffer, (PCHAR)&MeshHdr, sizeof(HEADER_802_11));
	FrameLen = sizeof(HEADER_802_11);

	// Mesh Header
	MeshFlag.word = 0;
	MeshFlag.field.AE = 1;	// Peer-Link manager frame never carry 6 addresses.
	InsertMeshHeader(pAd, (pOutBuffer + FrameLen), &FrameLen, MeshFlag.word,
		pAd->MeshTab.TTL, MeshSeq, pAd->MeshTab.CurrentAddress, NULL, NULL);

	// Action field
	InsertMeshActField(pAd, (pOutBuffer + FrameLen), &FrameLen, CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_CLOSE);

	// Reason code
	InsertReasonCode(pAd, (pOutBuffer + FrameLen), &FrameLen, ReasonCode);

	// Mesh Peer Link Management IE
	InsertMeshPeerLinkMngIE(pAd, (pOutBuffer + FrameLen), &FrameLen, SUBTYPE_PEER_LINK_CLOSE,
		LocalLinkId, PeerLinkId, ReasonCode);

	MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId=%x, PeerLinkId=%x, Reason=%d\n",
		__FUNCTION__, LocalLinkId, PeerLinkId, ReasonCode));

	return;
}

static BOOLEAN PeerLinkOpnRcvd(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	if ((pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_OPN_RCVD)
		|| (pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_ESTAB))
		return TRUE;
	else
		return FALSE;
}

static BOOLEAN PeerLinkCfnRcvd(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx)
{
	if ((pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_CFN_RCVD)
		|| (pAd->MeshTab.MeshLink[Idx].CurrentState == MESH_LINK_MNG_ESTAB))
		return TRUE;
	else
		return FALSE;
}

static USHORT MeshBuildOpen(
	IN PRTMP_ADAPTER pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN UCHAR MaxSupportedRateIn500Kbps,
	IN UCHAR SupRateLen,
	IN BOOLEAN bWmmCapable,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN UCHAR HtCapabilityLen)
{
	USHORT StatusCode = MLME_SUCCESS;
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

	if ((pAd->CommonCfg.PhyMode == PHY_11G) && (MaxSupportedRate < RATE_FIRST_OFDM_RATE))
		return MLME_ASSOC_REJ_DATA_RATE;

	// 11n only
	if (((pAd->CommonCfg.PhyMode == PHY_11N_2_4G) || (pAd->CommonCfg.PhyMode == PHY_11N_5G))
			&& (HtCapabilityLen == 0))
		return MLME_ASSOC_REJ_DATA_RATE;

	if (!pEntry)
		return MLME_UNSPECIFY_FAIL;

	do
	{
		// should qualify other parameters, for example - capablity, supported rates, listen interval, ... etc
		// to decide the Status Code
		pEntry->NoDataIdleCount = 0;

		DBGPRINT(RT_DEBUG_INFO, ("MeshPeerOpen - MaxSupportedRate = %x, pAd->CommonCfg.MaxTxRate = %x\n", 
			 MaxSupportedRate, pAd->CommonCfg.MaxTxRate));
		if (pEntry->Aid == 0)
			StatusCode = MLME_ASSOC_REJ_UNABLE_HANDLE_STA;
		else
		{
			pEntry->MaxSupportedRate = min(pAd->CommonCfg.MaxTxRate, MaxSupportedRate);
			if (pEntry->MaxSupportedRate < RATE_FIRST_OFDM_RATE)
			{
				pEntry->MaxHTPhyMode.field.MODE = MODE_CCK;
				pEntry->MaxHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
				pEntry->MinHTPhyMode.field.MODE = MODE_CCK;
				pEntry->MinHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
				pEntry->HTPhyMode.field.MODE = MODE_CCK;
				pEntry->HTPhyMode.field.MCS = pEntry->MaxSupportedRate;
			}
			else
			{
				pEntry->MaxHTPhyMode.field.MODE = MODE_OFDM;
				pEntry->MaxHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
				pEntry->MinHTPhyMode.field.MODE = MODE_OFDM;
				pEntry->MinHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
				pEntry->HTPhyMode.field.MODE = MODE_OFDM;
				pEntry->HTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
			}
            
#if 0
			//if (ClientRalinkIe & 0x00000004)
			if (ClientRalinkIe != 0x0)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
			else
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
			
			// Ralink proprietary Piggyback and Aggregation support for legacy RT61 chip
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
#ifdef AGGREGATION_SUPPORT
			if ((pAd->CommonCfg.bAggregationCapable) && (ClientRalinkIe & 0x00000001))
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC -RaAggregate= 1\n"));
			}
#endif // AGGREGATION_SUPPORT //
#ifdef PIGGYBACK_SUPPORT
			if ((pAd->CommonCfg.bPiggyBackCapable) && (ClientRalinkIe & 0x00000002))
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC -PiggyBack= 1\n"));
			}
#endif // PIGGYBACK_SUPPORT //
#endif

			// If this Entry supports 802.11n, upgrade to HT rate. 
			if ((HtCapabilityLen != 0) && (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
			{
				UCHAR	j, bitmask; //k,bitmask;
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
					if ( (pAd->MeshTab.DesiredHtPhyInfo.MCSSet[j]&bitmask) && (pHtCapability->MCSSet[j]&bitmask))
					{
						pEntry->MaxHTPhyMode.field.MCS = i;
						break;
					}
					if (i==0)
						break;
				}

				 
				if (pAd->MeshTab.DesiredTransmitSetting.field.MCS != MCS_AUTO)
				{

					printk("@@@ pAd->CommonCfg.RegTransmitSetting.field.MCS = %d\n",
						pAd->MeshTab.DesiredTransmitSetting.field.MCS);
					if (pAd->MeshTab.DesiredTransmitSetting.field.MCS == 32)
					{
						// Fix MCS as HT Duplicated Mode
						pEntry->MaxHTPhyMode.field.BW = 1;
						pEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
						pEntry->MaxHTPhyMode.field.STBC = 0;
						pEntry->MaxHTPhyMode.field.ShortGI = 0;
						pEntry->MaxHTPhyMode.field.MCS = 32;
					}
					else if (pEntry->MaxHTPhyMode.field.MCS > pAd->MeshTab.HTPhyMode.field.MCS)
					{
						// STA supports fixed MCS 
						pEntry->MaxHTPhyMode.field.MCS = pAd->MeshTab.HTPhyMode.field.MCS;
					}
				}

				pEntry->MaxHTPhyMode.field.STBC = (pHtCapability->HtCapInfo.RxSTBC & (pAd->CommonCfg.DesiredHtPhy.TxSTBC));
				pEntry->MpduDensity = pHtCapability->HtCapParm.MpduDensity;
				pEntry->MaxRAmpduFactor = pHtCapability->HtCapParm.MaxRAmpduFactor;
				pEntry->MmpsMode = (UCHAR)pHtCapability->HtCapInfo.MimoPs;
				pEntry->AMsduSize = (UCHAR)pHtCapability->HtCapInfo.AMsduSize;				
				pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

				// 1. The user-define configuration or 
				// 2. Not ralink-chipset and the encryption mode is WEP or TKIP (for fix Atheros STA issue in MacBook)
#if 0 // temporal remove since 11s doesn't specific how AMSDU work on Mesh network.
				if ((pAd->CommonCfg.DesiredHtPhy.AmsduEnable) || 
					(!(CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET)) && 
					 (pEntry->WepStatus == Ndis802_11WEPEnabled || pEntry->WepStatus == Ndis802_11Encryption2Enabled)))
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED);
#endif
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
			}


			pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

			NdisMoveMemory(&pEntry->HTCapability, pHtCapability, sizeof(HT_CAPABILITY_IE));

			pEntry->CurrTxRate = pEntry->MaxSupportedRate;

			if (bWmmCapable || (pEntry->MaxHTPhyMode.field.MODE >= MODE_HTMIX))
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}
			else
			{
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}
			
			// Set asic auto fall back
			if (pAd->MeshTab.bAutoTxRateSwitch == TRUE)
			{
				PUCHAR pTable;
				UCHAR TableSize = 0;
				
				MlmeSelectTxRateTableEx(pAd, pEntry, &pTable, &TableSize, &pEntry->CurrTxRateIndex);
				//AsicUpdateAutoFallBackTable(pAd, pTable);
				pEntry->bAutoTxRateSwitch = TRUE;
			}
			else
			{
				pEntry->HTPhyMode.field.MODE	= pAd->MeshTab.HTPhyMode.field.MODE;
				pEntry->HTPhyMode.field.MCS	= pAd->MeshTab.HTPhyMode.field.MCS;
				pEntry->bAutoTxRateSwitch = FALSE;
				
				RTMPUpdateLegacyTxSetting((UCHAR)pAd->MeshTab.DesiredTransmitSetting.field.FixedTxMode, pEntry);
			}

			pEntry->RateLen = SupRateLen;

			StatusCode = MLME_SUCCESS;
		}
	} while(FALSE);

    return StatusCode;
}

VOID MeshPeerLinkOpenProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk)
{
	PHEADER_802_11 pHeader = (PHEADER_802_11)pRxBlk->pHeader;
	PUCHAR pFrame;
	ULONG FrameLen;
	UINT MeshLinkId = 0;
	MESH_LINK_OPEN_MSG_STRUCT PeerLinkOpen;

    UINT16 CapabilityInfo;
	UCHAR SupRateLen;
	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR MeshIdLen;
	UCHAR MeshId[MAX_MESH_ID_LEN] = {0};
	UINT8 MeshSelPathId = 0xff;
	UINT8 MeshSelMetricId = 0xff;
	UINT32 CPI = 0;
	MESH_CAPABILITY MeshCapabilty;
	UINT16 PeerLinkId = 0;
	BOOLEAN bWmmCapable = FALSE;
	HT_CAPABILITY_IE HTCapability;
	UCHAR HTCapability_Len = 0;
	UCHAR MaxSupportedRate = 0;

	UCHAR RSNIE_Len;
    UCHAR RSN_IE[MAX_LEN_OF_RSNIE];
	MESH_SECURITY_CAPABILITY_IE	PeerMscIe;
	UCHAR PeerMsaIe[MESH_MAX_MSAIE_LEN];	
	UCHAR PeerMsaIeLen = 0;
	PMESH_LINK_ENTRY pMeshLinkEntry;
	
	MESH_LINK_MNG_EVENT result = MESH_LINK_MNG_OPEN_ACPT;
	UINT16 ReasonCode = MLME_SUCCESS;

	NdisZeroMemory(&HTCapability, sizeof(HT_CAPABILITY_IE));

	do 
	{
		UCHAR i;
		ULONG Idx;
		UCHAR MeshHdrLen;
		UCHAR pure_rsn_len = 0;		// exclude elementID, length and PMKID
		UCHAR pmkid_len = 0;

		MeshHdrLen = GetMeshHederLen(pRxBlk->pData);
		// skip Mesh Header
		pRxBlk->pData += MeshHdrLen;
		pRxBlk->DataSize -= MeshHdrLen;

		// skip Category and ActionCode
		pFrame = (PUCHAR)(pRxBlk->pData + 2);
		FrameLen = pRxBlk->DataSize - 2;

		NdisZeroMemory(RSN_IE, MAX_LEN_OF_RSNIE);
		NdisZeroMemory(PeerMsaIe, MESH_MAX_MSAIE_LEN);

		MeshLinkMngOpenSanity(	pAd,
							pFrame,
							FrameLen,
							&CapabilityInfo,
							SupRate,
							&SupRateLen,
							&MeshIdLen,
							MeshId,
							&MeshSelPathId,
							&MeshSelMetricId,
							&CPI,
							&MeshCapabilty,
							&PeerLinkId,
							&PeerMscIe,
							PeerMsaIe,
							&PeerMsaIeLen,
							RSN_IE,
							&RSNIE_Len,
							&bWmmCapable,
							&HTCapability,
							&HTCapability_Len);

		DBGPRINT(RT_DEBUG_TRACE, ("%s: PeerLinkId=%x, PeerMac=%02x:%02x:%02x:%02x:%02x:%02x\n", 
			__FUNCTION__, PeerLinkId, pHeader->Addr2[0], pHeader->Addr2[1], pHeader->Addr2[2],
			pHeader->Addr2[3], pHeader->Addr2[4], pHeader->Addr2[5]));

		Idx = GetMeshLinkId(pAd, pHeader->Addr2);
		if (Idx == BSS_NOT_FOUND)
		{
			Idx = MeshLinkAlloc(pAd, pHeader->Addr2, MESH_LINK_DYNAMIC);
			if (Idx == BSS_NOT_FOUND)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s() OpenRjct, PeerLink Not found.\n", __FUNCTION__));
				result = MESH_LINK_MNG_OPEN_RJCT;
				ReasonCode = MESH_MAX_NEIGHBORS;
				EnquePeerLinkCloseEx(pAd, pHeader->Addr2, RandomLinkId(pAd), PeerLinkId, ReasonCode);
				return;
			}
		}
		
		MeshLinkId = (UINT)Idx;
		pMeshLinkEntry = &pAd->MeshTab.MeshLink[MeshLinkId].Entry;

		if (MeshTableLookup(pAd, pHeader->Addr2, TRUE))
		{
			// peer link exist already.
			// check peer link Id.
			if (PeerLinkId != pAd->MeshTab.MeshLink[Idx].Entry.PeerLinkId)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s() OpenRjct, PeerLinkId not match. %x, %x\n",
					__FUNCTION__, PeerLinkId, pAd->MeshTab.MeshLink[Idx].Entry.PeerLinkId));
				result = MESH_LINK_MNG_OPEN_RJCT;
				ReasonCode = MLME_UNSPECIFY_FAIL;
			}
			break;
		}

		if (!(NdisEqualMemory(MeshId, pAd->MeshTab.MeshId, pAd->MeshTab.MeshIdLen)
			&& (MeshSelPathId == pAd->MeshTab.PathProtocolId)
			&& (MeshSelMetricId == pAd->MeshTab.PathMetricId)))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() OpenIgnr, Link Instance Not match.\n", __FUNCTION__));
			result = MESH_LINK_MNG_OPEN_IGNR;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			break;
		}

		if (!MeshCapabilty.field.AcceptPeerLinks)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() OpenRjct, Not AcceptPeerLinks.\n", __FUNCTION__));
			result = MESH_LINK_MNG_OPEN_RJCT;
			ReasonCode = MESH_MAX_NEIGHBORS;
			break;
		}

		// Verify the length of the received MSAIE
		if (PeerMsaIeLen < sizeof(MSA_HANDSHAKE_IE))
		{		
			DBGPRINT(RT_DEBUG_ERROR, ("The received length of MSAIE is too short(%d)\n", PeerMsaIeLen));		
			result = MESH_LINK_MNG_OPEN_RJCT;
			ReasonCode = MESH_SECURITY_FAILED_VERIFICATION;	
			break;
		}	

		// Verify that the "Default Role Negotiation" field included in the MSCIE of the peer link open frame
		// is identical to the value included in the local MP's MSCIE
		if (PeerMscIe.MeshSecurityConfig.field.DefaultRole != 
				pAd->MeshTab.LocalMSCIE.MeshSecurityConfig.field.DefaultRole)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("The Default Role Negotiation doesn't match(%d) \n", PeerMscIe.MeshSecurityConfig.field.DefaultRole));			
			result = MESH_LINK_MNG_OPEN_RJCT;
			ReasonCode = MESH_SECURITY_ROLE_NEGOTIATION_DIFFERS;
			break;
		}	

		// Verify that the local MP supports the peer MP's group cipher suite as indicated in the RSNIE
		// received in the peer link open frame. Further, verify that the pairwise cipher suite list and AKM
		// suite list in the received RSNIE each contain at least one entry that is also supported by the local MP.		
		if (!MeshValidateRSNIE(pAd, RSN_IE, RSNIE_Len, &pure_rsn_len, &pmkid_len))
		{
			result = MESH_LINK_MNG_OPEN_RJCT;
			ReasonCode = MESH_SECURITY_FAILED_VERIFICATION;
			break;
		}	

		hex_dump("Peer MSA IE", PeerMsaIe, PeerMsaIeLen);

		// Sanity check the selected AKM and pairwise-cipher of MSAIE in 
		// the received peer link open frame 
		if ((ReasonCode = MeshCheckPeerMsaIeCipherValidity(pAd,
												SUBTYPE_PEER_LINK_OPEN, 
												pMeshLinkEntry, 
												PeerMsaIe)) != MLME_SUCCESS)
		{	
			result = MESH_LINK_MNG_OPEN_RJCT;			
			break;
		}		

		// Determine if the local MP is the Authenticator MP
		ValidateLocalMPAsAuthenticator(pAd, MeshLinkId, (PUCHAR)&PeerMscIe, PeerMsaIe);

		// Key Selection Procedure	
		// If the key selection procedure resulted in an indication that Initial MSA Authentication shall occur,
		// the "Connected to MKD" bits contained in the received peer link open frame and as set by the local MP in its
		// Beacon frames and Probe Response frames shall be examined.
		if (!MeshKeySelectionAction(pAd, pMeshLinkEntry, (PUCHAR)&PeerMscIe, &RSN_IE[RSNIE_Len-pmkid_len], pmkid_len))		
		{						
			DBGPRINT(RT_DEBUG_ERROR, ("No any MP connect to MKD !!! \n"));
			result = MESH_LINK_MNG_OPEN_RJCT;
			ReasonCode = MESH_SECURITY_AUTHENTICATION_IMPOSSBLE;
			break;				
		}

		
		// If the local MP has received a peer link confirm frame from the candidate peer MP, 
		// it shall also verify the below conditions.		
		if (PeerLinkCfnRcvd(pAd, Idx))
		{						
			if ((ReasonCode = MeshValidateOpenAndCfnPeerLinkMsg(pAd, 
												SUBTYPE_PEER_LINK_OPEN, 
												pMeshLinkEntry, 
												(PUCHAR)&PeerMscIe,
												PeerMsaIe,
												PeerMsaIeLen, 
												RSN_IE, 
												RSNIE_Len,
												pure_rsn_len, 
												pmkid_len)) != MLME_SUCCESS)
			{	
				result = MESH_LINK_MNG_OPEN_RJCT;			
				break;
			}								
																							
		}
		// Record these information for peer link confirm frame check later
		else
		{			
			// Record the peer RSNIE
			if (RSNIE_Len > 0)
			{										
				// record the peer RSNIE except element-ID, length and the PMKID list.
				NdisMoveMemory(pMeshLinkEntry->RcvdRSNIE, &RSN_IE[2], pure_rsn_len);
				pMeshLinkEntry->RcvdRSNIE_Len = pure_rsn_len;

				hex_dump("RSNIE in PL-open ", pMeshLinkEntry->RcvdRSNIE, pMeshLinkEntry->RcvdRSNIE_Len);
														
				if (pmkid_len > 0)
				{
					// Record the PMKID fields (include PMKID-count and PMKID-list)
					NdisMoveMemory(pMeshLinkEntry->RcvdPMKID, &RSN_IE[RSNIE_Len-pmkid_len], pmkid_len);
					pMeshLinkEntry->RcvdPMKID_Len = pmkid_len;

					hex_dump("PMKID in PL-open ", pMeshLinkEntry->RcvdPMKID, pMeshLinkEntry->RcvdPMKID_Len);
				}
			}
			
			// Record the peer MSCIE			
			NdisMoveMemory((PUCHAR)&pMeshLinkEntry->RcvdMscIe, (PUCHAR)&PeerMscIe, sizeof(MESH_SECURITY_CAPABILITY_IE));
				
			// Record the peer MSAIE
			NdisMoveMemory(pMeshLinkEntry->RcvdMsaIe, PeerMsaIe, PeerMsaIeLen);
			pMeshLinkEntry->RcvdMsaIeLen = PeerMsaIeLen;
						
		}

	    // supported rates array may not be sorted. sort it and find the maximum rate
	    for (i=0; i<SupRateLen; i++)
	    {
	        if (MaxSupportedRate < (SupRate[i] & 0x7f)) 
	            MaxSupportedRate = SupRate[i] & 0x7f;
	    }
	} while (FALSE);

	PeerLinkOpen.CPI = CPI;
	PeerLinkOpen.MeshCapabilty = MeshCapabilty.word;
	PeerLinkOpen.PeerLinkId = PeerLinkId;
	PeerLinkOpen.ReasonCode = ReasonCode;
	PeerLinkOpen.MaxSupportedRate = MaxSupportedRate;
	PeerLinkOpen.SupRateLen = SupRateLen;
	PeerLinkOpen.bWmmCapable = bWmmCapable;
	PeerLinkOpen.HTCapability = HTCapability;
	PeerLinkOpen.HTCapability_Len = HTCapability_Len;
	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, result,
		sizeof(MESH_LINK_OPEN_MSG_STRUCT), &PeerLinkOpen, MeshLinkId);

	MeshMlmeHandler(pAd);

	return;
}

VOID MeshPeerLinkConfirmProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk)
{
	PHEADER_802_11 pHeader = (PHEADER_802_11)pRxBlk->pHeader;
	PUCHAR pFrame;
	ULONG FrameLen;
	UINT MeshLinkId = 0;
	MESH_LINK_CONFIRM_MSG_STRUCT PeerLinkCnf;
	
    UINT16 CapabilityInfo;
	UCHAR SupRateLen;
	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR MeshIdLen;
	UCHAR MeshId[MAX_MESH_ID_LEN] = {0};
	UINT8 MeshSelPathId = 0xff;
	UINT8 MeshSelMetricId = 0xff;
	UINT32 CPI = 0;
	MESH_CAPABILITY MeshCapabilty;
	UINT16 StatusCode;
	UINT16 Aid;
	UINT16 LocalLinkId = 0;
	UINT16 PeerLinkId = 0;
	UCHAR RSNIE_Len;
    UCHAR RSN_IE[MAX_LEN_OF_RSNIE];
	MESH_SECURITY_CAPABILITY_IE	PeerMscIe;
	UCHAR PeerMsaIe[MESH_MAX_MSAIE_LEN];	
	UCHAR PeerMsaIeLen = 0;
	HT_CAPABILITY_IE HTCapability;
	UCHAR HTCapability_Len;

	PMESH_LINK_ENTRY pMeshLinkEntry;

	MESH_LINK_MNG_EVENT result = MESH_LINK_MNG_CFN_ACPT;
	UINT16 ReasonCode = MLME_SUCCESS;
	
	do 
	{
		ULONG Idx;
		UCHAR MeshHdrLen;
		UCHAR 	pure_rsn_len = 0;		// exclude elementID, length and PMKID
		UCHAR	pmkid_len = 0;

		Idx = GetMeshLinkId(pAd, pHeader->Addr2);
		if (Idx == BSS_NOT_FOUND)
		{
			result = MESH_LINK_MNG_CFN_IGNR;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			return;
		}
		MeshLinkId = (UINT)Idx;
		pMeshLinkEntry = &pAd->MeshTab.MeshLink[MeshLinkId].Entry;

		MeshHdrLen = GetMeshHederLen(pRxBlk->pData);
		// skip Mesh Header
		pRxBlk->pData += MeshHdrLen;
		pRxBlk->DataSize -= MeshHdrLen;

		// skip Category and ActionCode
		pFrame = (PUCHAR)(pRxBlk->pData + 2);
		FrameLen = pRxBlk->DataSize - 2;

		MeshLinkMngCfnSanity(	pAd,
							pFrame,
							FrameLen,
							&CapabilityInfo,
							&StatusCode,
							&Aid,
							SupRate,
							&SupRateLen,
							&MeshIdLen,
							MeshId,
							&MeshSelPathId,
							&MeshSelMetricId,
							&CPI,
							&MeshCapabilty,
							&LocalLinkId,
							&PeerLinkId,
							&PeerMscIe,
							PeerMsaIe,
							&PeerMsaIeLen,
							RSN_IE,
							&RSNIE_Len,
							&HTCapability,
							&HTCapability_Len);

		DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId=%x, PeerLinkId=%x, PeerMac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			__FUNCTION__, LocalLinkId, PeerLinkId, pHeader->Addr2[0], pHeader->Addr2[1],
			pHeader->Addr2[2], pHeader->Addr2[3], pHeader->Addr2[4], pHeader->Addr2[5]));

		if (!(NdisEqualMemory(MeshId, pAd->MeshTab.MeshId, pAd->MeshTab.MeshIdLen)
			&& (MeshSelPathId == pAd->MeshTab.PathProtocolId)
			&& (MeshSelMetricId == pAd->MeshTab.PathMetricId)))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() CfnIgnr, Link Instance Not match.\n", __FUNCTION__));
			result = MESH_LINK_MNG_CFN_IGNR;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			break;
		}

		if (StatusCode != MLME_SUCCESS)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() CfnRjct, Status Code=%d.\n", __FUNCTION__, StatusCode));
			result = MESH_LINK_MNG_CFN_RJCT;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			break;
		}

		if (PeerLinkId != pAd->MeshTab.MeshLink[MeshLinkId].Entry.LocalLinkId)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() CfnRjct, Link Id Not match. %x, %x\n",
				__FUNCTION__, PeerLinkId, pAd->MeshTab.MeshLink[MeshLinkId].Entry.LocalLinkId));
			result = MESH_LINK_MNG_CFN_RJCT;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			break;
		}

		// Verify the received RSNIE
		if (!MeshValidateRSNIE(pAd, RSN_IE, RSNIE_Len, &pure_rsn_len, &pmkid_len))
		{
			result = MESH_LINK_MNG_CFN_RJCT;
			ReasonCode = MESH_SECURITY_FAILED_VERIFICATION;
			break;
		}	

		// Verify the length of the received MSAIE
		if (PeerMsaIeLen < sizeof(MSA_HANDSHAKE_IE))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("The received length of MSAIE in P.L. confirm frame is too short(%d)\n", PeerMsaIeLen));
			result = MESH_LINK_MNG_CFN_RJCT;
			ReasonCode = MESH_SECURITY_FAILED_VERIFICATION;
			break;
		}
		
		// If the local MP has received a peer link open
		// frame from the candidate peer MP, the local MP shall:
		if (PeerLinkOpnRcvd(pAd, Idx))
		{				
			if ((ReasonCode = MeshValidateOpenAndCfnPeerLinkMsg(pAd, 
												SUBTYPE_PEER_LINK_CONFIRM, 
												pMeshLinkEntry, 
												(PUCHAR)&PeerMscIe,
												PeerMsaIe,
												PeerMsaIeLen, 
												RSN_IE, 
												RSNIE_Len,
												pure_rsn_len, 
												pmkid_len)) != MLME_SUCCESS)
			{	
				result = MESH_LINK_MNG_OPEN_RJCT;			
				break;
			}																							
		}
		// On the other hand, if the local MP has not received a peer link open frame from the candidate peer MP,
		else
		{						
			// Sanity check the selected AKM and pairwise-cipher of MSAIE in 
			// the received peer link confirm frame 
			if ((ReasonCode = MeshCheckPeerMsaIeCipherValidity(pAd,
												SUBTYPE_PEER_LINK_CONFIRM, 
												pMeshLinkEntry, 
												PeerMsaIe)) != MLME_SUCCESS)
			{	
				result = MESH_LINK_MNG_OPEN_RJCT;			
				break;
			}	
		}

		
		// Record the received RSNIE, MSCIE and MSAIE
		{
			if (RSNIE_Len > 0)
			{										
				// record the peer RSNIE except element-ID, length and the PMKID list.
				NdisMoveMemory(pMeshLinkEntry->RcvdRSNIE, &RSN_IE[2], pure_rsn_len);
				pMeshLinkEntry->RcvdRSNIE_Len = pure_rsn_len;
	
				hex_dump("RSNIE in PL-confirm ", pMeshLinkEntry->RcvdRSNIE, pMeshLinkEntry->RcvdRSNIE_Len);
															
				if (pmkid_len > 0)
				{
					// Record the PMKID fields (include PMKID-count and PMKID-list)
					NdisMoveMemory(pMeshLinkEntry->RcvdPMKID, &RSN_IE[RSNIE_Len-pmkid_len], pmkid_len);
					pMeshLinkEntry->RcvdPMKID_Len = pmkid_len;

					hex_dump("PMKID in PL-confirm ", pMeshLinkEntry->RcvdPMKID, pMeshLinkEntry->RcvdPMKID_Len);
				}
			}

			NdisMoveMemory((PUCHAR)&pMeshLinkEntry->RcvdMscIe, (PUCHAR)&PeerMscIe, sizeof(MESH_SECURITY_CAPABILITY_IE));		

			NdisMoveMemory(pMeshLinkEntry->RcvdMsaIe, PeerMsaIe, PeerMsaIeLen);
			pMeshLinkEntry->RcvdMsaIeLen = PeerMsaIeLen;		
		}

		// Got Peer link confirm. reset OpenRetryCnt;
		pAd->MeshTab.MeshLink[MeshLinkId].Entry.OpenRetyCnt = 0;
	} while(FALSE);

	PeerLinkCnf.CPI = CPI;
	PeerLinkCnf.StatusCode = StatusCode;
	PeerLinkCnf.Aid = Aid;
	PeerLinkCnf.MeshCapabilty = MeshCapabilty.word;
	PeerLinkCnf.LocalLinkId = PeerLinkId;
	PeerLinkCnf.PeerLinkId = LocalLinkId;
	PeerLinkCnf.ReasonCode = ReasonCode;
	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, result,
		sizeof(MESH_LINK_OPEN_MSG_STRUCT), &PeerLinkCnf, MeshLinkId);

	MeshMlmeHandler(pAd);

	return;
}

VOID MeshPeerLinkCloseProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk)
{
	PHEADER_802_11 pHeader = (PHEADER_802_11)pRxBlk->pHeader;
	PUCHAR pFrame;
	ULONG FrameLen;
	UINT MeshLinkId = 0;
	MESH_LINK_CLOSE_MSG_STRUCT PeerLinkClose;

	UINT16 LocalLinkId = 0;
	UINT16 PeerLinkId = 0;
	UINT16 ReasonCode = 0;

	MESH_LINK_MNG_EVENT result = MESH_LINK_MNG_CLS_ACPT;

	do 
	{
		ULONG Idx;
		UCHAR MeshHdrLen;

		Idx = GetMeshLinkId(pAd, pHeader->Addr2);
		if (Idx == BSS_NOT_FOUND)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() ClsIgnore, PeerLink Not found.\n", __FUNCTION__));
			result = MESH_LINK_MNG_CLS_IGNR;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			return;
		}
		MeshLinkId = (UINT)Idx;

		MeshHdrLen = GetMeshHederLen(pRxBlk->pData);
		// skip Mesh Header
		pRxBlk->pData += MeshHdrLen;
		pRxBlk->DataSize -= MeshHdrLen;

		// skip Category and ActionCode
		pFrame = (PUCHAR)(pRxBlk->pData + 2);
		FrameLen = pRxBlk->DataSize - 2;

		NdisMoveMemory(&ReasonCode, pFrame, 2);
		pFrame += 2;
		FrameLen -= 2;

		MeshLinkMngClsSanity(	pAd,
							pFrame,
							FrameLen,
							&LocalLinkId,
							&PeerLinkId,
							&ReasonCode);

		DBGPRINT(RT_DEBUG_TRACE, ("%s: LocalLinkId=%x, PeerLinkId=%x, Reason=%d, PeerMac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			__FUNCTION__, LocalLinkId, PeerLinkId, ReasonCode, pHeader->Addr2[0], pHeader->Addr2[1],
			pHeader->Addr2[2], pHeader->Addr2[3], pHeader->Addr2[4], pHeader->Addr2[5]));

		if ((PeerLinkId != pAd->MeshTab.MeshLink[MeshLinkId].Entry.LocalLinkId)
			|| (LocalLinkId != pAd->MeshTab.MeshLink[MeshLinkId].Entry.PeerLinkId))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() ClsIgnr, Link Id Not match.\n", __FUNCTION__));
			result = MESH_LINK_MNG_CLS_IGNR;
			ReasonCode = MLME_UNSPECIFY_FAIL;
			break;
		}
	} while (FALSE);

	PeerLinkClose.LocalLinkId = LocalLinkId;
	PeerLinkClose.PeerLinkId = PeerLinkId;
	PeerLinkClose.ReasonCode = ReasonCode;
	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, result,
		sizeof(MESH_LINK_OPEN_MSG_STRUCT), &PeerLinkClose, MeshLinkId);

	MeshMlmeHandler(pAd);

	return;
}

VOID MeshLinkTableMaintenace(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR idx;
	ULONG Now;

    NdisGetSystemUpTime(&Now);

	for (idx = 0; idx < MAX_MESH_LINKS; idx++)
	{
		if (PeerLinkValidCheck(pAd, idx) == FALSE)
			continue;

		pAd->MeshTab.iw_stats.miss.beacon+=10-pAd->MeshTab.MeshLink[idx].Entry.OneSecBeaconCount;	
		pAd->MeshTab.MeshLink[idx].Entry.OneSecBeaconCount=0;


		if (RTMP_TIME_AFTER(Now, pAd->MeshTab.MeshLink[idx].Entry.LastBeaconTime + (MESH_AGEOUT_TIME * OS_HZ / 1000)))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("ageout MESH #%d.\n", idx));
			MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_CNCL, 0, NULL, idx);
		}
	}

	return;
}

