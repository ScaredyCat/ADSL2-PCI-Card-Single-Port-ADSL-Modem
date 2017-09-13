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

static VOID APFindCipherAlgorithm(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk);

static inline BOOLEAN ApAllowToSendPacket(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	OUT UCHAR		*pWcid)
{
	PACKET_INFO 	PacketInfo;
	PUCHAR			pSrcBufVA;
	UINT			SrcBufLen;
	PMAC_TABLE_ENTRY pEntry = NULL;
	SST 			Sst;
	USHORT			Aid;
	UCHAR			PsMode, Rate;
	BOOLEAN			allowed;
	
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
	
	pEntry = APSsPsInquiry(pAd, pSrcBufVA, &Sst, &Aid, &PsMode, &Rate);

	if ((pEntry && (Sst == SST_ASSOC)) || (*pSrcBufVA & 0x01))
	{
		// Record that orignal packet source is from NDIS layer,so that 
		// later on driver knows how to release this NDIS PACKET
		
		*pWcid = (UCHAR)Aid; //RTMP_SET_PACKET_WCID(pPacket, (UCHAR)Aid);

		NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
		pAd->RalinkCounters.PendingNdisPacketCount++;

		allowed = TRUE;
	}
	else
	{
		allowed = FALSE;
	}

	return allowed;
	
}



/*
========================================================================
Routine Description:
    Early checking and OS-depened parsing for Tx packet to AP device.

Arguments:
    NDIS_HANDLE 	MiniportAdapterContext	Pointer refer to the device handle, i.e., the pAd.
	PPNDIS_PACKET	ppPacketArray			The packet array need to do transmission.
	UINT			NumberOfPackets			Number of packet in packet array.
	
Return Value:
	NONE					

Note:
	This function do early checking and classification for send-out packet.
	You only can put OS-depened & AP related code in here.
========================================================================
*/
VOID	APSendPackets(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	PPNDIS_PACKET	ppPacketArray,
	IN	UINT			NumberOfPackets)
{
	UINT			Index;
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER) MiniportAdapterContext;
	PNDIS_PACKET	pPacket;
	BOOLEAN			allowToSend;
	UCHAR			wcid = MCAST_WCID;
	
	DBGPRINT(RT_DEBUG_INFO, ("====> APSendPackets\n"));
	
	for (Index = 0; Index < NumberOfPackets; Index++)
	{
		pPacket = ppPacketArray[Index];

   		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
			RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS) ||
			RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
		{
			// Drop send request since hardware is in reset state
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		/* The following code do comparison must base on the following sequence: 
				MIN_NET_DEVICE_FOR_APCLI> MIN_NET_DEVICE_FOR_WDS > Normal
		*/
#ifdef APCLI_SUPPORT
		if (RTMP_GET_PACKET_NET_DEVICE(pPacket) >= MIN_NET_DEVICE_FOR_APCLI)
			allowToSend = ApCliAllowToSendPacket(pAd, pPacket, &wcid);
		else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
		if (RTMP_GET_PACKET_NET_DEVICE(pPacket) >= MIN_NET_DEVICE_FOR_WDS)
			allowToSend = ApWdsAllowToSendPacket(pAd, pPacket, &wcid);
		else
#endif // WDS_SUPPORT //
			allowToSend = ApAllowToSendPacket(pAd, pPacket, &wcid);

		if (allowToSend)
		{
			// For packet send from OS, we need to set the wcid here, it will used directly in APSendPacket.
			RTMP_SET_PACKET_WCID(pPacket, wcid);
			RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);
			NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
			pAd->RalinkCounters.PendingNdisPacketCount++;
			
			APSendPacket(pAd, pPacket);
		}
		else
		{
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		}
	}

	// Dequeue outgoing frames from TxSwQueue0..3 queue and process it
	RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS);

}


/*
	========================================================================
	Routine Description:
		This routine is used to do packet parsing and classification for Tx packet 
		to AP device, and it will en-queue packets to our TxSwQueue depends on AC 
		class.
	
	Arguments:
		pAd    Pointer to our adapter
		pPacket 	Pointer to send packet

	Return Value:
		NDIS_STATUS_SUCCESS			If succes to queue the packet into TxSwQueue.
		NDIS_STATUS_FAILURE			If failed to do en-queue.

	pre: Before calling this routine, caller should have filled the following fields

		pPacket->MiniportReserved[6] - contains packet source
		pPacket->MiniportReserved[5] - contains RA's WDS index (if RA on WDS link) or AID 
									   (if RA directly associated to this AP)
	post:This routine should decide the remaining pPacket->MiniportReserved[] fields 
		before calling APHardTransmit(), such as:

		pPacket->MiniportReserved[4] - Fragment # and User PRiority
		pPacket->MiniportReserved[7] - RTS/CTS-to-self protection method and TX rate

	Note:
		You only can put OS-indepened & AP related code in here.


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

	if (SrcBufLen < 14)
	{
		DBGPRINT(RT_DEBUG_ERROR,("APSendPacket --> Ndis Packet buffer error !!!\n"));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return (NDIS_STATUS_FAILURE);
	}

	//
	// Check the Ethernet Frame type of this packet, and set the RTMP_SET_PACKET_SPECIFIC flags.
	//		Here we set the PACKET_SPECIFIC flags(LLC, VLAN, DHCP/ARP, EAPOL).
	RTMPCheckEtherType(pAd, pPacket);

	Wcid = RTMP_GET_PACKET_WCID(pPacket);
#ifdef APCLI_SUPPORT
	if(pAd->MacTab.Content[Wcid].ValidAsApCli == TRUE)
	{
		pMacEntry = &pAd->MacTab.Content[Wcid];
		Rate = pAd->MacTab.Content[Wcid].CurrTxRate;
	    if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
			 && (pMacEntry->PortSecured == WPA_802_1X_PORT_NOT_SECURED) 			 	 			 	 
        	 && (RTMP_GET_PACKET_EAPOL(pPacket)== FALSE))
		{
            DBGPRINT(RT_DEBUG_INFO, ("I/F(apcli%d) APSendPacket --> Drop packet before AP-Client secured !!!\n", pMacEntry->MatchAPCLITabIdx)); 
            RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE); 
            return (NDIS_STATUS_FAILURE);
        }
	}
	else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if (pAd->MacTab.Content[Wcid].ValidAsWDS == TRUE)
	{
		//b7 as WDS bit, b0-6 as WDS index when b7==1
		pMacEntry = &pAd->MacTab.Content[Wcid];
		Rate = pAd->MacTab.Content[Wcid].CurrTxRate;
	}
	else
#endif // WDS_SUPPORT //
	if ((Wcid == MCAST_WCID) || (pAd->MacTab.Content[Wcid].ValidAsCLI == TRUE))
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

        // in WPA mode, AP does not send packets before port secured.
        if ((pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA) && 
			(pAd->ApCfg.MBSSID[apidx].PortSecured == WPA_802_1X_PORT_NOT_SECURED) && 
			(RTMP_GET_PACKET_EAPOL(pPacket) == FALSE))
        {
            DBGPRINT(RT_DEBUG_INFO,("I/F(ra%d) APSendPacket --> Drop packet before AP secured !!!\n", apidx));          
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
            return NDIS_STATUS_FAILURE;			
        }
		
        // if 802.1x, AP does not send packets other than EAPOL before this session control port set to "accept all".
		if ((pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE) && 
			(pMacEntry->PrivacyFilter == Ndis802_11PrivFilter8021xWEP) && 
			(RTMP_GET_PACKET_EAPOL(pPacket) == FALSE))
		{
			DBGPRINT(RT_DEBUG_INFO, ("I/F(ra%d) APSendPacket --> Drop packet before this control port accept all !!!\n", apidx));
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
            return NDIS_STATUS_FAILURE;	
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("I/F(ra%d) APSendPacket --> Drop unknow packet !!!\n", apidx));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return NDIS_STATUS_FAILURE;
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

						vlan_id = cpu2be16(vlan_id);

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
	// 4. put to corrsponding TxSwQueue or Power-saving queue
	//

	// WDS and ApClient link should never go into power-save mode; just send out the frame
	if (pMacEntry && ((pMacEntry->ValidAsWDS == TRUE) || (pMacEntry->ValidAsApCli == TRUE) || (pMacEntry->ValidAsMesh == TRUE)))
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
			DBGPRINT(RT_DEBUG_TRACE, ("M/BCAST PSQ(=%ld) full, drop it!\n", pAd->MacTab.McastPsQueue.Number));
			return NDIS_STATUS_FAILURE;
		}
		else
		{
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
			InsertHeadQueue(&pAd->MacTab.McastPsQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
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
			 && ((((pMacEntry->ValidAsCLI) || (pMacEntry->ValidAsWDS) || (pMacEntry->ValidAsMesh)) && (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RALINK_CHIPSET))) || 
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


// --------------------------------------------------------
//  FIND ENCRYPT KEY AND DECIDE CIPHER ALGORITHM
//		Find the WPA key, either Group or Pairwise Key
//		LEAP + TKIP also use WPA key.
// --------------------------------------------------------
// Decide WEP bit and cipher suite to be used. Same cipher suite should be used for whole fragment burst
// In Cisco CCX 2.0 Leap Authentication
//		   WepStatus is Ndis802_11Encryption1Enabled but the key will use PairwiseKey
//		   Instead of the SharedKey, SharedKey Length may be Zero.
static inline VOID APFindCipherAlgorithm(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk)
{

	PCIPHER_KEY			pKey = NULL;
	UCHAR				CipherAlg = CIPHER_NONE;		// cipher alogrithm
	UCHAR				apidx;
	UCHAR				RAWcid;
	PMAC_TABLE_ENTRY	pMacEntry;

	apidx = pTxBlk->apidx;
	RAWcid = pTxBlk->Wcid;
	pMacEntry = pTxBlk->pMacEntry;

#ifdef APCLI_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bApCliPacket))
	{	
		PAPCLI_STRUCT	pApCliEntry;

		pApCliEntry = pTxBlk->pApCliEntry;

		if (RTMP_GET_PACKET_EAPOL(pTxBlk->pPacket)) 
		{			
			// These EAPoL frames must be clear before 4-way handshaking is completed.
			if ((!(TX_BLK_TEST_FLAG(pTxBlk, fTX_bClearEAPFrame))) && 
				(pMacEntry->PairwiseKey.CipherAlg) &&
				(pMacEntry->PairwiseKey.KeyLen))
			{
				CipherAlg  = pMacEntry->PairwiseKey.CipherAlg;
				if (CipherAlg)
					pKey = &pMacEntry->PairwiseKey;
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
			CipherAlg  = pMacEntry->PairwiseKey.CipherAlg;
			if (CipherAlg)
				pKey = &pMacEntry->PairwiseKey;
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
	if (TX_BLK_TEST_FLAG(pTxBlk,fTX_bWDSEntry))
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
	if ((RTMP_GET_PACKET_EAPOL(pTxBlk->pPacket))			||
		((pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption1Enabled) && (pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE)) || 
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption2Enabled)	||
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption3Enabled)	||
		(pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption4Enabled))
	{
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bClearEAPFrame))
		{
			DBGPRINT(RT_DEBUG_TRACE,("APHardTransmit --> clear eap frame !!!\n"));          
			CipherAlg = CIPHER_NONE;
			pKey = NULL;
		}
		else if (!pTxBlk->pMacEntry) 	   // M/BCAST to local BSS, use default key in shared key table
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

	pTxBlk->CipherAlg = CipherAlg;
	pTxBlk->pKey = pKey;
}

static inline VOID APBuildCache802_11Header(
	IN RTMP_ADAPTER		*pAd,
	IN TX_BLK			*pTxBlk,
	IN UCHAR			*pHeader)
{
	MAC_TABLE_ENTRY	*pMacEntry;
	PHEADER_802_11	pHeader80211;

	pHeader80211 = (PHEADER_802_11)pHeader;
	pMacEntry = pTxBlk->pMacEntry;

	// 
	// Update the cached 802.11 HEADER
	// 
	
	// normal wlan header size : 24 octets
	pTxBlk->MpduHeaderLen = sizeof(HEADER_802_11);
	
	// More Bit
	pHeader80211->FC.MoreData = TX_BLK_TEST_FLAG(pTxBlk, fTX_bMoreData);
	
	// Sequence
	pHeader80211->Sequence = pMacEntry->TxSeq[pTxBlk->UserPriority];
	pMacEntry->TxSeq[pTxBlk->UserPriority] = (pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
	
	// SA 
#ifdef WDS_SUPPORT
	if(pMacEntry->ValidAsWDS == TRUE)
	{	// The addr3 of WDS packet is Destination Mac address and Addr4 is the Source Mac address.
		COPY_MAC_ADDR(pHeader80211->Addr3, pTxBlk->pSrcBufHeader);
		COPY_MAC_ADDR(pHeader80211->Octet, pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);
		pTxBlk->MpduHeaderLen += MAC_ADDR_LEN; 
	}
	else
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
	if(pMacEntry->ValidAsApCli == TRUE)
	{	// The addr3 of Ap-client packet is Destination Mac address.
		COPY_MAC_ADDR(pHeader80211->Addr3, pTxBlk->pSrcBufHeader);
	}
	else
#endif // APCLI_SUPPORT //
	{	// The addr3 of normal packet send from DS is Src Mac address.
		COPY_MAC_ADDR(pHeader80211->Addr3, pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);
	}


}


static inline VOID APBuildCommon802_11Header(
	IN  PRTMP_ADAPTER   pAd,
	IN  TX_BLK          *pTxBlk)
{

	HEADER_802_11	*pHeader_802_11;

	// 
	// MAKE A COMMON 802.11 HEADER
	// 

	// normal wlan header size : 24 octets
	pTxBlk->MpduHeaderLen = sizeof(HEADER_802_11);

	pHeader_802_11 = (HEADER_802_11 *) &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];

	NdisZeroMemory(pHeader_802_11, sizeof(HEADER_802_11));

	pHeader_802_11->FC.FrDs = 1;
	pHeader_802_11->FC.Type = BTYPE_DATA;
	pHeader_802_11->FC.SubType = ((TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM)) ? SUBTYPE_QDATA : SUBTYPE_DATA);

	if (pTxBlk->pMacEntry)
	{
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bForceNonQoS))
		{			
			pHeader_802_11->Sequence = pTxBlk->pMacEntry->NonQosDataSeq;
			pTxBlk->pMacEntry->NonQosDataSeq = (pTxBlk->pMacEntry->NonQosDataSeq+1) & MAXSEQ;
		}
		else
		{		
    	    pHeader_802_11->Sequence = pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority];
    	    pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
    	}		
	}
	else
	{
		pHeader_802_11->Sequence = pAd->Sequence;
		pAd->Sequence = (pAd->Sequence+1) & MAXSEQ; // next sequence  
	}
	
	pHeader_802_11->Frag = 0;

	pHeader_802_11->FC.MoreData = TX_BLK_TEST_FLAG(pTxBlk, fTX_bMoreData);

#ifdef APCLI_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bApCliPacket))
	{
		pHeader_802_11->FC.ToDs = 1;
		pHeader_802_11->FC.FrDs = 0;
		COPY_MAC_ADDR(pHeader_802_11->Addr1, APCLI_ROOT_BSSID_GET(pAd, pTxBlk->Wcid));	// to AP2
		COPY_MAC_ADDR(pHeader_802_11->Addr2, pTxBlk->pApCliEntry->CurrentAddress);		// from AP1
		COPY_MAC_ADDR(pHeader_802_11->Addr3, pTxBlk->pSrcBufHeader);					// DA
	}
	else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry))
	{
		pHeader_802_11->FC.ToDs = 1;
		COPY_MAC_ADDR(pHeader_802_11->Addr1, pTxBlk->pMacEntry->Addr);					// to AP2
		COPY_MAC_ADDR(pHeader_802_11->Addr2, pAd->CurrentAddress);						// from AP1
		COPY_MAC_ADDR(pHeader_802_11->Addr3, pTxBlk->pSrcBufHeader);					// DA
		COPY_MAC_ADDR(&pHeader_802_11->Octet[0], pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);			// ADDR4 = SA

		pTxBlk->MpduHeaderLen += MAC_ADDR_LEN; 
	}
	else
#endif // WDS_SUPPORT //
	{
		// TODO: how about "MoreData" bit? AP need to set this bit especially for PS-POLL response
#ifdef IGMP_SNOOP_SUPPORT
		if (pTxBlk->Wcid != MCAST_WCID)
		{
			COPY_MAC_ADDR(pHeader_802_11->Addr1, pTxBlk->pMacEntry->Addr); // DA
		}
		else
#endif // IGMP_SNOOP_SUPPORT //
		{
		   	COPY_MAC_ADDR(pHeader_802_11->Addr1, pTxBlk->pSrcBufHeader);					// DA
		}
		COPY_MAC_ADDR(pHeader_802_11->Addr2, pAd->ApCfg.MBSSID[pTxBlk->apidx].Bssid);		// BSSID
		COPY_MAC_ADDR(pHeader_802_11->Addr3, pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);			// SA
	}


	if (pTxBlk->CipherAlg != CIPHER_NONE)
		pHeader_802_11->FC.Wep = 1;
}


static inline PUCHAR AP_Build_ARalink_Frame_Header(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK		*pTxBlk)
{
	PUCHAR			pHeaderBufPtr;//, pSaveBufPtr;
	HEADER_802_11	*pHeader_802_11;
	PNDIS_PACKET	pNextPacket;
	UINT32			nextBufLen;
	PQUEUE_ENTRY	pQEntry;
		
	APFindCipherAlgorithm(pAd, pTxBlk);
	APBuildCommon802_11Header(pAd, pTxBlk);


	pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];
	pHeader_802_11 = (HEADER_802_11 *) pHeaderBufPtr;

	// steal "order" bit to mark "aggregation"
	pHeader_802_11->FC.Order = 1;
	
	// skip common header
	pHeaderBufPtr += pTxBlk->MpduHeaderLen;

	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM))
	{
		//
		// build QOS Control bytes
		// 
		*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#ifdef UAPSD_AP_SUPPORT
		if (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
			&& (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry) == FALSE)
#endif // WDS_SUPPORT //
		)
		{
			/* 
			 * we can not use bMoreData bit to get EOSP bit because
			 * maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 
			 */
			 if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP))
				*pHeaderBufPtr |= (1 << 4);
		}
#endif // UAPSD_AP_SUPPORT //
	
		*(pHeaderBufPtr+1) = 0;
		pHeaderBufPtr +=2;
		pTxBlk->MpduHeaderLen += 2;
	}

	// padding at front of LLC header. LLC header should at 4-bytes aligment.
	pTxBlk->HdrPadLen = (ULONG)pHeaderBufPtr;
	pHeaderBufPtr = (PCHAR)ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen);

	
	// For RA Aggregation, 
	// put the 2nd MSDU length(extra 2-byte field) after QOS_CONTROL in little endian format
	pQEntry = pTxBlk->TxPacketList.Head;
	pNextPacket = QUEUE_ENTRY_TO_PKT(pQEntry);
	nextBufLen = GET_OS_PKT_LEN(pNextPacket);
	if (RTMP_GET_PACKET_VLAN(pNextPacket))
		nextBufLen -= LENGTH_802_1Q;
	
	*pHeaderBufPtr = (UCHAR)nextBufLen & 0xff;
	*(pHeaderBufPtr+1) = (UCHAR)(nextBufLen >> 8);

	pHeaderBufPtr += 2;
	pTxBlk->MpduHeaderLen += 2;
	
	return pHeaderBufPtr;
	
}


static inline PUCHAR AP_Build_AMSDU_Frame_Header(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK		*pTxBlk)
{
	PUCHAR			pHeaderBufPtr;//, pSaveBufPtr;
	HEADER_802_11	*pHeader_802_11;

	
	APFindCipherAlgorithm(pAd, pTxBlk);
	APBuildCommon802_11Header(pAd, pTxBlk);

	pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];
	pHeader_802_11 = (HEADER_802_11 *) pHeaderBufPtr;

	// skip common header
	pHeaderBufPtr += pTxBlk->MpduHeaderLen;

	//
	// build QOS Control bytes
	// 
	*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#ifdef UAPSD_AP_SUPPORT
	if (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
		&& (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry) == FALSE)
#endif // WDS_SUPPORT //
	)
	{
		/* 
		 * we can not use bMoreData bit to get EOSP bit because
		 * maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 
		 */
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP))
			*pHeaderBufPtr |= (1 << 4);
	}
#endif // UAPSD_AP_SUPPORT //


	//
	// A-MSDU packet
	// 
	*pHeaderBufPtr |= 0x80;

	*(pHeaderBufPtr+1) = 0;
	pHeaderBufPtr +=2;
	pTxBlk->MpduHeaderLen += 2;

#if 0
	//
	// build HTC+ 
	// HTC control filed following QoS field
	// 
	if ((pAd->CommonCfg.bRdg == TRUE) && CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
	{
		// mark HTC bit 
		pHeader_802_11->FC.Order = 1;

		NdisZeroMemory(pHeaderBufPtr, 4);
		*(pHeaderBufPtr+3) |= 0x80;
		pHeaderBufPtr += 4;
		pTxBlk->MpduHeaderLen += 4;
	}
#endif

	//pSaveBufPtr = pHeaderBufPtr;

	//
	// padding at front of LLC header
	// LLC header should locate at 4-octets aligment
	// 
	// @@@ MpduHeaderLen excluding padding @@@
	// 
	pTxBlk->HdrPadLen = (ULONG)pHeaderBufPtr;
	pHeaderBufPtr = (PCHAR) ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen);
		
	return pHeaderBufPtr;

}


VOID AP_AMPDU_Frame_Tx(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk)
{
	HEADER_802_11	*pHeader_802_11;
	PUCHAR			pHeaderBufPtr;
//	UCHAR			QueIdx = pTxBlk->QueIdx;
	USHORT			FreeNumber;
	MAC_TABLE_ENTRY	*pMacEntry;
	PQUEUE_ENTRY	pQEntry;

	
	ASSERT(pTxBlk);

	pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
	pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
	if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE)
	{
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return;
	}

	pMacEntry = pTxBlk->pMacEntry;
	if (pMacEntry->isCached)
	{
		//NdisZeroMemory((PUCHAR)(&pTxBlk->HeaderBuf[0]), sizeof(pTxBlk->HeaderBuf)); // It should be cleared!!!
		NdisMoveMemory((PUCHAR)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), (PUCHAR)(&pMacEntry->CachedBuf[0]), TXWI_SIZE + sizeof(HEADER_802_11));
		pHeaderBufPtr = (PUCHAR)(&pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE]);
		APBuildCache802_11Header(pAd, pTxBlk, pHeaderBufPtr);
	}
	else 
	{
		APFindCipherAlgorithm(pAd, pTxBlk);
		APBuildCommon802_11Header(pAd, pTxBlk);
			
		pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];
	}

	pHeader_802_11 = (HEADER_802_11 *) pHeaderBufPtr;
		
	// skip common header
	pHeaderBufPtr += pTxBlk->MpduHeaderLen;

	//
	// build QOS Control bytes
	// 
	*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#ifdef UAPSD_AP_SUPPORT
	if (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
		&& (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry) == FALSE)
#endif // WDS_SUPPORT //
		)
	{
		/* 
		 * we can not use bMoreData bit to get EOSP bit because
		 * maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 
		 */
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP))
			*pHeaderBufPtr |= (1 << 4);
	}
#endif // UAPSD_AP_SUPPORT //

	*(pHeaderBufPtr+1) = 0;
	pHeaderBufPtr +=2;
	pTxBlk->MpduHeaderLen += 2;

	//
	// build HTC+ 
	// HTC control filed following QoS field
	// 
	if ((pAd->CommonCfg.bRdg == TRUE) && CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
	{
		// mark HTC bit 
		pHeader_802_11->FC.Order = 1;
		NdisZeroMemory(pHeaderBufPtr, 4);
		*(pHeaderBufPtr+3) |= 0x80;
		
		pHeaderBufPtr += 4;
		pTxBlk->MpduHeaderLen += 4;
	}

	//pTxBlk->MpduHeaderLen = pHeaderBufPtr - pTxBlk->HeaderBuf - TXWI_SIZE - TXINFO_SIZE;
	ASSERT(pTxBlk->MpduHeaderLen >= 24);

	// skip 802.3 header
	pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
	pTxBlk->SrcBufLen  -= LENGTH_802_3;

	// skip vlan tag
	if (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket))
	{
		pTxBlk->pSrcBufData	+= LENGTH_802_1Q;
		pTxBlk->SrcBufLen	-= LENGTH_802_1Q;
	}

	//
	// padding at front of LLC header
	// LLC header should locate at 4-octets aligment
	// 
	// @@@ MpduHeaderLen excluding padding @@@
	// 
	pTxBlk->HdrPadLen = (ULONG)pHeaderBufPtr;
	pHeaderBufPtr = (PCHAR) ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen);


	//
	// Insert LLC-SNAP encapsulation - 8 octets
	//
	EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData-2, pTxBlk->pExtraLlcSnapEncap);
	if (pTxBlk->pExtraLlcSnapEncap)
	{
		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
		pHeaderBufPtr += 6;
		// get 2 octets (TypeofLen)
		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData-2, 2);
		pHeaderBufPtr += 2;
		pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
	}

	if (pMacEntry->isCached)
	{
		RTMPWriteTxWI_Cache(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);
	}
	else
	{
		RTMPWriteTxWI_Data(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);

		NdisZeroMemory((PUCHAR)(&pMacEntry->CachedBuf[0]), sizeof(pMacEntry->CachedBuf));
		NdisMoveMemory((PUCHAR)(&pMacEntry->CachedBuf[0]), (PUCHAR)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), (pHeaderBufPtr - (PUCHAR)(&pTxBlk->HeaderBuf[TXINFO_SIZE])));
		pMacEntry->isCached = TRUE;
	}

	// calculate Transmitted AMPDU count and ByteCount 	
	{
		pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart ++;
		pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.QuadPart += pTxBlk->SrcBufLen;		
	}

	// calculate Tx count and ByteCount per BSS
	if ((pMacEntry->ValidAsCLI) && (pMacEntry->apidx < pAd->ApCfg.BssidNum))
	{
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TransmittedByteCount += pTxBlk->SrcBufLen;
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TxCount ++;
	}

	//FreeNumber = GET_TXRING_FREENO(pAd, QueIdx);

	HAL_WriteTxResource(pAd, pTxBlk, TRUE, &FreeNumber);

#if 0	// remove to APBuildCommon802_11Header
	// Increase Sequence
	pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
#endif

	//
	// Kick out Tx
	// 
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;
	
}


VOID AP_AMSDU_Frame_Tx(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk)
{
	PUCHAR			pHeaderBufPtr;
//	UCHAR			QueIdx = pTxBlk->QueIdx;
	USHORT			FreeNumber;
	USHORT			subFramePayloadLen = 0;	// AMSDU Subframe length without AMSDU-Header / Padding.
	USHORT			totalMPDUSize=0;
	UCHAR			*subFrameHeader;
	UCHAR			padding = 0;
	USHORT			FirstTx = 0, LastTxIdx = 0;
	int 			frameNum = 0;
	PQUEUE_ENTRY	pQEntry;
		
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	PAPCLI_STRUCT   pApCliEntry = NULL;
#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

	
	ASSERT((pTxBlk->TxPacketList.Number > 1));

	while(pTxBlk->TxPacketList.Head)
	{
		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
		
		if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE)
		{
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		// skip 802.3 header
		pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
		pTxBlk->SrcBufLen  -= LENGTH_802_3;

		// skip vlan tag
		if (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket))
		{
			pTxBlk->pSrcBufData	+= LENGTH_802_1Q;
			pTxBlk->SrcBufLen	-= LENGTH_802_1Q;
		}
		
		if (frameNum == 0)
		{
			pHeaderBufPtr = AP_Build_AMSDU_Frame_Header(pAd, pTxBlk);

			// NOTE: TxWI->MPDUtotalByteCount will be updated after final frame was handled.
			RTMPWriteTxWI_Data(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);
		}
		else
		{
			pHeaderBufPtr = &pTxBlk->HeaderBuf[0];
			padding = ROUND_UP(LENGTH_AMSDU_SUBFRAMEHEAD + subFramePayloadLen, 4) - (LENGTH_AMSDU_SUBFRAMEHEAD + subFramePayloadLen);
			NdisZeroMemory(pHeaderBufPtr, padding + LENGTH_AMSDU_SUBFRAMEHEAD);
			pHeaderBufPtr += padding;
			pTxBlk->MpduHeaderLen = padding;
			pTxBlk->HdrPadLen += padding;
		}

		//
		// A-MSDU subframe
		//   DA(6)+SA(6)+Length(2) + LLC/SNAP Encap
		// 
		subFrameHeader = pHeaderBufPtr;
		subFramePayloadLen = pTxBlk->SrcBufLen;

		NdisMoveMemory(subFrameHeader, pTxBlk->pSrcBufHeader, 12);

#ifdef APCLI_SUPPORT
		if(TX_BLK_TEST_FLAG(pTxBlk, fTX_bApCliPacket))
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[pTxBlk->pMacEntry->MatchAPCLITabIdx];
			if (pApCliEntry->Valid)
				NdisMoveMemory(&subFrameHeader[6] , pApCliEntry->CurrentAddress, 6);
		}
#endif // APCLI_SUPPORT //


		pHeaderBufPtr += LENGTH_AMSDU_SUBFRAMEHEAD;
		pTxBlk->MpduHeaderLen += LENGTH_AMSDU_SUBFRAMEHEAD;



		//
		// Insert LLC-SNAP encapsulation - 8 octets
		// 
		EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData-2, pTxBlk->pExtraLlcSnapEncap);

		subFramePayloadLen = pTxBlk->SrcBufLen;

		if (pTxBlk->pExtraLlcSnapEncap)
		{
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
			pHeaderBufPtr += 6;
			// get 2 octets (TypeofLen)
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData-2, 2);
			pHeaderBufPtr += 2;
			pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
			subFramePayloadLen += LENGTH_802_1_H;
		}

		// update subFrame Length field
		subFrameHeader[12] = (subFramePayloadLen & 0xFF00) >> 8;
		subFrameHeader[13] = subFramePayloadLen & 0xFF;

		totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;


		//FreeNumber = GET_TXRING_FREENO(pAd, QueIdx);

		if (frameNum ==0)
			FirstTx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &FreeNumber);
		else
			LastTxIdx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &FreeNumber);	

		frameNum++;

		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;		
		

		// calculate Transmitted AMSDU Count and ByteCount		
		{
			pAd->RalinkCounters.TransmittedAMSDUCount.u.LowPart ++;
			pAd->RalinkCounters.TransmittedOctetsInAMSDU.QuadPart += totalMPDUSize;			
		}

		// calculate Tx count and ByteCount per BSS
		if ((pTxBlk->pMacEntry->ValidAsCLI) && (pTxBlk->pMacEntry->apidx < pAd->ApCfg.BssidNum))
		{
			pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TransmittedByteCount += totalMPDUSize;
			pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TxCount ++;
		}

	}

	HAL_FinalWriteTxResource(pAd, pTxBlk, totalMPDUSize, FirstTx);
	HAL_LastTxIdx(pAd, pTxBlk->QueIdx, LastTxIdx);

#if 0	// remove to APBuildCommon802_11Header
	// Increase Sequence
	pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
#endif
	
	//
	// Kick out Tx
	// 
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);
}


VOID AP_Legacy_Frame_Tx(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk)
{
	HEADER_802_11	*pHeader_802_11;
	PUCHAR			pHeaderBufPtr;
//	UCHAR			QueIdx = pTxBlk->QueIdx;
	USHORT			FreeNumber;
	BOOLEAN			bVLANPkt;
	PQUEUE_ENTRY	pQEntry;

	
	ASSERT(pTxBlk);


	pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
	pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);

	if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE)
	{
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return;
	}

	if (pTxBlk->TxFrameType == TX_MCAST_FRAME)
	{
		INC_COUNTER64(pAd->WlanCounters.MulticastTransmittedFrameCount);
	}

	bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);

	APFindCipherAlgorithm(pAd, pTxBlk);
	APBuildCommon802_11Header(pAd, pTxBlk);

	// skip 802.3 header
	pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
	pTxBlk->SrcBufLen  -= LENGTH_802_3;

	//printk("Dump Original packet: Len=%d!\n", GET_OS_PKT_LEN(pTxBlk->pTxPacket));
	//hex_dump("Pkt:", GET_OS_PKT_DATAPTR(pTxBlk->pTxPacket), GET_OS_PKT_LEN(pTxBlk->pTxPacket));

	// skip vlan tag
	if (bVLANPkt)
	{
		pTxBlk->pSrcBufData	+= LENGTH_802_1Q;
		pTxBlk->SrcBufLen	-= LENGTH_802_1Q;
	}

	// record these MCAST_TX frames for group key rekey  
	if (pTxBlk->TxFrameType == TX_MCAST_FRAME)
	{				
		if(pAd->ApCfg.REKEYTimerRunning && pAd->ApCfg.WPAREKEY.ReKeyMethod == PKT_REKEY)
		{
			pAd->ApCfg.REKEYCOUNTER += (pTxBlk->SrcBufLen);
		}
	}

	pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];
	pHeader_802_11 = (HEADER_802_11 *) pHeaderBufPtr;

	// skip common header
	pHeaderBufPtr += pTxBlk->MpduHeaderLen;

	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM))
	{
		//
		// build QOS Control bytes
		// 
		*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#ifdef UAPSD_AP_SUPPORT
		if (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
			&& (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry) == FALSE)
#endif // WDS_SUPPORT //
		)
		{
			/* 
			 * we can not use bMoreData bit to get EOSP bit because
			 * maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 
			 */
			if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP))
				*pHeaderBufPtr |= (1 << 4);
		}
#endif // UAPSD_AP_SUPPORT //
	
		*(pHeaderBufPtr+1) = 0;
		pHeaderBufPtr +=2;
		pTxBlk->MpduHeaderLen += 2;
	}

	//
	// padding at front of LLC header
	// LLC header should locate at 4-octets aligment
	// 
	pTxBlk->HdrPadLen = (ULONG)pHeaderBufPtr;
	pHeaderBufPtr = (PCHAR) ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen);


	//
	// Insert LLC-SNAP encapsulation - 8 octets
	// 
		//
   	// if original Ethernet frame contains no LLC/SNAP, 
	// then an extra LLC/SNAP encap is required 
	// 
	EXTRA_LLCSNAP_ENCAP_FROM_PKT_START(pTxBlk->pSrcBufHeader, pTxBlk->pExtraLlcSnapEncap);
	if (pTxBlk->pExtraLlcSnapEncap)
	{
		UCHAR vlan_size;

		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
		pHeaderBufPtr += 6;
		// skip vlan tag
		vlan_size =  (bVLANPkt) ? LENGTH_802_1Q : 0;
		// get 2 octets (TypeofLen)
		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufHeader+12+vlan_size, 2);
		pHeaderBufPtr += 2;
		pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
	}

	// calculate Tx count and ByteCount per BSS
	if (pTxBlk->pMacEntry && (pTxBlk->pMacEntry->ValidAsCLI) && (pTxBlk->pMacEntry->apidx < pAd->ApCfg.BssidNum))
	{
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TransmittedByteCount += pTxBlk->SrcBufLen;
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TxCount ++;
	}

	//
	// prepare for TXWI
	// use Wcid as Key Index
	//

	// update Hardware Group Key Index
	if (!pTxBlk->pMacEntry)
	{
		// use Wcid as Hardware Key Index
		GET_GroupKey_WCID(pTxBlk->Wcid, pTxBlk->apidx);
	}

	RTMPWriteTxWI_Data(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);

	//FreeNumber = GET_TXRING_FREENO(pAd, QueIdx);

	HAL_WriteTxResource(pAd, pTxBlk, TRUE, &FreeNumber);

	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;

	//
	// Kick out Tx
	// 
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

#if 0	// remove to APBuildCommon802_11Header
	// increase Sequence Number
	if (pTxBlk->pMacEntry)
		pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
	else
		pAd->Sequence = (pAd->Sequence+1) & MAXSEQ;
#endif

}


VOID AP_Fragment_Frame_Tx(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK		*pTxBlk)
{
	HEADER_802_11	*pHeader_802_11;
	PUCHAR			pHeaderBufPtr;
//	UCHAR			QueIdx = pTxBlk->QueIdx;
	USHORT			FreeNumber;
	UCHAR 			fragNum = 0;
	USHORT			EncryptionOverhead = 0;	
	UINT32			FreeMpduSize, SrcRemainingBytes;
	USHORT			AckDuration;
	UINT 			NextMpduSize;
	BOOLEAN			bVLANPkt;
	PQUEUE_ENTRY	pQEntry;
	PACKET_INFO		PacketInfo;
	
	
	ASSERT(pTxBlk);

	pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
	pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);

	if(RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE)
	{
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return;
	}

	ASSERT(TX_BLK_TEST_FLAG(pTxBlk, fTX_bAllowFrag));

	bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
	
	APFindCipherAlgorithm(pAd, pTxBlk);
	APBuildCommon802_11Header(pAd, pTxBlk);
	
	if (pTxBlk->CipherAlg == CIPHER_TKIP)
	{
		pTxBlk->pPacket = duplicate_pkt_with_TKIP_MIC(pAd, pTxBlk->pPacket);
		if (pTxBlk->pPacket == NULL)
			return;
		RTMP_QueryPacketInfo(pTxBlk->pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
	}
	
	// skip 802.3 header
	pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
	pTxBlk->SrcBufLen  -= LENGTH_802_3;


	// skip vlan tag
	if (bVLANPkt)
	{
		pTxBlk->pSrcBufData	+= LENGTH_802_1Q;
		pTxBlk->SrcBufLen	-= LENGTH_802_1Q;
	}

	pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];
	pHeader_802_11 = (HEADER_802_11 *)pHeaderBufPtr;


	// skip common header
	pHeaderBufPtr += pTxBlk->MpduHeaderLen;

	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM))
	{
		//
		// build QOS Control bytes
		// 
		*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#ifdef UAPSD_AP_SUPPORT
		if (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)
#ifdef WDS_SUPPORT
			&& (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWDSEntry) == FALSE)
#endif // WDS_SUPPORT //
		)
		{
			/* 
			 * we can not use bMoreData bit to get EOSP bit because
			 * maybe bMoreData = 1 & EOSP = 1 when Max SP Length != 0 
			 */
			if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP))
				*pHeaderBufPtr |= (1 << 4);
		}
#endif // UAPSD_AP_SUPPORT //
	
		*(pHeaderBufPtr+1) = 0;
		pHeaderBufPtr +=2;
		pTxBlk->MpduHeaderLen += 2;
	}

	//
	// padding at front of LLC header
	// LLC header should locate at 4-octets aligment
	// 
	pTxBlk->HdrPadLen = (ULONG)pHeaderBufPtr;
	pHeaderBufPtr = (PCHAR) ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen);


	//
	// Insert LLC-SNAP encapsulation - 8 octets
	// 
	//
   	// if original Ethernet frame contains no LLC/SNAP, 
	// then an extra LLC/SNAP encap is required 
	// 
	EXTRA_LLCSNAP_ENCAP_FROM_PKT_START(pTxBlk->pSrcBufHeader, pTxBlk->pExtraLlcSnapEncap);
	if (pTxBlk->pExtraLlcSnapEncap)
	{
		UCHAR vlan_size;

		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
		pHeaderBufPtr += 6;
		// skip vlan tag
		vlan_size =  (bVLANPkt) ? LENGTH_802_1Q : 0;
		// get 2 octets (TypeofLen)
		NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufHeader+12+vlan_size, 2);
		pHeaderBufPtr += 2;
		pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
	}

	// If TKIP is used and fragmentation is required. Driver has to
	//	append TKIP MIC at tail of the scatter buffer
	//	MAC ASIC will only perform IV/EIV/ICV insertion but no TKIP MIC
	if (pTxBlk->CipherAlg == CIPHER_TKIP)
	{
		RTMPCalculateMICValue(pAd, pTxBlk->pPacket, pTxBlk->pExtraLlcSnapEncap, pTxBlk->pKey, pTxBlk->apidx);

		// NOTE: DON'T refer the skb->len directly after following copy. Becasue the length is not adjust
		//			to correct lenght, refer to pTxBlk->SrcBufLen for the packet length in following progress.
		NdisMoveMemory(pTxBlk->pSrcBufData + pTxBlk->SrcBufLen, &pAd->PrivateInfo.Tx.MIC[0], 8);
		//skb_put((RTPKT_TO_OSPKT(pTxBlk->pPacket))->tail, 8);
		pTxBlk->SrcBufLen += 8;
		pTxBlk->TotalFrameLen += 8;
		pTxBlk->CipherAlg = CIPHER_TKIP_NO_MIC;
	}

	// calculate Tx count and ByteCount per BSS
	if ((pTxBlk->pMacEntry->ValidAsCLI) && (pTxBlk->pMacEntry->apidx < pAd->ApCfg.BssidNum))
	{
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TransmittedByteCount += pTxBlk->SrcBufLen;
		pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TxCount ++;
	}

	//
	// calcuate the overhead bytes that encryption algorithm may add. This
	// affects the calculate of "duration" field
	//
	if ((pTxBlk->CipherAlg == CIPHER_WEP64) || (pTxBlk->CipherAlg == CIPHER_WEP128)) 
		EncryptionOverhead = 8; //WEP: IV[4] + ICV[4];
	else if (pTxBlk->CipherAlg == CIPHER_TKIP_NO_MIC)
		EncryptionOverhead = 12;//TKIP: IV[4] + EIV[4] + ICV[4], MIC will be added to TotalPacketLength
	else if (pTxBlk->CipherAlg == CIPHER_TKIP)
		EncryptionOverhead = 20;//TKIP: IV[4] + EIV[4] + ICV[4] + MIC[8]
	else if (pTxBlk->CipherAlg == CIPHER_AES)
		EncryptionOverhead = 16;	// AES: IV[4] + EIV[4] + MIC[8]
	else
		EncryptionOverhead = 0;

#if 0	
	if (CKIP_CMIC_ON(pAd))
	{
		if (pAd->StaCfg.WepStatus == Ndis802_11Encryption2Enabled)
		{
			//
			// LEAP + TKIP, 
			// Type[2] + CKIP_MIC[4] + CKIP_SEQ[4] + IV[4] + EIV[4] + ICV[4] + MIC[8]
			//
			// 802_1H :    AA AA 03 00 00 00
			// Cisco SNAP: AA AA 03 00 40 96 00 02
			//                               ^^^^^ Type[2]  
			// 
			EncryptionOverhead = 30; 
		}
		if (pAd->StaCfg.WepStatus == Ndis802_11Encryption1Enabled)
		{
			//
			// LEAP + WEP
			// Type[2] + CKIP_MIC[4] + CKIP_SEQ[4] + IV[4] + ICV[4]
			//
			EncryptionOverhead = 18;
		}
	}
#endif

	// decide how much time an ACK/CTS frame will consume in the air
	AckDuration = RTMPCalcDuration(pAd, pAd->CommonCfg.ExpectedACKRate[pTxBlk->TxRate], 14);

	// Init the total payload length of this frame.
	SrcRemainingBytes = pTxBlk->SrcBufLen;
	
	pTxBlk->TotalFragNum = 0xff;

	do {

		FreeMpduSize = pAd->CommonCfg.FragmentThreshold - LENGTH_CRC;

		FreeMpduSize -= pTxBlk->MpduHeaderLen;

		if (SrcRemainingBytes <= FreeMpduSize)
		{	// this is the last or only fragment
		
			pTxBlk->SrcBufLen = SrcRemainingBytes;
			
			pHeader_802_11->FC.MoreFrag = 0;
			pHeader_802_11->Duration = pAd->CommonCfg.Dsifs + AckDuration;
			
			// Indicate the lower layer that this's the last fragment.
			pTxBlk->TotalFragNum = fragNum;
		}
		else
		{	// more fragment is required

			pTxBlk->SrcBufLen = FreeMpduSize;
			
			NextMpduSize = min(((UINT)SrcRemainingBytes - pTxBlk->SrcBufLen), ((UINT)pAd->CommonCfg.FragmentThreshold));
			pHeader_802_11->FC.MoreFrag = 1;
			pHeader_802_11->Duration = (3 * pAd->CommonCfg.Dsifs) + (2 * AckDuration) + RTMPCalcDuration(pAd, pTxBlk->TxRate, NextMpduSize + EncryptionOverhead);
		}

		if (fragNum == 0)
			pTxBlk->FrameGap = IFS_HTTXOP;
		else
			pTxBlk->FrameGap = IFS_SIFS;
		
		RTMPWriteTxWI_Data(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);
		
		//FreeNumber = GET_TXRING_FREENO(pAd, QueIdx);

		HAL_WriteFragTxResource(pAd, pTxBlk, fragNum, &FreeNumber);
		
		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;

		// Update the frame number, remaining size of the NDIS packet payload.
		if (fragNum == 0 && pTxBlk->pExtraLlcSnapEncap)
			pTxBlk->MpduHeaderLen -= LENGTH_802_1_H;	// space for 802.11 header.

		fragNum++;
		SrcRemainingBytes -= pTxBlk->SrcBufLen;
		pTxBlk->pSrcBufData += pTxBlk->SrcBufLen;
		
		pHeader_802_11->Frag++;	 // increase Frag #
		
	}while(SrcRemainingBytes > 0);

	//
	// Kick out Tx
	// 
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

#if 0	// remove to APBuildCommon802_11Header
	// Increase the sequence number.
	pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
#endif
		
}


VOID AP_ARalink_Frame_Tx(
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk)
{
	PUCHAR			pHeaderBufPtr;
//	UCHAR			QueIdx = pTxBlk->QueIdx;
	USHORT			FreeNumber;
	USHORT			totalMPDUSize=0;
	USHORT			FirstTx, LastTxIdx;
	int 			frameNum = 0;
	BOOLEAN			bVLANPkt;
	PQUEUE_ENTRY	pQEntry;


	ASSERT(pTxBlk);
	
	ASSERT((pTxBlk->TxPacketList.Number== 2));


	FirstTx = LastTxIdx = 0;  // Is it ok init they as 0?
	while(pTxBlk->TxPacketList.Head)
	{
		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
		if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE)
		{
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			continue;			
		}

		//pTxBlk->bVLANPkt = RTMP_GET_PACKET_VLAN(pTxBlk->pPacket);
		bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
		
		// skip 802.3 header
		pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
		pTxBlk->SrcBufLen  -= LENGTH_802_3;

		// skip vlan tag
		if (bVLANPkt)
		{
			pTxBlk->pSrcBufData	+= LENGTH_802_1Q;
			pTxBlk->SrcBufLen	-= LENGTH_802_1Q;
		}
		
		if (frameNum == 0)
		{	// For first frame, we need to create the 802.11 header + padding(optional) + RA-AGG-LEN + SNAP Header
		
			pHeaderBufPtr = AP_Build_ARalink_Frame_Header(pAd, pTxBlk);
			
			// It's ok write the TxWI here, because the TxWI->MPDUtotalByteCount 
			//	will be updated after final frame was handled.
			RTMPWriteTxWI_Data(pAd, (PTXWI_STRUC)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), pTxBlk);


			//
			// Insert LLC-SNAP encapsulation - 8 octets
			// 
			EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData-2, pTxBlk->pExtraLlcSnapEncap);

			if (pTxBlk->pExtraLlcSnapEncap)
			{
				NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
				pHeaderBufPtr += 6;
				// get 2 octets (TypeofLen)
				NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData-2, 2);
				pHeaderBufPtr += 2;
				pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
			}
		}
		else
		{	// For second aggregated frame, we need create the 802.3 header to headerBuf, because PCI will copy it to SDPtr0.
		
			pHeaderBufPtr = &pTxBlk->HeaderBuf[0];
			pTxBlk->MpduHeaderLen = 0;
			
			// A-Ralink sub-sequent frame header is the same as 802.3 header.
			//   DA(6)+SA(6)+FrameType(2)
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufHeader, 12);
			pHeaderBufPtr += 12;
			// get 2 octets (TypeofLen)
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData-2, 2);
			pHeaderBufPtr += 2;
			pTxBlk->MpduHeaderLen = LENGTH_ARALINK_SUBFRAMEHEAD;
		}

		totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
		
		//FreeNumber = GET_TXRING_FREENO(pAd, QueIdx);
		if (frameNum ==0)
			FirstTx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &FreeNumber);
		else
			LastTxIdx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &FreeNumber);

		frameNum++;
		
		pAd->RalinkCounters.OneSecTxAggregationCount++;
		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;
		
		// calculate Tx count and ByteCount per BSS
		if ((pTxBlk->pMacEntry->ValidAsCLI) && (pTxBlk->pMacEntry->apidx < pAd->ApCfg.BssidNum))
		{
			pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TransmittedByteCount += totalMPDUSize;
			pAd->ApCfg.MBSSID[pTxBlk->pMacEntry->apidx].TxCount ++;
		}
		
	}

	HAL_FinalWriteTxResource(pAd, pTxBlk, totalMPDUSize, FirstTx);
	HAL_LastTxIdx(pAd, pTxBlk->QueIdx, LastTxIdx);

#if 0	// remove to APBuildCommon802_11Header
	// Increase Sequence
	pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority] = (pTxBlk->pMacEntry->TxSeq[pTxBlk->UserPriority]+1) & MAXSEQ;
#endif

	//
	// Kick out Tx
	// 
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

}


/*
	========================================================================
	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware encryption before really
	sent out to air.

	Arguments:
		pAd 	   		Pointer to our adapter
		pTxBlk			Pointer to outgoing TxBlk structure.
		QueIdx			Queue index for processing

	Return Value:
		None
	========================================================================
*/
NDIS_STATUS APHardTransmit(	
	IN	PRTMP_ADAPTER	pAd,
	IN	TX_BLK			*pTxBlk,
	IN	UCHAR			QueIdx)
{
	PQUEUE_ENTRY	pQEntry;
	PNDIS_PACKET	pPacket;
//	PQUEUE_HEADER   pQueue;
	
	if ((pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE)
#ifdef CARRIER_DETECTION_SUPPORT
		||(isCarrierDetectExist(pAd) == TRUE)
#endif // CARRIER_DETECTION_SUPPORT //
		)
	{
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return NDIS_STATUS_FAILURE;
	}

	switch (pTxBlk->TxFrameType)
	{
		case TX_AMPDU_FRAME:
				AP_AMPDU_Frame_Tx(pAd, pTxBlk);
				break;
		case TX_LEGACY_FRAME:
		case TX_MCAST_FRAME:
				AP_Legacy_Frame_Tx(pAd, pTxBlk);
				break;
		case TX_AMSDU_FRAME:
				AP_AMSDU_Frame_Tx(pAd, pTxBlk);
				break;
		case TX_RALINK_FRAME:
				AP_ARalink_Frame_Tx(pAd, pTxBlk);
				break;
		case TX_FRAG_FRAME:
				AP_Fragment_Frame_Tx(pAd, pTxBlk);
				break;
		default:
			{
				// It should not happened!
				DBGPRINT(RT_DEBUG_ERROR, ("Send a pacekt was not classified!! It should not happen!\n"));
				while(pTxBlk->TxPacketList.Head)
				{	
					pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
					pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
					if (pPacket)
						RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				}
			}
			break;
	}

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
	IN	PRT28XX_RXD_STRUC		pRxD,
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
			else
			{
				// or transmit a (QoS) Null Frame
				BOOLEAN bQosNull = FALSE;

				if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
					bQosNull = TRUE;

	            ApEnqueueNullFrame(pAd, pMacEntry->Addr, pMacEntry->CurrTxRate,
    	                           Aid, pMacEntry->apidx, bQosNull, TRUE, 0);
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
		RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS);
		
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("rcv PS-POLL (AID=%d not match) from %02x:%02x:%02x:%02x:%02x:%02x\n", 
			  Aid, pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));

	}
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
		TCI = (mbss_p->VLAN_VID & 0x0fff) | ((mbss_p->VLAN_Priority & 0x7)<<13);

#ifndef RT_BIG_ENDIAN
		TCI = SWAP16(TCI);
#endif // RT_BIG_ENDIAN //

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
//static UCHAR is_on;
VOID detect_wmm_traffic(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UCHAR			UserPriority)
{
	/* For BE & BK case and TxBurst function is disabled */
	if ((pAd->CommonCfg.bEnableTxBurst == FALSE) &&
		(pAd->CommonCfg.bRdg == FALSE))
	{
		if (MapUserPriorityToAccessCategory[UserPriority] == QID_AC_BK)
		{
			/* has any BK traffic */
			if (pAd->flg_be_adjust == 0)
			{
				/* yet adjust */
				EDCA_AC_CFG_STRUC Ac0Cfg;

				RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &Ac0Cfg.word);
				Ac0Cfg.field.AcTxop = 0x20;
				RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Ac0Cfg.word);

				pAd->flg_be_adjust = 1;
				pAd->be_adjust_last_time = jiffies;

				DBGPRINT(RT_DEBUG_TRACE, ("wmm> adjust be!\n"));
			}
		}
		else
		{
			if (pAd->flg_be_adjust != 0)
			{
				PQUEUE_HEADER pQueue;

				/* has adjusted */
				pQueue = &pAd->TxSwQueue[QID_AC_BK];

				if ((pQueue == NULL) ||
					((pQueue != NULL) && (pQueue->Head == NULL)))
				{
					if ((jiffies - pAd->be_adjust_last_time) > TIME_ONE_SECOND)
					{
						/* no any BK traffic */
						EDCA_AC_CFG_STRUC Ac0Cfg;

						RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &Ac0Cfg.word);
						Ac0Cfg.field.AcTxop = 0x00;
						RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Ac0Cfg.word);

						pAd->flg_be_adjust = 0;

						DBGPRINT(RT_DEBUG_TRACE, ("wmm> recover be!\n"));
					}
				}
				else
					pAd->be_adjust_last_time = jiffies;
			}
		}
	}

	// count packets which priority is more than BE 
	if (UserPriority > 3)
	{
		pAd->OneSecondnonBEpackets++;

		if (pAd->MacTab.fAnyStationMIMOPSDynamic && pAd->OneSecondnonBEpackets > 100)
		{
			if (!pAd->is_on)
			{
				RTMP_IO_WRITE32(pAd,  EXP_ACK_TIME,	 0x005400ca );
				pAd->is_on = 1;
			}
		}
		else
		{
			if (pAd->is_on)
			{
				RTMP_IO_WRITE32(pAd,  EXP_ACK_TIME,	 0x002400ca );
				pAd->is_on = 0;
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
	UINT32 RegValue;
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
				// For CWC test, change txop from 0x30 to 0x20 in TxBurst mode
				RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE) ? (txop_value = 0x80) : (txop_value = 0x20);
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
	PRT28XX_RXD_STRUC			pRxD = &(pRxBlk->RxD);
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
      				RT28XX_HANDLE_COUNTER_MEASURE(pAd, pEntry);
      			}

			}

			// send wireless event - for icv error
			if (pEntry && ((pRxD->CipherErr & 1) == 1) && pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_ICV_ERROR_EVENT_FLAG, pEntry->Addr, 0, 0); 
		}
#ifdef RT2860
		DBGPRINT(RT_DEBUG_TRACE, ("Rx u2me DATA Cipher Err(SDL0=%d, MPDUsize=%d, WCID=%d, CipherErr=%d)\n", pRxD->SDL0, pRxWI->MPDUtotalByteCount, pRxWI->WirelessCliID, pRxD->CipherErr));
#endif // RT2860 //
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

// For TKIP frame, calculate the MIC value						
BOOLEAN APCheckTkipMICValue(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry,
	IN	RX_BLK			*pRxBlk)
{
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	UCHAR			*pData = pRxBlk->pData;
	USHORT			DataSize = pRxBlk->DataSize;
	UCHAR			UserPriority = pRxBlk->UserPriority;
	PCIPHER_KEY		pWpaKey;
	UCHAR			*pDA, *pSA;

	pWpaKey = &pEntry->PairwiseKey;

	if (RX_BLK_TEST_FLAG(pRxBlk, fRX_WDS))
	{
		pDA = pHeader->Addr3;
		pSA = (PUCHAR)pHeader + sizeof(HEADER_802_11);
	}
	else if (RX_BLK_TEST_FLAG(pRxBlk, fRX_APCLI))
	{
		pDA = pHeader->Addr1;
		pSA = pHeader->Addr3;		
	}
	else 
	{
		pDA = pHeader->Addr3;
		pSA = pHeader->Addr2;
	}

	if (RTMPTkipCompareMICValue(pAd,
								pData,
								pDA,
								pSA,
								pWpaKey->RxMic,
								UserPriority,
								DataSize) == FALSE)
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR,("Rx MIC Value error 2\n"));
		RT28XX_HANDLE_COUNTER_MEASURE(pAd, pEntry);
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
	PRT28XX_RXD_STRUC		pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC		pRxWI = pRxBlk->pRxWI;
	PHEADER_802_11	pHeader = pRxBlk->pHeader;
	PNDIS_PACKET	pRxPacket = pRxBlk->pRxPacket;

	do
	{

#ifdef IDS_SUPPORT	
		// Check if a rogue AP impersonats our mgmt frame to spoof clients 	
		if (RTMPSpoofedMgmtDetection(pAd, pHeader, pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2))
		{
			// This is a spoofed frame, so give up it.
			break;
		}
#endif // IDS_SUPPORT //

#ifdef IDS_SUPPORT
		// update sta statistics for traffic flooding detection later		
		RTMPUpdateStaMgmtCounter(pAd, pHeader->FC.SubType);
#endif // IDS_SUPPORT //
			
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

		if (pRxBlk->DataSize > MAX_RX_PKT_LEN)
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
	PRT28XX_RXD_STRUC		pRxD = &(pRxBlk->RxD);
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
	if (!RTMPCheckWPAframe(pAd, pEntry, pRxBlk->pData, pRxBlk->DataSize, FromWhichBSSID))
	{
		// drop all non-EAP DATA frame before
		// this client's Port-Access-Control is secured
		if (pEntry->PrivacyFilter == Ndis802_11PrivFilter8021xWEP)
		{
			// Special condition 
			// If received an encrypted non-EAP frame from peer associated STA and ASIC can't handle it,
			// AP would send de-authentication to this STA.
			if (pEntry->ValidAsCLI && (pEntry->IsReassocSta == FALSE) && pRxBlk->pHeader->FC.Wep && pRxBlk->RxD.Decrypted == 0)
			{		
				DBGPRINT(RT_DEBUG_WARN, ("==> De-Auth this STA(%02x:%02x:%02x:%02x:%02x:%02x)\n", PRINT_MAC(pEntry->Addr)));	
				MlmeDeAuthAction(pAd, pEntry, REASON_NO_LONGER_VALID);	             													
			}
		
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
			UINT16 protoType = OS_NTOHS(*((UINT16 *)(pData)));

			if (protoType == ETH_P_IP)
				IGMPSnooping(pAd, pDA, pSA,	pData, pAd->ApCfg.MBSSID[pEntry->apidx].MSSIDDev);
			else if (protoType == ETH_P_IPV6)
				MLDSnooping(pAd, pDA, pSA,	pData, pAd->ApCfg.MBSSID[pEntry->apidx].MSSIDDev);
		}
#endif // IGMP_SNOOP_SUPPORT //

		RX_BLK_CLEAR_FLAG(pRxBlk, fRX_EAP);
		if (!RX_BLK_TEST_FLAG(pRxBlk, fRX_ARALINK))
		{
			// Normal legacy, AMPDU or AMSDU
			CmmRxnonRalinkFrameIndicate(pAd, pRxBlk, FromWhichBSSID);
		}
		else
		{
			// ARALINK
			CmmRxRalinkFrameIndicate(pAd, pEntry, pRxBlk, FromWhichBSSID);
		}
	}
	else 
	{
		RX_BLK_SET_FLAG(pRxBlk, fRX_EAP);
		if (RX_BLK_TEST_FLAG(pRxBlk, fRX_AMPDU) && (pAd->CommonCfg.bDisableReordering == 0)) 
		{			
			Indicate_AMPDU_Packet(pAd, pRxBlk, FromWhichBSSID);
		} 
		else
		{
			// Determin the destination of the EAP frame
			//  to WPA state machine or upper layer
			APRxEAPOLFrameIndicate(pAd, pEntry, pRxBlk, FromWhichBSSID);
		}
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
	PRT28XX_RXD_STRUC						pRxD = &(pRxBlk->RxD);
	PRXWI_STRUC						pRxWI = pRxBlk->pRxWI;
	PHEADER_802_11					pHeader = pRxBlk->pHeader;
	PNDIS_PACKET					pRxPacket = pRxBlk->pRxPacket;
	BOOLEAN 						bFragment = FALSE;
	MAC_TABLE_ENTRY	    			*pEntry = NULL;
	UCHAR							FromWhichBSSID = BSS0;
	UCHAR							OldPwrMgmt = PWR_ACTIVE;	// UAPSD AP SUPPORT
	UCHAR							UserPriority = 0;
#ifdef WDS_SUPPORT
	BOOLEAN							bWdsPacket = FALSE;
#endif // WDS_SUPPORT //


	if (APCheckVaildDataFrame(pAd, pRxBlk) != TRUE)
	{
		goto err;		
	}

#ifdef IDS_SUPPORT
	// replay attack detection
	// Detect a spoofed data frame from a rogue AP, ignore it.
	if (pHeader->FC.FrDs == 1 && 
		(RTMPReplayAttackDetection(pAd, pHeader->Addr2, pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2) == TRUE))
	{
		goto err;
	}
#endif // IDS_SUPPORT //

	//
	// handle WDS
	//
	if ((pHeader->FC.FrDs == 1) && (pHeader->FC.ToDs == 1))
	{
		do
		{

#ifdef WDS_SUPPORT
			// handle WDS
			{
				bWdsPacket = TRUE;
				if (MAC_ADDR_EQUAL(pHeader->Addr1, pAd->CurrentAddress))
					pEntry = FindWdsEntry(pAd, pRxWI->WirelessCliID, pHeader->Addr2, pRxWI->PHYMODE);
				else
					pEntry = NULL;


				// have no valid wds entry exist 
				// then discard the incoming packet.
				if (!(pEntry && WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx)))
				{
					// drop the packet
					goto err;
				}

				RX_BLK_SET_FLAG(pRxBlk, fRX_WDS);
				FromWhichBSSID = pEntry->MatchWDSTabIdx + MIN_NET_DEVICE_FOR_WDS;
				break;
			}
#endif // WDS_SUPPORT //
		} while(FALSE);

		if (pEntry == NULL)
		{
			// have no WDS or MESH support
			// drop the packet
			goto err;
		}
	}
	// handle APCLI.
	else if ((pHeader->FC.FrDs == 1) && (pHeader->FC.ToDs == 0))
	{
#ifdef APCLI_SUPPORT
		if ((VALID_WCID(pRxWI->WirelessCliID)
			&& (pAd->MacTab.Content[pRxWI->WirelessCliID].ValidAsApCli)))
		{
			pEntry = ApCliTableLookUpByWcid(pAd, pRxWI->WirelessCliID, pHeader->Addr2);

			if (!(pEntry && APCLI_IF_UP_CHECK(pAd, pEntry->MatchAPCLITabIdx)))
			{
				goto err;
			}
			FromWhichBSSID = pEntry->MatchAPCLITabIdx + MIN_NET_DEVICE_FOR_APCLI;
			RX_BLK_SET_FLAG(pRxBlk, fRX_APCLI);

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
		}
		else
#endif // APCLI_SUPPORT //
		{
			// no APCLI support
			// release packet
			goto err;
		}
	}
	else
	{
		pEntry = PACInquiry(pAd, pRxWI->WirelessCliID);

		//	can't find associated STA entry then filter invlid data frame 
		if (!pEntry)
		{		
			goto err;
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
		// Increase received drop packet counter per BSS
		if (pHeader->FC.FrDs == 0 &&
			pRxD->U2M &&
			pRxWI->BSSID < pAd->ApCfg.BssidNum)
		{
			pAd->ApCfg.MBSSID[pRxWI->BSSID].RxDropCount ++;			
		}

		// release packet
		RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
		return;
	}

	//
	// update RxBlk->pData, DataSize
	// 802.11 Header, QOS, HTC, Hw Padding
	// 

	// 1. skip 802.11 HEADER
	// 1. skip 802.11 HEADER
	if ( FALSE
#ifdef WDS_SUPPORT
		|| bWdsPacket
#endif // WDS_SUPPORT //
		)
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
		UserPriority = *(pRxBlk->pData) & 0x0f;
		// count packets priroity more than BE
		detect_wmm_traffic(pAd, UserPriority);
		// bit 7 in QoS Control field signals the HT A-MSDU format
		if ((*pRxBlk->pData) & 0x80)
		{
			RX_BLK_SET_FLAG(pRxBlk, fRX_AMSDU);

			// calculate received AMSDU count and ByteCount
			pAd->RalinkCounters.ReceivedAMSDUCount.u.LowPart ++;

			if ( FALSE
#ifdef WDS_SUPPORT
				|| bWdsPacket
#endif // WDS_SUPPORT //
				)
			{
				pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart += (pRxBlk->DataSize + LENGTH_802_11_WITH_ADDR4);
			}
			else
			{
				pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart += (pRxBlk->DataSize + LENGTH_802_11);
			}
		}

		// skip QOS contorl field
		pRxBlk->pData += 2;
		pRxBlk->DataSize -=2;
	}
	pRxBlk->UserPriority = UserPriority;

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
			// Minus MIC length
			pRxBlk->DataSize -= 8;

			// For TKIP frame, calculate the MIC value						
			if (APCheckTkipMICValue(pAd, pEntry, pRxBlk) == FALSE)
			{
				return;
			}
		}

#ifdef IKANOS_VX_1X0
		RTMP_SET_PACKET_IF(pRxPacket, FromWhichBSSID);
#endif // IKANOS_VX_1X0 //
		APRxDataFrameAnnounce(pAd, pEntry, pRxBlk, FromWhichBSSID);
	}
	else
	{
		// just return 
		// because RTMPDeFragmentDataFrame() will release rx packet, 
		// if packet is fragmented
		return;
	}
	return;

err:
	// Increase received error packet counter per BSS
	if (pHeader->FC.FrDs == 0 &&
		pRxD->U2M &&
		pRxWI->BSSID < pAd->ApCfg.BssidNum)
	{
		pAd->ApCfg.MBSSID[pRxWI->BSSID].RxDropCount ++;
		pAd->ApCfg.MBSSID[pRxWI->BSSID].RxErrorCount ++;
	}

	// release packet
	RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_FAILURE);
	return;
	
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
	RT28XX_RXD_STRUC		*pRxD;
	UCHAR			*pData;
	PRXWI_STRUC		pRxWI;
	PNDIS_PACKET	pRxPacket;
	PHEADER_802_11	pHeader;
	RX_BLK			RxCell;


	RxProcessed = RxPending = 0;

	// process whole rx ring
	while (1)
	{

		if (RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RADIO_OFF |
								fRTMP_ADAPTER_RESET_IN_PROGRESS |
									fRTMP_ADAPTER_HALT_IN_PROGRESS)) || 
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
#ifdef RT2860
			NICUpdateFifoStaCounters(pAd);
#endif // RT2860 //
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
		pRxWI	= (PRXWI_STRUC)pData;
		pHeader = (PHEADER_802_11)(pData+RXWI_SIZE);

#ifdef RT_BIG_ENDIAN
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

		// Increase received byte counter per BSS
		if (pHeader->FC.FrDs == 0 &&
			pRxD->U2M &&
			pRxWI->BSSID < pAd->ApCfg.BssidNum)
		{
			pAd->ApCfg.MBSSID[pRxWI->BSSID].ReceivedByteCount +=  pRxWI->MPDUtotalByteCount;
			pAd->ApCfg.MBSSID[pRxWI->BSSID].RxCount ++;
		}

#ifdef RALINK_ATE
		if (ATE_ON(pAd))
		{
			pAd->ate.RxCntPerSec++;
			ATESampleRssi(pAd, pRxWI);
#ifdef RALINK_28xx_QA
			if (pAd->ate.bQARxStart == TRUE)
			{
				/* GetPacketFromRxRing() has copy the endian-changed RxD if it is necessary. */
				ATE_QA_Statistics(pAd, pRxWI, pRxD,	pHeader);
			}
#endif // RALINK_28xx_QA //
			RELEASE_NDIS_PACKET(pAd, pRxPacket, NDIS_STATUS_SUCCESS);
			continue;
		}
#endif // RALINK_ATE //
		
		// Check for all RxD errors
		if (APCheckRxError(pAd, pRxD, pRxWI->WirelessCliID) != NDIS_STATUS_SUCCESS)
		{
			APRxDErrorHandle(pAd, &RxCell);

			// Increase received error packet counter per BSS
			if (pHeader->FC.FrDs == 0 &&
				pRxD->U2M &&
				pRxWI->BSSID < pAd->ApCfg.BssidNum)
			{
				pAd->ApCfg.MBSSID[pRxWI->BSSID].RxDropCount ++;
				pAd->ApCfg.MBSSID[pRxWI->BSSID].RxErrorCount ++;
			}
			
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
#ifdef DBG_DIAGNOSE
//						if (pRxWI->MCS < 16)
						if (pRxWI->MCS < 24) // 3*3
						{	// The RxMCS must be smaller than 16.
							pAd->DiagStruct.RxDataCnt[pAd->DiagStruct.ArrayCurIdx]++;
							pAd->DiagStruct.RxMcsCnt[pAd->DiagStruct.ArrayCurIdx][pRxWI->MCS]++;
						}
#endif // DBG_DIAGNOSE //
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
    MAC_TABLE_ENTRY	*pEntry = NULL;
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
	else
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

		{
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
		}
		RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS); 	// Dequeue outgoing frames from TxSwQueue0..3 queue and process it
	}
	
	return bAnnounce;
}

