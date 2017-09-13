/*
   All functions in this file must be PCI-depended, or you should out your function
	in other files.

*/
#include	"rt_config.h"

extern RTMP_RF_REGS RF2850RegTable[];
extern UCHAR	NUM_OF_2850_CHNL;

USHORT RtmpPCI_WriteTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	BOOLEAN			bIsLast,
	OUT	USHORT			*FreeNumber)
{

	UCHAR			*pDMAHeaderBufVA;
	USHORT			TxIdx, RetTxIdx;
	PTXD_STRUC		pTxD;
	UINT32			BufBasePaLow;
	PRTMP_TX_RING	pTxRing;
	ULONG			SrcBufPA;
	USHORT			hwHeaderLen;


	SrcBufPA = PCI_MAP_SINGLE(pAd, (char *) pTxBlk->pSrcBufData, pTxBlk->SrcBufLen, PCI_DMA_TODEVICE);

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	TxIdx = pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx;
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	// copy TXINFO + TXWI + WLAN Header + LLC into DMA Header Buffer
	if (pTxBlk->TxFrameType == TX_AMSDU_FRAME)
	{
		//hwHeaderLen = ROUND_UP(pTxBlk->MpduHeaderLen-LENGTH_AMSDU_SUBFRAMEHEAD, 4)+LENGTH_AMSDU_SUBFRAMEHEAD;
		hwHeaderLen = pTxBlk->MpduHeaderLen - LENGTH_AMSDU_SUBFRAMEHEAD + pTxBlk->HdrPadLen + LENGTH_AMSDU_SUBFRAMEHEAD;
	}
	else
	{
		//hwHeaderLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);
		hwHeaderLen = pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;
	}
	NdisMoveMemory(pDMAHeaderBufVA, pTxBlk->HeaderBuf, TXINFO_SIZE + TXWI_SIZE + hwHeaderLen);

	pTxRing->Cell[TxIdx].pNdisPacket = pTxBlk->pPacket;
	pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;

	//
	// build Tx Descriptor
	// 

	pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	NdisZeroMemory(pTxD, TXD_SIZE);

	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = TXINFO_SIZE + TXWI_SIZE + hwHeaderLen; // include padding
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = pTxBlk->SrcBufLen;
	pTxD->LastSec0 = 0;
	pTxD->LastSec1 = (bIsLast) ? 1 : 0;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);

	RetTxIdx = TxIdx;
	//
	// Update Tx index
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pTxRing->TxCpuIdx = TxIdx;

	*FreeNumber -= 1;

	return RetTxIdx;
}


USHORT RtmpPCI_WriteSingleTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	BOOLEAN			bIsLast,
	OUT	USHORT			*FreeNumber)
{

	UCHAR			*pDMAHeaderBufVA;
	USHORT			TxIdx, RetTxIdx;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	UINT32			BufBasePaLow;
	PRTMP_TX_RING	pTxRing;
	ULONG			SrcBufPA;
	USHORT			hwHeaderLen;


	SrcBufPA = PCI_MAP_SINGLE(pAd, (char *) pTxBlk->pSrcBufData, pTxBlk->SrcBufLen, PCI_DMA_TODEVICE);

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	TxIdx = pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx;
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	// copy TXINFO + TXWI + WLAN Header + LLC into DMA Header Buffer
	//hwHeaderLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);
	hwHeaderLen = pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;

	NdisMoveMemory(pDMAHeaderBufVA, pTxBlk->HeaderBuf, TXINFO_SIZE + TXWI_SIZE + hwHeaderLen);

	pTxRing->Cell[TxIdx].pNdisPacket = pTxBlk->pPacket;
	pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;

	//
	// build Tx Descriptor
	// 
#ifndef BIG_ENDIAN
	pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
#else
	pDestTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
#endif
	NdisZeroMemory(pTxD, TXD_SIZE);

	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = TXINFO_SIZE + TXWI_SIZE + hwHeaderLen; // include padding
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = pTxBlk->SrcBufLen;
	pTxD->LastSec0 = 0;
	pTxD->LastSec1 = (bIsLast) ? 1 : 0;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)(pDMAHeaderBufVA + TXINFO_SIZE), TYPE_TXWI);
	RTMPFrameEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA + TXINFO_SIZE + TXWI_SIZE), DIR_WRITE, FALSE);
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);	
#endif // BIG_ENDIAN //

	RetTxIdx = TxIdx;
	//
	// Update Tx index
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pTxRing->TxCpuIdx = TxIdx;

	*FreeNumber -= 1;

	return RetTxIdx;
}


USHORT RtmpPCI_WriteMultiTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	UCHAR			frameNum,
	OUT	USHORT			*FreeNumber)
{
	BOOLEAN bIsLast;
	UCHAR			*pDMAHeaderBufVA;
	USHORT			TxIdx, RetTxIdx;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	UINT32			BufBasePaLow;
	PRTMP_TX_RING	pTxRing;
	ULONG			SrcBufPA;
	USHORT			hwHdrLen;
	UINT32			firstDMALen;

	bIsLast = ((frameNum == (pTxBlk->TotalFrameNum - 1)) ? 1 : 0);

	
	SrcBufPA = PCI_MAP_SINGLE(pAd, (char *) pTxBlk->pSrcBufData, pTxBlk->SrcBufLen, PCI_DMA_TODEVICE);

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	TxIdx = pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx;
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	if (frameNum == 0)
	{
		// copy TXINFO + TXWI + WLAN Header + LLC into DMA Header Buffer
		if (pTxBlk->TxFrameType == TX_AMSDU_FRAME)
			//hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen-LENGTH_AMSDU_SUBFRAMEHEAD, 4)+LENGTH_AMSDU_SUBFRAMEHEAD;
			hwHdrLen = pTxBlk->MpduHeaderLen - LENGTH_AMSDU_SUBFRAMEHEAD + pTxBlk->HdrPadLen + LENGTH_AMSDU_SUBFRAMEHEAD;
		else if (pTxBlk->TxFrameType == TX_RALINK_FRAME)
			//hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen-LENGTH_ARALINK_HEADER_FIELD, 4)+LENGTH_ARALINK_HEADER_FIELD;
			hwHdrLen = pTxBlk->MpduHeaderLen - LENGTH_ARALINK_HEADER_FIELD + pTxBlk->HdrPadLen + LENGTH_ARALINK_HEADER_FIELD;
		else
			//hwHdrLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);
			hwHdrLen = pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen;

		firstDMALen = TXINFO_SIZE + TXWI_SIZE + hwHdrLen;
	}
	else
	{
		firstDMALen = pTxBlk->MpduHeaderLen;
	}

	NdisMoveMemory(pDMAHeaderBufVA, pTxBlk->HeaderBuf, firstDMALen); 
		
	pTxRing->Cell[TxIdx].pNdisPacket = pTxBlk->pPacket;
	pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;
	
	//
	// build Tx Descriptor
	// 
#ifndef BIG_ENDIAN
	pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
#else
	pDestTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
#endif
	NdisZeroMemory(pTxD, TXD_SIZE);

	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = firstDMALen; // include padding
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = pTxBlk->SrcBufLen;
	pTxD->LastSec0 = 0;
	pTxD->LastSec1 = (bIsLast) ? 1 : 0;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);

#ifdef BIG_ENDIAN
	if (frameNum == 0)
		RTMPFrameEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA+ TXINFO_SIZE + TXWI_SIZE), DIR_WRITE, FALSE);
	
	if (frameNum != 0)
		RTMPWIEndianChange((PUCHAR)(pDMAHeaderBufVA + TXINFO_SIZE), TYPE_TXWI);
	
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
	WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif // BIG_ENDIAN //
	
	RetTxIdx = TxIdx;
	//
	// Update Tx index
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pTxRing->TxCpuIdx = TxIdx;

	*FreeNumber -= 1;

	return RetTxIdx;
	
}


VOID RtmpPCI_FinalWriteTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	USHORT			totalMPDUSize,
	IN	USHORT			FirstTxIdx)
{

	PTXWI_STRUC		pTxWI;
	PRTMP_TX_RING	pTxRing;

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	pTxWI = (PTXWI_STRUC) pTxRing->Cell[FirstTxIdx].DmaBuf.AllocVa;
	pTxWI->MPDUtotalByteCount = totalMPDUSize;
#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)pTxWI, TYPE_TXWI);
#endif // BIG_ENDIAN //

}


VOID RtmpPCIDataLastTxIdx(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			QueIdx,
	IN	USHORT			LastTxIdx)
{
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	PRTMP_TX_RING	pTxRing;

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[QueIdx];

	//
	// build Tx Descriptor
	// 
#ifndef BIG_ENDIAN
	pTxD = (PTXD_STRUC) pTxRing->Cell[LastTxIdx].AllocVa;
#else
	pDestTxD = (PTXD_STRUC) pTxRing->Cell[LastTxIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
#endif

	pTxD->LastSec1 = 1;

#ifdef BIG_ENDIAN
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif // BIG_ENDIAN //

}


USHORT	RtmpPCI_WriteFragTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	UCHAR			fragNum,
	OUT	USHORT			*FreeNumber)
{
	UCHAR			*pDMAHeaderBufVA;
	USHORT			TxIdx, RetTxIdx;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	UINT32			BufBasePaLow, SrcBufPA;
	PRTMP_TX_RING	pTxRing;
	USHORT			hwHeaderLen;
	UINT32			firstDMALen;


	//
	// Get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	TxIdx = pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx;
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	//
	// Copy TXINFO + TXWI + WLAN Header + LLC into DMA Header Buffer
	//
	hwHeaderLen = ROUND_UP(pTxBlk->MpduHeaderLen, 4);

	firstDMALen = TXINFO_SIZE + TXWI_SIZE + hwHeaderLen;
	NdisMoveMemory(pDMAHeaderBufVA, pTxBlk->HeaderBuf, firstDMALen); 
		

	//
	// Build Tx Descriptor
	// 
#ifndef BIG_ENDIAN
	pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
#else
	pDestTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
#endif
	NdisZeroMemory(pTxD, TXD_SIZE);
	SrcBufPA = PCI_MAP_SINGLE(pAd, (char *)(pTxBlk->pSrcBufData), pTxBlk->SrcBufLen, PCI_DMA_TODEVICE);
	
	if (fragNum == pTxBlk->TotalFragNum)
	{
		pTxRing->Cell[TxIdx].pNdisPacket = pTxBlk->pPacket;
		pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;
	}
	
	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = firstDMALen; // include padding
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = pTxBlk->SrcBufLen;
	pTxD->LastSec0 = 0;
	pTxD->LastSec1 = 1;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);

#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)(pDMAHeaderBufVA + TXINFO_SIZE), TYPE_TXWI);
	RTMPFrameEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA + TXINFO_SIZE + TXWI_SIZE), DIR_WRITE, FALSE);
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);	
#endif // BIG_ENDIAN //

	RetTxIdx = TxIdx;
	pTxBlk->Priv += pTxBlk->SrcBufLen;
	
	//
	// Update Tx index
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pTxRing->TxCpuIdx = TxIdx;

	*FreeNumber -= 1;

	return RetTxIdx;
	
}

#if 0
USHORT RtmpPCI_WriteSubTxResource(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	BOOLEAN			bIsLast,
	OUT	USHORT			*FreeNumber)
{

	UCHAR			*pDMAHeaderBufVA;
	USHORT			TxIdx;
	PTXD_STRUC		pTxD;
	UINT32			BufBasePaLow;
	PRTMP_TX_RING	pTxRing;
	ULONG			SrcBufPA;
	USHORT			LastTxIdx;


	SrcBufPA = PCI_MAP_SINGLE(pAd, (char *) pTxBlk->pSrcBufData, pTxBlk->SrcBufLen, PCI_DMA_TODEVICE);

	//
	// get Tx Ring Resource
	// 
	pTxRing = &pAd->TxRing[pTxBlk->QueIdx];
	TxIdx = pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx;
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	NdisMoveMemory(pDMAHeaderBufVA, pTxBlk->HeaderBuf, pTxBlk->MpduHeaderLen); 

	pTxRing->Cell[TxIdx].pNdisPacket = pTxBlk->pPacket;
	pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;

	//
	// build Tx Descriptor
	// 

	pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	NdisZeroMemory(pTxD, TXD_SIZE);

	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = pTxBlk->MpduHeaderLen; // include padding
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = pTxBlk->SrcBufLen;
	pTxD->LastSec0 = 0;
	pTxD->LastSec1 = (bIsLast) ? 1 : 0;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);

	LastTxIdx = TxIdx;

	//
	// Update Tx index
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pTxRing->TxCpuIdx = TxIdx;

	*FreeNumber -= 1;

	return LastTxIdx;
}


VOID RtmpPCIDataKickOut(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	UCHAR			QueIdx)
{
	RTMP_IO_WRITE32(pAd, TX_CTX_IDX0+(pTxBlk->QueIdx*0x10), pAd->TxRing[pTxBlk->QueIdx].TxCpuIdx);
}
#endif


/*
	Must be run in Interrupt context
	This function handle PCI specific TxDesc and cpu index update and kick the packet out.
 */
int RtmpPCIMgmtKickOut(
	IN RTMP_ADAPTER 	*pAd, 
	IN UCHAR 			QueIdx,
	IN PNDIS_PACKET		pPacket,
	IN PUCHAR			pSrcBufVA,
	IN UINT 			SrcBufLen)
{
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif
	ULONG			SwIdx = pAd->MgmtRing.TxCpuIdx;
	ULONG			SrcBufPA;

	
#ifdef BIG_ENDIAN
    pDestTxD  = (PTXD_STRUC)pAd->MgmtRing.Cell[SwIdx].AllocVa;
    TxD = *pDestTxD;
    pTxD = &TxD;
    RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#else
	pTxD  = (PTXD_STRUC) pAd->MgmtRing.Cell[SwIdx].AllocVa;
#endif

	pAd->MgmtRing.Cell[SwIdx].pNdisPacket = pPacket;
	pAd->MgmtRing.Cell[SwIdx].pNextNdisPacket = NULL;

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

	return 0;
}



VOID RT28xxPciMlmeRadioOn(
	IN PRTMP_ADAPTER pAd)
{    
    if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
   		return;
    
    DBGPRINT(RT_DEBUG_TRACE,("%s===>\n", __FUNCTION__));   
    
    if ((pAd->OpMode == OPMODE_AP) ||
        ((pAd->OpMode == OPMODE_STA) && (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_ADVANCE_POWER_SAVE_PCIE_DEVICE))))
    {
	    NICResetFromError(pAd);

    	RTMPRingCleanUp(pAd, QID_AC_BK);
    	RTMPRingCleanUp(pAd, QID_AC_BE);
    	RTMPRingCleanUp(pAd, QID_AC_VI);
    	RTMPRingCleanUp(pAd, QID_AC_VO);
    	RTMPRingCleanUp(pAd, QID_HCCA);
    	RTMPRingCleanUp(pAd, QID_MGMT);
    	RTMPRingCleanUp(pAd, QID_RX);

    	// Enable Tx/Rx
    	RTMPEnableRxTx(pAd);
    	
    	// Clear Radio off flag
    	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF);

	    // Set LED
	    RTMPSetLED(pAd, LED_RADIO_ON);
    }

}

VOID RT28xxPciMlmeRadioOFF(
	IN PRTMP_ADAPTER pAd)
{
    WPDMA_GLO_CFG_STRUC	GloCfg;
	UINT32	Value = 0, i;
    
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
    	return;
    
    DBGPRINT(RT_DEBUG_TRACE,("%s===>\n", __FUNCTION__));
            
	// Set LED
	RTMPSetLED(pAd, LED_RADIO_OFF);
	// Set Radio off flag
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF);


	// Disable Tx/Rx DMA
	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);	   // disable DMA 
	GloCfg.field.EnableTxDMA = 0;
	GloCfg.field.EnableRxDMA = 0;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);	   // abort all TX rings

	// Disable MAC Tx/Rx
	RTMP_IO_READ32(pAd, MAC_SYS_CTRL, &Value);
	Value &= (0xfffffff3);
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, Value);

	// Waiting for DMA idle
	i = 0;
	do
	{
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0) && (GloCfg.field.RxDMABusy == 0))
			break;
		
		RTMPusecDelay(1000);
	}while (i++ < 100);


}
