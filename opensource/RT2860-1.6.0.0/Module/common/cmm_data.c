#include "rt_config.h"

#define MAX_TX_IN_TBTT		(16)


#define GET_TXRING_FREENO(_pAd, _QueIdx) \
	(_pAd->TxRing[_QueIdx].TxSwFreeIdx > _pAd->TxRing[QueIdx].TxCpuIdx)	? \
			(_pAd->TxRing[_QueIdx].TxSwFreeIdx - _pAd->TxRing[_QueIdx].TxCpuIdx - 1) \
			 :	\
			(_pAd->TxRing[_QueIdx].TxSwFreeIdx + TX_RING_SIZE - _pAd->TxRing[_QueIdx].TxCpuIdx - 1);


#define GET_MGMTRING_FREENO(_pAd) \
	(_pAd->MgmtRing.TxSwFreeIdx > _pAd->MgmtRing.TxCpuIdx)	? \
			(_pAd->MgmtRing.TxSwFreeIdx - _pAd->MgmtRing.TxCpuIdx - 1) \
			 :	\
			(_pAd->MgmtRing.TxSwFreeIdx + MGMT_RING_SIZE - _pAd->MgmtRing.TxCpuIdx - 1);

UCHAR	SNAP_802_1H[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
UCHAR	SNAP_BRIDGE_TUNNEL[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8};
// Add Cisco Aironet SNAP heade for CCX2 support
UCHAR	SNAP_AIRONET[] = {0xaa, 0xaa, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
UCHAR	CKIP_LLC_SNAP[] = {0xaa, 0xaa, 0x03, 0x00, 0x40, 0x96, 0x00, 0x02};
UCHAR	EAPOL_LLC_SNAP[]= {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e};
UCHAR	EAPOL[] = {0x88, 0x8e};
UCHAR   TPID[] = {0x81, 0x00}; /* VLAN related */

UCHAR	IPX[] = {0x81, 0x37};
UCHAR	APPLE_TALK[] = {0x80, 0xf3};
UCHAR	RateIdToPlcpSignal[12] = { 
	 0, /* RATE_1 */	1, /* RATE_2 */ 	2, /* RATE_5_5 */	3, /* RATE_11 */	// see BBP spec
	11, /* RATE_6 */   15, /* RATE_9 */    10, /* RATE_12 */   14, /* RATE_18 */	// see IEEE802.11a-1999 p.14
	 9, /* RATE_24 */  13, /* RATE_36 */	8, /* RATE_48 */   12  /* RATE_54 */ }; // see IEEE802.11a-1999 p.14

UCHAR	 OfdmSignalToRateId[16] = {
	RATE_54,  RATE_54,	RATE_54,  RATE_54,	// OFDM PLCP Signal = 0,  1,  2,  3 respectively
	RATE_54,  RATE_54,	RATE_54,  RATE_54,	// OFDM PLCP Signal = 4,  5,  6,  7 respectively
	RATE_48,  RATE_24,	RATE_12,  RATE_6,	// OFDM PLCP Signal = 8,  9,  10, 11 respectively
	RATE_54,  RATE_36,	RATE_18,  RATE_9,	// OFDM PLCP Signal = 12, 13, 14, 15 respectively
};

UCHAR	 OfdmRateToRxwiMCS[12] = {
	0,  0,	0,  0,	 
	0,  1,	2,  3,	// OFDM rate 6,9,12,18 = rxwi mcs 0,1,2,3
	4,  5,	6,  7,	// OFDM rate 24,36,48,54 = rxwi mcs 4,5,6,7
};
UCHAR	 RxwiMCSToOfdmRate[12] = {
	RATE_6,  RATE_9,	RATE_12,  RATE_18,	 
	RATE_24,  RATE_36,	RATE_48,  RATE_54,	// OFDM rate 6,9,12,18 = rxwi mcs 0,1,2,3
	4,  5,	6,  7,	// OFDM rate 24,36,48,54 = rxwi mcs 4,5,6,7
};

char*   MCSToMbps[] = {"1Mbps","2Mbps","5.5Mbps","11Mbps","06Mbps","09Mbps","12Mbps","18Mbps","24Mbps","36Mbps","48Mbps","54Mbps","MM-0","MM-1","MM-2","MM-3","MM-4","MM-5","MM-6","MM-7","MM-8","MM-9","MM-10","MM-11","MM-12","MM-13","MM-14","MM-15","MM-32","ee1","ee2","ee3"};

UCHAR default_cwmin[]={CW_MIN_IN_BITS, CW_MIN_IN_BITS, CW_MIN_IN_BITS-1, CW_MIN_IN_BITS-2};
UCHAR default_cwmax[]={CW_MAX_IN_BITS, CW_MAX_IN_BITS, CW_MIN_IN_BITS, CW_MIN_IN_BITS-1};
UCHAR default_sta_aifsn[]={3,7,2,2};

UCHAR MapUserPriorityToAccessCategory[8] = {QID_AC_BE, QID_AC_BK, QID_AC_BK, QID_AC_BE, QID_AC_VI, QID_AC_VI, QID_AC_VO, QID_AC_VO};

/*
	========================================================================

	Routine Description:
		API for MLME to transmit management frame to AP (BSS Mode)
	or station (IBSS Mode)
		
	Arguments:
		pAd Pointer to our adapter
		pData		Pointer to the outgoing 802.11 frame
		Length		Size of outgoing management frame
		
	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
NDIS_STATUS MiniportMMRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			QueIdx,
	IN	PUCHAR			pData,
	IN	UINT			Length)
{
	PNDIS_PACKET	pPacket;
	NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;
	ULONG	 FreeNum;
	TXWI_STRUC		TXWI;
	ULONG	SW_TX_IDX; //TEMP, SW_TX_IDX;
	PTXD_STRUC		pTxD;
	unsigned long	IrqFlags = 0;
	UCHAR			IrqState;
	
	QueIdx=3;
	
	ASSERT(Length <= MGMT_DMA_BUFFER_SIZE);

	// 2860C use Tx Ring
	
	IrqState = pAd->irq_disabled;
	if ((pAd->MACVersion == 0x28600100) && (!IrqState))
		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	do 
	{
		// Reset is in progress, stop immediately
		if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
			 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)||
			 !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
	{
			Status = NDIS_STATUS_FAILURE;
			break;
	}

		// Check Free priority queue
		// Since we use PBF Queue2 for management frame.  Its corresponding DMA ring should be using TxRing.
	
	// 2860C use Tx Ring
	if (pAd->MACVersion == 0x28600100)
	{
		FreeNum = GET_TXRING_FREENO(pAd, QueIdx);
		SW_TX_IDX = pAd->TxRing[QueIdx].TxCpuIdx;
		pTxD  = (PTXD_STRUC) pAd->TxRing[QueIdx].Cell[SW_TX_IDX].AllocVa;
	}
	else
	{
		FreeNum = GET_MGMTRING_FREENO(pAd);		
		SW_TX_IDX = pAd->MgmtRing.TxCpuIdx;
		pTxD  = (PTXD_STRUC) pAd->MgmtRing.Cell[SW_TX_IDX].AllocVa;
	}
		if ((FreeNum > 0))
			{
			NdisZeroMemory(&TXWI, TXWI_SIZE);
			Status = RTMPAllocateNdisPacket(pAd, &pPacket, (PUCHAR)&TXWI, TXWI_SIZE, pData, Length);
			if (Status != NDIS_STATUS_SUCCESS)
				{					
				DBGPRINT(RT_DEBUG_WARN, ("MiniportMMRequest (error:: can't allocate NDIS PACKET)\n"));
					break;
				}

			//pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
			//pAd->CommonCfg.MlmeRate = RATE_2; 			

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
			UAPSD_MR_QOS_NULL_HANDLE(pAd, pData, pPacket);
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

			Status = MlmeHardTransmit(pAd, QueIdx, pPacket);
				if (Status != NDIS_STATUS_SUCCESS)
				RTMPFreeNdisPacket(pAd, pPacket);
			}
		else
			{
			pAd->RalinkCounters.MgmtRingFullCount++;
			DBGPRINT(RT_DEBUG_ERROR, ("Qidx(%d), not enough space in MgmtRing\n", QueIdx));
		}

	} while (FALSE);

	// 2860C use Tx Ring
	if ((pAd->MACVersion == 0x28600100) && (!IrqState))
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	return Status;
}


NDIS_STATUS MiniportMMRequestUnlock(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			QueIdx,
	IN	PUCHAR			pData,
	IN	UINT			Length)
{
	PNDIS_PACKET	pPacket;
	NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;
	ULONG	 FreeNum;
	TXWI_STRUC		TXWI;
	ULONG	SW_TX_IDX; //TEMP, SW_TX_IDX;
	PTXD_STRUC		pTxD;
	//ULONG			IrqFlags = 0;

	QueIdx = 3;
	ASSERT(Length <= MGMT_DMA_BUFFER_SIZE);

	do 
	{
		// Reset is in progress, stop immediately
		if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
			 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)||
			 !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		// Check Free priority queue
		// Since we use PBF Queue2 for management frame.  Its corresponding DMA ring should be using TxRing.
		// 2860C use Tx Ring
		if (pAd->MACVersion == 0x28600100)
		{	
			FreeNum = GET_TXRING_FREENO(pAd, QueIdx);
			SW_TX_IDX = pAd->TxRing[QueIdx].TxCpuIdx;
			pTxD  = (PTXD_STRUC) pAd->TxRing[QueIdx].Cell[SW_TX_IDX].AllocVa;
		}
		else
		{
			FreeNum = GET_MGMTRING_FREENO(pAd);		
			SW_TX_IDX = pAd->MgmtRing.TxCpuIdx;
			pTxD  = (PTXD_STRUC) pAd->MgmtRing.Cell[SW_TX_IDX].AllocVa;
		}
		if ((FreeNum > 0))
			{
			NdisZeroMemory(&TXWI, TXWI_SIZE);
			Status = RTMPAllocateNdisPacket(pAd, &pPacket, (PUCHAR)&TXWI, TXWI_SIZE, pData, Length);
			if (Status != NDIS_STATUS_SUCCESS)
				{					
				DBGPRINT(RT_DEBUG_WARN, ("MiniportMMRequest (error:: can't allocate NDIS PACKET)\n"));
					break;
				}

			//pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
			//pAd->CommonCfg.MlmeRate = RATE_2; 			

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
			UAPSD_MR_QOS_NULL_HANDLE(pAd, pData, pPacket);
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

			Status = MlmeHardTransmit(pAd, QueIdx, pPacket);
				if (Status != NDIS_STATUS_SUCCESS)
				RTMPFreeNdisPacket(pAd, pPacket);
			}
			else
			{
			pAd->RalinkCounters.MgmtRingFullCount++;
			DBGPRINT(RT_DEBUG_ERROR, ("Qidx(%d), not enough space in MgmtRing\n", QueIdx));
		}

	} while (FALSE);


	return Status;
}

/*
	========================================================================

	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware transmit function
	
	Arguments:
		pAd Pointer to our adapter
		pBuffer 	Pointer to	memory of outgoing frame
		Length		Size of outgoing management frame
		
	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
NDIS_STATUS MlmeHardTransmit(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	QueIdx,
	IN	PNDIS_PACKET	pPacket)
{	
	if ((pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
#ifdef CARRIER_DETECTION_SUPPORT
		||(isCarrierDetectExist(pAd) == TRUE)
#endif // CARRIER_DETECTION_SUPPORT //
		)
	{
		return NDIS_STATUS_FAILURE;
	}

	if ( pAd->MACVersion == 0x28600100 )
		return MlmeHardTransmitTxRing(pAd,QueIdx,pPacket);
	else
		return MlmeHardTransmitMgmtRing(pAd,QueIdx,pPacket);

}

NDIS_STATUS MlmeHardTransmitTxRing(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	QueIdx,
	IN	PNDIS_PACKET	pPacket)
{
	PACKET_INFO 	PacketInfo;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	PHEADER_802_11	pHeader_802_11;
	BOOLEAN 		bAckRequired, bInsertTimestamp;
	ULONG			SrcBufPA;
	//UCHAR			TxBufIdx;
	UCHAR			MlmeRate;
	ULONG			SwIdx = pAd->TxRing[QueIdx].TxCpuIdx;
	PTXWI_STRUC 	pFirstTxWI;
	//ULONG	i;
	//HTTRANSMIT_SETTING	MlmeTransmit;   //Rate for this MGMT frame.
	ULONG	 FreeNum;


#if 0	 	  
	// only one buffer is used in MGMT packet	
	NdisQueryPacket(
		pPacket,							// Ndis packet
		&PacketInfo.PhysicalBufferCount,	// Physical buffer count
		&PacketInfo.BufferCount,			// Number of buffer descriptor
		&PacketInfo.pFirstBuffer,			// Pointer to first buffer descripotr
		&PacketInfo.TotalPacketLength); 	// Ndis packet length

	NDIS_QUERY_BUFFER(PacketInfo.pFirstBuffer, &pSrcBufVA, &SrcBufLen);
	// can't get scatter-gather information from internal created NDIS PACKET via
	// NDIS_PER_PACKET_INFO_FROM_PACKET(pPacket, ScatterGatherListPacketInfo).
	// However we can get that information from the local TxBuf
	//
	TxBufIdx = RTMP_GET_PACKET_SOURCE(pPacket);
	ASSERT(TxBufIdx < NUM_OF_LOCAL_TXBUF);
	SrcBufPA = RTMP_GetPhysicalAddressLow(pAd->LocalTxBuf[TxBufIdx].AllocPa);
#else	
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);	
#endif


	if (pSrcBufVA == NULL)
	{
		// The buffer shouldn't be NULL
		return NDIS_STATUS_FAILURE;
	}

	// Make sure MGMT ring resource won't be used by other threads
	//NdisAcquireSpinLock(&pAd->TxRingLock);

	FreeNum = GET_TXRING_FREENO(pAd, QueIdx);

	if (FreeNum == 0)
	{
		//NdisReleaseSpinLock(&pAd->TxRingLock);
		return NDIS_STATUS_FAILURE;
	}

	SwIdx = pAd->TxRing[QueIdx].TxCpuIdx;

#ifndef BIG_ENDIAN	
	pTxD  = (PTXD_STRUC) pAd->TxRing[QueIdx].Cell[SwIdx].AllocVa;
#else
    pDestTxD  = (PTXD_STRUC)pAd->TxRing[QueIdx].Cell[SwIdx].AllocVa;
    TxD = *pDestTxD;
    pTxD = &TxD;
    RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif

	if (pAd->TxRing[QueIdx].Cell[SwIdx].pNdisPacket)
	{
		printk("MlmeHardTransmit Error\n");
		//NdisReleaseSpinLock(&pAd->TxRingLock);
		return NDIS_STATUS_FAILURE;
	}



	// outgoing frame always wakeup PHY to prevent frame lost 
	// if (pAd->StaCfg.Psm == PWR_SAVE)
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
		AsicForceWakeup(pAd);

	pFirstTxWI	=(PTXWI_STRUC)pSrcBufVA;
	
	pHeader_802_11			 = (PHEADER_802_11) (pSrcBufVA + TXWI_SIZE);
	if (pHeader_802_11->Addr1[0] & 0x01)
	{
		MlmeRate = pAd->CommonCfg.BasicMlmeRate;
	}
	else
	{
		MlmeRate = pAd->CommonCfg.MlmeRate;
	}
	
	// Verify Mlme rate for a / g bands.	
	if ((pAd->LatchRfRegs.Channel > 14) && (MlmeRate < RATE_6)) // 11A band
		MlmeRate = RATE_6;

	//
	// Should not be hard code to set PwrMgmt to 0 (PWR_ACTIVE)
	// Snice it's been set to 0 while on MgtMacHeaderInit
	// By the way this will cause frame to be send on PWR_SAVE failed.
	// 
	// pHeader_802_11->FC.PwrMgmt = 0; // (pAd->StaCfg.Psm == PWR_SAVE);
	//
	// In WMM-UAPSD, mlme frame should be set psm as power saving but probe request frame
    	if ((pHeader_802_11->FC.SubType == SUBTYPE_PROBE_REQ) || !(pAd->CommonCfg.bAPSDCapable && pAd->CommonCfg.APEdcaParm.bAPSDCapable))
    	{
    		pHeader_802_11->FC.PwrMgmt = PWR_ACTIVE;
    	}
    	else
    	{
    		pHeader_802_11->FC.PwrMgmt = pAd->CommonCfg.bAPSDForcePowerSave;
    	}
	
	bInsertTimestamp = FALSE;
	if (pHeader_802_11->FC.Type == BTYPE_CNTL) // must be PS-POLL
	{
		bAckRequired = FALSE;
	}
	else // BTYPE_MGMT or BTYPE_DATA(must be NULL frame)
	{
		if (pHeader_802_11->Addr1[0] & 0x01) // MULTICAST, BROADCAST
		{
			bAckRequired = FALSE;
			pHeader_802_11->Duration = 0;
		}
		else
		{
			bAckRequired = TRUE;
			pHeader_802_11->Duration = RTMPCalcDuration(pAd, MlmeRate, 14);
			if (pHeader_802_11->FC.SubType == SUBTYPE_PROBE_RSP)
			{
				bInsertTimestamp = TRUE;
			}
		}
	}
	pHeader_802_11->Sequence = pAd->Sequence++;
	if (pAd->Sequence > 0xfff)
		pAd->Sequence = 0;
	// Before radar detection done, mgmt frame can not be sent but probe req
	// Because we need to use probe req to trigger driver to send probe req in passive scan
	if ((pHeader_802_11->FC.SubType != SUBTYPE_PROBE_REQ)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE))
	{
		DBGPRINT(RT_DEBUG_ERROR,("MlmeHardTransmit --> radar detect not in normal mode !!!\n"));
		//NdisReleaseSpinLock(&pAd->TxRingLock);
		return (NDIS_STATUS_FAILURE);
	}

#ifdef BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (PUCHAR)pHeader_802_11, DIR_WRITE, FALSE);
#endif
	//
	// fill scatter-and-gather buffer list into TXD. Internally created NDIS PACKET
	// should always has only one ohysical buffer, and the whole frame size equals
	// to the first scatter buffer size
	//

	// Initialize TX Descriptor
	// For inter-frame gap, the number is for this frame and next frame
	// For MLME rate, we will fix as 2Mb to match other vendor's implement
//	pAd->CommonCfg.MlmeTransmit.field.MODE = 1;
	
// management frame doesn't need encryption. so use RESERVED_WCID no matter u are sending to specific wcid or not.
	// Only beacon use Nseq=TRUE. So here we use Nseq=FALSE.
	RTMPWriteTxWI(pAd, pFirstTxWI, FALSE, FALSE, bInsertTimestamp, FALSE, bAckRequired, FALSE, 
		0, RESERVED_WCID, (SrcBufLen - TXWI_SIZE), PID_MGMT, 0,  (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_BACKOFF, FALSE, &pAd->CommonCfg.MlmeTransmit);
	pAd->TxRing[QueIdx].Cell[SwIdx].pNdisPacket = pPacket;
	pAd->TxRing[QueIdx].Cell[SwIdx].pNextNdisPacket = NULL;
//	pFirstTxWI->MPDUtotalByteCount = SrcBufLen - TXWI_SIZE;
#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)pFirstTxWI, TYPE_TXWI);
#endif
	SrcBufPA = PCI_MAP_SINGLE(pAd, pSrcBufVA, SrcBufLen, PCI_DMA_TODEVICE);


	RTMPWriteTxDescriptor(pAd, pTxD, TRUE, FIFO_EDCA);
	pTxD->LastSec0 = 1;
	pTxD->LastSec1 = 1;
	pTxD->SDLen0 = SrcBufLen;
	pTxD->SDLen1 = 0;
	pTxD->SDPtr0 = SrcBufPA;
	pTxD->DMADONE = 0;

#ifdef BIG_ENDIAN
    RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif

	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;

   	// Increase TX_CTX_IDX, but write to register later.
	INC_RING_INDEX(pAd->TxRing[QueIdx].TxCpuIdx, TX_RING_SIZE);

	RTMP_IO_WRITE32(pAd, TX_CTX_IDX0 + QueIdx*0x10,  pAd->TxRing[QueIdx].TxCpuIdx);	

   	// Make sure to release MGMT ring resource
//	NdisReleaseSpinLock(&pAd->TxRingLock);

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS MlmeHardTransmitMgmtRing(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	QueIdx,
	IN	PNDIS_PACKET	pPacket)
{
	PACKET_INFO 	PacketInfo;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	PHEADER_802_11	pHeader_802_11;
	BOOLEAN 		bAckRequired, bInsertTimestamp;
	ULONG			SrcBufPA;
	//UCHAR			TxBufIdx;
	UCHAR			MlmeRate;
	ULONG			SwIdx = pAd->MgmtRing.TxCpuIdx;
	PTXWI_STRUC 	pFirstTxWI;
	UCHAR			IrqState;
	//ULONG	i;
	//HTTRANSMIT_SETTING	MlmeTransmit;   //Rate for this MGMT frame.

#if 0	 	  
	// only one buffer is used in MGMT packet	
	NdisQueryPacket(
		pPacket,							// Ndis packet
		&PacketInfo.PhysicalBufferCount,	// Physical buffer count
		&PacketInfo.BufferCount,			// Number of buffer descriptor
		&PacketInfo.pFirstBuffer,			// Pointer to first buffer descripotr
		&PacketInfo.TotalPacketLength); 	// Ndis packet length

	NDIS_QUERY_BUFFER(PacketInfo.pFirstBuffer, &pSrcBufVA, &SrcBufLen);
	// can't get scatter-gather information from internal created NDIS PACKET via
	// NDIS_PER_PACKET_INFO_FROM_PACKET(pPacket, ScatterGatherListPacketInfo).
	// However we can get that information from the local TxBuf
	//
	TxBufIdx = RTMP_GET_PACKET_SOURCE(pPacket);
	ASSERT(TxBufIdx < NUM_OF_LOCAL_TXBUF);
	SrcBufPA = RTMP_GetPhysicalAddressLow(pAd->LocalTxBuf[TxBufIdx].AllocPa);
#else	
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);	
#endif

	// Make sure MGMT ring resource won't be used by other threads
	IrqState = pAd->irq_disabled;
	if (!IrqState)
		RTMP_SEM_LOCK(&pAd->MgmtRingLock);

#ifndef BIG_ENDIAN		
	pTxD  = (PTXD_STRUC) pAd->MgmtRing.Cell[SwIdx].AllocVa;
#else
    pDestTxD  = (PTXD_STRUC)pAd->MgmtRing.Cell[SwIdx].AllocVa;
    TxD = *pDestTxD;
    pTxD = &TxD;
    RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif

	if (pSrcBufVA == NULL)
	{
		// The buffer shouldn't be NULL
		if (!IrqState)
			RTMP_SEM_UNLOCK(&pAd->MgmtRingLock);
		return NDIS_STATUS_FAILURE;
	}

	// outgoing frame always wakeup PHY to prevent frame lost 
	// if (pAd->StaCfg.Psm == PWR_SAVE)
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
		AsicForceWakeup(pAd);

	pFirstTxWI	=(PTXWI_STRUC)pSrcBufVA;
	
	pHeader_802_11			 = (PHEADER_802_11) (pSrcBufVA + TXWI_SIZE);
	if (pHeader_802_11->Addr1[0] & 0x01)
	{
		MlmeRate = pAd->CommonCfg.BasicMlmeRate;
	}
	else
	{
		MlmeRate = pAd->CommonCfg.MlmeRate;
	}
	
	// Verify Mlme rate for a / g bands.	
	if ((pAd->LatchRfRegs.Channel > 14) && (MlmeRate < RATE_6)) // 11A band
		MlmeRate = RATE_6;


	//
	// Should not be hard code to set PwrMgmt to 0 (PWR_ACTIVE)
	// Snice it's been set to 0 while on MgtMacHeaderInit
	// By the way this will cause frame to be send on PWR_SAVE failed.
	// 
	// pHeader_802_11->FC.PwrMgmt = 0; // (pAd->StaCfg.Psm == PWR_SAVE);
	//
	// In WMM-UAPSD, mlme frame should be set psm as power saving but probe request frame
    	if ((pHeader_802_11->FC.SubType == SUBTYPE_PROBE_REQ) || !(pAd->CommonCfg.bAPSDCapable && pAd->CommonCfg.APEdcaParm.bAPSDCapable))
    	{
    		pHeader_802_11->FC.PwrMgmt = PWR_ACTIVE;
    	}
    	else
    	{
    		pHeader_802_11->FC.PwrMgmt = pAd->CommonCfg.bAPSDForcePowerSave;
    	}
	
	bInsertTimestamp = FALSE;
	if (pHeader_802_11->FC.Type == BTYPE_CNTL) // must be PS-POLL
	{
		bAckRequired = FALSE;
	}
	else // BTYPE_MGMT or BTYPE_DATA(must be NULL frame)
	{
		//pAd->Sequence++;
		//pHeader_802_11->Sequence = pAd->Sequence;

		if (pHeader_802_11->Addr1[0] & 0x01) // MULTICAST, BROADCAST
		{
			bAckRequired = FALSE;
			pHeader_802_11->Duration = 0;
		}
		else
		{
			bAckRequired = TRUE;
			pHeader_802_11->Duration = RTMPCalcDuration(pAd, MlmeRate, 14);
			if (pHeader_802_11->FC.SubType == SUBTYPE_PROBE_RSP)
			{
				bInsertTimestamp = TRUE;
			}
		}
	}


	pHeader_802_11->Sequence = pAd->Sequence++;
	if (pAd->Sequence >0xfff)
		pAd->Sequence = 0;

	// Before radar detection done, mgmt frame can not be sent but probe req
	// Because we need to use probe req to trigger driver to send probe req in passive scan
	if ((pHeader_802_11->FC.SubType != SUBTYPE_PROBE_REQ)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE))
	{
		DBGPRINT(RT_DEBUG_ERROR,("MlmeHardTransmit --> radar detect not in normal mode !!!\n"));
		if (!IrqState)
			RTMP_SEM_UNLOCK(&pAd->MgmtRingLock);
		return (NDIS_STATUS_FAILURE);
	}

#ifdef BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (PUCHAR)pHeader_802_11, DIR_WRITE, FALSE);
#endif
	//
	// fill scatter-and-gather buffer list into TXD. Internally created NDIS PACKET
	// should always has only one ohysical buffer, and the whole frame size equals
	// to the first scatter buffer size
	//

	// Initialize TX Descriptor
	// For inter-frame gap, the number is for this frame and next frame
	// For MLME rate, we will fix as 2Mb to match other vendor's implement
//	pAd->CommonCfg.MlmeTransmit.field.MODE = 1;
	
// management frame doesn't need encryption. so use RESERVED_WCID no matter u are sending to specific wcid or not.
	RTMPWriteTxWI(pAd, pFirstTxWI, FALSE, FALSE, bInsertTimestamp, FALSE, bAckRequired, FALSE, 
		0, RESERVED_WCID, (SrcBufLen - TXWI_SIZE), PID_MGMT, 0,  (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_BACKOFF, FALSE, &pAd->CommonCfg.MlmeTransmit);
//	pFirstTxWI->MPDUtotalByteCount = SrcBufLen - TXWI_SIZE;
pAd->MgmtRing.Cell[SwIdx].pNdisPacket = pPacket;			  
	pAd->MgmtRing.Cell[SwIdx].pNextNdisPacket = NULL;	  
//	pFirstTxWI->MPDUtotalByteCount = SrcBufLen - TXWI_SIZE;
#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)pFirstTxWI, TYPE_TXWI);
#endif
	SrcBufPA = PCI_MAP_SINGLE(pAd, pSrcBufVA, SrcBufLen, PCI_DMA_TODEVICE);


	RTMPWriteTxDescriptor(pAd, pTxD, TRUE, FIFO_MGMT);
	pTxD->LastSec0 = 1;
	pTxD->LastSec1 = 1;
	pTxD->DMADONE = 0;
	pTxD->SDLen0 = SrcBufLen;
	pTxD->SDLen1 = 0;
	pTxD->SDPtr0 = SrcBufPA;
#ifdef BIG_ENDIAN
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif
	
//==================================================================
/*	DBGPRINT_RAW(RT_DEBUG_TRACE, ("MLMEHardTransmit\n"));
	for (i = 0; i < (TXWI_SIZE+24); i++)
	{
	
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%x:", *(pSrcBufVA+i)));
		if ( i%4 == 3)
			DBGPRINT_RAW(RT_DEBUG_TRACE, (" :: "));
		if ( i%16 == 15)
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n      "));
	}
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n      "));*/
//=======================================================================

	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;

	// Increase TX_CTX_IDX, but write to register later.
	INC_RING_INDEX(pAd->MgmtRing.TxCpuIdx, MGMT_RING_SIZE);

	RTMP_IO_WRITE32(pAd, TX_MGMTCTX_IDX,  pAd->MgmtRing.TxCpuIdx);
	// Make sure to release MGMT ring resource
	if (!IrqState)
		RTMP_SEM_UNLOCK(&pAd->MgmtRingLock);
	return NDIS_STATUS_SUCCESS;
}

/*
	========================================================================

	Routine Description:
		To do the enqueue operation and extract the first item of waiting 
		list. If a number of available shared memory segments could meet 
		the request of extracted item, the extracted item will be fragmented
		into shared memory segments.
	
	Arguments:
		pAd Pointer to our adapter
		pQueue		Pointer to Waiting Queue
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
VOID    RTMPDeQueuePacket(
						 IN  PRTMP_ADAPTER   pAd,
						 IN  BOOLEAN         bIntContext,
						 IN  UCHAR           Max_Tx_Packets)
{
	PQUEUE_ENTRY    pEntry;
	PNDIS_PACKET pPacket;
	UCHAR           MpduRequired;
	NDIS_STATUS     Status;
	UCHAR           Count=0;
	PQUEUE_HEADER   pQueue;
	ULONG           FreeNumber[NUM_OF_TX_RING]; //Number, FreeNumber[NUM_OF_TX_RING];
	CHAR			QueIdx;
	unsigned long	IrqFlags = 0;
	UCHAR 			TxRate;

	DBGPRINT(RT_DEBUG_INFO,("RTMPDeQueuePacket (Tx:%d)--> \n", Max_Tx_Packets));

	//Max_Tx_Packets = 32;

	for (QueIdx=0; QueIdx<=3; QueIdx++) 
	{
		Count=0;
	while (1)
	{
		if ((RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) ||
			(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))            ||
			(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS))   ||
			(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)))
			return;

		if (Count >= Max_Tx_Packets)
		{
			break;
		}

		if (bIntContext == FALSE)
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

    		//if ((pQueue = RTMPCheckTxSwQueue(pAd, &QueIdx)) != NULL)
    		pQueue = &pAd->TxSwQueue[QueIdx]; 
    		if (pQueue->Head != NULL)
		{
			// probe the Queue Head
			pEntry = pQueue->Head;
		}
		else
		{
			if (bIntContext == FALSE)
				RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
			break;
		}       

		// static rate also need NICUpdateFifoStaCounters() function.
		//if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
#ifdef CONFIG_AP_SUPPORT		
			NICUpdateFifoStaCounters(pAd);
#endif // CONFIG_AP_SUPPORT //

    		FreeNumber[QueIdx] = GET_TXRING_FREENO(pAd, QueIdx);

		pPacket = QUEUE_ENTRY_TO_PKT(pEntry);

		// RTS or CTS-to-self for B/G protection mode has been set already.
		// There is no need to re-do it here. 
		// Total fragment required = number of fragment  if required
		MpduRequired = RTMP_GET_PACKET_FRAGMENTS(pPacket);
		TxRate = RTMP_GET_PACKET_TXRATE(pPacket);

    		if (FreeNumber[QueIdx] <= 5)
    		{
    			// free Tx(QueIdx) resources
    			RTMPFreeTXDUponTxDmaDone(pAd, QueIdx);
    			FreeNumber[QueIdx] = GET_TXRING_FREENO(pAd, QueIdx);
    		}



		// rough estimate we will use 3 more descriptor. 
    		if (FreeNumber[QueIdx] >= (ULONG)(MpduRequired+3))
		{
			pEntry = RemoveHeadQueue(pQueue);

			// Enhance SW Aggregation Mechanism
//			if (pAd->CommonCfg.BACapability.field.AmsduEnable && ((FreeNumber[QueIdx] != (TX_RING_SIZE-1) && pAd->TxSwQueue[QueIdx].Number == 0) || (FreeNumber[QueIdx]<3)))
			if (((FreeNumber[QueIdx] != (TX_RING_SIZE-1)) && (pAd->TxSwQueue[QueIdx].Number == 0)) 
			     || (FreeNumber[QueIdx]<3))
			{
				if ((pAd->CommonCfg.BACapability.field.AmsduEnable)
					 || (pAd->CommonCfg.bAggregationCapable && (TxRate >= RATE_6) && (TxRate <= RATE_54)))
			{
				InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
				DBGPRINT(RT_DEBUG_LOUD, ("Hw Q(%d) Free = %lu, Sw Qlen = %lu\n", QueIdx, FreeNumber[QueIdx],
										 pAd->TxSwQueue[QueIdx].Number));
				if (bIntContext == FALSE)
					RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

				break;
				} 
			}

#ifdef CONFIG_AP_SUPPORT 
//			if (pAd->OpMode == OPMODE_AP)
				if ((Status = BASmartHardTransmit(pAd, pPacket, QueIdx, &FreeNumber[QueIdx])) != NDIS_STATUS_SUCCESS)
				{
					Status = APHardTransmit(pAd, pPacket, QueIdx, &FreeNumber[QueIdx]);
				}
#endif // CONFIG_AP_SUPPORT //					

			if (Status != NDIS_STATUS_SUCCESS)
			{
				DBGPRINT(RT_DEBUG_INFO,("RTMPHardTransmit return failed!!!\n"));

				if (bIntContext == FALSE)
					RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

				break;
			}
		}
		else
		{
			pAd->PrivateInfo.TxRingFullCnt++;

			DBGPRINT(RT_DEBUG_LOUD, ("Sw Q(%d) len = %lu\n", QueIdx, pAd->TxSwQueue[QueIdx].Number));
			DBGPRINT(RT_DEBUG_INFO,("DeqPkt -> Not enough free TxD[%d] (TX_CTX_IDX=%lu, TxSwFreeIdx=%lu)!!!\n",
									QueIdx, pAd->TxRing[QueIdx].TxCpuIdx, pAd->TxRing[QueIdx].TxSwFreeIdx));

			if (bIntContext == FALSE)
				RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

			break;
		}

		if (bIntContext == FALSE)
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

		Count++;
    	}

#ifdef BLOCK_NET_IF
		if ((pAd->blockQueueTab[QueIdx].SwTxQueueBlockFlag == TRUE)
			&& (pAd->TxSwQueue[QueIdx].Number < 1))
		{
			releaseNetIf(&pAd->blockQueueTab[QueIdx]);
		}
#endif // BLOCK_NET_IF //

	}
}

BOOLEAN  RTMPFreeTXDUponTxDmaDone(
	IN PRTMP_ADAPTER	pAd, 
	IN UCHAR			QueIdx)
{
	PRTMP_TX_RING pTxRing;
	PTXD_STRUC	  pTxD;
#ifdef	BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
#endif
	PNDIS_PACKET  pPacket;
	UCHAR	FREE = 0;
	TXD_STRUC	TxD, *pOriTxD;
	//ULONG		IrqFlags;
	BOOLEAN			bReschedule = FALSE;
#ifdef DBG
#if 0
	UCHAR BbpData;
#endif
#endif

	ASSERT(QueIdx < NUM_OF_TX_RING);
	pTxRing = &pAd->TxRing[QueIdx];

	RTMP_IO_READ32(pAd, TX_DTX_IDX0 + QueIdx * RINGREG_DIFF, &pTxRing->TxDmaIdx);
	while (pTxRing->TxSwFreeIdx != pTxRing->TxDmaIdx)
	{
//		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#ifdef RALINK_ATE
#ifdef RALINK_2860_QA
		PHEADER_802_11	pHeader80211;

		if ((pAd->ate.Mode != ATE_STOP) && (pAd->ate.bQATxStart == TRUE))
		{
			if (pAd->ate.QID == QueIdx)
			{
				pAd->ate.TxDoneCount++;
				//pAd->ate.Repeat++;
				pAd->RalinkCounters.KickTxCount++;
				if (pAd->ate.QID == 0)
					pAd->ate.TxAc0++;
				else if (pAd->ate.QID == 1)
					pAd->ate.TxAc1++;
				else if (pAd->ate.QID == 2)
					pAd->ate.TxAc2++;
				else if (pAd->ate.QID == 3)
					pAd->ate.TxAc3++;
				else if (pAd->ate.QID == 4)
					pAd->ate.TxHCCA++;
				else if (pAd->ate.QID == 5)
					pAd->ate.TxMgmt++;
        	
				pTxD = (PTXD_STRUC) (pTxRing->Cell[pTxRing->TxSwFreeIdx].AllocVa);
				pHeader80211 = pTxRing->Cell[pTxRing->TxSwFreeIdx].DmaBuf.AllocVa + sizeof(TXWI_STRUC);
				pHeader80211->Sequence = ++pAd->ate.seq;
				pTxD->DMADONE = 0;

				if  ((pAd->ate.bQATxStart == TRUE) && (pAd->ate.Mode & ATE_TXFRAME) && (pAd->ate.TxDoneCount < pAd->ate.TxCount))
				{
#if 0 // debug to monitor transmit
					if (((pAd->ate.TxDoneCount + 1) % 0x4000) == 0)
					{
						printk("%x\n", pAd->ate.TxDoneCount);
					}
					else if (((pAd->ate.TxDoneCount + 1) % 0x400) == 0)
						printk("*");
#endif
					pAd->RalinkCounters.TransmittedByteCount +=  (pTxD->SDLen1 + pTxD->SDLen0);
					pAd->RalinkCounters.OneSecDmaDoneCount[QueIdx] ++;
					INC_RING_INDEX(pTxRing->TxSwFreeIdx, TX_RING_SIZE);
					/* get tx_tdx_idx again */
					RTMP_IO_READ32(pAd, TX_DTX_IDX0 + QueIdx * RINGREG_DIFF ,  &pTxRing->TxDmaIdx);					
					goto kick_out;
				}
				else if (pAd->ate.TxDoneCount == pAd->ate.TxCount)
				{
					DBGPRINT(RT_DEBUG_TRACE,("all Tx is done\n"));
					// Tx status enters idle mode.
					pAd->ate.TxStatus = 0;
				}
				else if (!(pAd->ate.Mode & ATE_TXFRAME))
				{// TODO:not complete sending yet, but someone press the Stop TX bottom.
				 // TODO:Will this case happen ?
					DBGPRINT(RT_DEBUG_TRACE,("not complete sending yet, but someone pressed the Stop TX bottom\n"));
				}
				else
				{// TODO:Will this case happen ?
					DBGPRINT(RT_DEBUG_TRACE,("pTxRing->TxSwFreeIdx = %ld\n", pTxRing->TxSwFreeIdx));
#ifdef DBG
#if 0
					DBGPRINT(RT_DEBUG_TRACE,("pAd->ate.bQATxStart = %d\n", pAd->ate.bQATxStart));					
					DBGPRINT(RT_DEBUG_TRACE,("pAd->ate.Mode = %02x\n", pAd->ate.Mode));
					DBGPRINT(RT_DEBUG_TRACE,("pAd->ate.TxCount = %ld\n", pAd->ate.TxCount));
					//DBGPRINT(RT_DEBUG_TRACE,("pAd->ate.TxDoneCount = %ld\n", pAd->ate.TxDoneCount));
					ATE_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R22, &BbpData);
					DBGPRINT(RT_DEBUG_TRACE,("BBP_R22 = %02x\n", BbpData));
					printk("***********************************\n");
#endif
#endif
				}

				INC_RING_INDEX(pTxRing->TxSwFreeIdx, TX_RING_SIZE); 
				continue;
			}
		}
#endif // RALINK_2860_QA //
#endif // RALINK_ATE //

		// static rate also need NICUpdateFifoStaCounters() function.
		//if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
			NICUpdateFifoStaCounters(pAd);

		FREE++;       
#ifndef BIG_ENDIAN
                pTxD = (PTXD_STRUC) (pTxRing->Cell[pTxRing->TxSwFreeIdx].AllocVa);
		pOriTxD = pTxD;
                NdisMoveMemory(&TxD, pTxD, sizeof(TXD_STRUC));
		pTxD = &TxD;
#else
        pDestTxD = (PTXD_STRUC) (pTxRing->Cell[pTxRing->TxSwFreeIdx].AllocVa);
        pOriTxD = pDestTxD ;
        TxD = *pDestTxD;
        pTxD = &TxD;
        RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif

#if 0
		if ((pTxD->DMADONE == 0) && (Tx_Free >= (TX_RING_SIZE-5))
		{
			//need to reschedule !!!!
			// 
			//DBGPRINT(RT_DEBUG_ERROR, ("====> RTMPFreeTXDUponTxDmaDoneError.\n"));
//			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
			DBGPRINT(RT_DEBUG_ERROR, ("!!! Reschedule TxDmaDone [ TxSwFreeIdx(%d) != TxDmaIdx(%d) ].\n",
									   pTxRing->TxSwFreeIdx, pTxRing->TxDmaIdx));
			bReschedule = TRUE;
			break;
		}			
#endif
		pTxD->DMADONE = 0;

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
        UAPSD_SP_PacketCheck(pAd,
                             pTxRing->Cell[pTxRing->TxSwFreeIdx].pNdisPacket,
                             ((UCHAR *)pTxRing->Cell[\
                              pTxRing->TxSwFreeIdx].DmaBuf.AllocVa)+TXWI_SIZE);
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

#ifdef RALINK_ATE
		if (pAd->ate.Mode == ATE_STOP)
#endif // RALINK_ATE //
		{
			pPacket = pTxRing->Cell[pTxRing->TxSwFreeIdx].pNdisPacket;
			if (pPacket)
			{
#ifdef CONFIG_5VT_ENHANCE
				if (RTMP_GET_PACKET_5VT(pPacket))
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, 16, PCI_DMA_TODEVICE);
				else
#endif // CONFIG_5VT_ENHANCE //
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}
			//Always assign pNdisPacket as NULL after clear
			pTxRing->Cell[pTxRing->TxSwFreeIdx].pNdisPacket = NULL;

			pPacket = pTxRing->Cell[pTxRing->TxSwFreeIdx].pNextNdisPacket;

			ASSERT(pPacket == NULL);
			if (pPacket)
			{
#ifdef CONFIG_5VT_ENHANCE
				if (RTMP_GET_PACKET_5VT(pPacket))
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, 16, PCI_DMA_TODEVICE);
				else
#endif // CONFIG_5VT_ENHANCE //
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}
			//Always assign pNextNdisPacket as NULL after clear
			pTxRing->Cell[pTxRing->TxSwFreeIdx].pNextNdisPacket = NULL;
		}

		pAd->RalinkCounters.TransmittedByteCount +=  (pTxD->SDLen1 + pTxD->SDLen0);
		pAd->RalinkCounters.OneSecDmaDoneCount[QueIdx] ++;
		INC_RING_INDEX(pTxRing->TxSwFreeIdx, TX_RING_SIZE);
		/* get tx_tdx_idx again */
		RTMP_IO_READ32(pAd, TX_DTX_IDX0 + QueIdx * RINGREG_DIFF ,  &pTxRing->TxDmaIdx);
#ifndef BIG_ENDIAN
                NdisMoveMemory(pOriTxD, pTxD, sizeof(TXD_STRUC));
#else
        RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
        *pDestTxD = TxD;
#endif

#ifdef RALINK_ATE
#ifdef RALINK_2860_QA
kick_out:
#endif // RALINK_2860_QA //

		//
		// ATE_TXCONT mode also need to send some normal frames, so let it in.
		// ATE_STOP must be changed not to be 0xff
		// to prevent it from running into this block.
		//
		if ((pAd->ate.Mode & ATE_TXFRAME) && (pAd->ate.QID == QueIdx))
		{
			// TxDoneCount++ has been done if QA is used.
			if (pAd->ate.bQATxStart == FALSE)
		{
			pAd->ate.TxDoneCount++;
			}
			if (((pAd->ate.TxCount - pAd->ate.TxDoneCount + 1) >= TX_RING_SIZE))
			{
				INC_RING_INDEX(pAd->TxRing[QueIdx].TxCpuIdx, TX_RING_SIZE);
				pTxD = (PTXD_STRUC) (pTxRing->Cell[pAd->TxRing[QueIdx].TxCpuIdx].AllocVa);
				pTxD->DMADONE = 0;

				// kick Tx-Ring.
				RTMP_IO_WRITE32(pAd, TX_CTX_IDX0 + QueIdx * RINGREG_DIFF, pAd->TxRing[QueIdx].TxCpuIdx);
				pAd->RalinkCounters.KickTxCount++;
			}
		}
#endif // RALINK_ATE //
//         RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	}

	DBGPRINT(RT_DEBUG_LOUD, ("RTMPFreeTXDUponTxDmaDone %d.\n", FREE));
	return  bReschedule;

}	

/*
	========================================================================

	Routine Description:
		Calculates the duration which is required to transmit out frames 
	with given size and specified rate.
		
	Arguments:
		pAd 	Pointer to our adapter
		Rate			Transmit rate
		Size			Frame size in units of byte
		
	Return Value:
		Duration number in units of usec

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
USHORT	RTMPCalcDuration(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Rate,
	IN	ULONG			Size)
{
	ULONG	Duration = 0;

	if (Rate < RATE_FIRST_OFDM_RATE) // CCK
	{
		if ((Rate > RATE_1) && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED))
			Duration = 96;	// 72+24 preamble+plcp
		else
			Duration = 192; // 144+48 preamble+plcp

		Duration += (USHORT)((Size << 4) / RateIdTo500Kbps[Rate]);
		if ((Size << 4) % RateIdTo500Kbps[Rate])
			Duration ++;
	}
	else if (Rate <= RATE_LAST_OFDM_RATE)// OFDM rates
	{
		Duration = 20 + 6;		// 16+4 preamble+plcp + Signal Extension
		Duration += 4 * (USHORT)((11 + Size * 4) / RateIdTo500Kbps[Rate]);
		if ((11 + Size * 4) % RateIdTo500Kbps[Rate])
			Duration += 4;
	}
	else	//mimo rate
	{
		Duration = 20 + 6;		// 16+4 preamble+plcp + Signal Extension
	}
	
	return (USHORT)Duration;
}

/*
	========================================================================
	
	Routine Description:
		Calculates the duration which is required to transmit out frames 
	with given size and specified rate.
					  
	Arguments:
		pTxWI		Pointer to head of each MPDU to HW.
		Ack 		Setting for Ack requirement bit
		Fragment	Setting for Fragment bit
		RetryMode	Setting for retry mode
		Ifs 		Setting for IFS gap
		Rate		Setting for transmit rate
		Service 	Setting for service
		Length		Frame length
		TxPreamble	Short or Long preamble when using CCK rates
		QueIdx - 0-3, according to 802.11e/d4.4 June/2003
		
	Return Value:
		None
		
	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
    See also : BASmartHardTransmit()    !!!
	
	========================================================================
*/
VOID RTMPWriteTxWI(
	IN	PRTMP_ADAPTER	pAd,
	IN	PTXWI_STRUC 	pOutTxWI,	
	IN	BOOLEAN			FRAG,	
	IN	BOOLEAN			CFACK,
	IN	BOOLEAN			InsTimestamp,
	IN	BOOLEAN 		AMPDU,
	IN	BOOLEAN 		Ack,
	IN	BOOLEAN 		NSeq,		// HW new a sequence.
	IN	UCHAR			BASize,
	IN	UCHAR			WCID,
	IN	ULONG			Length,
	IN	UCHAR 			PID,
	IN	UCHAR			TID,
	IN	UCHAR			TxRate,
	IN	UCHAR			Txopmode,	
	IN	BOOLEAN			CfAck,	
	IN	HTTRANSMIT_SETTING	*pTransmit)
{
	PMAC_TABLE_ENTRY	pMac = NULL;
	TXWI_STRUC 		TxWI;
	PTXWI_STRUC 	pTxWI;

	if (WCID < MAX_LEN_OF_MAC_TABLE)
		pMac = &pAd->MacTab.Content[WCID];

	//
	// Always use Long preamble before verifiation short preamble functionality works well.
	// Todo: remove the following line if short preamble functionality works
	//
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
	NdisZeroMemory(&TxWI, TXWI_SIZE);
	pTxWI = &TxWI;

	pTxWI->FRAG= FRAG;

	pTxWI->CFACK = CFACK;
	pTxWI->TS= InsTimestamp;
	pTxWI->AMPDU = AMPDU;
	pTxWI->ACK = Ack;
	pTxWI->txop= Txopmode;
	
	pTxWI->NSEQ = NSeq;
	// John tune the performace with Intel Client in 20 MHz performance

	BASize = pAd->CommonCfg.TxBASize;

	if( BASize >7 )
		BASize =7;
		
	pTxWI->BAWinSize = BASize;	
	pTxWI->WirelessCliID = WCID;
	pTxWI->MPDUtotalByteCount = Length; 
	pTxWI->PacketId = PID; 
	
	// If CCK or OFDM, BW must be 20
	pTxWI->BW = (pTransmit->field.MODE <= MODE_OFDM) ? (BW_20) : (pTransmit->field.BW);
	pTxWI->ShortGI = pTransmit->field.ShortGI;
	pTxWI->STBC = pTransmit->field.STBC;
	
	pTxWI->MCS = pTransmit->field.MCS;
	pTxWI->PHYMODE = pTransmit->field.MODE;
	pTxWI->CFACK = CfAck;


	if (pMac)
	{		
        if (pAd->CommonCfg.bMIMOPSEnable)
        {
    		if ((pMac->MmpsMode == MMPS_DYNAMIC) && (pTransmit->field.MCS > 7))
		{
			// Dynamic MIMO Power Save Mode
			pTxWI->MIMOps = 1;
		} 
		else if (pMac->MmpsMode == MMPS_STATIC)
		{
			// Static MIMO Power Save Mode
			if (pTransmit->field.MODE >= MODE_HTMIX && pTransmit->field.MCS > 7)
			{			
				pTxWI->MCS = 7;
				pTxWI->MIMOps = 0;
			}
		}
        }
		//pTxWI->MIMOps = (pMac->PsMode == PWR_MMPS)? 1:0;
		if (pMac->bIAmBadAtheros && (pMac->WepStatus != Ndis802_11WEPDisabled))
		{
			pTxWI->MpduDensity = 7;
		}
		else 
		{
			pTxWI->MpduDensity = pMac->MpduDensity;
		}

	}

#if 0
	if ((pTxWI->PHYMODE == MODE_OFDM) && (TxRate <= RATE_54))
	{
		pTxWI->MCS = OfdmRateToRxwiMCS[TxRate];
	}
	else if ((pTxWI->PHYMODE == MODE_CCK) && (TxRate <= RATE_11))
	{
		// Refer to RXWI definition
		pTxWI->MCS = TxRate;
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("====> TXWI MCS=%d , _SHORT_PREAMBLE_INUSED\n", pTxWI->MCS));
			pTxWI->MCS += 8;			
		}
	}
#endif

	pTxWI->PacketId = pTxWI->MCS;
	NdisMoveMemory(pOutTxWI, &TxWI, sizeof(TXWI_STRUC));
}

/*
	========================================================================
	
	Routine Description:
		Calculates the duration which is required to transmit out frames 
	with given size and specified rate.
		
	Arguments:
		pTxD		Pointer to transmit descriptor
		Ack 		Setting for Ack requirement bit
		Fragment	Setting for Fragment bit
		RetryMode	Setting for retry mode
		Ifs 		Setting for IFS gap
		Rate		Setting for transmit rate
		Service 	Setting for service
		Length		Frame length
		TxPreamble	Short or Long preamble when using CCK rates
		QueIdx - 0-3, according to 802.11e/d4.4 June/2003
		
	Return Value:
		None
		
	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID RTMPWriteTxDescriptor(
	IN	PRTMP_ADAPTER	pAd,
	IN	PTXD_STRUC		pTxD,
	IN	BOOLEAN 		bWIV,
	IN	UCHAR			QueueSEL)
{
	//
	// Always use Long preamble before verifiation short preamble functionality works well.
	// Todo: remove the following line if short preamble functionality works
	//
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);

	pTxD->WIV	= (bWIV) ? 1: 0;
	pTxD->QSEL= (QueueSEL);
	//RT2860c??  fixed using EDCA queue for test...  We doubt Queue1 has problem.  2006-09-26 Jan
	//pTxD->QSEL= FIFO_EDCA;
	if (pAd->bGenOneHCCA == TRUE)
		pTxD->QSEL= FIFO_HCCA;
	pTxD->DMADONE = 0;
}

// should be called only when -
// 1. MEADIA_CONNECTED
// 2. AGGREGATION_IN_USED
// 3. Fragmentation not in used
// 4. either no previous frame (pPrevAddr1=NULL) .OR. previoud frame is aggregatible
BOOLEAN TxFrameIsAggregatible(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pPrevAddr1,
	IN	PUCHAR			p8023hdr)
{

	// can't aggregate EAPOL (802.1x) frame
	if ((p8023hdr[12] == 0x88) && (p8023hdr[13] == 0x8e))
		return FALSE;

	// can't aggregate multicast/broadcast frame
	if (p8023hdr[0] & 0x01)
		return FALSE;

	if (INFRA_ON(pAd)) // must be unicast to AP
		return TRUE;
	else if ((pPrevAddr1 == NULL) || MAC_ADDR_EQUAL(pPrevAddr1, p8023hdr)) // unicast to same STA
		return TRUE;
	else
		return FALSE;
}

/*
	========================================================================

	Routine Description:
	   Check the MSDU Aggregation policy
	1.HT aggregation is A-MSDU
	2.legaacy rate aggregation is software aggregation by Ralink.
	
	Arguments:
		
	Return Value:
	
	Note:
	
	========================================================================
*/
BOOLEAN PeerIsAggreOn(
	IN	PRTMP_ADAPTER	pAd,
	IN	ULONG		   TxRate,
	IN	PMAC_TABLE_ENTRY pMacEntry)
{
	ULONG	AFlags = (fCLIENT_STATUS_AMSDU_INUSED | fCLIENT_STATUS_AGGREGATION_CAPABLE);
	
	if (pMacEntry != NULL && CLIENT_STATUS_TEST_FLAG(pMacEntry, AFlags))
	{		
		//if (TxRate >= RATE_6)
		if (pMacEntry->HTPhyMode.field.MODE >= MODE_HTMIX)
		{			
			return TRUE;
		}
#ifdef AGGREGATION_SUPPORT
		else if (TxRate >= RATE_6 && pAd->CommonCfg.bAggregationCapable && (!(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))))
		{	// legacy  Ralink Aggregation support
			return TRUE;
		}
#endif // AGGREGATION_SUPPORT //
	}

	return FALSE;
		
	
#if 0
	// AMSDU not enable yet when operation as HT
	if ((TxRate >= RATE_FIRST_HT_RATE))
		return FALSE;
	else if (TxRate >= RATE_FIRST_HT_RATE)
		return TRUE;
	
	// legacy b/g rate, and doesn't turn on sw aggregation
	if (pMacEntry)
	{
		if ((TxRate <= RATE_54) && (TxRate >= RATE_6)  && 
		(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE)))
			return TRUE;
		else
			return FALSE;
	}
	
	if ((TxRate <= RATE_54) && (TxRate >= RATE_6)  && 
		(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED)))
		return TRUE;
	
	return FALSE;
#endif 
}

/*
	========================================================================

	Routine Description:
		Check and fine the packet waiting in SW queue with highest priority
		
	Arguments:
		pAd Pointer to our adapter
		
	Return Value:
		pQueue		Pointer to Waiting Queue

	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
PQUEUE_HEADER	RTMPCheckTxSwQueue(
	IN	PRTMP_ADAPTER	pAd,
	OUT PUCHAR			pQueIdx)
{

	ULONG	Number;
	// 2004-11-15 to be removed. test aggregation only
//	if ((OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED)) && (*pNumber < 2))
//		 return NULL;

	Number = pAd->TxSwQueue[QID_AC_BK].Number 
			 + pAd->TxSwQueue[QID_AC_BE].Number 
			 + pAd->TxSwQueue[QID_AC_VI].Number 
			 + pAd->TxSwQueue[QID_AC_VO].Number
			 + pAd->TxSwQueue[QID_HCCA].Number;

#if 0
	if (pAd->TxSwQueue[QID_AC_BE].Head != NULL)
	{
		*pQueIdx = QID_AC_BE;
		return (&pAd->TxSwQueue[QID_AC_BE]);
	}
#else
	if (pAd->TxSwQueue[QID_AC_VO].Head != NULL)
	{
		*pQueIdx = QID_AC_VO;
		return (&pAd->TxSwQueue[QID_AC_VO]);
	}
	else if (pAd->TxSwQueue[QID_AC_VI].Head != NULL)
	{
		*pQueIdx = QID_AC_VI;
		return (&pAd->TxSwQueue[QID_AC_VI]);
	}
	else if (pAd->TxSwQueue[QID_AC_BE].Head != NULL)
	{
		*pQueIdx = QID_AC_BE;
		return (&pAd->TxSwQueue[QID_AC_BE]);
	}
	else if (pAd->TxSwQueue[QID_AC_BK].Head != NULL)
	{
		*pQueIdx = QID_AC_BK;
		return (&pAd->TxSwQueue[QID_AC_BK]);
	}
	else if (pAd->TxSwQueue[QID_HCCA].Head != NULL)
	{
		*pQueIdx = QID_HCCA;
		return (&pAd->TxSwQueue[QID_HCCA]);
	}
#endif

	// No packet pending in Tx Sw queue
	*pQueIdx = QID_AC_BK;
	
	return (NULL);
}

/*
	========================================================================

	Routine Description:
		Process TX Rings DMA Done interrupt, running in DPC level

	Arguments:
		Adapter 	Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
BOOLEAN	RTMPHandleTxRingDmaDoneInterrupt(
	IN	PRTMP_ADAPTER	pAd,
	IN	INT_SOURCE_CSR_STRUC TxRingBitmap)
{
//	UCHAR			Count = 0;
//    unsigned long	IrqFlags;
	BOOLEAN			bReschedule = FALSE;
	
	// Make sure Tx ring resource won't be used by other threads
	NdisAcquireSpinLock(&pAd->TxRingLock);
	 
	//RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	if (TxRingBitmap.field.Ac0DmaDone)
		bReschedule = RTMPFreeTXDUponTxDmaDone(pAd, QID_AC_BE);

	if (TxRingBitmap.field.HccaDmaDone)
		bReschedule |= RTMPFreeTXDUponTxDmaDone(pAd, QID_HCCA);

	if (TxRingBitmap.field.Ac3DmaDone)
		bReschedule |= RTMPFreeTXDUponTxDmaDone(pAd, QID_AC_VO);

	if (TxRingBitmap.field.Ac2DmaDone)
		bReschedule |= RTMPFreeTXDUponTxDmaDone(pAd, QID_AC_VI);

	if (TxRingBitmap.field.Ac1DmaDone)
		bReschedule |= RTMPFreeTXDUponTxDmaDone(pAd, QID_AC_BK);

	// Make sure to release Tx ring resource
	NdisReleaseSpinLock(&pAd->TxRingLock);
	//RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	
	// Dequeue outgoing frames from TxSwQueue[] and process it
	RTMPDeQueuePacket(pAd, FALSE, MAX_TX_PROCESS);

	return  bReschedule;
}


/*
	========================================================================

	Routine Description:
		Process MGMT ring DMA done interrupt, running in DPC level

	Arguments:
		pAd 	Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:

	========================================================================
*/
VOID	RTMPHandleMgmtRingDmaDoneInterrupt(
	IN	PRTMP_ADAPTER	pAd)
{
	PTXD_STRUC	 pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	PNDIS_PACKET pPacket;
//	int 		 i;
	UCHAR	FREE = 0;
	PRTMP_MGMT_RING pMgmtRing = &pAd->MgmtRing;

	NdisAcquireSpinLock(&pAd->MgmtRingLock);

	RTMP_IO_READ32(pAd, TX_MGMTDTX_IDX, &pMgmtRing->TxDmaIdx);  
	while (pMgmtRing->TxSwFreeIdx!= pMgmtRing->TxDmaIdx)
	{
		FREE++;
#ifndef BIG_ENDIAN
		pTxD = (PTXD_STRUC) (pMgmtRing->Cell[pAd->MgmtRing.TxSwFreeIdx].AllocVa);
#else
        pDestTxD = (PTXD_STRUC) (pMgmtRing->Cell[pAd->MgmtRing.TxSwFreeIdx].AllocVa);
        TxD = *pDestTxD;
        pTxD = &TxD;
		RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif
#if 0
		if (pTxD->DMADONE != 1) {
			DBGPRINT(RT_DEBUG_ERROR, ("RTMPFreeMgmtDmaDoneError====> pTxD->DMADONE != 1.\n"));
			printk("pMgmtRing->TxSwFreeIdx= %08lx, pMgmtRing->TxDmaIdx = %08lx\n", 
					pMgmtRing->TxSwFreeIdx, pMgmtRing->TxDmaIdx);
		}
#endif			
		pTxD->DMADONE = 0;
		pPacket = pMgmtRing->Cell[pMgmtRing->TxSwFreeIdx].pNdisPacket;

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
{
        UAPSD_QoSNullTxDoneHandle(pAd,
                                  pPacket,
                                  GET_OS_PKT_DATAPTR(pPacket)+TXWI_SIZE);
}
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

		if (pPacket)
		{
			PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr0, pTxD->SDLen0, PCI_DMA_TODEVICE);
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
		}
		pMgmtRing->Cell[pMgmtRing->TxSwFreeIdx].pNdisPacket = NULL;

		pPacket = pMgmtRing->Cell[pMgmtRing->TxSwFreeIdx].pNextNdisPacket;
		if (pPacket)
		{
			PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, PCI_DMA_TODEVICE);
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
		}
		pMgmtRing->Cell[pMgmtRing->TxSwFreeIdx].pNextNdisPacket = NULL;
		INC_RING_INDEX(pMgmtRing->TxSwFreeIdx, MGMT_RING_SIZE);

#ifdef BIG_ENDIAN
        RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
        WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, TRUE, TYPE_TXD);
#endif	
	}
	NdisReleaseSpinLock(&pAd->MgmtRingLock);

}


/*
	========================================================================

	Routine Description:
	Arguments:
		Adapter 	Pointer to our adapter. Dequeue all power safe delayed braodcast frames after beacon.

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID	RTMPHandleTBTTInterrupt(
	IN PRTMP_ADAPTER pAd)
{
#ifdef CONFIG_AP_SUPPORT
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (pAd->OpMode == OPMODE_AP)
	{
		ReSyncBeaconTime(pAd);

#if 0
		//
		// step 7 - if DTIM, then move backlogged bcast/mcast frames from PSQ to TXQ whenever DtimCount==0
#if 0    
		// NOTE: This updated BEACON frame will be sent at "next" TBTT instead of at cureent TBTT. The reason is
		//       because ASIC already fetch the BEACON content down to TX FIFO before driver can make any
		//       modification. To compenstate this effect, the actual time to deilver PSQ frames will be
		//       at the time that we wrapping around DtimCount from 0 to DtimPeriod-1
		if ((pAd->ApCfg.DtimCount + 1) == pAd->ApCfg.DtimPeriod)
#else
		if (pAd->ApCfg.DtimCount == 0)
#endif
		{
			PQUEUE_ENTRY    pEntry;
			BOOLEAN			bPS = FALSE;

//			NdisAcquireSpinLock(&pAd->MacTabLock);
//			NdisAcquireSpinLock(&pAd->TxSwQueueLock);
			
			while (pAd->MacTab.McastPsQueue.Head)
			{
				bPS = TRUE;
                if (pAd->TxSwQueue[QID_AC_BE].Number <= (MAX_PACKETS_IN_QUEUE + (MAX_PACKETS_IN_MCAST_PS_QUEUE>>1))) 
				{
				pEntry = RemoveHeadQueue(&pAd->MacTab.McastPsQueue);
					if(pAd->MacTab.McastPsQueue.Number)
					{
						RTMP_SET_PACKET_MOREDATA(pEntry, TRUE);
					}
					InsertHeadQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);
				}
				else
				{
					break;
				}
			}
			DBGPRINT(RT_DEBUG_INFO, ("DTIM=%d/%d, tx mcast/bcast out...\n",pAd->ApCfg.DtimCount,pAd->ApCfg.DtimPeriod));
//			NdisReleaseSpinLock(&pAd->TxSwQueueLock);
//			NdisReleaseSpinLock(&pAd->MacTabLock);
			if (pAd->MacTab.McastPsQueue.Number == 0)
			{			
                UINT bss_index;

                /* clear MCAST/BCAST backlog bit for all BSS */
                for(bss_index=BSS0; bss_index<pAd->ApCfg.BssidNum; bss_index++)
					WLAN_MR_TIM_BCMC_CLEAR(bss_index);
                /* End of for */
			}
			pAd->MacTab.PsQIdleCount = 0;

			// Dequeue outgoing framea from TxSwQueue0..3 queue and process it
            if (bPS == TRUE) 
			{
				RTMPDeQueuePacket(pAd, TRUE, MAX_TX_IN_TBTT);
			}

		}
#else
		tasklet_hi_schedule(&pObj->tbtt_task);
#endif

		if ((pAd->CommonCfg.Channel > 14)
			&& (pAd->CommonCfg.bIEEE80211H == 1)
			&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("RTMPHandlePreTBTTInterrupt::Channel Switching...(%d/%d)\n", pAd->CommonCfg.RadarDetect.CSCount, pAd->CommonCfg.RadarDetect.CSPeriod));
			
			pAd->CommonCfg.RadarDetect.CSCount++;
			if (pAd->CommonCfg.RadarDetect.CSCount >= pAd->CommonCfg.RadarDetect.CSPeriod)
			{
				APStop(pAd);
				APStartUp(pAd);
			}
		}
	}
	else
#endif // CONFIG_AP_SUPPORT //
	{
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("RTMPHandleTBTTInterrupt...\n"));
		}
	}
}

/*
	========================================================================

	Routine Description:
	Arguments:
		Adapter 	Pointer to our adapter. Rewrite beacon content before next send-out.

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID	RTMPHandlePreTBTTInterrupt(
	IN PRTMP_ADAPTER pAd)
{
#ifdef CONFIG_AP_SUPPORT
	if (pAd->OpMode == OPMODE_AP)
	{
		APUpdateAllBeaconFrame(pAd);
	}
	else
#endif // CONFIG_AP_SUPPORT //
	{
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("RTMPHandlePreTBTTInterrupt...\n"));
		}
	}
}

VOID	RTMPHandleRxCoherentInterrupt(
	IN	PRTMP_ADAPTER	pAd)
{
	WPDMA_GLO_CFG_STRUC	GloCfg;
	
	DBGPRINT(RT_DEBUG_TRACE, ("==> RTMPHandleRxCoherentInterrupt \n"));	
	
	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG , &GloCfg.word);

	GloCfg.field.EnTXWriteBackDDONE = 0;
	GloCfg.field.EnableRxDMA = 0;
	GloCfg.field.EnableTxDMA = 0;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);

	RTMPRingCleanUp(pAd, QID_AC_BE);
	RTMPRingCleanUp(pAd, QID_AC_BK);
	RTMPRingCleanUp(pAd, QID_AC_VI);
	RTMPRingCleanUp(pAd, QID_AC_VO);
	RTMPRingCleanUp(pAd, QID_HCCA);
	RTMPRingCleanUp(pAd, QID_MGMT);
	RTMPRingCleanUp(pAd, QID_RX);

	RTMPEnableRxTx(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("<== RTMPHandleRxCoherentInterrupt \n"));	
}

VOID DBGPRINT_TX_RING(
	IN PRTMP_ADAPTER  pAd,
	IN UCHAR          QueIdx)
{
	ULONG      Ac0Base;
	ULONG      Ac0HwIdx = 0, Ac0SwIdx = 0, AC0freeIdx;
	int        i;
//	PULONG		pTxD;
	PULONG	ptemp;

	DBGPRINT_RAW(RT_DEBUG_TRACE, ("=====================================================\n "  ));
	switch (QueIdx)
	{
		case QID_AC_BE:
			RTMP_IO_READ32(pAd, TX_BASE_PTR0, &Ac0Base);
			RTMP_IO_READ32(pAd, TX_CTX_IDX0, &Ac0SwIdx);
			RTMP_IO_READ32(pAd, TX_DTX_IDX0, &Ac0HwIdx);
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("All QID_AC_BE DESCRIPTOR  \n "  ));
			for (i=0;i<TX_RING_SIZE;i++)
			{
				ptemp= (PULONG)pAd->TxRing[QID_AC_BE].Cell[i].AllocVa;
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08lx: %08lx: %08lx: %08lx\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
			}
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("  \n "  ));
			break;
		case QID_AC_BK:
			RTMP_IO_READ32(pAd, TX_BASE_PTR1, &Ac0Base);
			RTMP_IO_READ32(pAd, TX_CTX_IDX1, &Ac0SwIdx);
			RTMP_IO_READ32(pAd, TX_DTX_IDX1, &Ac0HwIdx);
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("All QID_AC_BK DESCRIPTOR  \n "  ));
			for (i=0;i<TX_RING_SIZE;i++)
			{
				ptemp= (PULONG)pAd->TxRing[QID_AC_BK].Cell[i].AllocVa;
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08lx: %08lx: %08lx: %08lx\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
			}
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("  \n "  ));
			break;
		case QID_AC_VI:
			RTMP_IO_READ32(pAd, TX_BASE_PTR2, &Ac0Base);
			RTMP_IO_READ32(pAd, TX_CTX_IDX2, &Ac0SwIdx);
			RTMP_IO_READ32(pAd, TX_DTX_IDX2, &Ac0HwIdx);
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("All QID_AC_VI DESCRIPTOR \n "  ));
			for (i=0;i<TX_RING_SIZE;i++)
			{
				ptemp= (PULONG)pAd->TxRing[QID_AC_VI].Cell[i].AllocVa;
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08lx: %08lx: %08lx: %08lx\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
			}
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("  \n "  ));
			break;
		case QID_AC_VO:
			RTMP_IO_READ32(pAd, TX_BASE_PTR3, &Ac0Base);
			RTMP_IO_READ32(pAd, TX_CTX_IDX3, &Ac0SwIdx);
			RTMP_IO_READ32(pAd, TX_DTX_IDX3, &Ac0HwIdx);
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("All QID_AC_VO DESCRIPTOR \n "  ));
			for (i=0;i<TX_RING_SIZE;i++)
			{
				ptemp= (PULONG)pAd->TxRing[QID_AC_VO].Cell[i].AllocVa;
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08lx: %08lx: %08lx: %08lx\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
			}
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("  \n "  ));
			break;
		case QID_MGMT:
			RTMP_IO_READ32(pAd, TX_BASE_PTR5, &Ac0Base);
			RTMP_IO_READ32(pAd, TX_CTX_IDX5, &Ac0SwIdx);
			RTMP_IO_READ32(pAd, TX_DTX_IDX5, &Ac0HwIdx);
			DBGPRINT_RAW(RT_DEBUG_TRACE, (" All QID_MGMT  DESCRIPTOR \n "  ));
			for (i=0;i<MGMT_RING_SIZE;i++)
			{
				ptemp= (PULONG)pAd->MgmtRing.Cell[i].AllocVa;
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08lx: %08lx: %08lx: %08lx\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
			}
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("  \n "  ));
			break;
			
		default:
			DBGPRINT_ERR(("DBGPRINT_TX_RING(Ring %d) not supported\n", QueIdx));
			break;
	}
	AC0freeIdx = pAd->TxRing[QueIdx].TxSwFreeIdx;

	DBGPRINT(RT_DEBUG_TRACE,("TxRing%d, TX_DTX_IDX=%ld, TX_CTX_IDX=%ld\n", QueIdx, Ac0HwIdx, Ac0SwIdx));
	DBGPRINT_RAW(RT_DEBUG_TRACE,(" 	TxSwFreeIdx[%ld]", AC0freeIdx));
	DBGPRINT_RAW(RT_DEBUG_TRACE,("	pending-NDIS=%ld\n", pAd->RalinkCounters.PendingNdisPacketCount));

	
}

VOID DBGPRINT_RX_RING(
	IN PRTMP_ADAPTER  pAd)
{
	ULONG      Ac0Base;
	ULONG      Ac0HwIdx, Ac0SwIdx, AC0freeIdx;
//	PULONG	 pTxD;
	int        i;
	UINT32	*ptemp;
//	PRXD_STRUC		pRxD;
	
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("=====================================================\n "  ));
	RTMP_IO_READ32(pAd, RX_BASE_PTR, &Ac0Base);
	RTMP_IO_READ32(pAd, RX_CRX_IDX, &Ac0SwIdx);
	RTMP_IO_READ32(pAd, RX_DRX_IDX, &Ac0HwIdx);
	AC0freeIdx = pAd->RxRing.RxSwReadIdx;

	DBGPRINT_RAW(RT_DEBUG_TRACE, ("All RX DSP  \n "  ));
	for (i=0;i<RX_RING_SIZE;i++)
	{
		ptemp = (UINT32 *)pAd->RxRing.Cell[i].AllocVa;
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("[%02d]  %08x: %08x: %08x: %08x\n " , i, *ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3)));
	}
	DBGPRINT(RT_DEBUG_TRACE,("RxRing, RX_DRX_IDX=%ld, RX_CRX_IDX=%ld \n", Ac0HwIdx, Ac0SwIdx));
	DBGPRINT_RAW(RT_DEBUG_TRACE,(" 	RxSwReadIdx [%ld]=", AC0freeIdx));
	DBGPRINT_RAW(RT_DEBUG_TRACE,("	pending-NDIS=%ld\n", pAd->RalinkCounters.PendingNdisPacketCount));
}

/*
	========================================================================

	Routine Description:
		Suspend MSDU transmission
		
	Arguments:
		pAd 	Pointer to our adapter
		
	Return Value:
		None
		
	Note:
	
	========================================================================
*/
VOID	RTMPSuspendMsduTransmission(
	IN	PRTMP_ADAPTER	pAd)
{
	DBGPRINT(RT_DEBUG_TRACE,("SCANNING, suspend MSDU transmission ...\n"));

	//
	// Before BSS_SCAN_IN_PROGRESS, we need to keep Current R66 value and
	// use Lowbound as R66 value on ScanNextChannel(...)
	//
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R66, &pAd->BbpTuning.R66CurrentValue);
	
	// set BBP_R66 to 0x30/0x40 when scanning (AsicSwitchChannel will set R66 according to channel when scanning)
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, (0x26 + GET_LNA_GAIN(pAd)));
	
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
	//RTMP_IO_WRITE32(pAd, TX_CNTL_CSR, 0x000f0000);		// abort all TX rings
}

/*
	========================================================================

	Routine Description:
		Resume MSDU transmission
		
	Arguments:
		pAd 	Pointer to our adapter
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
VOID RTMPResumeMsduTransmission(
	IN	PRTMP_ADAPTER	pAd)
{
    UCHAR			IrqState;
    
	DBGPRINT(RT_DEBUG_TRACE,("SCAN done, resume MSDU transmission ...\n"));

	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, pAd->BbpTuning.R66CurrentValue);

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
    IrqState = pAd->irq_disabled;
    if (IrqState)
	    RTMPDeQueuePacket(pAd, TRUE, MAX_TX_PROCESS);
    else
	    RTMPDeQueuePacket(pAd, FALSE, MAX_TX_PROCESS);
}

UINT deaggregate_AMSDU_announce(
	IN	PRTMP_ADAPTER	pAd,
	PNDIS_PACKET		pPacket,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize)	
{
	USHORT 			PayloadSize;
	USHORT 			SubFrameSize; 	
	PHEADER_802_3 	pAMSDUsubheader;	
	UINT			nMSDU;
    UCHAR			Header802_3[14];

	PUCHAR			pPayload, pDA, pSA, pRemovedLLCSNAP;
	PNDIS_PACKET	pClonePacket;

#ifdef CONFIG_AP_SUPPORT
	UCHAR FromWhichBSSID = RTMP_GET_PACKET_IF(pPacket);
	UCHAR VLAN_Size = (pAd->ApCfg.MBSSID[FromWhichBSSID].VLAN_VID != 0) ? LENGTH_802_1Q : 0;
#endif // CONFIG_AP_SUPPORT //

	nMSDU = 0;

	while (DataSize > LENGTH_802_3)
	{

		nMSDU++;

		//hex_dump("subheader", pData, 64);
		pAMSDUsubheader = (PHEADER_802_3)pData;
		//pData += LENGTH_802_3;
		PayloadSize = pAMSDUsubheader->Octet[1] + (pAMSDUsubheader->Octet[0]<<8);
		SubFrameSize = PayloadSize + LENGTH_802_3;


		if ((DataSize < SubFrameSize) || (PayloadSize > 1518 ))
		{
			break;
		}

		//printk("%d subframe: Size = %d\n",  nMSDU, PayloadSize);

		pPayload = pData + LENGTH_802_3;
		pDA = pData;
		pSA = pData + MAC_ADDR_LEN;

		// convert to 802.3 header 
		CONVERT_TO_802_3(Header802_3, pDA, pSA, pPayload, PayloadSize, pRemovedLLCSNAP);
#ifdef CONFIG_AP_SUPPORT
		if (pRemovedLLCSNAP)
		{
			pPayload -= (LENGTH_802_3 + VLAN_Size);
			PayloadSize += (LENGTH_802_3 + VLAN_Size);
			//NdisMoveMemory(pPayload, &Header802_3, LENGTH_802_3);
		}
		else
		{
			pPayload -= VLAN_Size;
			PayloadSize += VLAN_Size;
		}
		VLAN_8023_Header_Copy(pAd, Header802_3, LENGTH_802_3, pPayload, FromWhichBSSID);
#endif // CONFIG_AP_SUPPORT //

		pClonePacket = ClonePacket(pAd, pPacket, pPayload, PayloadSize);
		if (pClonePacket)
		{
			ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pClonePacket, RTMP_GET_PACKET_IF(pPacket));
		}


		// A-MSDU has padding to multiple of 4 including subframe header.
		// align SubFrameSize up to multiple of 4 
		SubFrameSize = (SubFrameSize+3)&(~0x3);


		if (SubFrameSize > 1528 || SubFrameSize < 32)
		{
			break;
		}

		if (DataSize > SubFrameSize)
		{
			pData += SubFrameSize;
			DataSize -= SubFrameSize;
		}
		else
		{   
			// end of A-MSDU
			DataSize = 0;
		}
	}
	
	// finally release original rx packet 
	RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);

	return nMSDU;
}


UINT BA_Reorder_AMSDU_Annnounce(
	IN	PRTMP_ADAPTER	pAd, 	
	IN	PNDIS_PACKET	pPacket)
{
	PUCHAR			pData;
	USHORT			DataSize;	
	UINT			nMSDU = 0;

	pData = (PUCHAR) GET_OS_PKT_DATAPTR(pPacket);	
	DataSize = (USHORT) GET_OS_PKT_LEN(pPacket);

	nMSDU = deaggregate_AMSDU_announce(pAd, pPacket, pData, DataSize);

	return nMSDU;
}


/*
	==========================================================================
	Description:
		Look up the MAC address in the MAC table. Return NULL if not found.
	Return:
		pEntry - pointer to the MAC entry; NULL is not found
	==========================================================================
*/
MAC_TABLE_ENTRY *MacTableLookup(
	IN PRTMP_ADAPTER pAd, 
	PUCHAR pAddr) 
{
	ULONG HashIdx;
	MAC_TABLE_ENTRY *pEntry = NULL;
	
	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = pAd->MacTab.Hash[HashIdx];

	while (pEntry && (pEntry->ValidAsCLI || pEntry->ValidAsWDS || pEntry->ValidAsApCli))
	{
		if (MAC_ADDR_EQUAL(pEntry->Addr, pAddr)) 
		{
			break;
		}
		else
			pEntry = pEntry->pNext;
	}

	return pEntry;
}


/*
	==========================================================================
	Description:
		This routine reset the entire MAC table. All packets pending in
		the power-saving queues are freed here.
	==========================================================================
 */
VOID STAMacTableReset(
	IN  PRTMP_ADAPTER  pAd)
{
	int         i;
//	BOOLEAN     Cancelled;
//	PCHAR       pOutBuffer = NULL;
//	NDIS_STATUS NStatus;
//	ULONG       FrameLen = 0;
//	HEADER_802_11 DisassocHdr;
	USHORT      Reason;

	DBGPRINT(RT_DEBUG_TRACE, ("STAMacTableReset\n"));
	NdisAcquireSpinLock(&pAd->MacTabLock);

	for (i=1; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		Reason = MAC_WCID_BASE + (i* HW_WCID_ENTRY_SIZE);	

		RTMP_IO_WRITE32(pAd, Reason, 0xffffffff);
		RTMP_IO_WRITE32(pAd, Reason+4, 0xffff);
		if (pAd->MacTab.Content[i].ValidAsCLI == TRUE)
	    {
			//RTMPCancelTimer(&pAd->MacTab.Content[i].RetryTimer, &Cancelled);

			// free resources of BA
			BASessionTearDownALL(pAd, i);

			pAd->MacTab.Content[i].ValidAsCLI = FALSE;

			Reason = MAC_WCID_BASE + (i* HW_WCID_ENTRY_SIZE);	
			RTMP_IO_WRITE32(pAd, Reason, 0xffffffff);
			RTMP_IO_WRITE32(pAd, Reason+4, 0xffff);
		}

	}

	NdisZeroMemory(&pAd->MacTab, sizeof(MAC_TABLE));

	NdisReleaseSpinLock(&pAd->MacTabLock);
}


MAC_TABLE_ENTRY *MacTableInsertEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR			pAddr,
	IN	UCHAR			apidx,
	IN BOOLEAN	CleanAll) 
{
	UCHAR HashIdx;
	int i;//, j;
	MAC_TABLE_ENTRY *pEntry = NULL, *pCurrEntry;
//	USHORT	offset;
//	ULONG	addr;

	// if FULL, return
	if (pAd->MacTab.Size >= MAX_LEN_OF_MAC_TABLE) 
		return NULL;

	// allocate one MAC entry
	NdisAcquireSpinLock(&pAd->MacTabLock);
	for (i = 1; i< MAX_LEN_OF_MAC_TABLE; i++)   // skip entry#0 so that "entry index == AID" for fast lookup
	{
		// pick up the first available vacancy
		if ((pAd->MacTab.Content[i].ValidAsCLI == FALSE) && 
			(pAd->MacTab.Content[i].ValidAsWDS == FALSE) &&
			(pAd->MacTab.Content[i].ValidAsApCli== FALSE))
		{
			pEntry = &pAd->MacTab.Content[i];
			if (CleanAll == TRUE)
			{
				pEntry->MaxSupportedRate = RATE_11;
				pEntry->CurrTxRate = RATE_11;
				NdisZeroMemory(pEntry, sizeof(MAC_TABLE_ENTRY));
				pEntry->PairwiseKey.KeyLen = 0;
				pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
			}
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
			if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
			{	// be a apcli-entry
				pEntry->ValidAsCLI = FALSE;
				pEntry->ValidAsWDS = FALSE;
				pEntry->ValidAsApCli = TRUE;
				pEntry->isCached = FALSE;
			}
			else
#endif // APCLI_SUPPORT //				
#ifdef WDS_SUPPORT
			if (apidx >= MIN_NET_DEVICE_FOR_WDS)
			{
				pEntry->ValidAsCLI = FALSE;
				pEntry->ValidAsWDS = TRUE;
				pEntry->ValidAsApCli = FALSE;
				pEntry->isCached = FALSE;
			}
			else
#endif // WDS_SUPPORT //
			{	// be a regular-entry
				pEntry->ValidAsCLI = TRUE;
				pEntry->ValidAsWDS = FALSE;
				pEntry->ValidAsApCli = FALSE;
			}
#endif // CONFIG_AP_SUPPORT //


			pEntry->bIAmBadAtheros = FALSE;
#ifdef CONFIG_AP_SUPPORT
			if (pEntry->ValidAsCLI) // Ralink WDS doesn't support key negotiation.
			{
#ifdef WIN_NDIS
				NdisMInitializeTimer(&pEntry->RetryTimer, pAd->AdapterHandle, WPARetryExec, pEntry);
#else
				RTMPInitTimer(pAd, &pEntry->RetryTimer, GET_TIMER_FUNCTION(WPARetryExec), pEntry, FALSE); 
//				RTMP_OS_Init_Timer(pAd, &pEntry->RetryTimer, GET_TIMER_FUNCTION(WPARetryExec), pAd); 

				// init EAPoL-start timer per entry
				RTMPInitTimer(pAd, &pEntry->EnqueueStartForPSKTimer, GET_TIMER_FUNCTION(EnqueueStartForPSKExec), pEntry, FALSE); 
#endif	 
			}
#endif // CONFIG_AP_SUPPORT //
			pEntry->pAd = pAd;
			pEntry->CMTimerRunning = FALSE;
			pEntry->EnqueueStartForPSKTimerRunning= FALSE;
			pEntry->RSNIE_Len = 0;
			NdisZeroMemory(pEntry->R_Counter, sizeof(pEntry->R_Counter));
			pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;

			if (pEntry->ValidAsApCli)
				pEntry->apidx = (apidx - MIN_NET_DEVICE_FOR_APCLI);
			else if (pEntry->ValidAsWDS)
				pEntry->apidx = (apidx - MIN_NET_DEVICE_FOR_WDS);
			else
				pEntry->apidx = apidx;
			
#ifdef CONFIG_AP_SUPPORT			
#ifdef APCLI_SUPPORT
			if (pEntry->ValidAsApCli)
			{
				pEntry->AuthMode = pAd->ApCfg.ApCliTab[pEntry->apidx].AuthMode;
				pEntry->WepStatus = pAd->ApCfg.ApCliTab[pEntry->apidx].WepStatus;
			
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
				pEntry->MatchAPCLITabIdx = pEntry->apidx;
				AsicRemovePairwiseKeyEntry(pAd, (pAd->ApCfg.BssidNum + pEntry->apidx), (UCHAR)i);
				
			}
			else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			if (pEntry->ValidAsWDS)
			{
				pEntry->AuthMode = Ndis802_11AuthModeOpen;
				pEntry->WepStatus = Ndis802_11EncryptionDisabled;
			
				pEntry->MatchWDSTabIdx = pEntry->apidx;
				AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, (UCHAR)i);
			}
			else
#endif // WDS_SUPPORT //
			{
				pEntry->AuthMode = pAd->ApCfg.MBSSID[apidx].AuthMode;
				pEntry->WepStatus = pAd->ApCfg.MBSSID[apidx].WepStatus;
			
				if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
					pEntry->WpaState = AS_NOTUSE;
				else
					pEntry->WpaState = AS_INITIALIZE;

				pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
				AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, (UCHAR)i);
				pAd->StaCount[apidx]++;
			}
#endif // CONFIG_AP_SUPPORT //


			pEntry->GTKState = REKEY_NEGOTIATING;
			pEntry->PairwiseKey.KeyLen = 0;
			pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
			pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
			pEntry->PMKID_CacheIdx = ENTRY_NOT_FOUND;
			COPY_MAC_ADDR(pEntry->Addr, pAddr);
			pEntry->Sst = SST_NOT_AUTH;
			pEntry->AuthState = AS_NOT_AUTH;
			pEntry->Aid = (USHORT)i;  //0;
			pEntry->CapabilityInfo = 0;
			pEntry->PsMode = PWR_ACTIVE;
			pEntry->PsQIdleCount = 0;
			pEntry->NoDataIdleCount = 0;
			InitializeQueueHeader(&pEntry->PsQueue);

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
			if (pEntry->ValidAsCLI) // Ralink WDS doesn't support any power saving.
			{
				/* init U-APSD enhancement related parameters */
				UAPSD_MR_ENTRY_INIT(pEntry);
			}
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

			pAd->MacTab.Size ++;
#if 0
			// Add this pAddr into ASIC WCID Table. No LEVEL issue, so write register here.
			offset = MAC_WCID_BASE + (i * HW_WCID_ENTRY_SIZE);	
			addr = (pEntry->Addr[0]) + (pEntry->Addr[1]<<8) +(pEntry->Addr[2]<<16) +(pEntry->Addr[3]<<24);
			RTMP_IO_WRITE32(pAd, offset, addr);
			addr = (pEntry->Addr[4]) + (pEntry->Addr[5]<<8);
			RTMP_IO_WRITE32(pAd, offset+4, addr);
#endif
			// Add this entry into ASIC RX WCID search table
			AsicUpdateRxWCIDTable(pAd, pEntry->Aid, pEntry->Addr);

			// If legacy WEP is used, set pair-wise cipherAlg into WCID attribute table for this entry
			if (pEntry->ValidAsCLI && pEntry->WepStatus == Ndis802_11WEPEnabled)
			{
#ifdef CONFIG_AP_SUPPORT		
				UCHAR KeyIdx		= pAd->ApCfg.MBSSID[pEntry->apidx].DefaultKeyId;
#endif // CONFIG_AP_SUPPORT //
				UCHAR CipherAlg 	= pAd->SharedKey[pEntry->apidx][KeyIdx].CipherAlg;
						
				RTMPAddWcidAttributeEntry(
							pAd, 
							pEntry->apidx, 
							KeyIdx, 
							CipherAlg, 
							pEntry);								    				
			}

#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_AP_SUPPORT
            pEntry->bWscCapable = FALSE;
            RTMPInitTimer(pAd, &pEntry->EnqueueEapolStartTimerForWsc, GET_TIMER_FUNCTION(WscEnqueueEapolStart), pEntry, FALSE);
            pEntry->EnqueueEapolStartTimerForWscRunning = FALSE;
            pEntry->Receive_EapolStart_EapRspId = 0;
#endif // WSC_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

			DBGPRINT(RT_DEBUG_TRACE, ("MacTableInsertEntry - allocate entry #%d, Total= %d\n",i, pAd->MacTab.Size));
			break;
		}
	}

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
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_AP_SUPPORT
        if ((pEntry->ValidAsCLI) &&
			(pEntry->apidx == MAIN_MBSSID) &&
             MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr))
        {
            pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;
            memset(pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr, 0, MAC_ADDR_LEN);
        }
#endif // WSC_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
	}

	NdisReleaseSpinLock(&pAd->MacTabLock);
	return pEntry;
}

#if 0
/*
    ==========================================================================
    Description:
	Add Group key Info into ASIC WCID attribute table and IVEIV table.
    Return:
    ==========================================================================
 */
VOID AsicAddSharedGroupKey(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			BssIdx,
	IN UCHAR		 	KeyIdx,
	IN UCHAR		 	CipherAlg,
	IN PUCHAR		 	pKey,
	IN PUCHAR		 	pTxMic,
	IN PUCHAR		 	pRxMic)	
{
	BOOLEAN 	bKeyRSC, bAuthenticator;		// indicate the receive  SC set by KeyRSC value.
	UCHAR		i;
	ULONG		WCIDAttri;
	USHORT		offset;
	UCHAR		IVEIV[8];
	USHORT		Wcid;
	PULONG		pValue;
	
#ifdef CONFIG_AP_SUPPORT
	GET_GroupKey_WCID(Wcid, BssIdx);
#endif // CONFIG_AP_SUPPORT //

	// Copy Key to MAC Table Entry. And write to WCID attribute table.
	offset = MAC_WCID_ATTRIBUTE_BASE + (Wcid* HW_WCID_ATTRI_SIZE);

	if ((CipherAlg == CIPHER_WEP64) ||(CipherAlg == CIPHER_WEP128) ||(CipherAlg == CIPHER_CKIP128) ||(CipherAlg == CIPHER_CKIP64) )
	{
		// WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:1=PAIRWISE KEY, BSSIdx is 0
		WCIDAttri = (BssIdx<<4) |(CipherAlg<<1)|SHAREDKEYTABLE;
		RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
	}
	else if (CipherAlg == CIPHER_NONE)
	{
		// WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:1=PAIRWISE KEY, BSSIdx is 0
		WCIDAttri = (BssIdx<<4) |SHAREDKEYTABLE;
		RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
	}
	else
	{
		// Add WPAPSK key 

	}
	
	offset = MAC_IVEIV_TABLE_BASE + (Wcid * HW_IVEIV_ENTRY_SIZE);
	NdisZeroMemory(IVEIV, 8);
	// IV/EIV
	if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES))
	{
		IVEIV[3] = 0x20;	// Eiv bit on. keyid always 0 for pairwise key

	}
	// default key idx needs to set.   in TKIP/AES KeyIdx = 0 , WEP KeyIdx is default tx key.  
	{
		IVEIV[3] |= (KeyIdx<< 6);	
	}

	for (i=0; i<8; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, IVEIV[i]);
	}

	DBGPRINT(RT_DEBUG_TRACE,("%s: Alg=%s, KeyLength = %d\n", __FUNCTION__, CipherName[CipherAlg], 
		pAd->SharedKey[BssIdx][KeyIdx].KeyLen));
	DBGPRINT(RT_DEBUG_TRACE,("WCID(%d) : offset = %x, WCIDAttri = %x\n", Wcid, offset, WCIDAttri));
	DBGPRINT(RT_DEBUG_TRACE,("BSSID(%d): GroupKeyId = %x\n", BssIdx, KeyIdx));
}
#endif

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
*/
VOID AssocParmFill(
	IN PRTMP_ADAPTER pAd, 
	IN OUT MLME_ASSOC_REQ_STRUCT *AssocReq, 
	IN PUCHAR                     pAddr, 
	IN USHORT                     CapabilityInfo, 
	IN ULONG                      Timeout, 
	IN USHORT                     ListenIntv) 
{
	COPY_MAC_ADDR(AssocReq->Addr, pAddr);
	// Add mask to support 802.11b mode only
	AssocReq->CapabilityInfo = CapabilityInfo & SUPPORTED_CAPABILITY_INFO; // not cf-pollable, not cf-poll-request
	AssocReq->Timeout = Timeout;
	AssocReq->ListenIntv = ListenIntv;
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
*/
VOID DisassocParmFill(
	IN PRTMP_ADAPTER pAd, 
	IN OUT MLME_DISASSOC_REQ_STRUCT *DisassocReq, 
	IN PUCHAR pAddr, 
	IN USHORT Reason) 
{
	COPY_MAC_ADDR(DisassocReq->Addr, pAddr);
	DisassocReq->Reason = Reason;
}

/*
	========================================================================

	Routine Description:
		Check the out going frame, if this is an DHCP or ARP datagram
	will be duplicate another frame at low data rate transmit.
		
	Arguments:
		pAd 		Pointer to our adapter
		pPacket 	Pointer to outgoing Ndis frame
		
	Return Value:		
		TRUE		To be duplicate at Low data rate transmit. (1mb)
		FALSE		Do nothing.

	IRQL = DISPATCH_LEVEL
	
	Note:

		MAC header + IP Header + UDP Header
		  14 Bytes	  20 Bytes
		  
		UDP Header
		00|01|02|03|04|05|06|07|08|09|10|11|12|13|14|15|
						Source Port
		16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|
					Destination Port

		port 0x43 means Bootstrap Protocol, server. 
		Port 0x44 means Bootstrap Protocol, client. 

	========================================================================
*/

BOOLEAN RTMPCheckDHCPFrame(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PNDIS_PACKET	pPacket)
{
	PACKET_INFO 	PacketInfo;
	ULONG			NumberOfBytesRead = 0;
	ULONG			CurrentOffset = 0;	
	PVOID			pVirtualAddress = NULL; 
	UINT			NdisBufferLength;
	PUCHAR			pSrc;
	USHORT			Protocol;	
	UCHAR			ByteOffset36 = 0;
	UCHAR			ByteOffset38 = 0;
	BOOLEAN 		ReadFirstParm = TRUE;
		
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, (PUCHAR *)&pVirtualAddress, &NdisBufferLength);

	NumberOfBytesRead += NdisBufferLength;
	pSrc = (PUCHAR) pVirtualAddress;
	Protocol = *(pSrc + 12) * 256 + *(pSrc + 13);

	//
	// Check DHCP & BOOTP protocol
	//
	while (NumberOfBytesRead <= PacketInfo.TotalPacketLength)
	{
		if ((NumberOfBytesRead >= 35) && (ReadFirstParm == TRUE))
		{
			CurrentOffset = 35 - (NumberOfBytesRead - NdisBufferLength); 
			ByteOffset36 = *(pSrc + CurrentOffset);
			ReadFirstParm = FALSE;
		}
		
		if (NumberOfBytesRead >= 37)
		{
			CurrentOffset = 37 - (NumberOfBytesRead - NdisBufferLength); 
			ByteOffset38 = *(pSrc + CurrentOffset);
			//End of Read
			break; 
		}
		return FALSE;
	}

	// Check for DHCP & BOOTP protocol
	if ((ByteOffset36 != 0x44) || (ByteOffset38 != 0x43))
		{			
		// 
		// 2054 (hex 0806) for ARP datagrams
		// if this packet is not ARP datagrams, then do nothing
		// ARP datagrams will also be duplicate at 1mb broadcast frames
		//
		if (Protocol != 0x0806 )
			return FALSE;
		}

	return TRUE;
}


VOID Update_Rssi_Sample(
	IN PRTMP_ADAPTER	pAd,
	IN RSSI_SAMPLE		*pRssi,
	IN PRXWI_STRUC		pRxWI)
		{
	CHAR	rssi0 = pRxWI->RSSI0;
	CHAR	rssi1 = pRxWI->RSSI1;
	CHAR	rssi2 = pRxWI->RSSI2;

	if (rssi0 != 0)
	{
		pRssi->LastRssi0	= ConvertToRssi(pAd, (CHAR)rssi0, RSSI_0);
		pRssi->AvgRssi0X8	= (pRssi->AvgRssi0X8 - pRssi->AvgRssi0) + pRssi->LastRssi0;
		pRssi->AvgRssi0	= pRssi->AvgRssi0X8 >> 3;
	}

	if (rssi1 != 0)
	{   
		pRssi->LastRssi1	= ConvertToRssi(pAd, (CHAR)rssi1, RSSI_1);
		pRssi->AvgRssi1X8	= (pRssi->AvgRssi1X8 - pRssi->AvgRssi1) + pRssi->LastRssi1;
		pRssi->AvgRssi1	= pRssi->AvgRssi1X8 >> 3;
	}

	if (rssi2 != 0)
	{
		pRssi->LastRssi2	= ConvertToRssi(pAd, (CHAR)rssi2, RSSI_2);
		pRssi->AvgRssi2X8  = (pRssi->AvgRssi2X8 - pRssi->AvgRssi2) + pRssi->LastRssi2;
		pRssi->AvgRssi2 = pRssi->AvgRssi2X8 >> 3;
	}
		}

PNDIS_PACKET GetPacketFromRxRing(
	IN		PRTMP_ADAPTER	pAd,
	OUT		PRXD_STRUC		pSaveRxD,
	OUT		BOOLEAN			*pbReschedule,
	IN OUT	UINT32			*pRxPending)
{
	PRXD_STRUC				pRxD;
#ifdef BIG_ENDIAN
	PRXD_STRUC				pDestRxD;
	RXD_STRUC				RxD;
#endif
	PNDIS_PACKET			pRxPacket = NULL;
	PNDIS_PACKET			pNewPacket;
	PVOID					AllocVa;
	NDIS_PHYSICAL_ADDRESS	AllocPa;
	BOOLEAN					bReschedule = FALSE;

	RTMP_SEM_LOCK(&pAd->RxRingLock);

	if (*pRxPending == 0)
	{
		// Get how may packets had been received
		RTMP_IO_READ32(pAd, RX_DRX_IDX , &pAd->RxRing.RxDmaIdx);

		if (pAd->RxRing.RxSwReadIdx == pAd->RxRing.RxDmaIdx)
		{
			// no more rx packets
			bReschedule = FALSE;
			goto done;
		}

		// get rx pending count
		if (pAd->RxRing.RxDmaIdx > pAd->RxRing.RxSwReadIdx)
			*pRxPending = pAd->RxRing.RxDmaIdx - pAd->RxRing.RxSwReadIdx;
		else
			*pRxPending	= pAd->RxRing.RxDmaIdx + RX_RING_SIZE - pAd->RxRing.RxSwReadIdx;

	}

#ifndef BIG_ENDIAN
	// Point to Rx indexed rx ring descriptor
	pRxD = (PRXD_STRUC) pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].AllocVa;
#else
	pDestRxD = (PRXD_STRUC) pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].AllocVa;
	RxD = *pDestRxD;
	pRxD = &RxD;
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#endif

	if (pRxD->DDONE == 0)
	{
		*pRxPending = 0;
		// DMAIndx had done but DDONE bit not ready
		bReschedule = TRUE;
		goto done;
	}


	// return rx descriptor
	NdisMoveMemory(pSaveRxD, pRxD, RXD_SIZE);

	pNewPacket = RTMP_AllocateRxPacketBuffer(pAd, RX_BUFFER_AGGRESIZE, FALSE, &AllocVa, &AllocPa);

	if (pNewPacket)
	{
		// unmap the rx buffer
		PCI_UNMAP_SINGLE(pAd, pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].DmaBuf.AllocPa,
					 pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].DmaBuf.AllocSize, PCI_DMA_FROMDEVICE);
		pRxPacket = pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].pNdisPacket;

		pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].DmaBuf.AllocSize	= RX_BUFFER_AGGRESIZE;
		pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].pNdisPacket		= (PNDIS_PACKET) pNewPacket;
		pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].DmaBuf.AllocVa	= AllocVa;
		pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].DmaBuf.AllocPa	= AllocPa;
		/* update SDP0 to new buffer of rx packet */
		pRxD->SDP0 = AllocPa;
	} 
	else 
	{
		//printk("No Rx Buffer\n");
		pRxPacket = NULL;
		bReschedule = TRUE;
	}

	pRxD->DDONE = 0;

	// had handled one rx packet
	*pRxPending = *pRxPending - 1;	

	// update rx descriptor and kick rx 
#ifdef BIG_ENDIAN
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
	WriteBackToDescriptor((PUCHAR)pDestRxD, (PUCHAR)pRxD, FALSE, TYPE_RXD);
#endif
	INC_RING_INDEX(pAd->RxRing.RxSwReadIdx, RX_RING_SIZE);

	pAd->RxRing.RxCpuIdx = (pAd->RxRing.RxSwReadIdx == 0) ? (RX_RING_SIZE-1) : (pAd->RxRing.RxSwReadIdx-1);
	RTMP_IO_WRITE32(pAd, RX_CRX_IDX, pAd->RxRing.RxCpuIdx);

done:
	RTMP_SEM_UNLOCK(&pAd->RxRingLock);
	*pbReschedule = bReschedule;
	return pRxPacket;
	}


// Normal legacy Rx packet indication
VOID Indicate_Legacy_Packet(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
	{
	PNDIS_PACKET	pRxPacket = pRxBlk->pRxPacket;
	UCHAR			Header802_3[LENGTH_802_3];

	// 1. get 802.3 Header
	// 2. remove LLC 
	// 		a. pointer pRxBlk->pData to payload 
	//      b. modify pRxBlk->DataSize
	RTMP_802_11_REMOVE_LLC_AND_CONVERT_TO_802_3(pRxBlk, Header802_3);

	if (pRxBlk->DataSize > 1520)
	{
		static int err_size;

		err_size++;
		if (err_size > 20) 
		{
			 printk("Legacy DataSize = %d\n", pRxBlk->DataSize);
			 hex_dump("802.3 Header", Header802_3, LENGTH_802_3);
			 hex_dump("Payload", pRxBlk->pData, 64);
			 err_size = 0;
		}
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}


	STATS_INC_RX_PACKETS(pAd, FromWhichBSSID);

	wlan_802_11_to_802_3_packet(pAd, pRxBlk, Header802_3, FromWhichBSSID);

		// 
	// pass this 802.3 packet to upper layer or forward this packet to WM directly
		//
	ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pRxPacket, FromWhichBSSID);

	}

// Normal, AMPDU or AMSDU
VOID CmmRxnonRalinkFrameIndicate(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
{

	if (RX_BLK_TEST_FLAG(pRxBlk, fRX_AMPDU) && (pAd->CommonCfg.bDisableReordering == 0)) 
	{
		Indicate_AMPDU_Packet(pAd, pRxBlk, FromWhichBSSID);
	} 
	else 
	{
		if (RX_BLK_TEST_FLAG(pRxBlk, fRX_AMSDU))
		{  
			// handle A-MSDU
			Indicate_AMSDU_Packet(pAd, pRxBlk, FromWhichBSSID);
		}
		else
		{						 
			Indicate_Legacy_Packet(pAd, pRxBlk, FromWhichBSSID);
		}
	}
}


VOID CmmRxRalinkFrameIndicate(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
{
	UCHAR			Header802_3[LENGTH_802_3];
	UINT16			Msdu2Size;
	UINT16 			Payload1Size, Payload2Size;
	PUCHAR 			pData2;
	PNDIS_PACKET	pPacket2 = NULL;



	Msdu2Size = *(pRxBlk->pData) + (*(pRxBlk->pData+1) << 8);

	if ((Msdu2Size <= 1536) && (Msdu2Size < pRxBlk->DataSize))
	{
		/* skip two byte MSDU2 len */
		pRxBlk->pData += 2;
		pRxBlk->DataSize -= 2;
	}
	else
	{
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}

	// get 802.3 Header and  remove LLC
	RTMP_802_11_REMOVE_LLC_AND_CONVERT_TO_802_3(pRxBlk, Header802_3);


	ASSERT(pRxBlk->pRxPacket);

	// Ralink Aggregation frame	
	pAd->RalinkCounters.OneSecRxAggregationCount ++;
	Payload1Size = pRxBlk->DataSize - Msdu2Size;
	Payload2Size = Msdu2Size - LENGTH_802_3;

	pData2 = pRxBlk->pData + Payload1Size + LENGTH_802_3;
#ifdef CONFIG_AP_SUPPORT
	pPacket2 = duplicate_pkt_with_VLAN(pAd, Header802_3, LENGTH_802_3, pData2, Payload2Size, FromWhichBSSID);
#endif // CONFIG_AP_SUPPORT //

	if (!pPacket2)
	{
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}

	// update payload size of 1st packet
	pRxBlk->DataSize = Payload1Size;
	wlan_802_11_to_802_3_packet(pAd, pRxBlk, Header802_3, FromWhichBSSID);

	ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pRxBlk->pRxPacket, FromWhichBSSID);

	if (pPacket2)
	{
		ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pPacket2, FromWhichBSSID);
	}
}

#define RESET_FRAGFRAME(_fragFrame) \
	{								\
		_fragFrame.RxSize = 0;		\
		_fragFrame.Sequence = 0;	\
		_fragFrame.LastFrag = 0;	\
		_fragFrame.Flags = 0;		\
	}

PNDIS_PACKET RTMPDeFragmentDataFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	PNDIS_PACKET	pRxPacket = pRxBlk->pRxPacket;
	UCHAR			*pData = pRxBlk->pData;
	USHORT			DataSize = pRxBlk->DataSize;
	PNDIS_PACKET	pRetPacket = NULL;
	UCHAR			*pFragBuffer = NULL;
	BOOLEAN 		bReassDone = FALSE;
	UCHAR			HeaderRoom = 0;


	ASSERT(pHeader);

	HeaderRoom = pData - (UCHAR *)pHeader;

	// Re-assemble the fragmented packets
	if (pHeader->Frag == 0)		// Frag. Number is 0 : First frag or only one pkt
	{
		// the first pkt of fragment, record it.
		if (pHeader->FC.MoreFrag)
		{
			ASSERT(pAd->FragFrame.pFragPacket);
			pFragBuffer = GET_OS_PKT_DATAPTR(pAd->FragFrame.pFragPacket);
			pAd->FragFrame.RxSize   = DataSize + HeaderRoom;
			NdisMoveMemory(pFragBuffer,	 pHeader, pAd->FragFrame.RxSize);
			pAd->FragFrame.Sequence = pHeader->Sequence;
			pAd->FragFrame.LastFrag = pHeader->Frag;	   // Should be 0
			ASSERT(pAd->FragFrame.LastFrag == 0);
			goto done;	// end of processing this frame
		}
	}
	else	//Middle & End of fragment
	{
		if ((pHeader->Sequence != pAd->FragFrame.Sequence) ||
			(pHeader->Frag != (pAd->FragFrame.LastFrag + 1)))
		{
			// Fragment is not the same sequence or out of fragment number order
			// Reset Fragment control blk
			RESET_FRAGFRAME(pAd->FragFrame);
			DBGPRINT(RT_DEBUG_ERROR, ("Fragment is not the same sequence or out of fragment number order.\n"));
			goto done; // give up this frame
		}
		else if ((pAd->FragFrame.RxSize + DataSize) > MAX_FRAME_SIZE)
		{
			// Fragment frame is too large, it exeeds the maximum frame size.
			// Reset Fragment control blk
			RESET_FRAGFRAME(pAd->FragFrame);
			DBGPRINT(RT_DEBUG_ERROR, ("Fragment frame is too large, it exeeds the maximum frame size.\n"));
			goto done; // give up this frame
		}

		pFragBuffer = GET_OS_PKT_DATAPTR(pAd->FragFrame.pFragPacket);

		// concatenate this fragment into the re-assembly buffer			
		NdisMoveMemory((pFragBuffer + pAd->FragFrame.RxSize), pData, DataSize);
		pAd->FragFrame.RxSize  += DataSize;
		pAd->FragFrame.LastFrag = pHeader->Frag;	   // Update fragment number

		// Last fragment
		if (pHeader->FC.MoreFrag == FALSE)
		{           
			bReassDone = TRUE;
		}
	}

done:
	// always release rx fragmented packet
	RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);

	// return defragmented packet if packet is reassembled completely
	// otherwise return NULL
	if (bReassDone)
	{
		PNDIS_PACKET pNewFragPacket;

		// allocate a new packet buffer for fragment
		pNewFragPacket = RTMP_AllocateFragPacketBuffer(pAd, RX_BUFFER_NORMSIZE);
		if (pNewFragPacket)
		{
			// update RxBlk
			pRetPacket = pAd->FragFrame.pFragPacket;
			pAd->FragFrame.pFragPacket = pNewFragPacket;
			pRxBlk->pHeader = (PHEADER_802_11) GET_OS_PKT_DATAPTR(pRetPacket);
			pRxBlk->pData = (UCHAR *)pRxBlk->pHeader + HeaderRoom;
			pRxBlk->DataSize = pAd->FragFrame.RxSize - HeaderRoom;
			pRxBlk->pRxPacket = pRetPacket;
		}
		else
		{
			RESET_FRAGFRAME(pAd->FragFrame);
		}
	}

	return pRetPacket;
}


VOID Indicate_AMSDU_Packet(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
{
	UINT			nMSDU;

	update_os_packet_info(pAd, pRxBlk, FromWhichBSSID);
	RTMP_SET_PACKET_IF(pRxBlk->pRxPacket, FromWhichBSSID);
	nMSDU = deaggregate_AMSDU_announce(pAd, pRxBlk->pRxPacket, pRxBlk->pData, pRxBlk->DataSize);
}

