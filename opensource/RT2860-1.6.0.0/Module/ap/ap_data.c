/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	ap_data.c

	Abstract:
	Data path subroutines

	Revision History:
	Who 		When			What
	--------	----------		----------------------------------------------
	Paul Lin	08-01-2002		created
	Paul Lin	07-01-2003		add encryption/decryption data flow
	John Chang	08-05-2003		modify 802.11 header for AP purpose
	John Chang	12-20-2004		modify for 2561/2661. merge into STA driver
	Jan Lee	1-20-2006		    modify for 2860.  
*/
#include "rt_config.h"


// IRQL = DISPATCH_LEVEL
VOID	APSendPackets(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	PPNDIS_PACKET	ppPacketArray,
	IN	UINT			NumberOfPackets)
{
	UINT			Index;
	//NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER) MiniportAdapterContext;
	PACKET_INFO 	PacketInfo;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	PMAC_TABLE_ENTRY pEntry = NULL;
	SST 			Sst;
	USHORT			Aid;
	UCHAR			PsMode, Rate;
	//UCHAR			i;

	PNDIS_PACKET	pPacket;

	DBGPRINT(RT_DEBUG_INFO, ("====> APSendPackets\n"));
	
	for (Index = 0; Index < NumberOfPackets; Index++)
	{
		pPacket = ppPacketArray[Index];

   		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
			RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS) ||
			(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)))
		{
			// Drop send request since hardware is in reset state
			//NdisMSendComplete(pAd->AdapterHandle, ppPacketArray[Index], NDIS_STATUS_FAILURE);			
			RTMP_SendComplete(pAd, pPacket, NDIS_STATUS_FAILURE);
			continue;
		}


		RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);

		/* The following code do comparison must base on the sequence: 
				MIN_NET_DEVICE_FOR_APCLI> MIN_NET_DEVICE_FOR_WDS > Normal
		*/
#ifdef APCLI_SUPPORT
		if (RTMP_GET_PACKET_NET_DEVICE(pPacket) >= MIN_NET_DEVICE_FOR_APCLI)
		{
			UCHAR apCliIdx;
			//printk("APSendPackets():Packet to ApCli interface!\n");
			apCliIdx = RTMP_GET_PACKET_NET_DEVICE(pPacket) - MIN_NET_DEVICE_FOR_APCLI;
			if (ValidApCliEntry(pAd, apCliIdx))
			{
				//printk("APSendPackets(): Set the WCID as %d!\n", pAd->ApCfg.ApCliTab[apCliIdx].MacTabWCID);
				RTMP_SET_PACKET_WCID(pPacket, pAd->ApCfg.ApCliTab[apCliIdx].MacTabWCID); // to ApClient links.
				APSendPacket(pAd, pPacket);
			}
			else
			{
				RTMP_SendComplete(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
		}
		else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
		if (RTMP_GET_PACKET_NET_DEVICE(pPacket) >= MIN_NET_DEVICE_FOR_WDS)
					{
			UCHAR wdsIndex;
			
			wdsIndex = RTMP_GET_PACKET_NET_DEVICE(pPacket) - MIN_NET_DEVICE_FOR_WDS;
			if (ValidWdsEntry(pAd, wdsIndex))
			{
				// 3. send out the packet .   b7 as WDS bit, b0-6 as WDS index when b7==1
				RTMP_SET_PACKET_WCID(pPacket, pAd->WdsTab.WdsEntry[wdsIndex].MacTabMatchWCID); // to all WDS links.
				APSendPacket(pAd, pPacket);
			}
			else
			{
				RTMP_SendComplete(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
		}
		else
#endif // WDS_SUPPORT //
		{
			pEntry = APSsPsInquiry(pAd, pSrcBufVA, &Sst, &Aid, &PsMode, &Rate);

		if ((pEntry && (Sst == SST_ASSOC)) || (*pSrcBufVA & 0x01))
		{
			// Record that orignal packet source is from NDIS layer,so that 
			// later on driver knows how to release this NDIS PACKET
			RTMP_SET_PACKET_WCID(ppPacketArray[Index], (UCHAR)Aid);
			RTMP_SET_PACKET_SOURCE(ppPacketArray[Index], PKTSRC_NDIS);
			NDIS_SET_PACKET_STATUS(ppPacketArray[Index], NDIS_STATUS_PENDING);
			pAd->RalinkCounters.PendingNdisPacketCount ++;

			APSendPacket(pAd, ppPacketArray[Index]);
		}
		else
		{
			RTMP_SendComplete(pAd, pPacket, NDIS_STATUS_FAILURE);
		}
	}
	}

	// Dequeue outgoing frames from TxSwQueue0..3 queue and process it
	RTMPDeQueuePacket(pAd, FALSE, MAX_TX_PROCESS);

}

/*
	========================================================================
	Routine Description:
		This routine is used to en-queue outgoing packets when
		there is no enough shread memory

	Arguments:
		pAd    Pointer to our adapter
		pPacket 	Pointer to send packet

	Return Value:
		None

	pre: Before calling this routine, caller should have filled the following fields

		pPacket->MiniportReserved[6] - contains packet source
		pPacket->MiniportReserved[5] - contains RA's WDS index (if RA on WDS link) or AID 
									   (if RA directly associated to this AP)
	post:This routine should decide the remaining pPacket->MiniportReserved[] fields 
		before calling APHardTransmit(), such as:

		pPacket->MiniportReserved[4] - Fragment # and User PRiority
		pPacket->MiniportReserved[7] - RTS/CTS-to-self protection method and TX rate
	========================================================================
*/
NDIS_STATUS APSendPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket)
{
	PACKET_INFO 	PacketInfo;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	UINT			AllowFragSize;
	UCHAR			NumberOfFrag;
	UCHAR			RTSRequired;
	UCHAR			QueIdx, UserPriority, apidx = MAIN_MBSSID;
	SST 			Sst = SST_ASSOC;
	UCHAR			PsMode = PWR_ACTIVE, Rate;
	USHORT			Wcid;
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	unsigned long	IrqFlags;
#ifdef IGMP_SNOOP_SUPPORT
	BOOLEAN			InIgmpGroup = FALSE;
	PMULTICAST_FILTER_TABLE_ENTRY pGroupEntry = NULL;
#endif // IGMP_SNOOP_SUPPORT //

	DBGPRINT(RT_DEBUG_INFO, ("====> APSendPacket\n"));

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);

	if (pSrcBufVA == NULL)
	{
		// Resourece is low, system did not allocate virtual address
		// return NDIS_STATUS_FAILURE directly to upper layer
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return NDIS_STATUS_FAILURE;
	}

	Wcid = RTMP_GET_PACKET_WCID(pPacket);
#ifdef APCLI_SUPPORT
	if(pAd->MacTab.Content[Wcid].ValidAsApCli == TRUE)
	{
		pMacEntry = &pAd->MacTab.Content[Wcid];
		Rate = pAd->MacTab.Content[Wcid].CurrTxRate;
	}
	else
#endif // APCLI_SUPPORT //
	if (pAd->MacTab.Content[Wcid].ValidAsWDS == TRUE)
	{
		//b7 as WDS bit, b0-6 as WDS index when b7==1
		pMacEntry = &pAd->MacTab.Content[Wcid];
		Rate = pAd->MacTab.Content[Wcid].CurrTxRate;
	}
	else
	{
		//USHORT Aid;
		pMacEntry = &pAd->MacTab.Content[Wcid];
		PsMode = pMacEntry->PsMode;
		Rate = pMacEntry->CurrTxRate;
		Sst = pMacEntry->Sst;
        apidx = RTMP_GET_PACKET_NET_DEVICE_MBSSID(pPacket);
        MBSS_MR_APIDX_SANITY_CHECK(apidx);

#ifdef IGMP_SNOOP_SUPPORT
		if (pAd->ApCfg.MBSSID[apidx].IgmpSnoopEnable)
		{
			if (IgmpPktInfoQuery(pAd, pSrcBufVA, pPacket, apidx, &InIgmpGroup, &pGroupEntry) != NDIS_STATUS_SUCCESS)
				return NDIS_STATUS_FAILURE;
		}
#endif // IGMP_SNOOP_SUPPORT //
	}

	// STEP 1. Decide number of fragments required to deliver this MSDU. 
	//	   The estimation here is not very accurate because difficult to 
	//	   take encryption overhead into consideration here. The result 
	//	   "NumberOfFrag" is then just used to pre-check if enough free 
	//	   TXD are available to hold this MSDU.

	if ((*pSrcBufVA & 0x01)	// fragmentation not allowed on multicast & broadcast
#ifdef IGMP_SNOOP_SUPPORT
		// multicast packets in IgmpSn table should never send to Power-Saving queue.
		&& (!InIgmpGroup)
#endif // IGMP_SNOOP_SUPPORT //
		)
		NumberOfFrag = 1;	 
	else if (pMacEntry && pMacEntry->ValidAsCLI && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE))
	{
		NumberOfFrag = 1;	// Aggregation overwhelms fragmentation
	}
	else
	{
		// The calculated "NumberOfFrag" is a rough estimation because of various 
		// encryption/encapsulation overhead not taken into consideration. This number is just
		// used to make sure enough free TXD are available before fragmentation takes place.
		// In case the actual required number of fragments of an NDIS packet 
		// excceeds "NumberOfFrag"caculated here and not enough free TXD available, the
		// last fragment (i.e. last MPDU) will be dropped in RTMPHardTransmit() due to out of 
		// resource, and the NDIS packet will be indicated NDIS_STATUS_FAILURE. This should 
		// rarely happen and the penalty is just like a TX RETRY fail. Affordable.

		AllowFragSize = (pAd->CommonCfg.FragmentThreshold) - LENGTH_802_11 - LENGTH_CRC;
		NumberOfFrag = ((PacketInfo.TotalPacketLength - LENGTH_802_3 + LENGTH_802_1_H) / AllowFragSize) + 1;
	}
	// Save fragment number to Ndis packet reserved field
	RTMP_SET_PACKET_FRAGMENTS(pPacket, NumberOfFrag);  

	// STEP 2. Check the requirement of RTS; decide packet TX rate
	//	   If multiple fragment required, RTS is required only for the first fragment
	//	   if the fragment size large than RTS threshold

	if (NumberOfFrag > 1)
		RTSRequired = (pAd->CommonCfg.FragmentThreshold > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	else
		RTSRequired = (PacketInfo.TotalPacketLength > pAd->CommonCfg.RtsThreshold) ? 1 : 0;

	// RTS/CTS may also be required in order to protect OFDM frame
	if ((Rate >= RATE_FIRST_OFDM_RATE) && 
		(Rate <= RATE_LAST_OFDM_RATE) && 
		OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
		RTSRequired = 1;

	// Save RTS requirement to Ndis packet reserved field
	RTMP_SET_PACKET_RTS(pPacket, RTSRequired);
	RTMP_SET_PACKET_TXRATE(pPacket, Rate);

	//
	// STEP 3. Traffic classification. outcome = <UserPriority, QueIdx>
	//
	UserPriority = 0;
	QueIdx		 = QID_AC_BE;
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ||
        (pAd->ApCfg.MBSSID[apidx].bWmmCapable))
	{
		USHORT Protocol;
		UCHAR  LlcSnapLen = 0, Byte0, Byte1;
		do
		{
			// get Ethernet protocol field
			Protocol = (USHORT)((pSrcBufVA[12] << 8) + pSrcBufVA[13]);
			if (Protocol <= 1500)
			{
				// get Ethernet protocol field from LLC/SNAP
#ifdef WIN_NDIS
				if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer, LENGTH_802_3 + 6, &Byte0, &Byte1) != NDIS_STATUS_SUCCESS)
					break;
#else
				if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer, LENGTH_802_3 + 6, &Byte0, &Byte1) != NDIS_STATUS_SUCCESS)
					break;
#endif

				Protocol = (USHORT)((Byte0 << 8) + Byte1);
				LlcSnapLen = 8;
			}

			// always AC_BE for non-IP packet
			if (Protocol == 0x8100)
			{
				/* 0x8100 means VLAN packets */

				/* Dest. MAC Address (6-bytes) +
				   Source MAC Address (6-bytes) +
				   Length/Type = 802.1Q Tag Type (2-byte) +
				   Tag Control Information (2-bytes) +
				   Length / Type (2-bytes) +
				   MAC Client Data (0-n bytes) +
				   Pad (0-p bytes) +
				   Frame Check Sequence (4-bytes) */

#ifdef RTL865X_SOC /* 2006/01/20 From Rory to modify WMM reference */
				if (pAd->CommonCfg.bEthWithVLANTag == TRUE)
				{
					/* the port allows to receive VLAN packets */
					UserPriority = (pSrcBufVA[14] & 0xe0) >> 5;

					if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer,
												LENGTH_802_3 + 4,
												&Byte0,
										  		&Byte1) != NDIS_STATUS_SUCCESS)
					{
						break;
					} /* End of if */
				}
				else
				{
					/* get length/type field behind VLAN tag field */
					if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer,
												LENGTH_802_3 + 4,
												&Byte0,
												&Byte1) == NDIS_STATUS_SUCCESS)
					{
						UserPriority = (Byte1 & 0xe0) >> 5;
					} /* End of if */
				} /* End of if */
#else
				/* only use VLAN tag */
				UserPriority = (pSrcBufVA[14] & 0xe0) >> 5;

				if (pAd->MacTab.Content[Wcid].ValidAsCLI)
				{
					if ((apidx < pAd->ApCfg.BssidNum) &&
						(pAd->ApCfg.MBSSID[apidx].VLAN_VID != 0))
					{
						/* check if the packet is my VLAN */
						/* VLAN tag: 3-bit UP + 1-bit CFI + 12-bit VLAN ID */
						USHORT vlan_id = *(USHORT *)&pSrcBufVA[14];

#ifndef BIG_ENDIAN
						vlan_id = SWAP16(vlan_id);
#endif // BIG_ENDIAN //

						vlan_id = vlan_id & 0x0FFF; /* 12 bit */

						if (vlan_id != pAd->ApCfg.MBSSID[apidx].VLAN_VID)
						{
							/* not my VLAN packet, discard it */
							RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
							return NDIS_STATUS_FAILURE;
						} /* End of if */
					} /* End of if */
				} /* End of if */

				if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer,
											LENGTH_802_3 + 4, /* 4: VLAN tag */
											&Byte0,
											&Byte1) != NDIS_STATUS_SUCCESS)
				{
					break;
				} /* End of if */
#endif // RTL865X_SOC //
			}
			else if (Protocol != 0x0800)
				break;
			else
			// get IP header
			if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer, LENGTH_802_3 + LlcSnapLen, &Byte0, &Byte1) != NDIS_STATUS_SUCCESS)
				break;

			// return AC_BE if packet is not IPv4
			if ((Byte0 & 0xf0) != 0x40)
				break;

			if (Protocol != 0x8100)
			    UserPriority = (Byte1 & 0xe0) >> 5;

			QueIdx = MapUserPriorityToAccessCategory[UserPriority];

			// have to check ACM bit. downgrade UP & QueIdx before passing ACM
			// NOTE: AP doesn't have to negotiate TSPEC. ACM is controlled purely via user setup, not protocol handshaking
			if (pAd->CommonCfg.APEdcaParm.bACM[QueIdx])
			{
				UserPriority = 0;
				QueIdx		 = QID_AC_BE;
			}
		} while (FALSE);
	}

	//
	// detect AC Category of trasmitting packets
	// to tune AC0(BE) TX_OP (MAC reg 0x1300)
	// 
	detect_wmm_traffic(pAd, UserPriority);

	RTMP_SET_PACKET_UP(pPacket, UserPriority);


	//
	// If DHCP datagram or ARP datagram , we need to send it as Low rates.
	//
	if (RTMPCheckDHCPFrame(pAd, pPacket))
	{
		RTMP_SET_PACKET_DHCP(pPacket, 1);
	}
	else
	{
		RTMP_SET_PACKET_DHCP(pPacket, 0);
	}


	//
	// 4. put to corrsponding TxSwQueue or Power-saving queue
	//

	// WDS and ApClient link should never go into power-save mode; just send out the frame
	if (pMacEntry && ((pMacEntry->ValidAsWDS == TRUE) || (pMacEntry->ValidAsApCli == TRUE)))
	{
		if (pAd->TxSwQueue[QueIdx].Number >= MAX_PACKETS_IN_QUEUE)
        {
#ifdef BLOCK_NET_IF
			StopNetIfQueue(pAd, QueIdx, pPacket);
#endif // BLOCK_NET_IF //
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			return NDIS_STATUS_FAILURE;			
		}
		else
		{
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
			InsertTailQueue(&pAd->TxSwQueue[QueIdx], PACKET_TO_QUEUE_ENTRY(pPacket));
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
		}
	}
	// M/BCAST frames are put to PSQ as long as there's any associated STA in power-save mode
	else if ((*pSrcBufVA & 0x01) && pAd->MacTab.fAnyStationInPsm
#ifdef IGMP_SNOOP_SUPPORT
		// multicast packets in IgmpSn table should never send to Power-Saving queue.
		&& (!InIgmpGroup)
#endif // IGMP_SNOOP_SUPPORT //
		)
	{
		// we don't want too many MCAST/BCAST backlog frames to eat up all buffers. So in case number of backlog 
		// MCAST/BCAST frames exceeds a pre-defined watermark within a DTIM period, simply drop coming new 
		// MCAST/BCAST frames. This design is similiar to "BROADCAST throttling in most manageable Ethernet Switch chip.
		if (pAd->MacTab.McastPsQueue.Number >= MAX_PACKETS_IN_MCAST_PS_QUEUE)
		{
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			DBGPRINT(RT_DEBUG_TRACE, ("too many frames (=%ld) in M/BCAST PSQ, drop this one\n", pAd->MacTab.McastPsQueue.Number));
			return NDIS_STATUS_FAILURE;
		}
		else
		{
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
			InsertTailQueue(&pAd->MacTab.McastPsQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

			WLAN_MR_TIM_BCMC_SET(apidx); // mark MCAST/BCAST TIM bit
			DBGPRINT(RT_DEBUG_INFO, ("at least 1 STA in psm, move M/BCAST to PSQ, TIM bitmap=%08x\n", pAd->ApCfg.MBSSID[apidx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET]));
		}
	}
	// else if the associted STA in power-save mode, frame also goes to PSQ
	else if (pMacEntry && pMacEntry->ValidAsCLI && (Sst == SST_ASSOC) && (PsMode == PWR_SAVE))
	{
#ifdef UAPSD_AP_SUPPORT
        /* put the U-APSD packet to its U-APSD queue by AC ID */
        UINT32 ac_id = QueIdx - QID_AC_BE; /* should be >= 0 */


        if (UAPSD_MR_IS_UAPSD_AC(pMacEntry, ac_id))
            UAPSD_PacketEnqueue(pAd, pMacEntry, pPacket, ac_id);
        else
#endif // UAPSD_AP_SUPPORT //
        {
			if (pMacEntry->PsQueue.Number >= MAX_PACKETS_IN_PS_QUEUE)
			{
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);			
				return NDIS_STATUS_FAILURE;			
			}
            else
            {
			    RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);			
			    InsertTailQueue(&pMacEntry->PsQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
			    RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
            }
        }

		// mark corresponding TIM bit in outgoing BEACON frame
#ifdef UAPSD_AP_SUPPORT
        if (UAPSD_MR_IS_NOT_TIM_BIT_NEEDED_HANDLED(pMacEntry, QueIdx))
        {
            /* 1. the station is UAPSD station;
               2. one of AC is non-UAPSD (legacy) AC;
               3. the destinated AC of the packet is UAPSD AC. */
            /* So we can not set TIM bit due to one of AC is legacy AC */
        }
        else
        {
#endif // UAPSD_AP_SUPPORT //

		WLAN_MR_TIM_BIT_SET(pAd, apidx, Wcid);
		DBGPRINT(RT_DEBUG_INFO, ("STA (AID=%d) in PSM, move to PSQ, Psqueue #=%ld\n", Wcid, pMacEntry->PsQueue.Number));
#ifdef UAPSD_AP_SUPPORT
        }
#endif // UAPSD_AP_SUPPORT //
	}
	// 3. otherwise, transmit the frame
	else // (PsMode == PWR_ACTIVE) || (PsMode == PWR_UNKNOWN)
	{

#ifdef IGMP_SNOOP_SUPPORT
		// if it's a mcast packet in igmp gourp.
		// ucast clone it for all members in the gourp.
		if(InIgmpGroup && pGroupEntry && (IgmpMemberCnt(&pGroupEntry->MemberList) > 0))
		{
			NDIS_STATUS PktCloneResult = IgmpPktClone(pAd, pPacket, QueIdx, pGroupEntry);
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			if (PktCloneResult != NDIS_STATUS_SUCCESS)
				return NDIS_STATUS_FAILURE;
		}
		else
#endif // IGMP_SNOOP_SUPPORT //
		{
			if (pAd->TxSwQueue[QueIdx].Number >= MAX_PACKETS_IN_QUEUE)
			{
#ifdef BLOCK_NET_IF
				StopNetIfQueue(pAd, QueIdx, pPacket);
#endif // BLOCK_NET_IF //
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				return NDIS_STATUS_FAILURE;			
			}
			else
			{			
				RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
				InsertTailQueue(&pAd->TxSwQueue[QueIdx], PACKET_TO_QUEUE_ENTRY(pPacket));
				RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
			}
		}
	}

	if (pMacEntry && (pMacEntry->NoBADataCountDown == 0) && IS_HT_STA(pMacEntry))
	{
		if (((pMacEntry->TXBAbitmap & (1<<UserPriority)) == 0) && (pMacEntry->PortSecured == WPA_802_1X_PORT_SECURED)
			 // For IOT compatibility, if  
			 // 1. It is Ralink chip or 			  
			 // 2. It is OPEN or AES mode, 
			 // then BA session can be bulit.			 
			 && ((((pMacEntry->ValidAsCLI) || (pMacEntry->ValidAsWDS)) && (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RALINK_CHIPSET))) || 			 	  			 	  		 	 
			 	 ((pMacEntry->ValidAsApCli) && (pAd->MlmeAux.APRalinkIe != 0x0)) || 
			 	 (pMacEntry->WepStatus == Ndis802_11WEPDisabled || pMacEntry->WepStatus == Ndis802_11Encryption3Enabled))
			)
		{
			BAOriSessionSetUp(pAd, pMacEntry, UserPriority, 0, 10, FALSE);
		}
	}

	pAd->RalinkCounters.OneSecOsTxCount[QueIdx]++; // TODO: for debug only. to be removed
	return NDIS_STATUS_SUCCESS;
}

/*
	========================================================================
	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware encryption before really
	sent out to air.

	Arguments:
		pAd 	   Pointer to our adapter
		PNDIS_PACKET	Pointer to outgoing Ndis frame
		NumberOfFrag	Number of fragment required

	Return Value:
		None
	========================================================================
*/

#define RT_PACKET_F_EAPOL		0x00000001
#define RT_PACKET_F_ACKREQ		0x00000002
#define RT_PACKET_F_CLONE		0x00000004
#define RT_PACKET_F_RTS			0x00000008
#define RT_PACKET_F_PIGGYBACK	0x00000010
#define RT_PACKET_F_AMSDU		0x00000020
#define RT_PACKET_F_WDS			0x00000040
#define RT_PACKET_F_TXBA		0x00000080
#define RT_PACKET_F_HTC			0x00000100
#define RT_PACKET_F_WMM			0x00000200
#define RT_PACKET_F_ALLOWFRAG	0x00000400

typedef struct __RT_PACKET
{
	UCHAR							MpduReqCount;
	UCHAR							UserPriority;
	UCHAR							TxRate;
	UCHAR							RAWcid;
	UCHAR							apidx;
	ULONG							Flags;
	MAC_TABLE_ENTRY					*pMacEntry;
	PRTMP_SCATTER_GATHER_LIST		pScatterGatherList;	

} RT_PACKET;


NDIS_STATUS APHardTransmit(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			QueIdx,
	OUT PULONG		pFreeTXDLeft)
{
	PACKET_INFO 	PacketInfo, NextPacketInfo;
	PUCHAR 			pDMAHeaderBufVA, pDMAHeaderBufPtr;
	UINT   			SrcBytesCopied;
	UINT   			FreeMpduSize, MpduSize = 0;
	PUCHAR			pSrcBufVA, pNextPacketBufVA;
	UINT			SrcBufLen, NextPacketBufLen;
	UCHAR			SrcBufIdx;
	ULONG			SrcBufPA;
	INT 				SrcRemainingBytes;
	UCHAR			FrameGap;
	HEADER_802_11	Header_802_11;
	PHEADER_802_11	pHeader80211;
	PUCHAR			pExtraLlcSnapEncap; // NULL: no extra LLC/SNAP is required
	UCHAR			CipherAlg = CIPHER_NONE;
	PTXD_STRUC		pTxD;
#ifdef BIG_ENDIAN
	TXD_STRUC		TxD;		/* Modified by Wu Xi-Kun 6/7/2006 */
    PTXD_STRUC      pDestTxD;
#endif
	PTXWI_STRUC 	pFirstTxWI;
	BOOLEAN 		bEAPOLFrame;
	BOOLEAN 		bAckRequired;
	//ULONG			Iv16, Iv32;
	//UCHAR			RetryMode = SHORT_RETRY;
	USHORT			AckDuration = 0;
	USHORT			EncryptionOverhead = 0;	
	//UCHAR			KeyIdx, KeyTable;
	PCIPHER_KEY 	pKey = NULL;
	UCHAR			PID = 0;
	PRTMP_TX_RING	pTxRing = &pAd->TxRing[QueIdx];
	PRTMP_SCATTER_GATHER_LIST	pScatterGatherList;
	RTMP_SCATTER_GATHER_LIST	LocalScatterGatherList;
	PNDIS_PACKET	pNextPacket;
	BOOLEAN 		bClonePacket;
	UCHAR			MpduRequired, RtsRequired, UserPriority;
	UCHAR			TxRate;
	UCHAR			RAWcid;
	PMAC_TABLE_ENTRY pMacEntry;
	//BOOLEAN 		bRTS_CTSFrame = FALSE;
	BOOLEAN			bPiggyBack = FALSE;
	BOOLEAN 		bAMSDU = FALSE; // TRUE if HT AMSDU used for this MPDU.
	BOOLEAN			bARALINK = FALSE; // TRUE if Ralink Aggregation used for this MPDU.
	BOOLEAN 		bTXBA = FALSE; // TRUE if use block ack.
	ULONG			FreeTXDLeft = *pFreeTXDLeft;
	BOOLEAN 	TXWIAMPDU = FALSE;	   //AMPDU bit in TXWI
	CHAR		TXWIBAWinSize  = 0; //BAWinSize field in TXWI
	ULONG 		TxIdx =pAd->TxRing[QueIdx].TxCpuIdx;
	UCHAR		SDPNowUsed = 1;
	BOOLEAN 	IsSD0 = TRUE;
#ifdef WDS_SUPPORT
	BOOLEAN		bWDSEntry = FALSE;
#endif // WDS_SUPPORT //
	//BOOLEAN 	bHTC = FALSE; // TRUE if HT AMSDU used for this MPDU.
	//PULONG		ptemp;
	//UCHAR		i;
	BOOLEAN		letusprint = FALSE;
	UCHAR		RABAOriIdx = 0;	// The  BA Originator table index

	BOOLEAN		bHTRate = FALSE;	// use HT AMSDU

	BOOLEAN		bAllowFrag = TRUE; // A-MPDU, A-MSDU, A-Ralink is not allowed to fragment
	BOOLEAN		bWMM = FALSE;
	BOOLEAN		bForceNonQoS = FALSE;
	BOOLEAN		bMoreData;
	UCHAR		AMSDUsubheader[14];
	UCHAR		padding = 0;
	UCHAR       apidx = BSS0;
	BOOLEAN		bForceLowRate = FALSE;
	BOOLEAN		bVLANPkt = FALSE, bVLANPktNext = FALSE;
	BOOLEAN		bFrag = FALSE;		// Indication of fragment packet
	HTTRANSMIT_SETTING	TxHTPhyMode;
	HTTRANSMIT_SETTING	*pTxHTPhyMode;
	BOOLEAN		bClearEAPFrame = FALSE;

#ifdef APCLI_SUPPORT	
	UINT			ApCliIfIdx	= 0;	
	BOOLEAN 		bApCliPacket = FALSE;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	PNDIS_PACKET 	apCliPkt = NULL;
#endif // APCLI_SUPPORT //
	
#ifdef UAPSD_AP_SUPPORT
    BOOLEAN     bWMM_UAPSD_EOSP; /* EOSP bit value */
#endif // UAPSD_AP_SUPPORT //
	
	if ((pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
#ifdef CARRIER_DETECTION_SUPPORT
		||(isCarrierDetectExist(pAd) == TRUE)
#endif // CARRIER_DETECTION_SUPPORT //
		)
	{
		DBGPRINT(RT_DEBUG_INFO,("RTMPHardTransmit --> radar detect not in normal mode !!!\n"));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return (NDIS_STATUS_FAILURE);
	}

	pMacEntry = NULL;

	MpduRequired = RTMP_GET_PACKET_FRAGMENTS(pPacket);
	RtsRequired  = RTMP_GET_PACKET_RTS(pPacket);
	UserPriority = RTMP_GET_PACKET_UP(pPacket);
	TxRate		 = RTMP_GET_PACKET_TXRATE(pPacket);
	bMoreData	 = RTMP_GET_PACKET_MOREDATA(pPacket);
	RAWcid	 	 = RTMP_GET_PACKET_WCID(pPacket);
	apidx		 = RTMP_GET_PACKET_IF(pPacket);
	bClearEAPFrame = RTMP_GET_PACKET_CLEAR_EAP_FRAME(pPacket);	

#ifdef UAPSD_AP_SUPPORT
    bWMM_UAPSD_EOSP = RTMP_GET_PACKET_EOSP(pPacket);
#endif // UAPSD_AP_SUPPORT //

	ASSERT(RAWcid< MAX_LEN_OF_MAC_TABLE);
	
	if (pAd->bForcePrintTX == TRUE)
		letusprint = TRUE;

	DBGPRINT(RT_DEBUG_INFO,("APHardTransmit(RTS=%d, Frag=%d)\n",RtsRequired,MpduRequired));

#ifdef WDS_SUPPORT
	// OneWDS  occupy both one WDS Table entry and one MAC Table entry.
	if (pAd->MacTab.Content[RAWcid].ValidAsWDS == TRUE)
	{
		pMacEntry = &pAd->MacTab.Content[RAWcid];
		bWDSEntry = TRUE;
		apidx = BSS0;
		DBGPRINT(RT_DEBUG_INFO,("APHardTransmit([%d].bWDSEntry = TRUE)\n",RAWcid));
	}
	else
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
	if (pAd->MacTab.Content[RAWcid].ValidAsApCli == TRUE)
	{
		pMacEntry = &pAd->MacTab.Content[RAWcid];
		ApCliIfIdx = pMacEntry->MatchAPCLITabIdx;
		pApCliEntry	= &pAd->ApCfg.ApCliTab[ApCliIfIdx];
		bApCliPacket = TRUE;
		// For each tx packet, update our MAT convert engine databases.
		apCliPkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pPacket, ApCliIfIdx);
		if(apCliPkt)
		{
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			pPacket = apCliPkt;
		}
		DBGPRINT(RT_DEBUG_INFO, ("APHardTransmit: ApClient - Rxwcid(%d), apidx(%d))\n",RAWcid, apidx));
	}
	else 
#endif // APCLI_SUPPORT //
	if (pAd->MacTab.Content[RAWcid].ValidAsCLI == TRUE)
	{
		pMacEntry = &pAd->MacTab.Content[RAWcid];
	}	
	else if (RAWcid == MCAST_WCID)
	{
		// This is multicast wireless-bridge packets. 
		pMacEntry = NULL;
	}
	else
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return (NDIS_STATUS_FAILURE);
	}

	// ---------------------------------------------
	// STEP 0. PARSING THE NDIS PACKET
	// ---------------------------------------------
	//
	// Prepare packet information structure which will be query for buffer descriptor
	//

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  SrcBufLen;		

	if (SrcBufLen < 14)
	{
		DBGPRINT(RT_DEBUG_ERROR,("RTMPHardTransmit --> Ndis Packet buffer error !!!\n"));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return (NDIS_STATUS_FAILURE);
	}
	
    if (((pSrcBufVA[12] << 8) + pSrcBufVA[13]) == 0x8100)
		bVLANPkt = TRUE;
    /* End of if */

	if (pMacEntry)
	{
		pMacEntry->DebugTxCount++;
	}

	/* clone is not necessary in Linux or uCOS-II */
	bClonePacket = OS_Need_Clone_Packet();

	pNextPacket  = NULL;   // no aggregation is required

	// check if aggregation applicable on this MSDU and the next one. If applicable
	// make sure both MSDUs use internally created NDIS PACKET structure so that both has
	// only one scatter-gather buffer
	// NOTE: aggregation not applicable when CKIP inused. because it's difficult for driver
	//		 to calculate CMIC on the aggregated MSDU
	if ((FreeTXDLeft > 1)								&& 
		(pAd->TxSwQueue[QueIdx].Head != NULL)			&&		
		PeerIsAggreOn(pAd, TxRate, pMacEntry)			&&
		TxFrameIsAggregatible(pAd, NULL, pSrcBufVA)		/*&&
		(pAd->ApCfg.bCkipOn == FALSE)*/)
	{
		PQUEUE_ENTRY pNextEntry;

		pNextEntry = RemoveHeadQueue(&pAd->TxSwQueue[QueIdx]);

		pNextPacket = QUEUE_ENTRY_TO_PACKET(pNextEntry); 

		RTMP_QueryPacketInfo(pNextPacket, &NextPacketInfo, &pNextPacketBufVA, &NextPacketBufLen);

		// Increase Total transmit byte counter
		pAd->RalinkCounters.TransmittedByteCount +=  NextPacketBufLen;				

	    if (NextPacketBufLen < 14)
	    {
			DBGPRINT(RT_DEBUG_ERROR,("APHardTransmit -->  Ndis NextPacket buffer error !!!\n"));
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			RELEASE_NDIS_PACKET(pAd, pNextPacket, NDIS_STATUS_FAILURE);
			return (NDIS_STATUS_FAILURE);
		}

		// 
		// Assumed the packet is linear from protocol stack 
		// 
		if (TxFrameIsAggregatible(pAd, pSrcBufVA, pNextPacketBufVA))
		{
#if 0
		    if (((pNextPacketBufVA[12] << 8) + pNextPacketBufVA[13]) == 0x8100)
			{
				/* VLAN packet, take off VLAN tag */
				NextPacketBufLen -= 4;
				bVLANPktNext = TRUE;
		    } /* End of if */
#endif
#ifdef APCLI_SUPPORT
			// For each tx packet, update our MAT convert engine databases.
			if (bApCliPacket)
			{				
				apCliPkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pNextPacket, ApCliIfIdx);
				if(apCliPkt)
				{
					RELEASE_NDIS_PACKET(pAd, pNextPacket, NDIS_STATUS_FAILURE);
					pNextPacket = apCliPkt;
					//Get the packet info again.
					RTMP_QueryPacketInfo(pNextPacket, &NextPacketInfo, &pNextPacketBufVA, &NextPacketBufLen);
				}
			}
#endif // APCLI_SUPPORT //
		}
		else
		{
			// can't aggregate. put next packe back to TxSwQueue
			InsertHeadQueue(&pAd->TxSwQueue[QueIdx], pNextEntry);
			pNextPacket = NULL;  
		}
	}

	// Decide if Packet Cloning is required -
	// 1. when fragmentation && TKIP is inused, we have to clone it into a single buffer
	//	  NDIS_PACKET so that driver can pre-cluculate and append TKIP MIC at tail of the
	//	  source packet before hardware fragmentation can be performed
	// 2. when CKIP MIC format is inused, we have to walk through the source NDIS packet to
	//	  pre-calculate CMIC and insert at front of 1st output MPDU
	// 3. if too many physical buffers in scatter-gather list (>4), we have to clone it into
	//	  a single buffer NDIS_PACKET before furthur processing
	if ((RTMP_GET_PACKET_SOURCE(pPacket) == PKTSRC_NDIS) && (bClonePacket == FALSE))
	{
#if 0	
		if ((MpduRequired > 1) &&
			(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption2Enabled))
			bClonePacket = TRUE;
		else
#endif			
		{
			pScatterGatherList = (PRTMP_SCATTER_GATHER_LIST) 
				GET_SG_LIST_FROM_PACKET(pPacket, &LocalScatterGatherList);
			// NDIS_PER_PACKET_INFO_FROM_PACKET(pPacket, ScatterGatherListPacketInfo);
			ASSERT(pScatterGatherList);

			if ((pScatterGatherList->NumberOfElements > 5) ||
				((pScatterGatherList->NumberOfElements == 5) && (SrcBufLen > LENGTH_802_3)))
				bClonePacket = TRUE;
		}
	}

#if 0
	// Clone then release the original NDIS packet
	if (bClonePacket)
	{
		PNDIS_PACKET pOutPacket;
		NDIS_STATUS  Status;

		printk("#--3\n");
		if (TxRate >= RATE_FIRST_HT_RATE )
		{
			Status = RTMPCloneNdisPacket(pAd, TRUE, pPacket, &pOutPacket);
		}
		else
			Status = RTMPCloneNdisPacket(pAd, FALSE, pPacket, &pOutPacket);
		
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			if (pNextPacket)
				RELEASE_NDIS_PACKET(pAd, pNextPacket, NDIS_STATUS_FAILURE);
			return Status;
		}

		// Use the new cloned packet from now on
		pPacket = pOutPacket;
		RTMP_SET_PACKET_UP(pPacket, UserPriority);
		RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
	}

	// use local scatter-gather structure for internally created NDIS PACKET
	// Can't use NDIS_PER_PACKET_INFO() to get scatter gather list
	if (RTMP_GET_PACKET_SOURCE(pPacket) != PKTSRC_NDIS)
	{
		/* 
		 * It is unnecessary for driver to use LocalTxBuf 
		 * in Linux or uCOS-II.
		 * We can clone a packet to use os packet buffer
		 */
		printk("RTMP_GET_PACKET_SOURCE(pPacket) = %d\n", RTMP_GET_PACKET_SOURCE(pPacket));
//		ASSERT(0);
		}
#endif 


	// if original Ethernet frame contains no LLC/SNAP, then an extra LLC/SNAP encap is required 
	EXTRA_LLCSNAP_ENCAP(pSrcBufVA, pExtraLlcSnapEncap);

	// link next MSDU into scatter list, if any
	if (pNextPacket)
	{
		// A-MSDU or A-Ralink
		if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AMSDU_INUSED))
		{
			USHORT subMSDULen;

			if (pExtraLlcSnapEncap)
				subMSDULen = SrcBufLen - LENGTH_802_3 + LENGTH_802_1_H;
			else
				subMSDULen = SrcBufLen - LENGTH_802_3;

			if (bVLANPkt == TRUE)
				subMSDULen -= 4; /* take off VLAN tag */

			// 2.1. Make sub-AMSDU header of first MSDU,			
			RTMPMoveMemory(&AMSDUsubheader[0], pSrcBufVA, 12);
#ifdef APCLI_SUPPORT
			if(bApCliPacket && pApCliEntry->Valid)
				NdisMoveMemory(&AMSDUsubheader[6] , pApCliEntry->CurrentAddress, 6);
#endif // APCLI_SUPPORT //
			AMSDUsubheader[12] = (subMSDULen & 0xFF00) >> 8;
			AMSDUsubheader[13]= subMSDULen & 0xFF;
			// This is a A-MSDU frame
			bAMSDU = TRUE;
		}
#ifdef AGGREGATION_SUPPORT
		else if(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE))
		{
#if 0	//We handle about the VLAN tag later. now just set the bARALINK flag as TRUE.
			//printk("AGGRE: 2nd MSDU aggregatible(len=%d)\n", NextPacketBufLen);
			// If having VLAN tag, skip it when transmit
			if (bVLANPktNext == TRUE)
			{
				*((u16 *)&RTPKT_TO_OSPKT(pNextPacket)->data[14]) = *((u16 *)&RTPKT_TO_OSPKT(pNextPacket)->data[10]);
				*((u32 *)&RTPKT_TO_OSPKT(pNextPacket)->data[10]) = *((u32 *)&RTPKT_TO_OSPKT(pNextPacket)->data[6]);
				*((u32 *)&RTPKT_TO_OSPKT(pNextPacket)->data[6]) = *((u32 *)&RTPKT_TO_OSPKT(pNextPacket)->data[2]);
				*((u16 *)&RTPKT_TO_OSPKT(pNextPacket)->data[4]) = *((u16 *)&RTPKT_TO_OSPKT(pNextPacket)->data[0]);

				RTPKT_TO_OSPKT(pNextPacket)->data += 12+4;	/* dest addr (6) + src addr (6) + vlan (4) */
				RTPKT_TO_OSPKT(pNextPacket)->len -= 12+4;
				//skb_pull(pNextSkb, 12+4); 				//printk("@: %d\n", pNextSkb->len);
				skb_push(RTPKT_TO_OSPKT(pNextPacket), 12);
				pNextPacketBufVA = (PVOID)RTPKT_TO_OSPKT(pNextPacket)->data;
				NextPacketBufLen = RTPKT_TO_OSPKT(pNextPacket)->len;				
			}
#endif
			bARALINK = TRUE;
			pAd->RalinkCounters.OneSecTxAggregationCount++;  			//Add by shiang for debug
		}
		else
		{
			printk("Error, it's impossible didn't support RaLink Aggregation Or AMSDU but have pNextPacket!\n");
		}
#endif // AGGREGATION_SUPPORT //

		if (((pNextPacketBufVA[12] << 8) + pNextPacketBufVA[13]) == 0x8100)
		{
			bVLANPktNext = TRUE;
			NextPacketBufLen -= 4; 	// VLAN packet, take off VLAN tag
		}
		/* End of if */

		bAllowFrag = FALSE;

		LocalScatterGatherList.NumberOfElements = 2;
		LocalScatterGatherList.Elements[1].Length = NextPacketBufLen;
		LocalScatterGatherList.Elements[1].Address = pNextPacketBufVA; 
		PacketInfo.TotalPacketLength += NextPacketBufLen;
	}
	pScatterGatherList = &LocalScatterGatherList;


	// ----------------------------------------
	// STEP 0.1 Add 802.1x protocol check.
	// ----------------------------------------

	// For non-WPA network, 802.1x message should not encrypt even privacy is on.
	if (NdisEqualMemory(EAPOL, pSrcBufVA + 12, 2))
		bEAPOLFrame = TRUE;
	else
		bEAPOLFrame = FALSE;


	if (RTMP_GET_PACKET_DHCP(pPacket) || bEAPOLFrame)
	{
	   bForceLowRate = TRUE;
	}

    if ((pMacEntry) && (pMacEntry->MaxHTPhyMode.field.MODE == MODE_CCK) &&(pMacEntry->MaxHTPhyMode.field.MCS == RATE_1))
	{
	   bForceLowRate = TRUE;
	}

	if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED) && 
		(pAd->CommonCfg.FixedTxMode == FIXED_TXMODE_CCK   ||pAd->CommonCfg.FixedTxMode == FIXED_TXMODE_OFDM))
	{
	   bForceLowRate = TRUE;	
	}

	if ((pMacEntry) && IS_HT_RATE(pMacEntry))
	{
		if (!bForceLowRate)
		{
			bHTRate = TRUE;
		}
	}



	//
	// WPA 802.1x secured port control - drop all non-802.1x frame before port secured
	//
#ifdef APCLI_SUPPORT
	if(bApCliPacket)
	{
	    if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
			 && (pMacEntry->PortSecured == WPA_802_1X_PORT_NOT_SECURED) 			 	 			 	 
        	 && (bEAPOLFrame == FALSE))
		{
            DBGPRINT(RT_DEBUG_INFO, ("I/F(apcli%d) APHardTransmit --> Drop packet before AP-Client secured !!!\n", ApCliIfIdx)); 
            RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE); 
            return (NDIS_STATUS_FAILURE);
        }
	}
	else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if(!bWDSEntry)
#endif // WDS_SUPPORT //
	{
        // in WPA mode, AP does not send packets before port secured.
        if ((pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        	&& (pAd->ApCfg.MBSSID[apidx].PortSecured == WPA_802_1X_PORT_NOT_SECURED) && (bEAPOLFrame == FALSE))
        {
            DBGPRINT(RT_DEBUG_INFO,("I/F(ra%d) APHardTransmit --> Drop packet before AP secured !!!\n", apidx));          
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
            return NDIS_STATUS_FAILURE;			
        }
		
        // if 802.1x, AP does not send packets other than EAPOL before this session control port set to "accept all".
		if ((pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE) && (pMacEntry) &&
			(pMacEntry->PrivacyFilter == Ndis802_11PrivFilter8021xWEP) && (bEAPOLFrame == FALSE))
		{
			DBGPRINT(RT_DEBUG_INFO, ("APHardTransmit --> Drop packet before this control port accept all !!!\n"));
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
            return NDIS_STATUS_FAILURE;	
		}		
    }	

	// TODO: 2004-12-27 lookup MAC table, check if 802.1x port secured or not
	// NOTE: no 802.1x check for WDS link

	if ((bHTRate == TRUE) && (pMacEntry) && 
		(bMoreData == FALSE) &&
		((pMacEntry->TXBAbitmap & (1<<UserPriority)) != 0)) // && ((*pSrcBufVA & 0x01) == 0))
	{
		bTXBA = TRUE;
		bAllowFrag = FALSE;
	}
		

	// -----------------------------------------------------------------
	// STEP 2. MAKE A COMMON 802.11 HEADER SHARED BY ENTIRE FRAGMENT BURST.
	// -----------------------------------------------------------------

	NdisZeroMemory(&Header_802_11, sizeof(HEADER_802_11));
	Header_802_11.FC.FrDs = 1;
	Header_802_11.FC.Type = BTYPE_DATA;
	Header_802_11.FC.SubType = SUBTYPE_DATA;
	Header_802_11.Sequence	 = pAd->Sequence;
	Header_802_11.Frag = 0;

    if (bMoreData == TRUE)
    	Header_802_11.FC.MoreData = 1;
    /* End of if */

#ifdef APCLI_SUPPORT
    if (bApCliPacket)
	{
		Header_802_11.FC.ToDs = 1;
		Header_802_11.FC.FrDs = 0;
        COPY_MAC_ADDR(Header_802_11.Addr1, APCLI_ROOT_BSSID_GET(pAd, RAWcid));// to AP2
        COPY_MAC_ADDR(Header_802_11.Addr2, pApCliEntry->CurrentAddress); // from AP1
        COPY_MAC_ADDR(Header_802_11.Addr3, pSrcBufVA); //DA
	}
    else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if (bWDSEntry == TRUE)
	{
		Header_802_11.FC.ToDs = 1;
		COPY_MAC_ADDR(Header_802_11.Addr1, pMacEntry->Addr);								// to AP2
		COPY_MAC_ADDR(Header_802_11.Addr2, pAd->CurrentAddress);							// from AP1
		COPY_MAC_ADDR(Header_802_11.Addr3, pSrcBufVA);										// DA

		// NOTE: ADDR4(=SA) will be appended later directly into pTxD->TxBuf0
	}
	else
#endif // WDS_SUPPORT //
	{
		// TODO: how about "MoreData" bit? AP need to set this bit especially for PS-POLL response
#ifdef IGMP_SNOOP_SUPPORT
		if (RAWcid != MCAST_WCID)
		{
			COPY_MAC_ADDR(Header_802_11.Addr1, pMacEntry->Addr); // DA
		}
		else
#endif // IGMP_SNOOP_SUPPORT //
		COPY_MAC_ADDR(Header_802_11.Addr1, pSrcBufVA);				  // DA
		COPY_MAC_ADDR(Header_802_11.Addr2, pAd->ApCfg.MBSSID[apidx].Bssid);  // BSSID
		COPY_MAC_ADDR(Header_802_11.Addr3, pSrcBufVA + MAC_ADDR_LEN); // SA
		Header_802_11.FC.MoreData = bMoreData;
	}

	if (Header_802_11.Addr1[0] & 0x01) // Multicast or Broadcast
	{
		bAckRequired = FALSE;
		INC_COUNTER64(pAd->WlanCounters.MulticastTransmittedFrameCount);
	}
	else if (pAd->CommonCfg.AckPolicy[QueIdx] != NORMAL_ACK)
	{
		bAckRequired = FALSE;
	}
	else if (bTXBA)
	{
		// A-MPDU needs ACK setting as TRUE.
		bAckRequired = TRUE;
	}
	else
	{
		bAckRequired = TRUE;
	}

	// -------------------------------------------
	// STEP 0.2. some early parsing
	// -------------------------------------------

#if 0
	if (bHTRate == TRUE)
		FrameGap = IFS_HTTXOP;		// Default frame gap mode
	else if ((pMacEntry) && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
		FrameGap = IFS_HTTXOP;
	// 1. traditional TX burst
	else if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE) && (pMacEntry) && 
			 (MapUserPriorityToAccessCategory[UserPriority] == QID_AC_BE)
			 && (pMacEntry->TxSeq[UserPriority] & 0x7))
		FrameGap = IFS_SIFS;  
	// 2. frame belonging to AC that has non-zero TXOP
	else if (pAd->CommonCfg.APEdcaParm.bValid && pAd->CommonCfg.APEdcaParm.Txop[QueIdx])
		FrameGap = IFS_SIFS;
	// 3. otherwise, always BACKOFF before transmission
	else
		FrameGap = IFS_BACKOFF; 	// Default frame gap mode
#endif


	// ASIC determine FrameGap
	FrameGap = IFS_HTTXOP;


	//	AP's WMM-inused or not should depends on MAC table. It's per-client not per-system
	if ((bEAPOLFrame == FALSE) && (bAMSDU || (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) && pMacEntry && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))))
	{
		Header_802_11.FC.SubType = SUBTYPE_QDATA;
		bWMM = TRUE;
		
		if ((pMacEntry) && IS_HT_STA(pMacEntry) && (!bHTRate) 
			&& (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
			&& (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RALINK_CHIPSET)))
		{
			Header_802_11.FC.SubType = SUBTYPE_DATA;
			bWMM = FALSE;
			bForceNonQoS = TRUE;
		}
	}

	// --------------------------------------------------------
	// STEP 3. FIND ENCRYPT KEY AND DECIDE CIPHER ALGORITHM
	//		Find the WPA key, either Group or Pairwise Key
	//		LEAP + TKIP also use WPA key.
	// --------------------------------------------------------
	// Decide WEP bit and cipher suite to be used. Same cipher suite should be used for whole fragment burst
	// In Cisco CCX 2.0 Leap Authentication
	//		   WepStatus is Ndis802_11Encryption1Enabled but the key will use PairwiseKey
	//		   Instead of the SharedKey, SharedKey Length may be Zero.
	pKey = NULL;

#ifdef APCLI_SUPPORT
	if (bApCliPacket)
	{	
		if (bEAPOLFrame) 
		{			
			// These EAPoL frames must be clear before 4-way handshaking is completed.
			if ((bClearEAPFrame == FALSE) && 
				(pAd->MacTab.Content[RAWcid].PairwiseKey.CipherAlg) &&
				(pAd->MacTab.Content[RAWcid].PairwiseKey.KeyLen))
			{
				CipherAlg  = pAd->MacTab.Content[RAWcid].PairwiseKey.CipherAlg;
				if (CipherAlg)
					pKey = &pAd->MacTab.Content[RAWcid].PairwiseKey;
			}
			else
			{
				CipherAlg = CIPHER_NONE;
				pKey	  = NULL;
			}

		}
		else if (pMacEntry->WepStatus == Ndis802_11Encryption1Enabled)
		{
			CipherAlg  = pApCliEntry->SharedKey[pApCliEntry->DefaultKeyId].CipherAlg;
			if (CipherAlg)
				pKey = &pApCliEntry->SharedKey[pApCliEntry->DefaultKeyId];
		}		
		else if (pMacEntry->WepStatus == Ndis802_11Encryption2Enabled ||
	 			 pMacEntry->WepStatus == Ndis802_11Encryption3Enabled)
		{
			CipherAlg  = pAd->MacTab.Content[RAWcid].PairwiseKey.CipherAlg;
			if (CipherAlg)
				pKey = &pAd->MacTab.Content[RAWcid].PairwiseKey;
		}
		else
		{
			CipherAlg = CIPHER_NONE;
			pKey	  = NULL;
		}			
	}
	else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if (bWDSEntry)
	{
		if (pAd->WdsTab.WdsEntry[pMacEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption1Enabled ||
			pAd->WdsTab.WdsEntry[pMacEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption2Enabled ||
			pAd->WdsTab.WdsEntry[pMacEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption3Enabled)		
		{
			CipherAlg  = pAd->WdsTab.WdsEntry[pMacEntry->MatchWDSTabIdx].WdsKey.CipherAlg;
			if (CipherAlg)
				pKey = &pAd->WdsTab.WdsEntry[pMacEntry->MatchWDSTabIdx].WdsKey;
		}
		else
		{
			CipherAlg = CIPHER_NONE;
			pKey = NULL;
		}
	}
	else
#endif // WDS_SUPPORT //
	if ((bEAPOLFrame)												||
		((pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption1Enabled) && (pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE)) ||
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption2Enabled)	||
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption3Enabled)	||
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption4Enabled))
	{
		if (bClearEAPFrame && bEAPOLFrame)
		{
			DBGPRINT(RT_DEBUG_TRACE,("APHardTransmit --> clear eap frame !!!\n"));          
			CipherAlg = CIPHER_NONE;
			pKey = NULL;
		}
		else if (Header_802_11.Addr1[0] & 0x01) 	   // M/BCAST to local BSS, use default key in shared key table
		{
			CipherAlg  = pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg;
			if (CipherAlg)
				pKey = &pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId];			
		}
		else						// unicast to local BSS
		{
			CipherAlg  = pAd->MacTab.Content[RAWcid].PairwiseKey.CipherAlg;
			if (CipherAlg)
				pKey = &pAd->MacTab.Content[RAWcid].PairwiseKey;
		}
	}
	else if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption1Enabled) // WEP always uses shared key table
	{
		CipherAlg  = pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg;
		if (CipherAlg)
			pKey = &pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId];
	}
	else
	{
		CipherAlg = CIPHER_NONE;
		pKey	  = NULL;
	}

	DBGPRINT(RT_DEBUG_INFO,("APHardTransmit(EAPOL=%d, Mcast=%d) to AID#%d, Alg=%s \n", 
		bEAPOLFrame, Header_802_11.Addr1[0] & 0x01, RAWcid, CipherName[CipherAlg]));

	if (CipherAlg != CIPHER_NONE)
		Header_802_11.FC.Wep = 1;

	// STEP 3.1 if TKIP is used and fragmentation is required. Driver has to
	//			append TKIP MIC at tail of the scatter buffer (This must be the
	//			ONLY scatter buffer in the NDIS PACKET). 
	//			MAC ASIC will only perform IV/EIV/ICV insertion but no TKIP MIC
	if ((MpduRequired > 1) && (CipherAlg == CIPHER_TKIP))
	{
		ASSERT(pScatterGatherList->NumberOfElements == 1);
		RTMPCalculateMICValue(pAd, pPacket, pExtraLlcSnapEncap, pKey, apidx);
		NdisMoveMemory(pSrcBufVA + SrcBufLen, pAd->PrivateInfo.Tx.MIC, 8);
		SrcBufLen += 8;
		pScatterGatherList->Elements[0].Length += 8;
		PacketInfo.TotalPacketLength += 8;
		CipherAlg = CIPHER_TKIP_NO_MIC;
	}
	else
	{
		//
		// Snice the ScatterGather may allocate on the contiguous range of the base physical address
		// In that case, the pScatterGatherList->Elements[0].Length will equal to PacketInfo.TotalPacketLength
		// And the pScatterGatherList->NumberOfElements will be reduce one snice the contiguous range of the data.
		// So we need to reset the SrcBufLen as pScatterGatherList->Elements[0].Length as our first Packet length.
		// Otherwise will not get the next pScatterGatherList Elements's length and cause TxRing Full.
		// This is ok on PKTSRC_NDIS case or not the PKTSRC_NDIS case.
		//	
		SrcBufLen		  = pScatterGatherList->Elements[0].Length;
	}

	//
	// calcuate the overhead bytes that encryption algorithm may add. This
	// affects the calculate of "duration" field
	//
	if ((CipherAlg == CIPHER_WEP64) || (CipherAlg == CIPHER_WEP128)) 
		EncryptionOverhead = 8; //WEP: IV[4] + ICV[4];
	else if (CipherAlg == CIPHER_TKIP_NO_MIC)
		EncryptionOverhead = 12;//TKIP: IV[4] + EIV[4] + ICV[4], MIC will be added to TotalPacketLength
	else if (CipherAlg == CIPHER_TKIP)
		EncryptionOverhead = 20;//TKIP: IV[4] + EIV[4] + ICV[4] + MIC[8]
	else if (CipherAlg == CIPHER_AES)
		EncryptionOverhead = 16;	// AES: IV[4] + EIV[4] + MIC[8]
	else
		EncryptionOverhead = 0;
	

	// ----------------------------------------------------------------
	// STEP 4. Make RTS frame or CTS-to-self frame if required
	// ----------------------------------------------------------------

	// decide how much time an ACK/CTS frame will consume in the air
	AckDuration = RTMPCalcDuration(pAd, pAd->CommonCfg.ExpectedACKRate[TxRate], 14);

	if (RtsRequired)
	{
#if 0
		unsigned int NextMpduSize;

		// If fragment required, MPDU size is maximum fragment size
		// Else, MPDU size should be frame with 802.11 header & CRC
		if (MpduRequired > 1)
			NextMpduSize = pAd->CommonCfg.FragmentThreshold;
		else
		{
			NextMpduSize = PacketInfo.TotalPacketLength + LENGTH_802_11 + LENGTH_CRC - LENGTH_802_3;
			if (pExtraLlcSnapEncap)
				NextMpduSize += LENGTH_802_1_H;
		}

		RTMPSendRTSFrame(pAd, 
						 Header_802_11.Addr1, 
						 NextMpduSize + EncryptionOverhead, 
						 TxRate,
						 pAd->CommonCfg.RtsRate, 
						 AckDuration,
						 QueIdx,
						 FrameGap);

		// RTS/CTS-protected frame should use LONG_RETRY (=4) and SIFS
		RetryMode = LONG_RETRY;
		FrameGap = IFS_SIFS;
		bRTS_CTSFrame = TRUE;
#endif
	}

	// decide is the need of piggy-back
	if ((bHTRate == FALSE)&& pMacEntry && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE))
	{
		bPiggyBack = TRUE;
	}
	else
	{
		bPiggyBack = FALSE;
	}

    if (RAWcid != MCAST_WCID) 
    {
		ASSERT(pMacEntry);

        if (bTXBA)
        {	
            RABAOriIdx = pMacEntry->BAOriWcidArray[UserPriority];
            TXWIBAWinSize = pAd->BATable.BAOriEntry[RABAOriIdx].BAWinSize - 1;
			ASSERT(TXWIBAWinSize >= 0);
            TXWIAMPDU = TRUE;
            Header_802_11.Frag = 0;
        }

        if (bForceNonQoS)
		{			
			Header_802_11.Sequence = pMacEntry->NonQosDataSeq;
			pMacEntry->NonQosDataSeq = (pMacEntry->NonQosDataSeq+1) & MAXSEQ;
		}
		else
		{		
        	Header_802_11.Sequence = pMacEntry->TxSeq[UserPriority];
        	pMacEntry->TxSeq[UserPriority] = (pMacEntry->TxSeq[UserPriority]+1) & MAXSEQ;
    	}
    }
    else
    {
		Header_802_11.Sequence = pAd->Sequence;
		pAd->Sequence = (pAd->Sequence+1) & MAXSEQ; // next sequence 		
    }


	// --------------------------------------------------------
	// STEP 5. START MAKING MPDU(s)
	//		Start Copy Ndis Packet into Ring buffer.
	//		For frame required more than one ring buffer (fragment), all ring buffers
	//		have to be filled before kicking start tx bit.
	//		Make sure TX ring resource won't be used by other threads
	// --------------------------------------------------------


	SrcBufIdx		  = 0;

	SrcRemainingBytes = PacketInfo.TotalPacketLength;
	{
		ULONG	OffsetSrcVA = (ULONG) pScatterGatherList->Elements[0].Address;

		OffsetSrcVA      += LENGTH_802_3;  // skip 802.3 header
		SrcBufLen        -= LENGTH_802_3;  // skip 802.3 header
		
        if (bVLANPkt == TRUE)
        {
			OffsetSrcVA += 4; /* skip VLAN protocol & tag */
			SrcBufLen   -= 4; /* skip VLAN protocol & tag */
        } /* End of if */

#ifdef CONFIG_5VT_ENHANCE
		if ( RTMP_GET_PACKET_5VT(pPacket) )
		{
			SrcBufPA = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
								16, PCI_DMA_TODEVICE);
		}						
		else
#endif
		{
			SrcBufPA = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
								SrcBufLen, PCI_DMA_TODEVICE);
		}						
 						
//		SrcBufPA         += LENGTH_802_3;  // skip 802.3 header
        if (bVLANPkt == TRUE)
			SrcRemainingBytes = PacketInfo.TotalPacketLength - LENGTH_802_3 - 4;
		else
		SrcRemainingBytes = PacketInfo.TotalPacketLength - LENGTH_802_3;
	} 

	//NdisAcquireSpinLock(&pAd->TxRingLock);

	//
	// Update SW_TX_IDX[QueIdx] and *pFreeTXDLeft after finish the copy while-loop
	// FreeTXDLeft is used to record current Free TXD.  
	// If return middle way, real *pFreeTXDLeft don't change.
	// 
	// FreeTXDLeft must be >= 1 and >=2 when aggregation 
	// 
	do
	{
		ULONG  BufBasePaLow;


		//
		// STEP 5.1 ACQUIRE TXD
		//
		pFirstTxWI = (PTXWI_STRUC)pTxRing->Cell[TxIdx].DmaBuf.AllocVa; 
		pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
		BufBasePaLow  = RTMP_GetPhysicalAddressLow (pTxRing->Cell[TxIdx].DmaBuf.AllocPa);
		pTxRing->Cell[TxIdx].pNdisPacket = NULL; 			 
		pTxRing->Cell[TxIdx].pNextNdisPacket= NULL;		

		// need to fill MPDU total byte count field at last step
		pDMAHeaderBufPtr = pDMAHeaderBufVA;
		//
		// STEP 6.3 ACQUIRE TXD. 
		//
#ifndef BIG_ENDIAN
		pTxD  = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
#else
		pDestTxD  = (PTXD_STRUC)pTxRing->Cell[TxIdx].AllocVa;
		TxD = *pDestTxD;
		pTxD = &TxD;
#endif
		//pTxD = &TxD;
		NdisZeroMemory(pTxD, TXD_SIZE);
//		RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
		//
		// STEP 6.2 COPY TXWI and 802.11 HEADER INTO 1ST DMA BUFFER. IV/EIV will set by ASIC after searching on-chip WC table
		//

		pDMAHeaderBufPtr += TXWI_SIZE;
		pDMAHeaderBufPtr += sizeof(Header_802_11);
		MpduSize = sizeof(Header_802_11);

		FreeTXDLeft--;

		//
		// Fragmentation is not allowed on 
		//  1. multicast & broadcast
		//  2. A-MPDU, A-MSDU, A-Ralink
		// So, we need to used the MAX_FRAG_THRESHOLD instead of pAd->CommonCfg.FragmentThreshold
		// otherwise if PacketInfo.TotalPacketLength > pAd->CommonCfg.FragmentThreshold then
		// packet will be fragment on multicast & broadcast.
		//		
		if (RAWcid == MCAST_WCID || bAllowFrag == FALSE)
		{
			FreeMpduSize = MAX_AGGREGATION_SIZE - sizeof(Header_802_11) - LENGTH_CRC;
		}
		else
		{
			FreeMpduSize = pAd->CommonCfg.FragmentThreshold - sizeof(Header_802_11) - LENGTH_CRC;
		}

#ifdef WDS_SUPPORT
		// Add ADDR4 field if WDS frame
		if (bWDSEntry == TRUE)
		{
			COPY_MAC_ADDR(pDMAHeaderBufPtr, pSrcBufVA + MAC_ADDR_LEN); // ADDR4 = SA
			pDMAHeaderBufPtr		 += MAC_ADDR_LEN;
			MpduSize += MAC_ADDR_LEN;
			FreeMpduSize -= MAC_ADDR_LEN;
		}
#endif // WDS_SUPPORT //

		// TODO: 2005-02-10 also check if the STA is WMM capable
		// NOTE: no QOS DATA format on WDS link so far
//		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) && pMacEntry && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
		if (bWMM)
		{
			// copy QOS CONTROL bytes
			if (bTXBA)
				*pDMAHeaderBufPtr        =  (UserPriority & 0x0f);
			else
				*pDMAHeaderBufPtr		 =  (UserPriority & 0x0f) | (pAd->CommonCfg.AckPolicy[QueIdx]<<5);

#ifdef UAPSD_AP_SUPPORT
			if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
				&& (bWDSEntry == FALSE)
#endif // WDS_SUPPORT //
				)
			{
				/* we can not use bMoreData bit to get EOSP bit because
					maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 */
				*pDMAHeaderBufPtr |= (bWMM_UAPSD_EOSP << 4);
			} /* End of if */
#endif // UAPSD_AP_SUPPORT //

			if (bAMSDU == TRUE)
			{
				*pDMAHeaderBufPtr |= 0x80;
			}
			*(pDMAHeaderBufPtr+1)	  =  0;
			pDMAHeaderBufPtr		  += 2;
			MpduSize += 2;
			FreeMpduSize  -= 2;
		}

		if (pMacEntry && bHTRate)
		{
			// peer is HT STA
			if ((pAd->HTCEnable == TRUE) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_HTC_CAPABLE)
							&& CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
			{
			// HTC bit share with order bit
				Header_802_11.FC.Order = 1;	

				// HTC Control field is following QOS field.
				NdisZeroMemory(pDMAHeaderBufPtr, 4);

				if ((pAd->CommonCfg.bRdg == TRUE) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
				{						
					*(pDMAHeaderBufPtr+3)|=0x80;
				}

				if ((pAd->bLinkAdapt == TRUE) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_MCSFEEDBACK_CAPABLE))
				{	
					//bit2MRQ=1. Request for MCS feedback
					*(pDMAHeaderBufPtr) |= 0x4;
					// Set identifier as 2
					*(pDMAHeaderBufPtr) |= ((pAd->MRS)<<3);
				}
					
				pDMAHeaderBufPtr		  += 4;
				//pDMAHeaderBufPtr		  += 4;
				MpduSize += 4;
			}
		}

		/*
		** padding at front of LLC header.
		** according to Jerry comment. LLC header should at 4-bytes aligment.
		*/
		pDMAHeaderBufPtr = (PCHAR)ROUND_UP(pDMAHeaderBufPtr, 4);

		// Handle Aggregation (A-MSUD for 11n station in HTMode or A-Ralink in Legacy 11g Ralink station, )
		if (bARALINK)
		{
			// For RA Aggregation, 
			// put the 2nd MSDU length(extra 2-byte field) after QOS_CONTROL in little endian format
			*pDMAHeaderBufPtr = (UCHAR)NextPacketBufLen & 0xff;
			*(pDMAHeaderBufPtr+1) = (UCHAR)(NextPacketBufLen >> 8);

			// steal "order" bit to mark "aggregation"
			Header_802_11.FC.Order = 1;
				
			pDMAHeaderBufPtr += 2;
			MpduSize += 2;
		}
		else if (bAMSDU)
		{
			/* For 11n AMSDU, A-MSDU format: DA + SA + Length + MSDU + Padding */
     		//
     		// use 1st dma buffer to store subframe header 
     		// DA(6)+SA(6)+Len(2)
     		// 							
     		// build subframe header
     		NdisMoveMemory(pDMAHeaderBufPtr, &AMSDUsubheader[0], 14);
     		pDMAHeaderBufPtr += 14;
			MpduSize += 14;
		}


		//
		// STEP 5.4 COPY LLC/SNAP, CKIP MIC INTO 1ST DMA BUFFER ONLY WHEN THIS 
		//			MPDU IS THE 1ST OR ONLY FRAGMENT 
		//
		if (Header_802_11.Frag == 0)
		{
			//if (pExtraLlcSnapEncap && (bAMSDU == FALSE))
            if (pExtraLlcSnapEncap)
			{
#if 0
#ifdef CONFIG_CKIP_SUPPORT
				if ((pAd->ApCfg.CkipFlag & 0x08) && (bEAPOLFrame == FALSE))
				{
					// CKIP_LLC_SNAP is inused. The frame format must be -
					//	   802.11 header + CKIP_LLCSNAP(8) + MIC(4) + TxSEQ(4) + Proto(2) + Payload
					NdisMoveMemory(pDMAHeaderBufPtr, CKIP_LLC_SNAP, sizeof(CKIP_LLC_SNAP));
					pDMAHeaderBufPtr += sizeof(CKIP_LLC_SNAP);
					//
					// 1.) Insert MIC [4]
					//
					RTMPCkipInsertCMIC(pAd, pDMAHeaderBufPtr, (PUCHAR)&Header_802_11, pPacket, pKey, CKIP_LLC_SNAP);					
					pDMAHeaderBufPtr += 4 ;
					//
					// 2.) Insert TX Sequence.
					//					
					NdisMoveMemory(pDMAHeaderBufPtr, pAd->ApCfg.TxSEQ, 4);
					pDMAHeaderBufPtr += 4;
					if (bVLANPkt == TRUE)
						NdisMoveMemory(pDMAHeaderBufPtr, pSrcBufVA + 12+4, 2); /* skip VLAN tag */
					else
					    NdisMoveMemory(pDMAHeaderBufPtr, pSrcBufVA + 12, 2);
					pDMAHeaderBufPtr += 2;
					MpduSize += 10;
					FreeMpduSize -= (sizeof(CKIP_LLC_SNAP) + 10);

					// Update TxSEQ for next TX
					{ 
						int i = 3;
						pAd->ApCfg.TxSEQ[i] = pAd->ApCfg.TxSEQ[i] + 2;
						while (pAd->ApCfg.TxSEQ[i] == 0x00)
						{
							i--;
							if (i < 0) break;
							pAd->ApCfg.TxSEQ[i] ++;
						}
					}

				}
				else
#endif /* CONFIG_CKIP_SUPPORT */
#endif
				{
					// Insert LLC-SNAP encapsulation
					NdisMoveMemory(pDMAHeaderBufPtr, pExtraLlcSnapEncap, 6);
					pDMAHeaderBufPtr += 6;

					if (bVLANPkt == TRUE)
						NdisMoveMemory(pDMAHeaderBufPtr, pSrcBufVA + 12+4, 2); /* skip VLAN tag */
					else
					    NdisMoveMemory(pDMAHeaderBufPtr, pSrcBufVA + 12, 2);
					pDMAHeaderBufPtr += 2;
					MpduSize += LENGTH_802_1_H;
					FreeMpduSize -= LENGTH_802_1_H;
				}
			}
		}

		if (bAMSDU)
		{
			//hex_dump("1st MSDU", pDMAHeaderBufVA+TXWI_SIZE+28, 22);
		}
     		//
		// STEP 5.5 TRAVERSE NDIS PACKET TO BUILD THE MPDU PAYLOAD
     		// 							
		// TX buf0 size fixed here

		// 
		// set 1st necessary header size into TX DESC 
		//   --- TxWI + 802.11 Header 
		//   --- if (!A-MSDU) 
		//            + LLC/SNAP Encap 
		//       else
		//            + DA(6)+SA(6)+Length(2)
		// 
		pTxD->SDPtr0 = BufBasePaLow;
		pTxD->SDLen0 = (ULONG)(pDMAHeaderBufPtr - pDMAHeaderBufVA);
		if (letusprint == TRUE)
			DBGPRINT(RT_DEBUG_TRACE,("1.== Use pTxD[%ld]->SDLen0=%d   nextSrcBufLen=%d \n", TxIdx, pTxD->SDLen0,SrcBufLen));
		SrcBytesCopied	 = 0;
		// Inner while loop is for getting enough TXD to send this MPDU. 
		// SDPNowUsed starts from 1 because WI and 802.11Header already use SDP0 of 1st TXD.
		SDPNowUsed = 1;
		IsSD0 = TRUE;
		do
		{
			PUCHAR pSubHeader;
			UCHAR  SDLen0 = 0;

			// Assign Currently used TxD.  
			SDPNowUsed ++;
			if (SrcBufLen <= FreeMpduSize)
			{
				// scatter-gather buffer still fit into current MPDU
				if (SrcBufLen != 0)
				{
					switch (SDPNowUsed%2) 
					{
						case 1:  							
							ASSERT(FreeTXDLeft);

							FreeTXDLeft--;
							INC_RING_INDEX(TxIdx, TX_RING_SIZE);		  
							//
							// AMSDU packet: 2nd subframe
							// 
							//pTxRing->Cell[TxIdx].pNdisPacket = pNextPacket; 			 
							//pTxRing->Cell[TxIdx].pNextNdisPacket= NULL;		
							// Init TxRing 
#ifndef BIG_ENDIAN
							pTxD  = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
#else
							//Swap the endian of old pTxD and prepare to handle next pTxD.
							RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
							WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
		
							pDestTxD  = (PTXD_STRUC)pTxRing->Cell[TxIdx].AllocVa;
							TxD = *pDestTxD;
							pTxD = &TxD;
#endif
							NdisZeroMemory(pTxD, TXD_SIZE);
							RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);

							//
							// use 1st dma buffer to store subframe header 
							// pervious subframe padding + DA(6)+ SA(6) + Len(2) + LLC/SNAP(8)
							// 
							BufBasePaLow  = RTMP_GetPhysicalAddressLow (pTxRing->Cell[TxIdx].DmaBuf.AllocPa);
							pSubHeader = pTxRing->Cell[TxIdx].DmaBuf.AllocVa;

							if (bAMSDU)
							{
							ASSERT(padding < 4);

							pSubHeader += padding;
										
							// build subframe header
							NdisMoveMemory(pSubHeader ,pNextPacketBufVA, 12);
							
                           	// if orginal Ethernet frame contains no LLC/SNAP, then an extra LLC/SNAP encap is required 
                            EXTRA_LLCSNAP_ENCAP(pNextPacketBufVA, pExtraLlcSnapEncap);

							SDLen0 = padding + LENGTH_802_3;

							NextPacketBufLen -= LENGTH_802_3;

							if (pExtraLlcSnapEncap)
							{
								NextPacketBufLen += LENGTH_802_1_H;
								NdisMoveMemory(pSubHeader+14, pExtraLlcSnapEncap, 6);

//								if (((pNextPacketBufVA[12] << 8) + pNextPacketBufVA[13]) == 0x8100)
								if (bVLANPktNext == TRUE)
									NdisMoveMemory(pSubHeader+14+6, pNextPacketBufVA+12+4, 2);
								else
								    NdisMoveMemory(pSubHeader+14+6, pNextPacketBufVA+12, 2); 								
								SDLen0 += LENGTH_802_1_H;
							}
							

							pSubHeader[12] = (NextPacketBufLen & 0xFF00) >> 8;
							pSubHeader[13] = NextPacketBufLen & 0xFF;
							}
							else if (bARALINK)
							{
								NdisMoveMemory(pSubHeader , pNextPacketBufVA, 12);
#ifdef APCLI_SUPPORT
								if(bApCliPacket && pApCliEntry->Valid)
									NdisMoveMemory(pSubHeader+6 , pApCliEntry->CurrentAddress, 6);
#endif // APCLI_SUPPORT //
								if (bVLANPktNext == TRUE)
									NdisMoveMemory(pSubHeader+12, pNextPacketBufVA+12+4, 2);
								else
									NdisMoveMemory(pSubHeader+12, pNextPacketBufVA+12, 2);
								
								SDLen0 = LENGTH_802_3;

								//Here we didn't need to subtract the VLAN tag length, because we do it in previous.
								NextPacketBufLen -= LENGTH_802_3;
							}
														
							//hex_dump("2nd MSDU", pSubHeader, 22);

							// update MPDU size 
							MpduSize += SDLen0;

							// build TX DESC
							pTxD->SDPtr0 = BufBasePaLow;
							pTxD->SDLen0 = SDLen0;
							pTxD->SDPtr1 = SrcBufPA;
							pTxD->SDLen1 = SrcBufLen;

							//printk("### A-MSDU ###\n");
							//IsSD0 = TRUE;

							break;
						case 0: 
							//
							// build first Tx Desc
							// 

							pTxD->SDPtr1 = SrcBufPA;
							pTxD->SDLen1 = SrcBufLen;
							IsSD0 = FALSE;
							
							// if aggregatible, store Packet poiner to next Cell for release 
							if (pNextPacket)
							{
								RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
								pTxRing->Cell[TxIdx].pNdisPacket = pPacket;
								pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;							
							}

							if (letusprint == TRUE)
								DBGPRINT(RT_DEBUG_TRACE,(" Use pTxD[%ld] SDPtr1,  Len = %d pTxD->SDLen1!!\n", TxIdx, pTxD->SDLen1));
							break;
						default:    // should never happen
							break;
					}
				}
				else
					SDPNowUsed--;
				
				SrcBytesCopied		+= SrcBufLen;
				//pDMAHeaderBufPtr	+= SrcBufLen;
				FreeMpduSize		-= SrcBufLen;
				MpduSize			+= SrcBufLen;
				SrcBufPA			+= SrcBufLen;				

				// 
				// calcuate padding bytes for subframe
				// put the padding bytes into 1st DMA buffer of next DESC
				// 
				if (bAMSDU && (SrcBufIdx == 0))
				{
					ULONG AlignLen, EncapLen;

					EncapLen = 0;
					
					if (pExtraLlcSnapEncap)
					{
						EncapLen = LENGTH_802_1_H;
					}

					AlignLen = ((14+EncapLen+SrcBufLen)+3) & ~(0x3);
					padding = AlignLen-(14+EncapLen+SrcBufLen);	
					//printk("# 1st subframe len = %d padding = %d\n", 14+SrcBufLen, padding);
				}

				// advance to next scatter-gather BUFFER
				SrcBufIdx++;

				if (SrcBufIdx < pScatterGatherList->NumberOfElements)
				{
					pTxD->LastSec0 = 0;
					pTxD->LastSec1 = 0;

					{
						ULONG	OffsetSrcVA = (ULONG) pScatterGatherList->Elements[SrcBufIdx].Address;
						UCHAR   *Header = (UCHAR *)OffsetSrcVA;

						SrcBufLen = pScatterGatherList->Elements[SrcBufIdx].Length;					

						OffsetSrcVA      += LENGTH_802_3;  // skip 802.3 header
						SrcBufLen        -= LENGTH_802_3;  // skip 802.3 header
														   						
					    if (((Header[12] << 8) + Header[13]) == 0x8100)
						{
							/* VLAN packet, take off VLAN tag */
							/* do NOT add "SrcBufLen -= 4;" here, because SrcBufLen is
							   already -4 before; SrcBufLen is NextPacketBufLen;
							   NextPacketBufLen is already -4 before */
							OffsetSrcVA += 4;
						}

						SrcBufPA          = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
												SrcBufLen, PCI_DMA_TODEVICE);  
						SrcRemainingBytes -= LENGTH_802_3;
					} 
				}
				else 
				{
						pTxD->LastSec1 = 1;
					SrcBufLen = 0;
				}

				if (SrcBufLen == 0)
					break;
			}
			else
			{
				//scatter-gather buffer exceed current MPDU. leave some of the buffer to next MPDU
				switch (SDPNowUsed%2)
				{
					case 1:         // Init TxRing 
						NdisZeroMemory(pTxD, sizeof(pTxD));
						pTxD->SDPtr0 = SrcBufPA;
						pTxD->SDLen0 = FreeMpduSize;
						pTxD->LastSec0 = 1;
						DBGPRINT(RT_DEBUG_TRACE,(" Last Fraged TXD. Use pTxD[] SDPtr0, Len = %d pTxD->SDLen0!!\n",pTxD->SDLen0));

						break;
					case 0: 
						pTxD->SDPtr1 = SrcBufPA;
						pTxD->SDLen1     = FreeMpduSize;
						pTxD->LastSec1 = 1;
						DBGPRINT(RT_DEBUG_TRACE,(" Last Fraged TXD. Use pTxD[] SDPtr1,  Len = %d pTxD->SDLen1!!\n", pTxD->SDLen1));

						break;
					default:    // should never happen
						break;
				}
				SrcBytesCopied += FreeMpduSize;
				pDMAHeaderBufPtr          += FreeMpduSize;
				MpduSize       += FreeMpduSize;
				SrcBufPA       += FreeMpduSize;
				SrcBufLen      -= FreeMpduSize;

				// a complete MPDU is built. break out to write TXD of this MPDU
				break;
			}

		} while (TRUE);		// End of copying payload

		// remaining size of the NDIS packet payload
		SrcRemainingBytes -= SrcBytesCopied;

		if (bAMSDU || bARALINK)
		{
			ASSERT(SrcBufIdx == 2);
		}
		//
		// STEP 5.6 MODIFY MORE_FRAGMENT BIT & DURATION FIELD. WRITE TXD
		//
		//pHeader80211 = (PHEADER_802_11)((PUCHAR)pFirstTxWI + TXWI_SIZE);
		pHeader80211 = &Header_802_11;
		if (SrcRemainingBytes > 0) // more fragment is required
		{
			UINT NextMpduSize;

			NextMpduSize = min(((UINT)SrcRemainingBytes), ((UINT)pAd->CommonCfg.FragmentThreshold));
			pHeader80211->FC.MoreFrag = 1;
			pHeader80211->Duration = (3 * pAd->CommonCfg.Dsifs) + (2 * AckDuration) + RTMPCalcDuration(pAd, TxRate, NextMpduSize + EncryptionOverhead);

			RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
			pTxD->Burst = 1;
			// Although WIV is filled by FCE, byte count need driver to fill for ASIC initialization
			pFirstTxWI->MPDUtotalByteCount = MpduSize;
			FrameGap = IFS_SIFS;	 // use SIFS for all subsequent fragments

			// store 802.11 header before the frag number is counted
			NdisMoveMemory(pDMAHeaderBufVA+TXWI_SIZE, &Header_802_11, sizeof(Header_802_11));

			Header_802_11.Frag ++;	 // increase Frag #
			pTxRing->Cell[TxIdx].pNdisPacket = NULL;              
			pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;
		}
		else // this is the last or only fragment
		{
			pHeader80211->FC.MoreFrag = 0;
			if (pHeader80211->Addr1[0] & 0x01) // multicast/broadcast
				pHeader80211->Duration = 0;
			else
				pHeader80211->Duration = pAd->CommonCfg.Dsifs + AckDuration;

			// store 802.11 header
			NdisMoveMemory(pDMAHeaderBufVA+TXWI_SIZE, &Header_802_11, sizeof(Header_802_11));
#if 0
			if ((bEAPOLFrame) && (bHTRate == TRUE))
			{
				pFirstTxWI->PHYMODE = MODE_OFDM;
				pFirstTxWI->BW = 0;
				pFirstTxWI->MCS = 0;
			}
#endif

			RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
			if (bAMSDU || bARALINK)
			{
				pTxRing->Cell[TxIdx].pNdisPacket = pNextPacket;              
			}
			else
			{
				pTxRing->Cell[TxIdx].pNdisPacket = pPacket;              
				//pTxRing->Cell[TxIdx].pNextNdisPacket = pNextPacket;
			}

			pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;
		}

//		printk("bTXBA = %d, bPiggyBack = %d\n", bTXBA, bPiggyBack); 

		ASSERT(MpduSize < 4096);

		if (pMacEntry == NULL)
		{
			pMacEntry = &pAd->MacTab.Content[MCAST_WCID]; 		
			GET_GroupKey_WCID(RAWcid, apidx);			
		}

		// Inform MAC ASIC TKIP engine this is a fragment, so that TKIP MIC is appended by driver at the last fragment
		bFrag = ((pHeader80211->FC.MoreFrag) || (Header_802_11.Frag != 0));

		if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
		{
			if (pAd->CommonCfg.RegTransmitSetting.field.MCS == 32)
			{
				TxHTPhyMode.field.BW = 1;
				TxHTPhyMode.field.ShortGI = 0 ;
				TxHTPhyMode.field.STBC = 0;
				TxHTPhyMode.field.MCS = 32; 
				TxHTPhyMode.field.MODE = MODE_HTMIX;
			}
			else
			{
				TxHTPhyMode.field.BW =  pMacEntry->HTPhyMode.field.BW;
				TxHTPhyMode.field.ShortGI = pAd->CommonCfg.RegTransmitSetting.field.ShortGI; //pMacEntry->HTPhyMode.field.ShortGI;
				TxHTPhyMode.field.STBC = 0;	
				TxHTPhyMode.field.MCS = /*pMacEntry->HTPhyMode.field.MCS; */ pAd->CommonCfg.RegTransmitSetting.field.MCS; 
				if (pAd->CommonCfg.RegTransmitSetting.field.HTMODE == HTMODE_GF)
				{
					TxHTPhyMode.field.MODE = MODE_HTGREENFIELD;
				}
				else
				{
					TxHTPhyMode.field.MODE = pMacEntry->HTPhyMode.field.MODE;
				}

				// Override HT Tx Mode by Fixed Legency Tx Mode, if specified.
				if (pAd->CommonCfg.FixedTxMode == FIXED_TXMODE_CCK || pAd->CommonCfg.FixedTxMode == FIXED_TXMODE_OFDM)
				{
					TxHTPhyMode.field.STBC = 0;
					TxHTPhyMode.field.BW = 0;
					TxHTPhyMode.field.ShortGI = 0 ;		
				
					if (pAd->CommonCfg.FixedTxMode == FIXED_TXMODE_CCK)
						TxHTPhyMode.field.MODE = MODE_CCK;
					else
						TxHTPhyMode.field.MODE = MODE_OFDM;
				} 
				else 
				{
					; // No override needed
				}
			}

			pTxHTPhyMode = &TxHTPhyMode; 
			//NdisMoveMemory(pDMAHeaderBufVA+TXWI_SIZE, &Header_802_11, sizeof(Header_802_11));	
		}
		else 
		{
			//printk("%02x:%02x:%02x:%02x:%02x:%02x\n",  PRINT_MAC(pMacEntry->Addr));
			//printk("MODE = %d\n", pMacEntry->HTPhyMode.field.MODE);			

			if (bForceLowRate)
			{			
				pTxHTPhyMode = &pAd->MacTab.Content[MCAST_WCID].HTPhyMode; 
			}
			else
			{
				pTxHTPhyMode = &pMacEntry->HTPhyMode; 
			}
			//NdisMoveMemory(pDMAHeaderBufVA+TXWI_SIZE, &Header_802_11, sizeof(Header_802_11));
		}

#ifdef MCAST_RATE_SPECIFIC
		// specific transmit rate for multicast packet.
		if ((pAd->CommonCfg.McastTransmitPhyMode != MCAST_DISABLE)
			&& IsGroupKeyWCID(RAWcid)
			&& (*pSrcBufVA != 0xff)) // it's a Multicast packet.
		{
			TxHTPhyMode.field.BW = 0;
			TxHTPhyMode.field.ShortGI = 0;
			TxHTPhyMode.field.STBC = 0;
			TxHTPhyMode.field.MCS = pAd->CommonCfg.McastTransmitMcs;
			if (pAd->CommonCfg.McastTransmitPhyMode == MCAST_CCK)
				TxHTPhyMode.field.MODE = MODE_CCK;
			else if (pAd->CommonCfg.McastTransmitPhyMode == MCAST_OFDM)
				TxHTPhyMode.field.MODE = MODE_OFDM;
			else
				TxHTPhyMode.field.MODE = MODE_OFDM;

			pTxHTPhyMode = &TxHTPhyMode; 
		}
#endif // MCAST_RATE_SPECIFIC //


		RTMPWriteTxWI(pAd, pFirstTxWI, bFrag, FALSE, FALSE,  TXWIAMPDU, bAckRequired, FALSE, 
					  TXWIBAWinSize, RAWcid, MpduSize, PID, UserPriority, TxRate, FrameGap, bPiggyBack, pTxHTPhyMode);

		//
		// Cache TXWI and 802.11 header for BASmartHardTransmit
		//
		if (bWMM && bTXBA)
		{
			pMacEntry->isCached = TRUE;
			NdisMoveMemory(pMacEntry->CachedBuf, pDMAHeaderBufVA, 64);
			//hex_dump("Cached", pMacEntry->CachedBuf, pTxD->SDLen0);
		}

		//pTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
		//memcpy(pTxD, &TxD, sizeof(TXD_STRUC));

		// calculate Transmitted AMSDU Count and ByteCount
		if (bAMSDU)
		{
			pAd->RalinkCounters.TransmittedAMSDUCount.u.LowPart ++;
			pAd->RalinkCounters.TransmittedOctetsInAMSDU.QuadPart += MpduSize;			
		}

		// calculate Transmitted AMPDU count and ByteCount 
		if (TXWIAMPDU)
		{
			pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart ++;
			pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.QuadPart += MpduSize;		
		}

#ifdef BIG_ENDIAN
		// First check if we need to do endian conversion for the TX_WI.
		RTMPWIEndianChange((PTXWI_STRUC)pFirstTxWI, TYPE_TXWI);

		// Do endian conversion for the 802.11 header
		RTMPFrameEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA+TXWI_SIZE), DIR_WRITE, FALSE);
		
		// Do endian converison for the Tx Descriptor.
		RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
		WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);
#endif
		INC_RING_INDEX(TxIdx, TX_RING_SIZE);
		pAd->RalinkCounters.KickTxCount++;
 		pAd->RalinkCounters.OneSecTxDoneCount++;

	} while (SrcRemainingBytes > 0);

	// Success , Update True FreeTXDLeft
	*pFreeTXDLeft = FreeTXDLeft;
	// Update  SW_TX_IDX
	pAd->TxRing[QueIdx].TxCpuIdx = TxIdx;
	
	RTMP_IO_WRITE32(pAd, TX_CTX_IDX0 + QueIdx * 0x10 , pAd->TxRing[QueIdx].TxCpuIdx);
	// the NDIS PACKET "pPacket" will be released at DMA done interrupt service routine

	// Make sure to release Tx ring resource
	// NdisReleaseSpinLock(&pAd->TxRingLock);

	return (NDIS_STATUS_SUCCESS);
}

/*
	========================================================================
	Routine Description:
		Check Rx descriptor, return NDIS_STATUS_FAILURE if any error found
	========================================================================
*/
NDIS_STATUS APCheckRxError(
	IN	PRTMP_ADAPTER	pAd,
	IN	PRXD_STRUC		pRxD,
	IN	UCHAR			Wcid)
{
	if (pRxD->Crc || pRxD->CipherErr)
	{
		// WCID equ to 255 mean MAC couldn't find any matched entry in Asic-MAC table.
		// The incoming packet mays come from WDS or AP-Client link.
		// We need them for further process. Can't drop the packet here.
		if ((pRxD->U2M)
			&& (pRxD->CipherErr)
			&& (Wcid == 255)
#ifdef WDS_SUPPORT
			&& (pAd->WdsTab.Mode == WDS_LAZY_MODE)
#endif // WDS_SUPPORT //
		)
		{
			// pass those packet for further process.
			return NDIS_STATUS_SUCCESS;
		}
		else
			return NDIS_STATUS_FAILURE;
	}
	else
	{
		return NDIS_STATUS_SUCCESS;
	}
}

/*
  ========================================================================
  Description:
	This routine checks if a received frame causes class 2 or class 3
	error, and perform error action (DEAUTH or DISASSOC) accordingly
  ========================================================================
*/
BOOLEAN APCheckClass2Class3Error(
	IN	PRTMP_ADAPTER	pAd,
	IN 	ULONG Wcid, 
	IN	PHEADER_802_11	pHeader)
{
	// software MAC table might be smaller than ASIC on-chip total size.
	// If no mathed wcid index in ASIC on chip, do we need more check???  need to check again. 06-06-2006
	if (Wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		APCls2errAction(pAd, MAX_LEN_OF_MAC_TABLE, pHeader);
		return TRUE;
	}

	if (pAd->MacTab.Content[Wcid].Sst == SST_ASSOC)
		; // okay to receive this DATA frame
	else if (pAd->MacTab.Content[Wcid].Sst == SST_AUTH)
	{
		APCls3errAction(pAd, Wcid, pHeader);
		return TRUE; 
	}
	else
	{
		APCls2errAction(pAd, Wcid, pHeader);
		return TRUE; 
	}
	return FALSE;
}

/*
  ========================================================================
  Description:
	This routine frees all packets in PSQ that's destined to a specific DA.
	BCAST/MCAST in DTIMCount=0 case is also handled here, just like a PS-POLL 
	is received from a WSTA which has MAC address FF:FF:FF:FF:FF:FF
  ========================================================================
*/
VOID APHandleRxPsPoll(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pAddr,
	IN	USHORT			Aid,
    IN	BOOLEAN			isActive)
{ 
	PQUEUE_ENTRY	  pEntry;
	PMAC_TABLE_ENTRY  pMacEntry;
	unsigned long		IrqFlags;

	//DBGPRINT(RT_DEBUG_TRACE,("rcv PS-POLL (AID=%d) from %02x:%02x:%02x:%02x:%02x:%02x\n", 
	//	  Aid, pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));

	pMacEntry = &pAd->MacTab.Content[Aid];
	if (RTMPEqualMemory(pMacEntry->Addr, pAddr, MAC_ADDR_LEN))
	{
#ifdef UAPSD_AP_SUPPORT
        if (UAPSD_MR_IS_ALL_AC_UAPSD(isActive, pMacEntry))
            return; /* all AC are U-APSD, can not use PS-Poll */
        /* End of if */
#endif // UAPSD_AP_SUPPORT //

		//NdisAcquireSpinLock(&pAd->MacTabLock);
		//NdisAcquireSpinLock(&pAd->TxSwQueueLock);
        RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
        if (isActive == FALSE)
        {
			if (pMacEntry->PsQueue.Head)
			{
				pEntry = RemoveHeadQueue(&pMacEntry->PsQueue);
				if ( pMacEntry->PsQueue.Number >=1 )
					RTMP_SET_PACKET_MOREDATA(RTPKT_TO_OSPKT(pEntry), TRUE);
				InsertTailQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);				
			}
        }
        else
        {
#ifdef UAPSD_AP_SUPPORT
            UAPSD_AllPacketDeliver(pAd, pMacEntry);
#endif // UAPSD_AP_SUPPORT //

			while(pMacEntry->PsQueue.Head)
			{
//				if (pAd->TxSwQueue[QID_AC_BE].Number <=
//                    (pAd->PortCfg.TxQueueSize + (MAX_PACKETS_IN_PS_QUEUE>>1)))
                {
					pEntry = RemoveHeadQueue(&pMacEntry->PsQueue);
					InsertTailQueue(&pAd->TxSwQueue[QID_AC_BE], pEntry);
				}
//                else
//					break;
				/* End of if */
			} /* End of while */
        } /* End of if */

		//NdisReleaseSpinLock(&pAd->TxSwQueueLock);
		//NdisReleaseSpinLock(&pAd->MacTabLock);

		if ((Aid > 0) && (Aid < MAX_LEN_OF_MAC_TABLE) &&
			(pMacEntry->PsQueue.Number == 0))
		{
			// clear corresponding TIM bit because no any PS packet
			WLAN_MR_TIM_BIT_CLEAR(pAd, pMacEntry->apidx, Aid);
			pMacEntry->PsQIdleCount = 0;
		}

		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

		// Dequeue outgoing frames from TxSwQueue0..3 queue and process it
		// TODO: 2004-12-27 it's not a good idea to handle "More Data" bit here. because the
		// RTMPDeQueue process doesn't guarantee to de-queue the desired MSDU from the corresponding
		// TxSwQueue/PsQueue when QOS in-used. We should consider "HardTransmt" this MPDU
		// using MGMT queue or things like that.
		RTMPDeQueuePacket(pAd, FALSE, MAX_TX_PROCESS);

	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("rcv PS-POLL (AID=%d not match) from %02x:%02x:%02x:%02x:%02x:%02x\n", 
			  Aid, pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));

	}
}

NDIS_STATUS BASmartHardTransmit(
						  IN  PRTMP_ADAPTER		pAd,
						  IN  PNDIS_PACKET      pPacket,
						  IN  UCHAR             QueIdx,
						  OUT PULONG            pFreeTXDLeft)
{
	PACKET_INFO 	PacketInfo;
	PUCHAR 			pDMAHeaderBufVA;//, pDMAHeaderBufPtr;
	UINT   			MpduSize = 0;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	ULONG			SrcBufPA;	
	
	PHEADER_802_11	pHeader80211;
	PUCHAR			pExtraLlcSnapEncap; // NULL: no extra LLC/SNAP is required

	PTXD_STRUC		pTxD;	
#ifdef BIG_ENDIAN
    PTXD_STRUC      pDestTxD;
    TXD_STRUC       TxD;
#endif

	PTXWI_STRUC 	pCachedTxWI;

	//UCHAR			PID = 0;
	PRTMP_TX_RING	pTxRing;

	UCHAR			UserPriority;	
	UCHAR			RAWcid;
	PMAC_TABLE_ENTRY pMacEntry;

	ULONG			FreeTXDLeft = *pFreeTXDLeft;
	ULONG 			TxIdx;	

	HTTRANSMIT_SETTING	*pTxHTPhyMode;

	PUCHAR			pBuffer;
	UCHAR			DMAHeaderSize;
	PUCHAR			pCachedBufPtr;
	PUCHAR			OffsetSrcVA;
	ULONG			BufBasePaLow;
	BOOLEAN			bRejectSmartTx = FALSE;

	if ((pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
#ifdef CARRIER_DETECTION_SUPPORT
		||(isCarrierDetectExist(pAd) == TRUE)
#endif // CARRIER_DETECTION_SUPPORT //
		)
	{
		return NDIS_STATUS_FAILURE;
	}

	UserPriority = RTMP_GET_PACKET_UP(pPacket);	
	RAWcid       = RTMP_GET_PACKET_WCID(pPacket);


	pMacEntry = &pAd->MacTab.Content[RAWcid];

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);

	// reject EAPOL Frame
	if (NdisEqualMemory(EAPOL, pSrcBufVA + 12, 2))
		bRejectSmartTx = TRUE;

	// reject DHCP frame
	if (RTMP_GET_PACKET_DHCP(pPacket))
	{
		bRejectSmartTx = TRUE;
	}

	if (RTMP_GET_PACKET_MOREDATA(pPacket))
	{
		bRejectSmartTx = TRUE;
	}


	//
	//	Rule for packets to transmit with Fast Path
	// 
	//		1. Unicast packets
	// 		2. Non EAPOL, DHCP
	// 		3. Setup an BA session
	// 		4. use HT rate		
	// 

	if ((RAWcid == MCAST_WCID) || (pMacEntry->isCached == FALSE) || 
		(bRejectSmartTx) || 
		(pMacEntry->HTPhyMode.field.MODE < MODE_HTMIX) ||
		((pMacEntry->TXBAbitmap & 1<<(UserPriority)) == 0))
	{
		return NDIS_STATUS_FAILURE;
	}

	// Increase Total transmit byte counter
	pAd->RalinkCounters.TransmittedByteCount +=  SrcBufLen;			

	pMacEntry->DebugTxCount++;

	pCachedBufPtr = (PUCHAR) &pMacEntry->CachedBuf[0];
	pCachedTxWI = (PTXWI_STRUC) pCachedBufPtr;

	OffsetSrcVA		  = pSrcBufVA;
	OffsetSrcVA      += LENGTH_802_3;  // skip 802.3 header
	SrcBufLen        -= LENGTH_802_3;  // skip 802.3 header

	if (((pSrcBufVA[12] << 8) + pSrcBufVA[13]) == 0x8100)
	{
		/* skip VLAN tag */
		OffsetSrcVA += 4;
		SrcBufLen -= 4;
	}

#ifndef CONFIG_5VT_ENHANCE
	SrcBufPA          = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
									   SrcBufLen, PCI_DMA_TODEVICE);
#else
	if (RTMP_GET_PACKET_5VT(pPacket))
	{	
		SrcBufPA          = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
									   16, PCI_DMA_TODEVICE);
	}
	else
	{
		SrcBufPA          = PCI_MAP_SINGLE(pAd, (char *)OffsetSrcVA,
									   SrcBufLen, PCI_DMA_TODEVICE);
	}
#endif
	

	TxIdx = pAd->TxRing[QueIdx].TxCpuIdx;

	//
	// ACQUIRE TXD
	//
	
	FreeTXDLeft--;

	pTxRing = &pAd->TxRing[QueIdx];

#ifndef BIG_ENDIAN
	pTxD  = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;	
#else
	pDestTxD = (PTXD_STRUC) pTxRing->Cell[TxIdx].AllocVa;
	TxD = *pDestTxD;
	pTxD = &TxD;
#endif
	pDMAHeaderBufVA = (PUCHAR) pTxRing->Cell[TxIdx].DmaBuf.AllocVa;
	BufBasePaLow  = RTMP_GetPhysicalAddressLow(pTxRing->Cell[TxIdx].DmaBuf.AllocPa);

	NdisZeroMemory(pTxD, TXD_SIZE);

	//
	// Update 802.11 Header 
	// 

	pHeader80211 = (PHEADER_802_11)((PUCHAR)pCachedBufPtr + TXWI_SIZE);

	// More Bit
	pHeader80211->FC.MoreData = RTMP_GET_PACKET_MOREDATA(pPacket);	
	// SA 
#ifdef WDS_SUPPORT
	// Addr4 mean SA of WDS packet.
	if(pMacEntry->ValidAsWDS == TRUE)
	{ // WDS packet.
		COPY_MAC_ADDR(pHeader80211->Addr3, pSrcBufVA);
		COPY_MAC_ADDR(pHeader80211->Octet, pSrcBufVA + MAC_ADDR_LEN);
	}
	else
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
	// Addr4 mean SA of WDS packet.
	if(pMacEntry->ValidAsApCli == TRUE)
	{ // Ap-client packet.
		COPY_MAC_ADDR(pHeader80211->Addr3, pSrcBufVA);		
	}
	else
#endif // APCLI_SUPPORT //
	{
		COPY_MAC_ADDR(pHeader80211->Addr3, pSrcBufVA + MAC_ADDR_LEN);
	}
	// Sequence
	pHeader80211->Sequence = pMacEntry->TxSeq[UserPriority];
	pMacEntry->TxSeq[UserPriority] = (pMacEntry->TxSeq[UserPriority]+1) & MAXSEQ;

	// QOS 
#ifdef WDS_SUPPORT
	// Qos field following Address4 for WDS packet.
	if(pMacEntry->ValidAsWDS == TRUE) // WDS packet.
	{
		pHeader80211->Octet[6] = (UserPriority & 0x0f);
	}
	else
#endif
	{
		pHeader80211->Octet[0] = (UserPriority & 0x0f);
	}

	// 2 bytes QoS field
#ifdef WDS_SUPPORT
	/* the length of A WDS packet is 36 bytes
	** (24 bytes 802_11 head + 6 bytes Address4 + 2 bytes Qos).
	** So it doesn't need to padding for WDS packet.
	*/
	if (pMacEntry->ValidAsWDS == TRUE)
	{
		MpduSize += sizeof(HEADER_802_11) + 6 + 2;
	}
	else
#endif // WDS_SUPPORT //
	{
		MpduSize += sizeof(HEADER_802_11) + 2;
	}

	
	// MpduSize -- total 802.11 header len
	if ((pAd->CommonCfg.bRdg == TRUE) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
	{
		MpduSize += 4;
	}
	
	// TXWI_SIZE + 802.11 header size + 2 bytes ASIC padding
	DMAHeaderSize = TXWI_SIZE + MpduSize;
	DMAHeaderSize = (DMAHeaderSize + 3) & ~3;

	// MpduSize -- whole 802.11 frame
	MpduSize += SrcBufLen;

	//
	// if orginal Ethernet frame contains no LLC/SNAP, 
	// then an extra LLC/SNAP encap is required 
	// 
	EXTRA_LLCSNAP_ENCAP(pSrcBufVA, pExtraLlcSnapEncap);

	if (pExtraLlcSnapEncap)
	{
		pBuffer = pCachedBufPtr + DMAHeaderSize;
		// Insert LLC-SNAP encapsulation
		NdisMoveMemory(pBuffer, pExtraLlcSnapEncap, 6);
		pBuffer += 6;
		if (((pSrcBufVA[12] << 8) + pSrcBufVA[13]) == 0x8100)
			NdisMoveMemory(pBuffer, pSrcBufVA + 12+4, 2); /* skip VLAN tag */
		else
			NdisMoveMemory(pBuffer, pSrcBufVA + 12, 2);
		pBuffer += 2;
		MpduSize += LENGTH_802_1_H;
		DMAHeaderSize += LENGTH_802_1_H;
	}


	//
	// update TXWI
	// 
	pTxHTPhyMode = &pMacEntry->HTPhyMode;

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
	{				
		pCachedTxWI->txop = IFS_HTTXOP;
		
		// If CCK or OFDM, BW must be 20
		pCachedTxWI->BW = (pTxHTPhyMode->field.MODE <= MODE_OFDM) ? (BW_20) : (pTxHTPhyMode->field.BW);
		pCachedTxWI->ShortGI = pTxHTPhyMode->field.ShortGI;
		pCachedTxWI->STBC = pTxHTPhyMode->field.STBC;

		pCachedTxWI->MCS = pTxHTPhyMode->field.MCS;
		pCachedTxWI->PHYMODE = pTxHTPhyMode->field.MODE;

		// set PID for TxRateSwitching
		pCachedTxWI->PacketId = pTxHTPhyMode->field.MCS;
	}

	if (pMacEntry->NoBADataCountDown == 0)
	{
		pCachedTxWI->AMPDU = TRUE;

		// increment the transmitted AMPDU packet count and data ByteCount.
		pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart ++;
		pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.QuadPart += MpduSize;	
	}
	else
	{
		pCachedTxWI->AMPDU = FALSE;
	}

    pCachedTxWI->MIMOps = 0;

	if (pMacEntry->bIAmBadAtheros && (pMacEntry->WepStatus != Ndis802_11WEPDisabled))
	{
		pCachedTxWI->MpduDensity = 7;
	}


    if (pAd->CommonCfg.bMIMOPSEnable)
    {
	// MIMO Power Save Mode
		if ((pMacEntry->MmpsMode == MMPS_DYNAMIC) && (pTxHTPhyMode->field.MCS > 7))
	{
		// Dynamic MIMO Power Save Mode
		pCachedTxWI->MIMOps = 1;
	} 
		else if (pMacEntry->MmpsMode == MMPS_STATIC)
	{
		// Static MIMO Power Save Mode
		if ((pTxHTPhyMode->field.MODE >= MODE_HTMIX) && (pTxHTPhyMode->field.MCS > 7))
		{			
			pCachedTxWI->MCS = 7;
			pCachedTxWI->MIMOps = 0;
		}
	}
    }
	//printk("pCachedTxWI->MCS = %d\n",  pCachedTxWI->MCS);
	
	pCachedTxWI->MPDUtotalByteCount = MpduSize;

	// copy TXWI + 802.11 header into the first DMA Header bufer
	NdisMoveMemory(pDMAHeaderBufVA, pCachedBufPtr, DMAHeaderSize);
#ifdef BIG_ENDIAN
	RTMPWIEndianChange((PUCHAR)pDMAHeaderBufVA, TYPE_TXWI);
	RTMPFrameEndianChange(pAd, (PUCHAR)(pDMAHeaderBufVA+ TXWI_SIZE), DIR_WRITE, FALSE);
#endif // BIG_ENDIAN //
	//
	// set up TX Descriptor
	// 
	pTxD->SDPtr0 = BufBasePaLow;
	pTxD->SDLen0 = (ULONG) DMAHeaderSize;
	pTxD->LastSec0 = 0;
	pTxD->SDPtr1 = SrcBufPA;
	pTxD->SDLen1 = SrcBufLen; 
	pTxD->LastSec1 = 1;

	RTMPWriteTxDescriptor(pAd, pTxD, FALSE, FIFO_EDCA);
#ifdef BIG_ENDIAN
	RTMPDescriptorEndianChange((PUCHAR)pTxD, TYPE_TXD);
    WriteBackToDescriptor((PUCHAR)pDestTxD, (PUCHAR)pTxD, FALSE, TYPE_TXD);	
#endif // BIG_ENDIAN //
	
	// keep pPacket for TX DMA Done
	pTxRing->Cell[TxIdx].pNdisPacket = pPacket;
	pTxRing->Cell[TxIdx].pNextNdisPacket = NULL;


	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;

	// Update free TX Desc Count
	*pFreeTXDLeft = FreeTXDLeft;

	//
	// Kick TX
	// 
	INC_RING_INDEX(TxIdx, TX_RING_SIZE);
	pAd->TxRing[QueIdx].TxCpuIdx = TxIdx;
	RTMP_IO_WRITE32(pAd, TX_CTX_IDX0 + QueIdx * 0x10 , pAd->TxRing[QueIdx].TxCpuIdx);

	return(NDIS_STATUS_SUCCESS);

}


UCHAR VLAN_8023_Header_Copy(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			pHeader802_3,
	IN	UINT            HdrLen,
	OUT PUCHAR			pData,
	IN	UCHAR			FromWhichBSSID)
{
	extern UCHAR TPID[];
	UINT16 TCI;
	UCHAR VLAN_Size = 0;


	if ((FromWhichBSSID < pAd->ApCfg.BssidNum) && (pAd->ApCfg.MBSSID[FromWhichBSSID].VLAN_VID != 0))
	{
		MULTISSID_STRUCT *mbss_p = &pAd->ApCfg.MBSSID[FromWhichBSSID];

		/* need to insert VLAN tag */
		VLAN_Size = LENGTH_802_1Q;

		/* make up TCI field */
#ifdef BIG_ENDIAN
		TCI = (mbss_p->VLAN_Priority << 13) + mbss_p->VLAN_VID;
#else
		TCI = (mbss_p->VLAN_VID << 8) + (mbss_p->VLAN_Priority << 5);
#endif

		/* copy dst + src MAC (12B) */
		memcpy(pData, pHeader802_3, LENGTH_802_3_NO_TYPE);

		/* copy VLAN tag (4B) */
		/* do NOT use memcpy to speed up */
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE) = *(UINT16 *)TPID;
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE+2) = TCI;

		/* copy type/len (2B) */
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE+LENGTH_802_1Q) = \
				*(UINT16 *)&pHeader802_3[LENGTH_802_3-LENGTH_802_3_TYPE];

		/* copy tail if exist */
		if (HdrLen >= LENGTH_802_3)
		{
			memcpy(pData+LENGTH_802_3+LENGTH_802_1Q,
					pHeader802_3+LENGTH_802_3,
					HdrLen - LENGTH_802_3);
		} /* End of if */
	}
	else
	{
		/* no VLAN tag is needed to insert */
		memcpy(pData, pHeader802_3, HdrLen);
	} /* End of if */

	return VLAN_Size;
} /* End of VLAN_Tag_Insert */

//
// detect AC Category of trasmitting packets
// to turn AC0(BE) TX_OP (MAC reg 0x1300)
// 
static UCHAR is_on;
VOID detect_wmm_traffic(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UCHAR			UserPriority)
{
	// count packets which priority is more than BE 
	if (UserPriority > 3)
	{
		pAd->OneSecondnonBEpackets++;

		if (pAd->MacTab.fAnyStationMIMOPSDynamic && pAd->OneSecondnonBEpackets > 100)
		{
			if (!is_on)
			{
				RTMP_IO_WRITE32(pAd,  EXP_ACK_TIME,	 0x005400ca );
				is_on = 1;
			}
		}
		else
		{
			if (is_on)
			{
				RTMP_IO_WRITE32(pAd,  EXP_ACK_TIME,	 0x002400ca );
				is_on = 0;
			}
		}		
	}
}

//
// Wirte non-zero value to AC0 TXOP to boost performace
// To pass WMM, AC0 TXOP must be zero.
// It is necessary to turn AC0 TX_OP dynamically.
// 

VOID dynamic_tune_be_tx_op(
						  IN  PRTMP_ADAPTER   pAd,
						  IN  ULONG           nonBEpackets)
{
	ULONG RegValue;
	AC_TXOP_CSR0_STRUC csr0;

	if (pAd->CommonCfg.bEnableTxBurst || pAd->CommonCfg.bRdg)
	{

		if ((pAd->WIFItestbed.bGreenField && pAd->MacTab.fAnyStationNonGF == TRUE) ||
			((pAd->OneSecondnonBEpackets > nonBEpackets) || pAd->MacTab.fAnyStationMIMOPSDynamic) || 
			(pAd->MacTab.fAnyTxOPForceDisable))
		{
			if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE))
			{
				RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &RegValue);
				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE))
				{
					TX_LINK_CFG_STRUC   TxLinkCfg;

					RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
					TxLinkCfg.field.TxRDGEn = 0;
					RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);

					RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
				}
				// disable AC0(BE) TX_OP
				RegValue  &= 0xFFFFFF00; // for WMM test
				//if ((RegValue & 0x0000FF00) == 0x00004300)
				//	RegValue += 0x00001100;
				RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, RegValue);
				if (pAd->CommonCfg.APEdcaParm.Txop[QID_AC_VO] != 102)
				{
					csr0.field.Ac0Txop = 0;		// QID_AC_BE
				}
				else
				{	
					// for legacy b mode STA
					csr0.field.Ac0Txop = 10;		// QID_AC_BE
				}
				csr0.field.Ac1Txop = 0;		// QID_AC_BK
				RTMP_IO_WRITE32(pAd, WMM_TXOP0_CFG, csr0.word);
				RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE);				
			}
		}
		else
		{
			if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE)==0)
			{
				// enable AC0(BE) TX_OP
				UCHAR   txop_value;

				RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &RegValue);
				RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE) ? (txop_value = 0x80) : (txop_value = 0x30);
				RegValue  &= 0xFFFFFF00;
				//if ((RegValue & 0x0000FF00) == 0x00005400)
				//	RegValue -= 0x00001100;
				//txop_value = 0;
				RegValue  |= txop_value;  // for performance, set the TXOP to non-zero			
				RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, RegValue);
				csr0.field.Ac0Txop = txop_value;	// QID_AC_BE
				csr0.field.Ac1Txop = 0;				// QID_AC_BK
				RTMP_IO_WRITE32(pAd, WMM_TXOP0_CFG, csr0.word);
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE);				
			}
		}
	}
	pAd->OneSecondnonBEpackets = 0;
}


VOID APRxDErrorHandle(	
	IN	PRTMP_ADAPTER	pAd, 
	IN	RX_BLK			*pRxBlk)
{
	MAC_TABLE_ENTRY		*pEntry;
	PRXD_STRUC			pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC			pRxWI = pRxBlk->pRxWI;


	if (pRxD->U2M && pRxD->CipherErr)
	{		
		if (pRxWI->WirelessCliID < MAX_LEN_OF_MAC_TABLE)
		{
			pEntry = &pAd->MacTab.Content[pRxWI->WirelessCliID];

			// MIC error
			// Before verifying the MIC, the receiver shall check FCS, ICV and TSC. 
			// This avoids unnecessary MIC failure events. 
			if ((pAd->MacTab.Content[pRxWI->WirelessCliID].WepStatus == Ndis802_11Encryption2Enabled)
					&& (pRxD->CipherErr == 2))
			{
      			if (pEntry)
      			{
      				HandleCounterMeasure(pAd, pEntry);
      			}

			}

			// send wireless event - for icv error
			if (pEntry && ((pRxD->CipherErr & 1) == 1) && pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_ICV_ERROR_EVENT_FLAG, pEntry); 
		}
		DBGPRINT(RT_DEBUG_TRACE, ("Rx u2me DATA Cipher Err(SDL0=%d, MPDUsize=%d, WCID=%d, CipherErr=%d)\n", pRxD->SDL0, pRxWI->MPDUtotalByteCount, pRxWI->WirelessCliID, pRxD->CipherErr));
	}

	pAd->Counters8023.RxErrors++;
	DBGPRINT(RT_DEBUG_TRACE, ("APCheckRxError\n"));

}


BOOLEAN APCheckVaildDataFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	PRXWI_STRUC		pRxWI = pRxBlk->pRxWI;

	BOOLEAN isVaild = FALSE;

	do
	{
#ifndef APCLI_SUPPORT
		// should not drop Ap-Client packet.
		if (pHeader->FC.ToDs == 0)
			break; // give up this frame
#endif // APCLI_SUPPORT //
	
		// check if Class2 or 3 error
		if ((pHeader->FC.FrDs == 0) && (APCheckClass2Class3Error(pAd, pRxWI->WirelessCliID, pHeader))) 
			break; // give up this frame
	
		if(pAd->ApCfg.BANClass3Data == TRUE)
			break; // give up this frame

		isVaild = TRUE;
	} while (0);

	return isVaild;
}

#ifdef WDS_SUPPORT
MAC_TABLE_ENTRY *FindWdsEntry(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR 			Wcid,
	IN PUCHAR			pAddr,
	IN UINT32			PhyMode)
{
	MAC_TABLE_ENTRY *pEntry;

	// lookup the match wds entry for the incoming packet.
	pEntry = WdsTableLookupByWcid(pAd, Wcid, pAddr, TRUE);

	// Only Lazy mode will auto learning, match with FrDs=1 and ToDs=1
	if((pEntry == NULL) && (pAd->WdsTab.Mode >= WDS_LAZY_MODE))
	{
		LONG WdsIdx = WdsEntryAlloc(pAd, pAddr);
		if (WdsIdx >= 0)
		{
			// user doesn't specific a phy mode for WDS link.
			if (pAd->WdsTab.WdsEntry[WdsIdx].PhyMode == 0xff)
				pAd->WdsTab.WdsEntry[WdsIdx].PhyMode = PhyMode;
			pEntry = MacTableInsertWDSEntry(pAd, pAddr, (UCHAR)WdsIdx);
		}
		else 
			pEntry = NULL;
	}

	return pEntry;
}
#endif // WDS_SUPPORT //


// For TKIP frame, calculate the MIC value						
BOOLEAN APCheckTkipMICValue(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk)
{
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	UCHAR			*pData = pRxBlk->pData;
	USHORT			DataSize = pRxBlk->DataSize;
	PCIPHER_KEY		pWpaKey;
	UCHAR			*pDA, *pSA;

	pWpaKey = &pEntry->PairwiseKey;

	// Minus MIC length
	DataSize -= 8;

	if (!RX_BLK_TEST_FLAG(pRxBlk, fRX_WDS))
	{
		pDA = pHeader->Addr3;
		pSA = pHeader->Addr2;
	}
	else 
	{
		pDA = pHeader->Addr3;
		pSA = (PUCHAR)pHeader + sizeof(HEADER_802_11);
	}

	if (RTMPTkipCompareMICValue(pAd,
								pData,
								pDA,
								pSA,
								pWpaKey->RxMic,
								DataSize) == FALSE)
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR,("Rx MIC Value error 2\n"));
		HandleCounterMeasure(pAd, pEntry);
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
		return FALSE;
	}

	return TRUE;
}

VOID APHandleRxMgmtFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PRXD_STRUC		pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC		pRxWI = pRxBlk->pRxWI;
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	PNDIS_PACKET	pRxPacket = pRxBlk->pRxPacket;

	do
	{
		if (!pRxD->U2M)
		{
			if ((pHeader->FC.SubType != SUBTYPE_BEACON) && (pHeader->FC.SubType != SUBTYPE_PROBE_REQ))
			{
				// give up this frame
				break;
			}
		}
	
		if (pAd->ApCfg.BANClass3Data == TRUE)
		{
			// disallow new association
			if ((pHeader->FC.SubType == SUBTYPE_ASSOC_REQ) || (pHeader->FC.SubType == SUBTYPE_AUTH))
			{
				DBGPRINT(RT_DEBUG_TRACE, ("   Disallow new Association \n "));
				// give up this frame
				break;
			}
		}

		if (pRxBlk->DataSize > 1520)
		{
			printk("DataSize  = %d\n", pRxBlk->DataSize);
			hex_dump("MGMT ???", (UCHAR *)pHeader, pRxBlk->pData - (UCHAR *) pHeader);
			break;;
		}

		REPORT_MGMT_FRAME_TO_MLME(pAd, pRxWI->WirelessCliID, pHeader, pRxWI->MPDUtotalByteCount, 
								  pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2, pRxD->PlcpSignal);
	} while (0);

	RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_SUCCESS);
	return;
}

VOID APHandleRxControlFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PRXWI_STRUC		pRxWI = pRxBlk->pRxWI;
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	PNDIS_PACKET	pRxPacket = pRxBlk->pRxPacket;

	switch (pHeader->FC.SubType)
	{
		case SUBTYPE_BLOCK_ACK_REQ:
			{
				CntlEnqueueForRecv(pAd, pRxWI->WirelessCliID, (pRxWI->MPDUtotalByteCount), (PFRAME_BA_REQ)pHeader);
			}
			break;
		// handle PS-POLL here
		case SUBTYPE_PS_POLL:
			{
				USHORT Aid = pHeader->Duration & 0x3fff;
				PUCHAR pAddr = pHeader->Addr2;

				if (Aid < MAX_LEN_OF_MAC_TABLE)
					APHandleRxPsPoll(pAd, pAddr, Aid, FALSE);
			}
			break;
		case SUBTYPE_BLOCK_ACK:
		case SUBTYPE_ACK:
		default:		
			DBGPRINT(RT_DEBUG_INFO,("ignore CNTL (subtype=%d)\n", pHeader->FC.SubType));
			break;
	}

	RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
	return;
}

VOID APRxEAPOLFrameIndicate(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
{
	PRXD_STRUC		pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC		pRxWI = pRxBlk->pRxWI;
	BOOLEAN 		CheckPktSanity = TRUE;
	UCHAR			*pTmpBuf;

	// Sanity Check		
	if(pRxBlk->DataSize < (LENGTH_802_1_H + LENGTH_EAPOL_H))
	{
		CheckPktSanity = FALSE;
		DBGPRINT(RT_DEBUG_ERROR, ("Total pkts size is too small.\n"));
	}	
	else if (!RTMPEqualMemory(SNAP_802_1H, pRxBlk->pData, 6))
	{
		CheckPktSanity = FALSE;	
		DBGPRINT(RT_DEBUG_ERROR, ("Can't find SNAP_802_1H parameter.\n"));
	}	 
	else if (!RTMPEqualMemory(EAPOL, pRxBlk->pData+6, 2))
	{
		CheckPktSanity = FALSE;	
		DBGPRINT(RT_DEBUG_ERROR, ("Can't find EAPOL parameter.\n"));	
	}	
	else if(*(pRxBlk->pData+9) > EAPOLASFAlert)
	{
		CheckPktSanity = FALSE;	
		DBGPRINT(RT_DEBUG_ERROR, ("Unknown EAP type(%d).\n", *(pRxBlk->pData+9)));	
	}

	if(CheckPktSanity == FALSE)
	{
		goto done;
	}

	// sent this frame to upper layer TCPIP
	if ((pEntry) && (pEntry->WpaState < AS_INITPMK) && 
		((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2) || pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))
	{
#ifdef WSC_AP_SUPPORT                                
		if ((pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE) &&
            (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx != WSC_INIT_ENTRY_APIDX))
		{
			pTmpBuf = pRxBlk->pData - LENGTH_802_11;
			NdisMoveMemory(pTmpBuf, pRxBlk->pHeader, LENGTH_802_11);
			REPORT_MGMT_FRAME_TO_MLME(pAd, pRxWI->WirelessCliID, pTmpBuf, pRxBlk->DataSize + LENGTH_802_11, pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2, pRxD->PlcpSignal);
            pRxBlk->pHeader = (PHEADER_802_11)pTmpBuf;
		}       
#endif // WSC_AP_SUPPORT //

		Indicate_Legacy_Packet(pAd, pRxBlk, FromWhichBSSID);
		return; 
	}
	else	// sent this frame to WPA state machine
	{
		pTmpBuf = pRxBlk->pData - LENGTH_802_11;
		NdisMoveMemory(pTmpBuf, pRxBlk->pHeader, LENGTH_802_11);
		REPORT_MGMT_FRAME_TO_MLME(pAd, pRxWI->WirelessCliID, pTmpBuf, pRxBlk->DataSize + LENGTH_802_11, pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2, pRxD->PlcpSignal);
	}

done:
	RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
	return;

}

VOID Announce_or_Forward_802_3_Packet(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			FromWhichBSSID)
{
	if (APFowardWirelessStaToWirelessSta(pAd, pPacket, FromWhichBSSID))
	{
		announce_802_3_packet(pAd, pPacket);
	}
	else
	{
		// release packet
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
	}
}


VOID APRxDataFrameAnnounce(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk,
	IN	UCHAR			FromWhichBSSID)
{

	// non-EAP frame
	if (!RTMPCheckWPAframe(pAd, pRxBlk->pData, pRxBlk->DataSize))
	{
		// drop all non-EAP DATA frame before
		// this client's Port-Access-Control is secured
		if (pEntry->PrivacyFilter == Ndis802_11PrivFilter8021xWEP)
		{
			// release packet
			RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_FAILURE);
			return;
		}

#ifdef IGMP_SNOOP_SUPPORT
		if (pEntry
			&& (pEntry->ValidAsCLI == TRUE)
			&& (pAd->ApCfg.MBSSID[FromWhichBSSID].IgmpSnoopEnable) 
			&& IS_MULTICAST_MAC_ADDR(pRxBlk->pHeader->Addr3))
		{
			PUCHAR pDA = pRxBlk->pHeader->Addr3;
			PUCHAR pSA = pRxBlk->pHeader->Addr2;
			PUCHAR pData = NdisEqualMemory(SNAP_802_1H, pRxBlk->pData, 6) ? (pRxBlk->pData + 6) : pRxBlk->pData;

			IGMPSnooping(pAd, pDA, pSA,	pData,
				pAd->ApCfg.MBSSID[pEntry->apidx].MSSIDDev);
		}
#endif // IGMP_SNOOP_SUPPORT //

		if (!RX_BLK_TEST_FLAG(pRxBlk, fRX_ARALINK))
		{
			// Normal legacy, AMPDU or AMSDU
			CmmRxnonRalinkFrameIndicate(pAd, pEntry, pRxBlk, FromWhichBSSID);
			
		}
		else
		{
			// ARALINK
			CmmRxRalinkFrameIndicate(pAd, pEntry, pRxBlk, FromWhichBSSID);
		}
	}
	else 
	{
		// Determin the destination of the EAP frame
		//  to WPA state machine or upper layer
		APRxEAPOLFrameIndicate(pAd, pEntry, pRxBlk, FromWhichBSSID);
	}
}



//
// All Rx routines use RX_BLK structure to hande rx events
// It is very important to build pRxBlk attributes
//  1. pHeader pointer to 802.11 Header
//  2. pData pointer to payload including LLC (just skip Header)
//  3. set payload size including LLC to DataSize
//  4. set some flags with RX_BLK_SET_FLAG()
// 
VOID APHandleRxDataFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PRXD_STRUC						pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC						pRxWI = pRxBlk->pRxWI;
	PHEADER_802_11					pHeader = pRxBlk->pHeader;
	PNDIS_PACKET					pRxPacket = pRxBlk->pRxPacket;
	BOOLEAN 						bFragment = FALSE;
	BOOLEAN							bWdsPacket = FALSE;
	MAC_TABLE_ENTRY	    			*pEntry = NULL;
	UCHAR							FromWhichBSSID = BSS0;
	UCHAR							OldPwrMgmt = PWR_ACTIVE;	// UAPSD AP SUPPORT



	if (APCheckVaildDataFrame(pAd, pRxBlk) != TRUE)
	{
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}

	//
	// handle WDS
	//
	if ((pHeader->FC.FrDs == 1) && (pHeader->FC.ToDs == 1))
	{
#ifdef WDS_SUPPORT
		bWdsPacket = TRUE;
		pEntry = FindWdsEntry(pAd, pRxWI->WirelessCliID, pHeader->Addr2, pRxWI->PHYMODE);


		// have no valid wds entry exist 
		// then discard the incoming packet.
		if (!(pEntry && WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx)))
		{
			// release packet
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
			return;
		}

		RX_BLK_SET_FLAG(pRxBlk, fRX_WDS);
		FromWhichBSSID = pEntry->MatchWDSTabIdx + MIN_NET_DEVICE_FOR_WDS;

#else	
		// no WDS support
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
#endif // WDS_SUPPORT //
	}
	// handle APCLI.
	else if ((pHeader->FC.FrDs == 1) && (pHeader->FC.ToDs == 0))
	{
#ifdef APCLI_SUPPORT
		pEntry = ApCliTableLookUpByWcid(pAd, pRxWI->WirelessCliID, pHeader->Addr2);

		if (!(pEntry && APCLI_IF_UP_CHECK(pAd, pEntry->MatchAPCLITabIdx)))
		{
			// release packet
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
			return;
		}
		FromWhichBSSID = pEntry->MatchAPCLITabIdx + MIN_NET_DEVICE_FOR_APCLI;

		// Process broadcast packets
		if (pRxD->U2M == 0)
		{
			// Process the received broadcast frame for AP-Client.			
			if (!ApCliHandleRxBroadcastFrame(pAd, pRxBlk, pEntry, FromWhichBSSID))			
			{
				// release packet
				RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
			}
			return;
		}
#else
		// no APCLI support
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
#endif // APCLI_SUPPORT //
	}
	else
	{
		pEntry = PACInquiry(pAd, pRxWI->WirelessCliID);

		//	can't find associated STA entry then filter invlid data frame 
		if (!pEntry)
		{		
			// release packet
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
			return;
		}

		FromWhichBSSID = pEntry->apidx;
	}

	ASSERT(pEntry->Aid == pRxWI->WirelessCliID);


	// check Atheros Client
	if (!pEntry->bIAmBadAtheros && (pRxD->AMPDU == 1) && (pHeader->FC.Retry ) && (pAd->CommonCfg.bHTProtect == TRUE))
	{
		if (pAd->CommonCfg.IOTestParm.bRTSLongProtOn == FALSE)
		AsicUpdateProtect(pAd, 8, ALLN_SETPROTECT, FALSE, FALSE);
		DBGPRINT(RT_DEBUG_INFO, ("Atheros Problem. Turn on RTS/CTS!!!\n"));
		pEntry->bIAmBadAtheros = TRUE;
	}

   	// update rssi sample 
   	Update_Rssi_Sample(pAd, &pEntry->RssiSample, pRxWI);

   	DBGPRINT(RT_DEBUG_INFO, ("Rcv packet to IF(ra%d)\n", FromWhichBSSID));					


   	// Gather PowerSave information from all valid DATA frames. IEEE 802.11/1999 p.461
   	// must be here, before no DATA check 


	pRxBlk->pData = (UCHAR *)pHeader;


   	// 1: PWR_SAVE, 0: PWR_ACTIVE
   	OldPwrMgmt = APPsIndicate(pAd, pHeader->Addr2, pEntry->Aid, pHeader->FC.PwrMgmt);
#ifdef UAPSD_AP_SUPPORT
   	if (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_APSD_CAPABLE))
   	{
   		UCHAR  OldUP;

		OldUP = (*(pRxBlk->pData+LENGTH_802_11) & 0x07);
    	if (pHeader->FC.PwrMgmt && (OldPwrMgmt == PWR_SAVE))
    		UAPSD_TriggerFrameHandle(pAd, pEntry, OldUP);
    	/* End of if */
    } /* End of if */
#endif // UAPSD_AP_SUPPORT //

	// Drop NULL, CF-ACK(no data), CF-POLL(no data), and CF-ACK+CF-POLL(no data) data frame
	if ((pHeader->FC.SubType & 0x04) && (pHeader->FC.Order == 0)) // bit 2 : no DATA
	{
		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}

	//
	// update RxBlk->pData, DataSize
	// 802.11 Header, QOS, HTC, Hw Padding
	// 

	// 1. skip 802.11 HEADER
	if (bWdsPacket)
	{
		pRxBlk->pData += LENGTH_802_11_WITH_ADDR4;
		pRxBlk->DataSize -= LENGTH_802_11_WITH_ADDR4;
	}
	else
	{
		pRxBlk->pData += LENGTH_802_11;
		pRxBlk->DataSize -= LENGTH_802_11;
	}

	// 2. QOS
	if (pHeader->FC.SubType & 0x08)
	{
		RX_BLK_SET_FLAG(pRxBlk, fRX_QOS);
		// count packets priroity nmore than BE
		detect_wmm_traffic(pAd, *(pRxBlk->pData) & 0x0f);
		// bit 7 in QoS Control field signals the HT A-MSDU format
		if ((*pRxBlk->pData) & 0x80)
		{
			RX_BLK_SET_FLAG(pRxBlk, fRX_AMSDU);

			// calculate received AMSDU count and ByteCount
			pAd->RalinkCounters.ReceivedAMSDUCount.u.LowPart ++;

			if (bWdsPacket)
				pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart += (pRxBlk->DataSize + LENGTH_802_11_WITH_ADDR4);
			else
				pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart += (pRxBlk->DataSize + LENGTH_802_11);
		}

		// skip QOS contorl field
		pRxBlk->pData += 2;
		pRxBlk->DataSize -=2;
	}

	// 3. Order bit: A-Ralink or HTC+
	if (pHeader->FC.Order)
	{
#ifdef AGGREGATION_SUPPORT
		if ((pRxWI->PHYMODE < MODE_HTMIX) && (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE)))
		{
			RX_BLK_SET_FLAG(pRxBlk, fRX_ARALINK);
		}
		else
#endif
		{
			RX_BLK_SET_FLAG(pRxBlk, fRX_HTC);
			// skip HTC contorl field
			pRxBlk->pData += 4;
			pRxBlk->DataSize -= 4;
		}
	}

	// 4. skip HW padding 
	if (pRxD->L2PAD)
	{
		// just move pData pointer 
		// because DataSize excluding HW padding 
		RX_BLK_SET_FLAG(pRxBlk, fRX_PAD);
		pRxBlk->pData += 2;
	}

	if (pRxD->BA)
	{
		RX_BLK_SET_FLAG(pRxBlk, fRX_AMPDU);

		// incremented by the number of MPDUs
		// received in the A-MPDU when an A-MPDU is received.
		pAd->RalinkCounters.MPDUInReceivedAMPDUCount.u.LowPart ++;
	}

	if (!((pHeader->Frag == 0) && (pHeader->FC.MoreFrag == 0)))
	{
		// re-assemble the fragmented packets
		// return complete frame (pRxPacket) or NULL   
		bFragment = TRUE;
		pRxPacket = RTMPDeFragmentDataFrame(pAd, pRxBlk);
	}

	if (pRxPacket)
	{
		// process complete frame
		if (bFragment && (pHeader->FC.Wep) && (pEntry->WepStatus == Ndis802_11Encryption2Enabled))
		{
			// For TKIP frame, calculate the MIC value						
			if (APCheckTkipMICValue(pAd, pEntry, pRxBlk) == FALSE)
			{
				return;
			}
		}

		APRxDataFrameAnnounce(pAd, pEntry, pRxBlk, FromWhichBSSID);
	}
	else
	{
		// just return 
		// because RTMPDeFragmentDataFrame() will release rx packet, 
		// if packet is fragmented
		return;
	}
}

/*
		========================================================================
		Routine Description:
			Process RxDone interrupt, running in DPC level

		Arguments:
			pAd    Pointer to our adapter

		Return Value:
			None

		Note:
			This routine has to maintain Rx ring read pointer.
	========================================================================
*/


#undef	MAX_RX_PROCESS_CNT
#define MAX_RX_PROCESS_CNT	(32)

BOOLEAN APRxDoneInterruptHandle(
	IN	PRTMP_ADAPTER	pAd) 
{
	UINT32			RxProcessed, RxPending;
	BOOLEAN			bReschedule = FALSE;
	RXD_STRUC		*pRxD;
	UCHAR			*pData;
	PRXWI_STRUC		pRxWI;
	PNDIS_PACKET	pRxPacket;
	PHEADER_802_11	pHeader;
	RX_BLK			RxCell;


	RxProcessed = RxPending = 0;

	// process whole rx ring
	while (1)
	{

		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF |
								fRTMP_ADAPTER_RESET_IN_PROGRESS |
								fRTMP_ADAPTER_HALT_IN_PROGRESS) || 
			!RTMP_TEST_FLAG(pAd,fRTMP_ADAPTER_START_UP))
		{
			break;
		}

		if (RxProcessed++ > MAX_RX_PROCESS_CNT)
		{
			// need to reschedule rx handle 
			bReschedule = TRUE;
			break;
		}

		// static rate also need NICUpdateFifoStaCounters() function.
		//if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
			NICUpdateFifoStaCounters(pAd);
		
		// 1. allocate a new data packet into rx ring to replace received packet 
		//    then processing the received packet
		// 2. the callee must take charge of release of packet
		// 3. As far as driver is concerned ,
		//    the rx packet must 
		//      a. be indicated to upper layer or 
		//      b. be released if it is discarded
		pRxPacket = GetPacketFromRxRing(pAd, &(RxCell.RxD), &bReschedule, &RxPending);
		if (pRxPacket == NULL)
		{
			// no more packet to process
			break;
		}

		// get rx ring descriptor
		pRxD = &(RxCell.RxD);
		// get rx data buffer
		pData	= GET_OS_PKT_DATAPTR(pRxPacket);
		pRxWI	= (PRXWI_STRUC) pData;
		pHeader = (PHEADER_802_11) (pData+RXWI_SIZE) ;

#ifdef BIG_ENDIAN
		RTMPFrameEndianChange(pAd, (PUCHAR)pHeader, DIR_READ, TRUE);
		RTMPWIEndianChange((PUCHAR)pRxWI, TYPE_RXWI);
#endif
		// build RxCell
		RxCell.pRxWI = pRxWI;
		RxCell.pHeader = pHeader;
		RxCell.pRxPacket = pRxPacket;
		RxCell.pData = (UCHAR *) pHeader;
		RxCell.DataSize = pRxWI->MPDUtotalByteCount;
		RxCell.Flags = 0;

		// Increase Total receive byte counter after real data received no mater any error or not
		pAd->RalinkCounters.ReceivedByteCount +=  pRxWI->MPDUtotalByteCount;
		pAd->RalinkCounters.RxCount ++;
		pAd->RalinkCounters.OneSecRxCount ++;

		INC_COUNTER64(pAd->WlanCounters.ReceivedFragmentCount);

#ifdef RALINK_ATE
		if (pAd->ate.Mode != ATE_STOP)
		{
			pAd->ate.RxCntPerSec++;
			ATESampleRssi(pAd, pRxWI);
#ifdef RALINK_2860_QA
			if (pAd->ate.bQARxStart == TRUE)
			{
				ATE_QA_Statistics(pAd, pRxWI, pRxD,	pHeader);
			}
#endif // RALINK_2860_QA //
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_SUCCESS);
			continue;
		}
#endif // RALINK_ATE //
		
		// Check for all RxD errors
		if (APCheckRxError(pAd, pRxD, pRxWI->WirelessCliID) != NDIS_STATUS_SUCCESS)
		{
			APRxDErrorHandle(pAd, &RxCell);
			// discard this frame
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		// All frames to AP are directed except probe_req. IEEE 802.11/1999 - p.463
		// Do this before checking "duplicate frame".
		// 2003-08-20 accept BEACON to decide if OLBC (Overlapping Legacy BSS Condition) happens
		// TODO: consider move this code to be inside "APCheckRxError()"
		switch (pHeader->FC.Type)
		{
			// CASE I, receive a DATA frame
			case BTYPE_DATA:
				{
					if (pRxD->U2M)
					{
						Update_Rssi_Sample(pAd, &pAd->ApCfg.RssiSample, pRxWI);
						pAd->ApCfg.NumOfAvgRssiSample ++;
					}
					// process DATA frame
					APHandleRxDataFrame(pAd, &RxCell);
				}
				break;
			// CASE II, receive a MGMT frame
			case BTYPE_MGMT:
				{
					APHandleRxMgmtFrame(pAd, &RxCell);
				}
				break;
			// CASE III. receive a CNTL frame
			case BTYPE_CNTL:
				{
					APHandleRxControlFrame(pAd, &RxCell);
				}
				break;
			// discard other type
			default:
				RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
				break;
		}
	}

#ifdef UAPSD_AP_SUPPORT
	/* dont remove the function or UAPSD will fail */
    UAPSD_SP_CloseInRVDone(pAd);
#endif // UAPSD_AP_SUPPORT //

	return bReschedule;
}


BOOLEAN APFowardWirelessStaToWirelessSta(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	ULONG			FromWhichBSSID)
{
    MAC_TABLE_ENTRY	*pEntry;
    BOOLEAN			bAnnounce, bDirectForward;
	UCHAR			*pHeader802_3;
	PNDIS_PACKET	pForwardPacket;

#ifdef APCLI_SUPPORT
	// have no need to forwad the packet to WM
	if (FromWhichBSSID >= MIN_NET_DEVICE_FOR_APCLI)
	{
		// need annouce to upper layer
		return TRUE;
	}
#endif // APCLI_SUPPORT //

#ifdef WDS_SUPPORT
	// have no need to forwad the packet to WM
	if (FromWhichBSSID >= MIN_NET_DEVICE_FOR_WDS)
	{
		// need annouce to upper layer
		return TRUE;
	}
#endif // WDS_SUPPORT //

	pEntry = NULL;
	bAnnounce = TRUE;
	bDirectForward = FALSE;

	pHeader802_3 = GET_OS_PKT_DATAPTR(pPacket);

	if (pHeader802_3[0] & 0x01) 
	{
		/*
		** In the case, the BSS have only one STA behind.
		** AP have no necessary to forward the M/Bcase packet back to STA again.
		*/
		if (pAd->StaCount[FromWhichBSSID] > 1)
			bDirectForward  = TRUE;

		/* tell caller to deliver the packet to upper layer */
		bAnnounce = TRUE;
	}		
	else
	{
		// if destinated STA is a associated wireless STA
		pEntry = MacTableLookup(pAd, pHeader802_3);

		if (pEntry && pEntry->Sst == SST_ASSOC)
		{
			bDirectForward = TRUE;
			bAnnounce = FALSE;

			if (FromWhichBSSID == pEntry->apidx)
			{// STAs in same SSID
				if ((pAd->ApCfg.MBSSID[pEntry->apidx].IsolateInterStaTraffic == 1))
				{
					// release the packet				
					bDirectForward = FALSE;
					bAnnounce = FALSE;
				}
			}
			else
			{// STAs in different SSID
				if (pAd->ApCfg.IsolateInterStaTrafficBTNBSSID == 1)
				{
					bDirectForward = FALSE;
					bAnnounce = FALSE;
				}
			}
		}
		else
		{
			// announce this packet to upper layer (bridge)
			bDirectForward = FALSE;
			bAnnounce = TRUE;
		}
	}

	if (bDirectForward)
	{
		// build an NDIS packet			
		pForwardPacket = DuplicatePacket(pAd, pPacket, FromWhichBSSID);			

		if (pForwardPacket == NULL)
		{
			return bAnnounce;
		}

		// 1.1 apidx != 0, then we need set packet mbssid attribute.
		RTMP_SET_PACKET_NET_DEVICE_MBSSID(pForwardPacket, MAIN_MBSSID);	// set a default value
		if(pEntry && (pEntry->apidx != 0))
			RTMP_SET_PACKET_NET_DEVICE_MBSSID(pForwardPacket, pEntry->apidx);

		/* send bc/mc frame back to the same bss */
		if (!pEntry)
			RTMP_SET_PACKET_NET_DEVICE_MBSSID(pForwardPacket, FromWhichBSSID);

		RTMP_SET_PACKET_WCID(pForwardPacket, pEntry ? pEntry->Aid : MCAST_WCID);			
		RTMP_SET_PACKET_SOURCE(pForwardPacket, PKTSRC_NDIS);
		RTMP_SET_PACKET_MOREDATA(pForwardPacket, FALSE);
						
		APSendPacket(pAd, pForwardPacket);
		RTMPDeQueuePacket(pAd, FALSE, MAX_TX_PROCESS); 	// Dequeue outgoing frames from TxSwQueue0..3 queue and process it
	}
	
	return bAnnounce;
}


#if 0
VOID TxHardTransmit(	
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket)
{
	if (pMacEntry)
	{
		// unicast 
		if (!EAPOL)
		{
			switch ()
			{
				case TX_AMPDU_FRAME:
					{
						AMPDU_Frame_Tx();
					}
					break;
				case TX_AMSDU_FRAME:
					{
						AMSDU_Frame_Tx():
					}
					break;
				case TX_RALINK_FRAME:
					{
						Ralink_Frame_Tx();
					}
					break;
				default:
					break;
			}
		}
		else
		{
			Legacy_Frame_Tx();
		}
	}
	else 
	{
		// multicast
		Legacy_Frame_Tx();
	}
}
#endif






