/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	2860_rtmp_init.c

	Abstract:
	Miniport generic portion header file

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Paul Lin    2002-08-01    created
    John Chang  2004-08-20    RT2561/2661 use scatter-gather scheme
    Jan Lee  2006-09-15    RT2860. Change for 802.11n , EEPROM, Led, BA, HT.
*/
#include	"rt_config.h"




/*
	========================================================================
	
	Routine Description:
		Allocate DMA memory blocks for send, receive

	Arguments:
		Adapter		Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_FAILURE
		NDIS_STATUS_RESOURCES

	IRQL = PASSIVE_LEVEL

	Note:
	
	========================================================================
*/
NDIS_STATUS	RTMPAllocTxRxRingMemory(
	IN	PRTMP_ADAPTER	pAd)
{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	ULONG			RingBasePaHigh;
	ULONG			RingBasePaLow;
	PVOID			RingBaseVa;
	INT				index, num;
	PTXD_STRUC		pTxD;
	PRXD_STRUC		pRxD;
	ULONG			ErrorValue = 0;
	PRTMP_TX_RING	pTxRing;
	PRTMP_DMABUF	pDmaBuf;
	PNDIS_PACKET	pPacket;
//	PRTMP_REORDERBUF	pReorderBuf;

	DBGPRINT(RT_DEBUG_TRACE, ("--> RTMPAllocTxRxRingMemory\n"));
	do
	{
#ifdef WIN_NDIS
#ifndef UNDER_CE
#if ME_98
		//
		// NDIS constraint
		//   1.) ScatterGatherDma is not support on Win9x/Me, used NdisMAllocateMapRegisters instead.
		//   2.) The NDIS-imposed limit of 64 map registers per miniport driver.
		//
		//       Our send-packet size was 2304 bytes, given a page size of 4K bytes
		//       a buffer of 2304 bytes can span two physical pages, NDIS may allocate
		//       two map registers per requested base register. So to stay within
		//       the limit of 64 map register, our miniport driver must request no more
		//       than 32 base map registers.
		//
		pAd->MapRegisters = MAX_MAP_REGISTERS_NEEDED;
		Status = NDIS_STATUS_RESOURCES;

		while(pAd->MapRegisters > (MIN_MAP_REGISTERS_NEEDED - 1))
		{
			Status = NdisMAllocateMapRegisters(
												pAd->AdapterHandle,     // handle input to MiniportInitialize.
												0, 
												NDIS_DMA_32BITS,        // address size that the NIC uses for DMA operations
												pAd->MapRegisters,      // maximum number of map registers
												NIC_MAX_PACKET_SIZE);   // maximum number of bytes that the NIC can
												                        // transfer in a single DMA operation
			//
			// If Call succeeded, Maximum number of map registers
			// requested have been allocated else the requested number
			// of map refpacketgisters could not be allocated due to system
			// resource constraints.
			//			
			if(Status == NDIS_STATUS_SUCCESS)
			{
				break;
			}

			//
			// Else Reduce the request size and try again till you succeed or are out of resources.
			//
			pAd->MapRegisters--;
		}

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGPRINT_ERR(("NdisMAllocateMapRegisters failed, out of resources, Status[=0x%08x]\n", Status));
			break;
		}

		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_MAP_REGISTER);
		DBGPRINT(RT_DEBUG_TRACE, ("Number[=%d] of map registers requested have been allocated\n", pAd->MapRegisters));
#else
		//
		// Windows 2K or Higher suppport ScatterGather DMA.
		// First, Try to use the ScatterGather method , this is the preferred way
		// 
		Status = NdisMInitializeScatterGatherDma(
						pAd->AdapterHandle,
						FALSE,
						1000);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Use Scatter-Gather DMA\n"));
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_SCATTER_GATHER);
		}
		else
		{
			DBGPRINT(RT_DEBUG_WARN, ("Failed to init scatter-gather DMA, Status[=0x%08x]\n", Status));

			//
			// Second, if ScatterGatherDma failed, then try NdisMAllocateMapRegisters instead.
			//
			pAd->MapRegisters = MAX_MAP_REGISTERS_NEEDED;
			while(pAd->MapRegisters > (MIN_MAP_REGISTERS_NEEDED - 1))
			{
				Status = NdisMAllocateMapRegisters(
													pAd->AdapterHandle,     // handle input to MiniportInitialize.
													0, 
													NDIS_DMA_32BITS,        // address size that the NIC uses for DMA operations
													pAd->MapRegisters,      // maximum number of map registers
													NIC_MAX_PACKET_SIZE);   // maximum number of bytes that the NIC can
													                        // transfer in a single DMA operation
				//
				// If Call succeeded, Maximum number of map registers
				// requested have been allocated else the requested number
				// of map registers could not be allocated due to system
				// resource constraints.
				//			
				if(Status == NDIS_STATUS_SUCCESS)
				{
					break;
				}

				//
				// Else Reduce the request size and try again till you succeed or are out of resources.
				//
				pAd->MapRegisters--;
			}

			if (Status == NDIS_STATUS_SUCCESS)
			{
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_MAP_REGISTER);
				DBGPRINT(RT_DEBUG_TRACE, ("Number[=%d] of map registers requested have been allocated\n", pAd->MapRegisters));
			}
			else
			{
				ErrorValue = ERRLOG_OUT_OF_MAP_REGISTERS;
				DBGPRINT_ERR(("Failed to allocate map registers, Status[=0x%08x]\n", Status));
				break;  
			}
		}
#endif //#if ME_98
#endif // #ifndef UNDER_CE

#else
		/* 
		 * reserves system resources for subsequent bus-master DMA operations
		 */

		/* RTMP_AllocateDMAMapRegister(pAd); */
#endif	/* WIN_NDIS */


#ifdef WIN_NDIS
		//
		// pre-allocate a big enough NDIS PACKET pool for internal created NDIS_PACKET usgae
		// we may create internal NDIS_PACKET upon -
		//      1. need to transmit a MGMT frame, RTS frame, NULL frame,..
		//      2. an NDIS packet passed from Windows contains too many physical buffers that exceeds
		//         what TXD can take. In this case, we have to clone this packet into a internal one 
		//         which has less physical buffers
		// NOTE: internally created NDIS PACKET should always has only one NDIS BUFFER
		//
		NdisAllocatePacketPool(
			&Status,
			&pAd->FreeNdisPacketPoolHandle,
			MAX_NUM_OF_FREE_NDIS_PACKET,
			sizeof(PVOID) * 4);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			ErrorValue = ERRLOG_OUT_OF_PACKET_POOL;
			DBGPRINT_ERR(("NdisAllocatePacketPool failed, Status[=0x%08x]\n", Status));
			break;
		}

		NdisAllocateBufferPool(
			&Status,
			&pAd->FreeNdisBufferPoolHandle,
			MAX_NUM_OF_FREE_NDIS_PACKET);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			NdisFreePacketPool(pAd->FreeNdisPacketPoolHandle);
			ErrorValue = ERRLOG_OUT_OF_BUFFER_POOL;
			DBGPRINT_ERR(("NdisAllocateBufferPool failed, Status[=0x%08x]\n", Status));
			break;
		}
#else
		/* Status = RTMP_Allocate_Packet_Buffer_Pool(pAd) */
#endif

#ifdef WIN_NDIS
		InitializeQueueHeader(&pAd->LocalTxBufQueue);
		for (index = 0; index < NUM_OF_LOCAL_TXBUF; index++)
		{
			pAd->LocalTxBuf[index].AllocSize = LOCAL_TXBUF_SIZE;
#ifdef WIN_NDIS
			NdisMAllocateSharedMemory(
				pAd->AdapterHandle,
				pAd->LocalTxBuf[index].AllocSize,
				TRUE,
				&pAd->LocalTxBuf[index].AllocVa,
				&pAd->LocalTxBuf[index].AllocPa);
#else
			RTMP_AllocateSharedMemory(
				pAd,
				pAd->LocalTxBuf[index].AllocSize,
				TRUE,
				&pAd->LocalTxBuf[index].AllocVa,
				&pAd->LocalTxBuf[index].AllocPa);
#endif


			if (!pAd->LocalTxBuf[index].AllocVa)
			{
				ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
				DBGPRINT_ERR(("Failed to allocate local TxBuf memory\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
			pAd->LocalTxBuf[index].Index = (UCHAR)index;
			NdisAcquireSpinLock(&pAd->LocalTxBufQueueLock);
			InsertHeadQueue(&pAd->LocalTxBufQueue, &pAd->LocalTxBuf[index]);
			NdisReleaseSpinLock(&pAd->LocalTxBufQueueLock);
		}
		DBGPRINT(RT_DEBUG_TRACE,("total %d Shared TxBUF pre-allocated !!!\n", pAd->LocalTxBufQueue.Number));
#endif /* WIN_NDIS */

		//
		// Allocate all ring descriptors, include TxD, RxD, MgmtD.
		// Although each size is different, to prevent cacheline and alignment
		// issue, I intentional set them all to 64 bytes.
		//
		for (num=0; num<NUM_OF_TX_RING; num++)
		{
			ULONG  BufBasePaHigh;
			ULONG  BufBasePaLow;
			PVOID  BufBaseVa;
			
			// 
			// Allocate Tx ring descriptor's memory (5 TX rings = 4 ACs + 1 HCCA)
			//
			pAd->TxDescRing[num].AllocSize = TX_RING_SIZE * TXD_SIZE;
#ifdef WIN_NDIS
			NdisMAllocateSharedMemory(
				pAd->AdapterHandle,
				pAd->TxDescRing[num].AllocSize,
				FALSE,
				&pAd->TxDescRing[num].AllocVa,
				&pAd->TxDescRing[num].AllocPa);
#else
			RTMP_AllocateTxDescMemory(
				pAd,
				num,
				pAd->TxDescRing[num].AllocSize,
				FALSE,
				&pAd->TxDescRing[num].AllocVa,
				&pAd->TxDescRing[num].AllocPa);
#endif

			if (pAd->TxDescRing[num].AllocVa == NULL)
			{
				ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
				DBGPRINT_ERR(("Failed to allocate a big buffer\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			// Zero init this memory block
			NdisZeroMemory(pAd->TxDescRing[num].AllocVa, pAd->TxDescRing[num].AllocSize);

			// Save PA & VA for further operation
#ifdef WIN_NDIS
			RingBasePaHigh = NdisGetPhysicalAddressHigh(pAd->TxDescRing[num].AllocPa);
			RingBasePaLow  = NdisGetPhysicalAddressLow (pAd->TxDescRing[num].AllocPa);
#else
			RingBasePaHigh = RTMP_GetPhysicalAddressHigh(pAd->TxDescRing[num].AllocPa);
			RingBasePaLow  = RTMP_GetPhysicalAddressLow (pAd->TxDescRing[num].AllocPa);
#endif			
			RingBaseVa     = pAd->TxDescRing[num].AllocVa;

			// 
			// Allocate all 1st TXBuf's memory for this TxRing
			//
			pAd->TxBufSpace[num].AllocSize = TX_RING_SIZE * TX_DMA_1ST_BUFFER_SIZE;
#ifdef WIN_NDIS
			NdisMAllocateSharedMemory(
				pAd->AdapterHandle,
				pAd->TxBufSpace[num].AllocSize,
				FALSE,
				&pAd->TxBufSpace[num].AllocVa,
				&pAd->TxBufSpace[num].AllocPa);
#else
			RTMP_AllocateFirstTxBuffer(
				pAd,
				num,
				pAd->TxBufSpace[num].AllocSize,
				FALSE,
				&pAd->TxBufSpace[num].AllocVa,
				&pAd->TxBufSpace[num].AllocPa);
#endif


			if (pAd->TxBufSpace[num].AllocVa == NULL)
			{
				ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
				DBGPRINT_ERR(("Failed to allocate a big buffer\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			// Zero init this memory block
			NdisZeroMemory(pAd->TxBufSpace[num].AllocVa, pAd->TxBufSpace[num].AllocSize);

			// Save PA & VA for further operation
#ifdef WIN_NDIS
			BufBasePaHigh = NdisGetPhysicalAddressHigh(pAd->TxBufSpace[num].AllocPa);
			BufBasePaLow  = NdisGetPhysicalAddressLow (pAd->TxBufSpace[num].AllocPa);
#else
			BufBasePaHigh = RTMP_GetPhysicalAddressHigh(pAd->TxBufSpace[num].AllocPa);
			BufBasePaLow  = RTMP_GetPhysicalAddressLow (pAd->TxBufSpace[num].AllocPa);
#endif			
			BufBaseVa     = pAd->TxBufSpace[num].AllocVa;

			//
			// Initialize Tx Ring Descriptor and associated buffer memory
			//
			pTxRing = &pAd->TxRing[num];
			for (index = 0; index < TX_RING_SIZE; index++)
			{
				pTxRing->Cell[index].pNdisPacket = NULL;
				pTxRing->Cell[index].pNextNdisPacket = NULL;
				// Init Tx Ring Size, Va, Pa variables
				pTxRing->Cell[index].AllocSize = TXD_SIZE;
				pTxRing->Cell[index].AllocVa = RingBaseVa;
#ifdef WIN_NDIS
				NdisSetPhysicalAddressHigh(pTxRing->Cell[index].AllocPa, RingBasePaHigh);
				NdisSetPhysicalAddressLow (pTxRing->Cell[index].AllocPa, RingBasePaLow);
#else
				RTMP_SetPhysicalAddressHigh(pTxRing->Cell[index].AllocPa, RingBasePaHigh);
				RTMP_SetPhysicalAddressLow (pTxRing->Cell[index].AllocPa, RingBasePaLow);
#endif

				// Setup Tx Buffer size & address. only 802.11 header will store in this space
				pDmaBuf = &pTxRing->Cell[index].DmaBuf;
				pDmaBuf->AllocSize = TX_DMA_1ST_BUFFER_SIZE;
				pDmaBuf->AllocVa = BufBaseVa;
#ifdef WIN_NDIS
				NdisSetPhysicalAddressHigh(pDmaBuf->AllocPa, BufBasePaHigh);
				NdisSetPhysicalAddressLow(pDmaBuf->AllocPa, BufBasePaLow);
#else
				RTMP_SetPhysicalAddressHigh(pDmaBuf->AllocPa, BufBasePaHigh);
				RTMP_SetPhysicalAddressLow(pDmaBuf->AllocPa, BufBasePaLow);
#endif

				// link the pre-allocated TxBuf to TXD
				pTxD = (PTXD_STRUC) pTxRing->Cell[index].AllocVa;
				pTxD->SDPtr0 = BufBasePaLow;
				// advance to next ring descriptor address
				pTxD->DMADONE = 1;
#ifdef BIG_ENDIAN
				RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif
				RingBasePaLow += TXD_SIZE;
				RingBaseVa = (PUCHAR) RingBaseVa + TXD_SIZE;

				// advance to next TxBuf address
				BufBasePaLow += TX_DMA_1ST_BUFFER_SIZE;
				BufBaseVa = (PUCHAR) BufBaseVa + TX_DMA_1ST_BUFFER_SIZE;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("TxRing[%d]: total %d entry allocated\n", num, index));
		}
		if (Status == NDIS_STATUS_RESOURCES)
			break;

		//
		// Allocate MGMT ring descriptor's memory except Tx ring which allocated eariler
		//
		pAd->MgmtDescRing.AllocSize = MGMT_RING_SIZE * TXD_SIZE;
#ifdef WIN_NDIS
		NdisMAllocateSharedMemory(
			pAd->AdapterHandle,
			pAd->MgmtDescRing.AllocSize,
			FALSE,
			&pAd->MgmtDescRing.AllocVa,
			&pAd->MgmtDescRing.AllocPa);
#else
		RTMP_AllocateMgmtDescMemory(
			pAd,
			pAd->MgmtDescRing.AllocSize,
			FALSE,
			&pAd->MgmtDescRing.AllocVa,
			&pAd->MgmtDescRing.AllocPa);
#endif

		if (pAd->MgmtDescRing.AllocVa == NULL)
		{
			ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
			DBGPRINT_ERR(("Failed to allocate a big buffer\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		// Zero init this memory block
		NdisZeroMemory(pAd->MgmtDescRing.AllocVa, pAd->MgmtDescRing.AllocSize);

		// Save PA & VA for further operation
#ifdef WIN_NDIS
		RingBasePaHigh = NdisGetPhysicalAddressHigh(pAd->MgmtDescRing.AllocPa);
		RingBasePaLow  = NdisGetPhysicalAddressLow (pAd->MgmtDescRing.AllocPa);
#else
		RingBasePaHigh = RTMP_GetPhysicalAddressHigh(pAd->MgmtDescRing.AllocPa);
		RingBasePaLow  = RTMP_GetPhysicalAddressLow (pAd->MgmtDescRing.AllocPa);
#endif
		RingBaseVa     = pAd->MgmtDescRing.AllocVa;

		//
		// Initialize MGMT Ring and associated buffer memory
		//
		for (index = 0; index < MGMT_RING_SIZE; index++)
		{
			pAd->MgmtRing.Cell[index].pNdisPacket = NULL;
			pAd->MgmtRing.Cell[index].pNextNdisPacket = NULL;
			// Init MGMT Ring Size, Va, Pa variables
			pAd->MgmtRing.Cell[index].AllocSize = TXD_SIZE;
			pAd->MgmtRing.Cell[index].AllocVa = RingBaseVa;
#ifdef WIN_NDIS
			NdisSetPhysicalAddressHigh(pAd->MgmtRing.Cell[index].AllocPa, RingBasePaHigh);
			NdisSetPhysicalAddressLow (pAd->MgmtRing.Cell[index].AllocPa, RingBasePaLow);
#else
			RTMP_SetPhysicalAddressHigh(pAd->MgmtRing.Cell[index].AllocPa, RingBasePaHigh);
			RTMP_SetPhysicalAddressLow (pAd->MgmtRing.Cell[index].AllocPa, RingBasePaLow);
#endif

			// Offset to next ring descriptor address
			RingBasePaLow += TXD_SIZE;
			RingBaseVa = (PUCHAR) RingBaseVa + TXD_SIZE;

			// link the pre-allocated TxBuf to TXD
			pTxD = (PTXD_STRUC) pAd->MgmtRing.Cell[index].AllocVa;
			pTxD->DMADONE = 1;

#ifdef BIG_ENDIAN
			RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
#endif
			// no pre-allocated buffer required in MgmtRing for scatter-gather case
		}
		DBGPRINT(RT_DEBUG_TRACE, ("MGMT Ring: total %d entry allocated\n", index));

		//
		// Allocate RX ring descriptor's memory except Tx ring which allocated eariler
		//
		pAd->RxDescRing.AllocSize = RX_RING_SIZE * RXD_SIZE;
#ifdef WIN_NDIS
		NdisMAllocateSharedMemory(
			pAd->AdapterHandle,
			pAd->RxDescRing.AllocSize,
			FALSE,
			&pAd->RxDescRing.AllocVa,
			&pAd->RxDescRing.AllocPa);
#else
		RTMP_AllocateRxDescMemory(
			pAd,
			pAd->RxDescRing.AllocSize,
			FALSE,
			&pAd->RxDescRing.AllocVa,
			&pAd->RxDescRing.AllocPa);
#endif		

		if (pAd->RxDescRing.AllocVa == NULL)
		{
			ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
			DBGPRINT_ERR(("Failed to allocate a big buffer\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		// Zero init this memory block
		NdisZeroMemory(pAd->RxDescRing.AllocVa, pAd->RxDescRing.AllocSize);


		printk("RX DESC %p  size = %ld\n", pAd->RxDescRing.AllocVa,
					pAd->RxDescRing.AllocSize);

		// Save PA & VA for further operation
#ifdef WIN_NDIS
		RingBasePaHigh = NdisGetPhysicalAddressHigh(pAd->RxDescRing.AllocPa);
		RingBasePaLow  = NdisGetPhysicalAddressLow (pAd->RxDescRing.AllocPa);
#else
		RingBasePaHigh = RTMP_GetPhysicalAddressHigh(pAd->RxDescRing.AllocPa);
		RingBasePaLow  = RTMP_GetPhysicalAddressLow (pAd->RxDescRing.AllocPa);
#endif
		RingBaseVa     = pAd->RxDescRing.AllocVa;

		//
		// Initialize Rx Ring and associated buffer memory
		//
		for (index = 0; index < RX_RING_SIZE; index++)
		{
			// Init RX Ring Size, Va, Pa variables
			pAd->RxRing.Cell[index].AllocSize = RXD_SIZE;
			pAd->RxRing.Cell[index].AllocVa = RingBaseVa;
#ifdef WIN_NDIS
			NdisSetPhysicalAddressHigh(pAd->RxRing.Cell[index].AllocPa, RingBasePaHigh);
			NdisSetPhysicalAddressLow (pAd->RxRing.Cell[index].AllocPa, RingBasePaLow);
#else
			RTMP_SetPhysicalAddressHigh(pAd->RxRing.Cell[index].AllocPa, RingBasePaHigh);
			RTMP_SetPhysicalAddressLow (pAd->RxRing.Cell[index].AllocPa, RingBasePaLow);
#endif

			//NdisZeroMemory(RingBaseVa, RXD_SIZE);

			// Offset to next ring descriptor address
			RingBasePaLow += RXD_SIZE;
			RingBaseVa = (PUCHAR) RingBaseVa + RXD_SIZE;

			// Setup Rx associated Buffer size & allocate share memory
			pDmaBuf = &pAd->RxRing.Cell[index].DmaBuf;
			pDmaBuf->AllocSize = RX_BUFFER_AGGRESIZE;
#ifdef WIN_NDIS
			NdisMAllocateSharedMemory(
				pAd->AdapterHandle,
				pDmaBuf->AllocSize,
				FALSE,
				&pDmaBuf->AllocVa,
				&pDmaBuf->AllocPa);
#else
			pPacket = RTMP_AllocateRxPacketBuffer(
				pAd,
				pDmaBuf->AllocSize,
				FALSE,
				&pDmaBuf->AllocVa,
				&pDmaBuf->AllocPa);
			
			/* keep allocated rx packet */
			pAd->RxRing.Cell[index].pNdisPacket = pPacket;

#endif

			// Error handling
			if (pDmaBuf->AllocVa == NULL)
			{
				ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
				DBGPRINT_ERR(("Failed to allocate RxRing's 1st buffer\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			// Zero init this memory block
			NdisZeroMemory(pDmaBuf->AllocVa, pDmaBuf->AllocSize);

			// Write RxD buffer address & allocated buffer length
			pRxD = (PRXD_STRUC) pAd->RxRing.Cell[index].AllocVa;
#ifdef WIN_NDIS
			pRxD->SDP0 = NdisGetPhysicalAddressLow(pDmaBuf->AllocPa);
#else
			pRxD->SDP0 = RTMP_GetPhysicalAddressLow(pDmaBuf->AllocPa);
#endif
			pRxD->DDONE = 0;

#ifdef RX_SCATTERED
			// Setup Rx associated Buffer size & allocate share memory
			pDmaBuf = &pAd->RxRing.Cell[index].NextDmaBuf;
			pDmaBuf->AllocSize = 2048;

			pPacket = RTMP_AllocateRxPacketBuffer(
				pAd,
				pDmaBuf->AllocSize,
				FALSE,
				&pDmaBuf->AllocVa,
				&pDmaBuf->AllocPa);

			pAd->RxRing.Cell[index].pNextNdisPacket = pPacket;

			// Error handling
			if (pDmaBuf->AllocVa == NULL)
			{
				ErrorValue = ERRLOG_OUT_OF_SHARED_MEMORY;
				DBGPRINT_ERR(("Failed to allocate RxRing's 2nd buffer\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
			// Zero init this memory block
			NdisZeroMemory(pDmaBuf->AllocVa, pDmaBuf->AllocSize);

			pRxD->SDP1 = RTMP_GetPhysicalAddressLow(pDmaBuf->AllocPa);
#endif 						
#ifdef BIG_ENDIAN
			RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#endif
		}

		DBGPRINT(RT_DEBUG_TRACE, ("Rx Ring: total %d entry allocated\n", index));

	}	while (FALSE);


	NdisZeroMemory(&pAd->FragFrame, sizeof(FRAGMENT_FRAME));
	pAd->FragFrame.pFragPacket =  RTMP_AllocateFragPacketBuffer(pAd, RX_BUFFER_NORMSIZE);

	if (pAd->FragFrame.pFragPacket == NULL)
	{
		Status = NDIS_STATUS_RESOURCES;
	}

	if (Status != NDIS_STATUS_SUCCESS)
	{
		// Log error inforamtion
		NdisWriteErrorLogEntry(
			pAd->AdapterHandle,
			NDIS_ERROR_CODE_OUT_OF_RESOURCES,
			1,
			ErrorValue);
	}

	DBGPRINT_S(Status, ("<-- RTMPAllocTxRxRingMemory, Status=%x\n", Status));
	return Status;
}


/*
	========================================================================
	
	Routine Description:
		Initialize transmit data structures

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		Initialize all transmit releated private buffer, include those define
		in RTMP_ADAPTER structure and all private data structures.
		
	========================================================================
*/
VOID	NICInitTxRxRingAndBacklogQueue(
	IN	PRTMP_ADAPTER	pAd)
{
	//WPDMA_GLO_CFG_STRUC	GloCfg;
	int i;
	
	DBGPRINT(RT_DEBUG_TRACE, ("<--> NICInitTxRxRingAndBacklogQueue\n"));

	// Initialize all transmit related software queues
	InitializeQueueHeader(&pAd->TxSwQueue[QID_AC_BE]);
	InitializeQueueHeader(&pAd->TxSwQueue[QID_AC_BK]);
	InitializeQueueHeader(&pAd->TxSwQueue[QID_AC_VI]);
	InitializeQueueHeader(&pAd->TxSwQueue[QID_AC_VO]);
	InitializeQueueHeader(&pAd->TxSwQueue[QID_HCCA]);

	// Disable DMA.
/*	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
	GloCfg.word &= 0xff0;
	GloCfg.field.EnTXWriteBackDDONE =1;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);*/
	// Init RX Ring index pointer
	pAd->RxRing.RxSwReadIdx = 0;
	pAd->RxRing.RxCpuIdx = RX_RING_SIZE - 1;
	//RTMP_IO_WRITE32(pAd, RX_CRX_IDX, pAd->RxRing.RX_CRX_IDX0);
	
	// Init TX rings index pointer
		for (i=0; i<NUM_OF_TX_RING; i++)
		{
			pAd->TxRing[i].TxSwFreeIdx = 0;
			pAd->TxRing[i].TxCpuIdx = 0;
			//RTMP_IO_WRITE32(pAd, (TX_CTX_IDX0 + i * 0x10) ,  pAd->TxRing[i].TX_CTX_IDX);
		}

	// init MGMT ring index pointer
	pAd->MgmtRing.TxSwFreeIdx = 0;
	pAd->MgmtRing.TxCpuIdx = 0;
	//RTMP_IO_WRITE32(pAd, TX_MGMTCTX_IDX,  pAd->MgmtRing.TX_CTX_IDX);

	pAd->PrivateInfo.TxRingFullCnt       = 0;
}


/*
	========================================================================
	
	Routine Description:
		Reset NIC Asics. Call after rest DMA. So reset TX_CTX_IDX to zero.

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
		Reset NIC to initial state AS IS system boot up time.
		
	========================================================================
*/
VOID	RTMPRingCleanUp(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			RingType)
{
	PTXD_STRUC		pTxD;
	PRXD_STRUC		pRxD;
	PQUEUE_ENTRY	pEntry;
	PNDIS_PACKET	pPacket;
	int				i;
	PRTMP_TX_RING	pTxRing;
	unsigned long	IrqFlags;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPRingCleanUp(RingIdx=%d, Pending-NDIS=%ld)\n", RingType, pAd->RalinkCounters.PendingNdisPacketCount));
	switch (RingType)
	{
		case QID_AC_BK:
		case QID_AC_BE:
		case QID_AC_VI:
		case QID_AC_VO:
		case QID_HCCA:
#ifdef WIN_NDIS
			NdisAcquireSpinLock(&pAd->TxRingLock);
#else
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#endif
			pTxRing = &pAd->TxRing[RingType];
			
			RTMP_IO_READ32(pAd, TX_DTX_IDX0 + RingType * 0x10, &pTxRing->TxDmaIdx);
			pTxRing->TxSwFreeIdx = pTxRing->TxDmaIdx;
			pTxRing->TxCpuIdx = pTxRing->TxDmaIdx;
			RTMP_IO_WRITE32(pAd, TX_CTX_IDX0 + RingType * 0x10, pTxRing->TxCpuIdx);

			// We have to clean all descriptors in case some error happened with reset
			for (i=0; i<TX_RING_SIZE; i++) // We have to scan all TX ring
			{
				pTxD  = (PTXD_STRUC) pTxRing->Cell[i].AllocVa;

				pPacket = (PNDIS_PACKET) pTxRing->Cell[i].pNdisPacket;
				// release scatter-and-gather NDIS_PACKET
				if (pPacket)
				{
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
					pTxRing->Cell[i].pNdisPacket = NULL;
				}

				pPacket = (PNDIS_PACKET) pTxRing->Cell[i].pNextNdisPacket;
				// release scatter-and-gather NDIS_PACKET
				if (pPacket)
				{
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
					pTxRing->Cell[i].pNextNdisPacket = NULL;
				}
			}
#ifdef WIN_NDIS
			NdisReleaseSpinLock(&pAd->TxRingLock);
#else
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif

#ifdef WIN_NDIS
			NdisAcquireSpinLock(&pAd->TxSwQueueLock);
#else
			//RTMP_SEM_LOCK(&pAd->TxSwQueueLock);
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#endif			
			while (pAd->TxSwQueue[RingType].Head != NULL)
			{
				pEntry = RemoveHeadQueue(&pAd->TxSwQueue[RingType]);
#ifdef WIN_NDIS
				pPacket = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReservedEx);
#else
				pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
#endif
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				DBGPRINT(RT_DEBUG_TRACE,("Release 1 NDIS packet from s/w backlog queue\n"));
			}
#ifdef WIN_NDIS
			NdisReleaseSpinLock(&pAd->TxSwQueueLock);
#else
			//RTMP_SEM_UNLOCK(&pAd->TxSwQueueLock);
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif
			break;

		case QID_MGMT:
			// We have to clean all descriptors in case some error happened with reset
			NdisAcquireSpinLock(&pAd->MgmtRingLock);
			
			RTMP_IO_READ32(pAd, TX_MGMTDTX_IDX, &pAd->MgmtRing.TxDmaIdx);
			pAd->MgmtRing.TxSwFreeIdx = pAd->MgmtRing.TxDmaIdx;
			pAd->MgmtRing.TxCpuIdx = pAd->MgmtRing.TxDmaIdx;
			RTMP_IO_WRITE32(pAd, TX_MGMTCTX_IDX, pAd->MgmtRing.TxCpuIdx);

			for (i=0; i<MGMT_RING_SIZE; i++)
			{
				pTxD  = (PTXD_STRUC) pAd->MgmtRing.Cell[i].AllocVa;

				pPacket = (PNDIS_PACKET) pAd->MgmtRing.Cell[i].pNdisPacket;
				// rlease scatter-and-gather NDIS_PACKET
				if (pPacket)
				{
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr0, pTxD->SDLen0, PCI_DMA_TODEVICE);
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				}
				pAd->MgmtRing.Cell[i].pNdisPacket = NULL;

				pPacket = (PNDIS_PACKET) pAd->MgmtRing.Cell[i].pNextNdisPacket;
				// release scatter-and-gather NDIS_PACKET
				if (pPacket)
				{
					PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, PCI_DMA_TODEVICE);			
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
				pAd->MgmtRing.Cell[i].pNextNdisPacket = NULL;

			}
			NdisReleaseSpinLock(&pAd->MgmtRingLock);
			pAd->RalinkCounters.MgmtRingFullCount = 0;
			break;
			
		case QID_RX:
			// We have to clean all descriptors in case some error happened with reset
			NdisAcquireSpinLock(&pAd->RxRingLock);
			
			RTMP_IO_READ32(pAd, RX_DRX_IDX, &pAd->RxRing.RxDmaIdx);
			pAd->RxRing.RxSwReadIdx = pAd->RxRing.RxDmaIdx;
			pAd->RxRing.RxCpuIdx = ((pAd->RxRing.RxDmaIdx == 0) ? (RX_RING_SIZE-1) : (pAd->RxRing.RxDmaIdx-1));
			RTMP_IO_WRITE32(pAd, RX_CRX_IDX, pAd->RxRing.RxCpuIdx);

			for (i=0; i<RX_RING_SIZE; i++)
			{
				pRxD  = (PRXD_STRUC) pAd->RxRing.Cell[i].AllocVa;
                pRxD->DDONE = 0 ;
			}
			NdisReleaseSpinLock(&pAd->RxRingLock);
			break;
			
		default:
			break;
	}
}


NDIS_STATUS AdapterBlockAllocateMemory(
	IN PVOID	handle,
	OUT	PVOID	*ppAd)
{
	PPCI_DEV pci_dev;
	dma_addr_t	*phy_addr;
	POS_COOKIE pObj = (POS_COOKIE) handle;
	
	pci_dev = pObj->pci_dev;
	phy_addr = &pObj->pAd_pa;
	
	*ppAd = (PVOID)vmalloc(sizeof(RTMP_ADAPTER)); //pci_alloc_consistent(pci_dev, sizeof(RTMP_ADAPTER), phy_addr);
    
	if (*ppAd) 
	{
		NdisZeroMemory(*ppAd, sizeof(RTMP_ADAPTER));
		((PRTMP_ADAPTER)*ppAd)->OS_Cookie = handle;
		return (NDIS_STATUS_SUCCESS);
	} else {
		return (NDIS_STATUS_FAILURE);
	}
}


void RTMP_AllocateTxDescMemory(
	IN	PRTMP_ADAPTER pAd,
	IN	UINT	Index,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

	*VirtualAddress = (PVOID)pci_alloc_consistent(pObj->pci_dev,sizeof(char)*Length, PhysicalAddress);

}

void RTMP_AllocateMgmtDescMemory(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

	*VirtualAddress = (PVOID)pci_alloc_consistent(pObj->pci_dev,sizeof(char)*Length, PhysicalAddress);

}

void RTMP_AllocateRxDescMemory(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

	*VirtualAddress = (PVOID)pci_alloc_consistent(pObj->pci_dev,sizeof(char)*Length, PhysicalAddress);

}

void RTMP_FreeRxDescMemory(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	PVOID	VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	
	pci_free_consistent(pObj->pci_dev, Length, VirtualAddress, PhysicalAddress);
}


void RTMP_AllocateFirstTxBuffer(
	IN	PRTMP_ADAPTER pAd,
	IN	UINT	Index,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

	*VirtualAddress = (PVOID)pci_alloc_consistent(pObj->pci_dev,sizeof(char)*Length, PhysicalAddress);
}

/*
 * FUNCTION: Allocate a common buffer for DMA
 * ARGUMENTS:
 *     AdapterHandle:  AdapterHandle
 *     Length:  Number of bytes to allocate
 *     Cached:  Whether or not the memory can be cached
 *     VirtualAddress:  Pointer to memory is returned here
 *     PhysicalAddress:  Physical address corresponding to virtual address
 */

void RTMP_AllocateSharedMemory(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

	*VirtualAddress = (PVOID)pci_alloc_consistent(pObj->pci_dev,sizeof(char)*Length, PhysicalAddress);	
}


VOID RTMPFreeTxRxRingMemory(
    IN  PRTMP_ADAPTER   pAd)
{
	int index, num , j;
	PRTMP_TX_RING pTxRing;
	PTXD_STRUC	  pTxD;
	PNDIS_PACKET  pPacket;
	unsigned int  IrqFlags;
		
	POS_COOKIE pObj =(POS_COOKIE) pAd->OS_Cookie;
	
	DBGPRINT(RT_DEBUG_TRACE, ("--> RTMPFreeTxRxRingMemory\n"));

	// Free TxSwQueue Packet
	for (index=0; index <NUM_OF_TX_RING; index++)
	{
		PQUEUE_ENTRY pEntry;
		PNDIS_PACKET pPacket;
		PQUEUE_HEADER   pQueue;

		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
		pQueue = &pAd->TxSwQueue[index];
		while (pQueue->Head)
		{
			pEntry = RemoveHeadQueue(pQueue);
			pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		}
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	}

	// Free Tx Ring Packet
	for (index=0;index< NUM_OF_TX_RING;index++)
	{
		pTxRing = &pAd->TxRing[index];
		
		for (j=0; j< TX_RING_SIZE; j++)
		{	
			pTxD = (PTXD_STRUC) (pTxRing->Cell[j].AllocVa);
			pPacket = pTxRing->Cell[j].pNdisPacket;

			if (pPacket)
			{
            	PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr0, pTxD->SDLen0, PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}		
			//Always assign pNdisPacket as NULL after clear
			pTxRing->Cell[j].pNdisPacket = NULL;
					
			pPacket = pTxRing->Cell[j].pNextNdisPacket;
			
			if (pPacket)
			{
            	PCI_UNMAP_SINGLE(pAd, pTxD->SDPtr1, pTxD->SDLen1, PCI_DMA_TODEVICE);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
			}
			//Always assign pNextNdisPacket as NULL after clear
			pTxRing->Cell[pTxRing->TxSwFreeIdx].pNextNdisPacket = NULL;

		}
	}	
	
	for (index = RX_RING_SIZE - 1 ; index >= 0; index--)
	{
		if ((pAd->RxRing.Cell[index].DmaBuf.AllocVa) && (pAd->RxRing.Cell[index].pNdisPacket))
		{
			pci_unmap_single(pObj->pci_dev, pAd->RxRing.Cell[index].DmaBuf.AllocPa, pAd->RxRing.Cell[index].DmaBuf.AllocSize, PCI_DMA_FROMDEVICE);
			RELEASE_NDIS_PACKET(pAd, pAd->RxRing.Cell[index].pNdisPacket, NDIS_STATUS_SUCCESS);
		}
#ifdef RX_SCATTERED

		if ((pAd->RxRing.Cell[index].NextDmaBuf.AllocVa) && (pAd->RxRing.Cell[index].pNextNdisPacket))
		{
			pci_unmap_single(pObj->pci_dev, pAd->RxRing.Cell[index].NextDmaBuf.AllocPa, pAd->RxRing.Cell[index].NextDmaBuf.AllocSize, PCI_DMA_FROMDEVICE);
			RELEASE_NDIS_PACKET(pAd, pAd->RxRing.Cell[index].pNextNdisPacket, NDIS_STATUS_SUCCESS);
		}
#endif		

	}
	NdisZeroMemory(pAd->RxRing.Cell, RX_RING_SIZE * sizeof(RTMP_DMACB));
	
	if (pAd->RxDescRing.AllocVa)
    {
    	pci_free_consistent(pObj->pci_dev, pAd->RxDescRing.AllocSize, pAd->RxDescRing.AllocVa, pAd->RxDescRing.AllocPa);
    }
    NdisZeroMemory(&pAd->RxDescRing, sizeof(RTMP_DMABUF));

	if (pAd->MgmtDescRing.AllocVa)
	{
		pci_free_consistent(pObj->pci_dev, pAd->MgmtDescRing.AllocSize,	pAd->MgmtDescRing.AllocVa, pAd->MgmtDescRing.AllocPa);
	}
	NdisZeroMemory(&pAd->MgmtDescRing, sizeof(RTMP_DMABUF));

	for (num = 0; num < NUM_OF_TX_RING; num++)
	{
    	if (pAd->TxBufSpace[num].AllocVa)
	    {
	    	pci_free_consistent(pObj->pci_dev, pAd->TxBufSpace[num].AllocSize, pAd->TxBufSpace[num].AllocVa, pAd->TxBufSpace[num].AllocPa);
	    }
	    NdisZeroMemory(&pAd->TxBufSpace[num], sizeof(RTMP_DMABUF));
	    
    	if (pAd->TxDescRing[num].AllocVa)
	    {
	    	pci_free_consistent(pObj->pci_dev, pAd->TxDescRing[num].AllocSize, pAd->TxDescRing[num].AllocVa, pAd->TxDescRing[num].AllocPa);
	    }
	    NdisZeroMemory(&pAd->TxDescRing[num], sizeof(RTMP_DMABUF));
	}

	if (pAd->FragFrame.pFragPacket)
		RELEASE_NDIS_PACKET(pAd, pAd->FragFrame.pFragPacket, NDIS_STATUS_SUCCESS);

	DBGPRINT(RT_DEBUG_TRACE, ("<-- RTMPFreeTxRxRingMemory\n"));
}	


/*
 * FUNCTION: Allocate a packet buffer for DMA
 * ARGUMENTS:
 *     AdapterHandle:  AdapterHandle
 *     Length:  Number of bytes to allocate
 *     Cached:  Whether or not the memory can be cached
 *     VirtualAddress:  Pointer to memory is returned here
 *     PhysicalAddress:  Physical address corresponding to virtual address
 * Notes:
 *     Cached is ignored: always cached memory
 */
PNDIS_PACKET RTMP_AllocateRxPacketBuffer(
	IN	PRTMP_ADAPTER pAd,
	IN	ULONG	Length,
	IN	BOOLEAN	Cached,
	OUT	PVOID	*VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
	struct sk_buff *pkt;

	pkt = dev_alloc_skb(Length);

	if (pkt == NULL) {
		DBGPRINT(RT_DEBUG_ERROR, ("can't allocate rx %ld size packet\n",Length));
	}

	if (pkt) {
		RTMP_SET_PACKET_SOURCE(OSPKT_TO_RTPKT(pkt), PKTSRC_NDIS);
		*VirtualAddress = (PVOID) pkt->data;	
//#ifdef CONFIG_5VT_ENHANCE
//		*PhysicalAddress = PCI_MAP_SINGLE(pAd, *VirtualAddress, 1600, PCI_DMA_FROMDEVICE);
//#else
		*PhysicalAddress = PCI_MAP_SINGLE(pAd, *VirtualAddress, Length, PCI_DMA_FROMDEVICE);
//#endif
	} else {
		*VirtualAddress = (PVOID) NULL;
		*PhysicalAddress = (NDIS_PHYSICAL_ADDRESS) NULL;
	}	

	return (PNDIS_PACKET) pkt;
}


VOID Invalid_Remaining_Packet(
	IN	PRTMP_ADAPTER pAd,
	IN	 ULONG VirtualAddress)
{
	NDIS_PHYSICAL_ADDRESS PhysicalAddress;

	PhysicalAddress = PCI_MAP_SINGLE(pAd, (void *)(VirtualAddress+1600), RX_BUFFER_NORMSIZE-1600, PCI_DMA_FROMDEVICE);
}


/*
 * invaild or writeback cache 
 * and convert virtual address to physical address 
 */
dma_addr_t linux_pci_map_single(void *handle, void *ptr, size_t size, int direction)
{
	PRTMP_ADAPTER pAd;
	POS_COOKIE pObj;
	
	pAd = (PRTMP_ADAPTER)handle;
	pObj = (POS_COOKIE)pAd->OS_Cookie;
	
	return pci_map_single(pObj->pci_dev, ptr, size, direction);

}

void linux_pci_unmap_single(void *handle, dma_addr_t dma_addr, size_t size, int direction)
{
	PRTMP_ADAPTER pAd;
	POS_COOKIE pObj;

	pAd=(PRTMP_ADAPTER)handle;
	pObj = (POS_COOKIE)pAd->OS_Cookie;
	
	pci_unmap_single(pObj->pci_dev, dma_addr, size, direction);
	
}


PNDIS_PACKET GetPacketFromRxRing(
	IN		PRTMP_ADAPTER	pAd,
	OUT		PRT28XX_RXD_STRUC	pSaveRxD,
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

#ifdef BIG_ENDIAN
	pDestRxD = (PRXD_STRUC) pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].AllocVa;
	RxD = *pDestRxD;
	pRxD = &RxD;
	RTMPDescriptorEndianChange((PUCHAR)pRxD, TYPE_RXD);
#else
	// Point to Rx indexed rx ring descriptor
	pRxD = (PRXD_STRUC) pAd->RxRing.Cell[pAd->RxRing.RxSwReadIdx].AllocVa;
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

#if 0
/*
========================================================================
Routine Description:
    Handle periodical events.

Arguments:
	pAd					device control block

Return Value:
    None

Note:
========================================================================
*/
VOID RT28XXMlmePeriodExec(
	IN		PRTMP_ADAPTER		pAd)
{
	ULONG			TxTotalCnt;
	
		// add the most up-to-date h/w raw counters into software variable, so that
		// the dynamic tuning mechanism below are based on most up-to-date information
		NICUpdateRawCounters(pAd);																										

   		// Need statistics after read counter. So put after NICUpdateRawCounters
		ORIBATimerTimeout(pAd);


		// if MGMT RING is full more than twice within 1 second, we consider there's
		// a hardware problem stucking the TX path. In this case, try a hardware reset
		// to recover the system
	//	if (pAd->RalinkCounters.MgmtRingFullCount >= 2)
	//		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR);
	//	else
	//		pAd->RalinkCounters.MgmtRingFullCount = 0;

		// The time period for checking antenna is according to traffic
		if (pAd->Mlme.bEnableAutoAntennaCheck)
		{
			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
							 pAd->RalinkCounters.OneSecTxRetryOkCount + 
							 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt > 50)
			{
				if (pAd->Mlme.OneSecPeriodicRound % 10 == 0)
				{
					AsicEvaluateRxAnt(pAd);
				}
			}
			else
			{
				if (pAd->Mlme.OneSecPeriodicRound % 3 == 0)
				{
					AsicEvaluateRxAnt(pAd);
				}
			}
		}

#if 0
		if (pAd->CommonCfg.bRcvBSSWidthTriggerEvents)
		{
			if (RTMP_TIME_AFTER(pAd->Mlme.Now32, pAd->CommonCfg.LastRcvBSSWidthTriggerEventsTime+(30*60*OS_HZ)))
			{				
				pAd->CommonCfg.bRcvBSSWidthTriggerEvents = FALSE;
				pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;	
				pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
				//Recvery_to_Forty_Mhz(pAd);
			}
		}
#endif


#ifdef CONFIG_AP_SUPPORT
	if (pAd->OpMode == OPMODE_AP)
		APMlmePeriodicExec(pAd);
#endif // CONFIG_AP_SUPPORT //

	pAd->RalinkCounters.LastOneSecRxOkDataCnt = pAd->RalinkCounters.OneSecRxOkDataCnt;
	// clear all OneSecxxx counters.
	pAd->RalinkCounters.OneSecBeaconSentCnt = 0;
	pAd->RalinkCounters.OneSecFalseCCACnt = 0;
	pAd->RalinkCounters.OneSecRxFcsErrCnt = 0;
	pAd->RalinkCounters.OneSecRxOkCnt = 0;
	pAd->RalinkCounters.OneSecTxFailCount = 0;
	pAd->RalinkCounters.OneSecTxNoRetryOkCount = 0;
	pAd->RalinkCounters.OneSecTxRetryOkCount = 0;
	pAd->RalinkCounters.OneSecRxOkDataCnt = 0;

	// TODO: for debug only. to be removed
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BE] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BK] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VI] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VO] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BE] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BK] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VI] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VO] = 0;
	pAd->RalinkCounters.OneSecTxDoneCount = 0;
	pAd->RalinkCounters.OneSecRxCount = 0;
	pAd->RalinkCounters.OneSecTxAggregationCount = 0;
	pAd->RalinkCounters.OneSecRxAggregationCount = 0;

	RT28XX_MLME_HANDLER(pAd);
}
#endif

/* End of 2860_rtmp_init.c */
