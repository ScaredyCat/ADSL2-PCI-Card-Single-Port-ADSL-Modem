/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2005, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attempt
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mesh.h

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2007-06-25		To Support Mesh (802.11s) feature.
*/

#ifndef __MESH_H__
#define __MESH_H__

#ifdef CONFIG_AP_SUPPORT
#define MESH_BEACON_IDX(__pAd)		(__pAd)->ApCfg.BssidNum
#endif // CONFIG_AP_SUPPORT //

#define PRINT_MAC(addr)	\
	addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#define MESH_ON(_pAd) \
	(((_pAd)->flg_mesh_init) \
		&& ((_pAd)->MeshTab.dev) \
		&& ((_pAd)->MeshTab.dev->flags & IFF_UP))

#define INT24(_a)				(((_a) & 0x800000) ? (~(_a)) : (_a))
#define MESH_SEQ_AFTER(a, b)	(INT24(b) - INT24(a) < 0)
#define MESH_SEQ_SUB(a, b)		(((a) >= (b)) ? ((a) - (b)) : ((a) + 0x007fffff - (b)))

#define MESH_MAC_ADDR_HASH_INDEX(_Addr)		(MAC_ADDR_HASH((_Addr)) % MAX_HASH_NEIGHBOR_MP)
#define BMPKT_MAC_ADDR_HASH_INDEX(_Addr)	(MAC_ADDR_HASH((_Addr)) % MAX_HASH_BMPKTSIG_TAB_SIZE)
#define INC_MESH_SEQ(_idx)					((_idx) = (((_idx) + 1) & (0x00ffffff)))

#define IS_MULTICAST_MAC_ADDR(Addr)			((((Addr[0]) & 0x01) == 0x01) && ((Addr[0]) != 0xff))

#ifdef CONFIG_AP_SUPPORT
#define GET_CAPABILITY_INFO(pAd) ((pAd)->ApCfg.MBSSID[MAIN_MBSSID].CapabilityInfo)
#endif // CONFIG_AP_SUPPORT //

static inline VOID MeshChannelInit(IN PRTMP_ADAPTER pAd)
{
	if (!((pAd->net_dev) && (pAd->net_dev->flags & IFF_UP)))
	{
		AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
		AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
	}
}

extern UCHAR MeshOUI[];
extern UINT8 LenPeerLinkMngIE[];

INT Set_MeshId_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshHostName_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshAutoLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshAddLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshDelLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshMaxTxRate_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshAuthMode_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshEncrypType_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshDefaultkey_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshWEPKEY_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshWPAKEY_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

BOOLEAN MeshValid(
	PMESH_STRUCT pMesh);

BOOLEAN PeerLinkMngRuning(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

BOOLEAN PeerLinkValidCheck(
	IN PRTMP_ADAPTER pAd,
	IN USHORT Idx);

PMESH_LINK_ENTRY MeshLinkLookUp(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr);

ULONG MeshLinkAlloc(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN UCHAR LinkType);

VOID MeshLinkDelete(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN UINT MeshLinkIdx);

VOID MeshPeerLinkOpenProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk);

VOID MeshPeerLinkConfirmProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk);

VOID MeshPeerLinkCloseProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk);

VOID MeshLinkTableMaintenace(
	IN PRTMP_ADAPTER pAd);

BOOLEAN MeshAcceptPeerLink(
	IN PRTMP_ADAPTER pAd);

MAC_TABLE_ENTRY *MacTableInsertMeshEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr,
	IN  UINT MeshLinkIdx);

BOOLEAN MacTableDeleteMeshEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr);

MAC_TABLE_ENTRY *MeshTableLookup(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount);

MAC_TABLE_ENTRY *MeshTableLookupByWcid(
    IN  PRTMP_ADAPTER   pAd, 
	IN UCHAR wcid,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount);

MAC_TABLE_ENTRY *FindMeshEntry(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR 			Wcid,
	IN PUCHAR			pAddr);

BOOLEAN NeighborMPCheck(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pMeshId,
	IN UINT8 PathSelProtocolId,
	IN UINT8 PathSelMetricId);

PMESH_NEIGHBOR_ENTRY InsertNeighborMP(
	IN PRTMP_ADAPTER pAd,
	IN CHAR RealRssi,
	IN PUCHAR pPeerMacAddr,
	IN UCHAR Channel,
	IN UINT8 ChBW,
	IN UINT8 ExtChOffset,
	IN PUCHAR pHostName,
	IN PUCHAR pMeshId,
	IN UCHAR MeshIdLen,
	IN UCHAR MeshCipher,
	IN PMESH_CONFIGURAION_IE pMeshConfig);

VOID DeleteNeighborMP(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pPeerMacAddr);

PMESH_NEIGHBOR_ENTRY NeighborSearch(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pPeerMacAddr);

VOID NeighborTableUpdate(
	IN PRTMP_ADAPTER pAd);

VOID CandidateMPSelect(
	IN PRTMP_ADAPTER pAd);

void NeighborTableReset(
	IN PRTMP_ADAPTER pAd);

void NeighborTableInit(
	IN PRTMP_ADAPTER pAd);

void NeighborTableDestroy(
	IN PRTMP_ADAPTER pAd);

void ValidateLocalMPAsSelector(
	IN PRTMP_ADAPTER pAd,
	IN INT		 idx);

void ValidateLocalMPAsAuthenticator(
	IN PRTMP_ADAPTER 	pAd,
	IN UCHAR		 	idx,
	IN PUCHAR			pMscIe,
	IN PUCHAR			pMsaIe);

BOOLEAN MeshKeySelectionAction(
	IN PRTMP_ADAPTER 		pAd,
	IN PMESH_LINK_ENTRY  	pMeshLinkEntry,
	IN PUCHAR				pMscIe,
	IN PUCHAR				pPmkId,
	IN UCHAR				PmkIdLen);

VOID MeshUp(
	IN PRTMP_ADAPTER pAd);

VOID MeshDown(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN WaitFlag);

VOID MeshHalt(
	IN PRTMP_ADAPTER pAd);

VOID MeshHeaderInit(
    IN PRTMP_ADAPTER pAd,
    OUT PHEADER_802_11 pMeshHdr, 
    IN PUCHAR Addr1, 
    IN PUCHAR Addr2,
    IN PUCHAR Addr3);

VOID InsertMeshHeader(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 MeshFlag,
	IN UINT8 TTL,
	IN UINT32 MeshSeq,
	IN PUCHAR Addr4,
	IN PUCHAR Addr5,
	IN PUCHAR Addr6);

VOID InsertMeshActField(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 Category,
	IN UINT8 ActCode);

VOID InsertCapabilityInfo(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT16 CapabilityInfo);

VOID InsertStatusCode(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT16 StatusCode);

VOID InsertReasonCode(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT16 ReasonCode);

VOID InsertAID(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen, 
	IN UINT16 Aid);

VOID InsertSupRateIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen);

VOID InsertExtRateIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen);

VOID InsertMeshConfigurationIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN BOOLEAN AcptLink);

VOID InsertMeshIdIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen);

VOID InsertMSCIE(
	IN PRTMP_ADAPTER 	pAd,
	OUT PUCHAR 			pFrameBuf,
	OUT PULONG 			pFrameLen);

VOID InsertRSNIE(
	IN PRTMP_ADAPTER 	pAd,
	IN PUCHAR			pAddr,
	OUT PUCHAR 			pFrameBuf,
	OUT PULONG 			pFrameLen);

VOID InsertMSAIE(
	IN PRTMP_ADAPTER 	pAd,
	IN UCHAR			LinkIdx,
	IN UCHAR			state,
	OUT PUCHAR 			pFrameBuf,
	OUT PULONG 			pFrameLen);

VOID InsertMeshPeerLinkMngIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 SubType,
	IN UINT16 LocalLinkId,
	IN UINT16 PeerLinkId,
	IN UINT16 ReasonCode);

VOID InsertMeshPathRequestIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 MeshPreqFlag,
	IN UCHAR HopCount,
	IN UCHAR TTL,
	IN ULONG ReqID,
	IN PUCHAR pOrigAddr,
	IN ULONG OrigSeq,
	IN PUCHAR pProxyAddr,
	IN ULONG LifeTime,
	IN ULONG Metric,
	IN UCHAR DestCount,
	IN MESH_DEST_ENTRY MeshDestEntry);

VOID InsertMeshPathResponseIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 MeshPrepFlag,
	IN UCHAR HopCount,
	IN UCHAR TTL,
	IN PUCHAR pDestAddr,
	IN ULONG DestSeq,
	IN PUCHAR pDestProxyAddr,
	IN ULONG LifeTime,
	IN ULONG Metric,
	IN PUCHAR pOrigAddr,
	IN ULONG OrigSeq,
	IN UCHAR DependMPCount,
	IN PMESH_PREP_DEPENDENT_ENTRY pPrepDependEntry);

VOID InsertMeshPathErrorIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UCHAR DestCount,
	IN PMESH_PERR_ENTRY MeshPerrEntry);

VOID InsertMeshHostNameIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen);

VOID InsertMeshCongestionNotofocationIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN PUCHAR	pPeerMac,
	IN UCHAR	SwitchOn);

VOID InsertMeshChSwAnnIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT	PULONG	pFrameLen,
	IN UINT8 ChSwMode,
	IN UINT8 NewCh,
	IN UINT32 NewCPI,
	IN UINT8 ChSwCnt,
	IN PUCHAR pMeshSA);

VOID InsertHtCapIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT	PULONG	pFrameLen,
	IN PHT_CAPABILITY_IE pHtCapability);

VOID InsertAddHtInfoIE(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT	PULONG	pFrameLen,
	IN PADD_HT_INFO_IE pAddHTInfo);

INT Set_MeshRouteAdd_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshRouteDelete_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshRouteUpdate_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshPortalEnable_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshMultiCastAgeOut_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_NeighborInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshRouteInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshEntryInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MultipathInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MultiCastAgeOut_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_MeshOnly_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

INT Set_PktSig_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg);

VOID RTMP_Mesh_Init(
	IN PRTMP_ADAPTER ad_p,
	IN PNET_DEV main_dev_p,
	IN PUCHAR pHostName);

VOID RTMP_Mesh_Close(
	IN PRTMP_ADAPTER ad_p);

VOID RTMP_Mesh_Remove(
	IN PRTMP_ADAPTER ad_p);

VOID MeshCtrlStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE_EX *Sm,
	OUT STATE_MACHINE_FUNC_EX Trans[]);

VOID MeshLinkMngStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE_EX *Sm,
	OUT STATE_MACHINE_FUNC_EX Trans[]);

VOID MlmeHandleRxMeshFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk);

VOID CmmRxMeshMgmtFrameHandle(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk);

VOID MeshPreqCreate(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pProxyAddr,
	IN PUCHAR pDestAddr,
	IN ULONG DestDsn);

VOID MeshPreqRcvProcess(
	IN PRTMP_ADAPTER pAd,
	IN	RX_BLK	*pRxBlk);

VOID
MeshPreqForward(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 MeshTTL,
	IN UINT32 MeshSeq,
	IN UINT8 MeshPreqFlag,
	IN UCHAR HopCount,
	IN UCHAR PreqTTL,
	IN ULONG ReqID,
	IN PUCHAR pOrigAddr,
	IN ULONG OrigSeq,
	IN PUCHAR pProxyAddr,
	IN ULONG LifeTime,
	IN ULONG Metric,
	IN UCHAR DestCount,
	IN MESH_DEST_ENTRY MeshDestEntry,
	IN PUCHAR TransmissionAddr);

VOID
MeshPrepCreate(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR	pNextHop,
	IN PUCHAR	pOrigAddr,
	IN ULONG	OrigDsn,
	IN PUCHAR	pProxyAddr);

VOID
MeshPrepRcvProcess(
	IN PRTMP_ADAPTER pAd,
	IN	RX_BLK	*pRxBlk);

VOID
MeshPrepForward(
	IN PRTMP_ADAPTER pAd,
	IN UINT8 MeshTTL,
	IN UINT32 MeshSeq,
	IN UINT8 MeshPrepFlag,
	IN UCHAR HopCount,
	IN UCHAR PrepTTL,
	IN PUCHAR pDestAddr,
	IN ULONG DestSeq,
	IN PUCHAR pProxyAddr,
	IN ULONG LifeTime,
	IN ULONG Metric,
	IN PUCHAR pOrigAddr,
	IN ULONG OrigSeq);

VOID
MeshPerrRcvProcess(
	IN PRTMP_ADAPTER pAd,
	IN	RX_BLK	*pRxBlk);

VOID
MeshPerrCreate(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR			DestCount,
	IN PMESH_PERR_ENTRY	pDestEntry);
VOID
MeshPerrForward(
	IN PRTMP_ADAPTER pAd,
	IN UINT8			MeshTTL,
	IN UINT32		MeshSeq,
	IN UCHAR			DestCount,
	IN PMESH_PERR_ENTRY	pDestEntry,
	IN PUCHAR 		TxAddr);

NDIS_STATUS
MeshRoutingTable_Init(
	IN PRTMP_ADAPTER	pAd);

NDIS_STATUS
MeshRoutingTable_Exit(
	IN PRTMP_ADAPTER	pAd);

PMESH_ROUTING_ENTRY
MeshRoutingTableLookup(
    IN  PRTMP_ADAPTER   pAd, 
    IN  PUCHAR          pAddr);

PMESH_ROUTING_ENTRY
MeshRoutingTableInsert(
    IN  PRTMP_ADAPTER   pAd,
    IN	PUCHAR			pMeshDestAddr,
    IN	UINT32			Dsn,
    IN	PUCHAR			pNextHop,
    IN	UCHAR			Wcid,
    IN	UINT32			Metric);

BOOLEAN
MeshRoutingTableDelete(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr);

PMESH_ROUTING_ENTRY
MeshRoutingTableUpdate(
	IN PRTMP_ADAPTER pAd,
    IN	PUCHAR			MeshDestAddr,
    IN	UINT32			Dsn,
    IN	PUCHAR			NextHop,
    IN	UCHAR			NextHopLinkID,
    IN	UINT32			Metric);

PMESH_ROUTING_ENTRY
MeshRoutingTablePrecursorUpdate(
	IN PRTMP_ADAPTER	pAd,
	IN	PUCHAR		MeshDestAddr,
	IN	PUCHAR		Precursor);

NDIS_STATUS
MeshEntryTable_Init(
	IN PRTMP_ADAPTER	pAd);

NDIS_STATUS
MeshEntryTable_Exit(
	IN PRTMP_ADAPTER	pAd);

PMESH_ENTRY
MeshEntryTableLookUp(
    IN  PRTMP_ADAPTER   pAd,
	IN PUCHAR			DestAddr);

PMESH_ENTRY
MeshEntryTableInsert(
    IN  PRTMP_ADAPTER   pAd,
    IN  PUCHAR          DestAddr,
    IN	UCHAR			Idx);

BOOLEAN
MeshEntryTableDelete(
    IN  PRTMP_ADAPTER   pAd, 
	IN PUCHAR			DestAddr);

PMESH_ENTRY
MeshEntryTableUpdate(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			DestAddr,
	IN UCHAR			Idx);

VOID
MeshEntryTableGet(
	IN PRTMP_ADAPTER	pAd);

NDIS_STATUS
MeshProxyEntryTable_Init(
	IN PRTMP_ADAPTER	pAd);

NDIS_STATUS
MeshProxyEntryTable_Exit(
	IN PRTMP_ADAPTER	pAd);

PMESH_PROXY_ENTRY
MeshProxyEntryTableLookUp(
    IN  PRTMP_ADAPTER   pAd,
	IN	PUCHAR			pSA);

PMESH_PROXY_ENTRY
MeshProxyEntryTableInsert(
    IN  PRTMP_ADAPTER   pAd,
    IN  PUCHAR          pMeshSA,
    IN  PUCHAR          pSA);

BOOLEAN
MeshProxyEntryTableDelete(
    IN  PRTMP_ADAPTER   pAd, 
	IN	PUCHAR			pSA);

PMESH_PROXY_ENTRY
MeshProxyEntryTableUpdate(
	IN PRTMP_ADAPTER	pAd,
	IN  PUCHAR          pMeshSA,
	IN	PUCHAR			pSA);

VOID
MeshCreatePreqAction(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pProxyAddr,
	IN PUCHAR pDestAddr);

VOID
MeshPathReqTimeoutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID
MeshCreatePerrAction(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBreakMeshPoint);

BOOLEAN
MeshAllowToSendPathResponse(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR		pSA);

VOID
MeshMultipathNotice(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR	pPeerMac,
	IN PUCHAR pMeshSA,
	IN UCHAR		SwitchOn);

VOID
MeshMultipathNoticeRcvProcess(
	IN PRTMP_ADAPTER pAd,
	IN	RX_BLK	*pRxBlk);

VOID
MeshChSwAnnounceProcess(
	IN PRTMP_ADAPTER pAd,
	IN RX_BLK *pRxBlk);

LONG
PathRouteIDSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR	DestAddr);

UCHAR
PathMeshLinkIDSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	RouteIdx);

PUCHAR
PathRouteAddrSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	RouteIdx);

UINT GetMeshHederLen(
	IN PUCHAR pSrcBufVA);

UINT8 GetMeshFlag(
	IN PUCHAR pSrcBufVA);

UINT8 GetMeshFlagAE(
	IN PUCHAR pSrcBufVA);

UINT8 GetMeshTTL(
	IN PUCHAR pSrcBufVA);

UINT32 GetMeshSeq(
	IN PUCHAR pSrcBufVA);

PUCHAR GetMeshAddr4(
	IN PUCHAR pSrcBufVA);

PUCHAR GetMeshAddr5(
	IN PUCHAR pSrcBufVA);

PUCHAR GetMeshAddr6(
	IN PUCHAR pSrcBufVA);

ULONG GetMeshLinkId(
	IN PRTMP_ADAPTER pAd,
	IN PCHAR PeerMacAddr);

VOID MeshClonePacket(
	IN PRTMP_ADAPTER pAd,
	IN PNDIS_PACKET pPacket,
	IN UINT8 MeshSrc,
	IN UINT8 MeshLinkIdx);

VOID MultipathPoolInit(
	IN PRTMP_ADAPTER pAd);

VOID MultipathPoolExit(
	IN PRTMP_ADAPTER pAd);

PMESH_MULTIPATH_ENTRY MultipathEntyAlloc(
	IN PRTMP_ADAPTER pAd);

VOID MultipathEntyFree(
	IN PRTMP_ADAPTER pAd,
	IN PMESH_MULTIPATH_ENTRY pEntry);

VOID MultipathEntryInsert(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx,
	IN PUCHAR pMac);

VOID MultipathEntryDelete(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx,
	IN PUCHAR pMac);

VOID MultipathListDelete(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx);

PMESH_MULTIPATH_ENTRY MultipathEntryLookUp(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx,
	IN PUCHAR pMac);

VOID MultipathEntryMaintain(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR LinkIdx);

VOID MeshDataPktProcess(
	IN PRTMP_ADAPTER pAd,
	IN PNDIS_PACKET pPacket,
	IN USHORT MeshLinkIdx,
	OUT PNDIS_PACKET *pMeshForwardPacket,
	OUT BOOLEAN *pbDirectForward,
	OUT BOOLEAN *pbAnnounce);

ULONG InsertPktMeshHeader(
	IN PRTMP_ADAPTER pAd,
	IN TX_BLK *pTxBlk, 
	IN PUCHAR *pHeaderBufPtr);

UINT32 RandomMeshCPI(
	IN PRTMP_ADAPTER pAd);

UINT16 RandomLinkId(
	IN PRTMP_ADAPTER pAd);

UINT8 RandomChSwWaitTime(
	IN PRTMP_ADAPTER pAd);

void rtmp_read_mesh_from_file(	
	IN  PRTMP_ADAPTER pAd,	
	char *tmpbuf,	
	char *buffer);

VOID BMPktSigTabInit(
	IN PRTMP_ADAPTER pAd);

VOID BMPktSigTabExit(
	IN PRTMP_ADAPTER pAd);

PMESH_BMPKTSIG_ENTRY BMPktSigTabLookUp(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			MeshSA);

PMESH_BMPKTSIG_ENTRY BMPktSigTabInsert(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			MeshSA);

VOID BMPktSigTabDelete(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			MeshSA);

BOOLEAN PktSigCheck(
	IN PRTMP_ADAPTER	pAd,
	IN PUCHAR			pTA,
	IN PUCHAR			pDA,
	IN PUCHAR			pSA,
	IN UINT32			MeshSeq,
	IN ULONG			FrameType);

VOID LocalMsaIeInit(
	IN PRTMP_ADAPTER pAd,
	IN INT			 idx);

VOID RTMPIoctlQueryMeshSecurityInfo(
		IN PRTMP_ADAPTER pAd, 
		IN struct iwreq *wrq);

INT RTMPIoctlSetMeshSecurityInfo(
		IN PRTMP_ADAPTER pAd, 
		IN struct iwreq *wrq);

UCHAR MeshCheckPeerMpCipher(
		IN USHORT 		 CapabilityInfo, 
		IN PUCHAR 		 pVIE,
		IN UCHAR		 LenVIE);

BOOLEAN MeshAllowToSendPacket(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	OUT UCHAR		*pWcid);

VOID PerpareMeshHeader(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	OUT PMESH_FLAG pMeshFlag,
	OUT UINT16 *pMeshTTL,
	OUT UINT32 *pMeshSeq,
	OUT PUCHAR *ppMeshAddr5,
	OUT PUCHAR *ppMeshAddr6);

BOOLEAN MeshChCheck(
	IN RTMP_ADAPTER *pAd,
	IN PMESH_NEIGHBOR_ENTRY pNeighborEntry);

VOID MeshMakeBeacon(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx);

VOID MeshUpdateBeaconFrame(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx);

VOID MeshCleanBeaconFrame(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx);

VOID AsicEnableMESHSync(
	IN PRTMP_ADAPTER pAd);


#endif // __MESH_H__ //

