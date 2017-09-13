/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************
 
    Module Name:
    mlme.c
 
    Abstract:
    Major MLME state machiones here
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"
#include <stdarg.h>


void ReturnContrlChannel(IN PRTMP_ADAPTER pAd);
void DetectExtChannel(IN PRTMP_ADAPTER pAd);
//BOOLEAN isExtChannel;
int DetectOverlappingPeriodicRound;


void ReturnContrlChannel(
   	IN PRTMP_ADAPTER pAd)
{

//	ULONG		Value, byteValue = 0;

    APSwitchChannel(pAd, pAd->CommonCfg.Channel);

	printk("***********************\n");
#if 0
   	if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE))
	{
		//  RX : control channel at lower 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue &= (~0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);
	}
	else if ((pAd->CommonCfg.Channel > 2) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW))
	{
		//  RX : control channel at upper 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue |= (0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);
	}

    RTMPusecDelay(50);
#endif

//   	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &Value);
//		Value |= 0x00000001;
//		RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, Value);

    pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
    RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
	RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS);
}


extern VOID Send_Probe_Request(
	IN PRTMP_ADAPTER pAd);

void DetectExtChannel(
   	IN PRTMP_ADAPTER pAd)
{
   	UINT32		Value;
   	UCHAR		byteValue = 0;

//	RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &Value);
//	Value &= 0xFFFFFFFE;
//	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, Value);

	printk("$$$$$$$$$$$$$$$$\n");
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
    pAd->Mlme.ApSyncMachine.CurrState = AP_SCAN_LISTEN;

	RTMPusecDelay(50);

	//APSwitchChannel(pAd, 52);

#if 1

	if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE))
	{
   		//  TX : control channel at upper 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value |= (0x1);		
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at upper 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue |= (0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);
	}
	else if ((pAd->CommonCfg.Channel > 2) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW))
	{
		//  TX : control channel at lower 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x1);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at lower 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue &= (~0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);
	}
#endif

	Send_Probe_Request(pAd);
    Send_Probe_Request(pAd);
//	AsicSwitchChannel(pAd, CentralChannel, TRUE);
}

VOID APDetectOverlappingExec(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3) 
{
	PRTMP_ADAPTER	pAd = (RTMP_ADAPTER *)FunctionContext;

	if (DetectOverlappingPeriodicRound == 0)
	{
		// switch back 20/40		
		if ((pAd->CommonCfg.Channel <=14) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40))
		{
            pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;	
			pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;			
		}
	}
	else
	{
		if ((DetectOverlappingPeriodicRound == 25) || (DetectOverlappingPeriodicRound == 1))
		{   
			
   			if ((pAd->CommonCfg.Channel <=14) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth==BW_40))
			{                                     
				SendBeaconRequest(pAd, 1);			
				SendBeaconRequest(pAd, 2);
                SendBeaconRequest(pAd, 3);
			}

		}
		DetectOverlappingPeriodicRound--;
	}


#if 0
	if (DetectOverlappingPeriodicRound % (250*2) == 0)
	{
		isExtChannel = TRUE;
		DetectExtChannel(pAd);
	}
	else
	{
		if (isExtChannel)
		{
			ReturnContrlChannel(pAd);
			isExtChannel = FALSE;
		}
	}
	DetectOverlappingPeriodicRound++;
#endif

}


/*
    ==========================================================================
    Description:
        This routine is executed every second -
        1. Decide the overall channel quality
        2. Check if need to upgrade the TX rate to any client
        3. perform MAC table maintenance, including ageout no-traffic clients, 
           and release packet buffer in PSQ is fail to TX in time.
    ==========================================================================
 */
VOID APMlmePeriodicExec(
    PRTMP_ADAPTER pAd)
{
    // Reqeust by David 2005/05/12
    // It make sense to disable Adjust Tx Power on AP mode, since we can't take care all of the client's situation    
    // ToDo: need to verify compatibility issue with WiFi product.
    // 
//
// We return here in ATE mode, because the statistics 
// that ATE need are not collected via this routine.
//
#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	// Disable Adjust Tx Power for WPA WiFi-test. 
	// Because high TX power results in the abnormal disconnection of Intel BG-STA.  
//#ifndef WIFI_TEST    
	if (pAd->CommonCfg.bWiFiTest == FALSE)	
		AsicAdjustTxPower(pAd);
//#endif // WIFI_TEST //

	/* BBP TUNING: dynamic tune BBP R66 to find a balance between sensibility
		and noise isolation */
//	AsicBbpTuning2(pAd);

    // walk through MAC table, see if switching TX rate is required
#if 0 // move to mlme.c::MlmePeriodicExec
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
        APMlmeDynamicTxRateSwitching(pAd);
#endif

    // MAC table maintenance
	if (pAd->Mlme.PeriodicRound % MLME_TASK_EXEC_MULTIPLE == 0)
	{
		// one second timer
	    MacTableMaintenance(pAd);
		RTMPMaintainPMKIDCache(pAd);

#ifdef WDS_SUPPORT
		WdsTableMaintenance(pAd);
#endif // WDS_SUPPORT //

#ifdef MESH_SUPPORT
		if(MESH_ON(pAd))
		{
			LONG idx;
			// period update Neighbor table.
			NeighborTableUpdate(pAd);
			// update Mesh Link
			MeshLinkTableMaintenace(pAd);
			// update Mesh multipath entry.
			for (idx = 0; idx < MAX_MESH_LINKS; idx ++)
			{
				if (PeerLinkValidCheck(pAd, idx))
					MultipathEntryMaintain(pAd, idx);
			}
		}
#endif // MESH_SUPPORT //
	}
	
	APUpdateCapabilityAndErpIe(pAd);


//#ifdef WIFI_PLUGFEST

#ifdef APCLI_SUPPORT
	if (pAd->Mlme.OneSecPeriodicRound % 2 == 0)
		ApCliIfMonitor(pAd);

	if (pAd->Mlme.OneSecPeriodicRound % 2 == 1)
		ApCliIfUp(pAd);
#endif // APCLI_SUPPORT //

    if (pAd->CommonCfg.bHTProtect)
    {
    	//APUpdateCapabilityAndErpIe(pAd);
    	APUpdateOperationMode(pAd);
		if (pAd->CommonCfg.IOTestParm.bRTSLongProtOn == FALSE)
		{
        	AsicUpdateProtect(pAd, (USHORT)pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, ALLN_SETPROTECT, FALSE, pAd->MacTab.fAnyStationNonGF);
    	}
    }
//#endif

#ifdef CONFIG_AP_SUPPORT
	if ( (pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		)
	{
		ApRadarDetectPeriodic(pAd);
	}

#ifdef DFS_SUPPORT
	if ( (pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& RadarChannelCheck(pAd, pAd->CommonCfg.Channel)
		&& OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)
		&& (pAd->CommonCfg.RadarDetect.RDMode == RD_NORMAL_MODE))
	{
		AdaptRadarDetection(pAd);
	}
#endif // DFS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
}

VOID APMlmeSelectTxRateTable(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PUCHAR				*ppTable,
	IN PUCHAR				pTableSize,
	IN PUCHAR				pInitTxRateIdx)
{
	MlmeSelectTxRateTableEx(pAd, pEntry, ppTable, pTableSize, pInitTxRateIdx);
}

VOID APMlmeSetTxRate(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PRTMP_TX_RATE_SWITCH	pTxRate)
{
	MlmeSetTxRateEx(pAd, pEntry, pTxRate);
}

/*
    ==========================================================================
    Description:
        This routine walks through the MAC table, see if TX rate change is 
        required for each associated client. 
    Output:
        pEntry->CurrTxRate - 
    NOTE:
        call this routine every second
    ==========================================================================
 */
VOID APMlmeDynamicTxRateSwitching(
    IN PRTMP_ADAPTER pAd)
{
	MlmeDynamicTxRateSwitchingEx(pAd, 1);
}

/*! \brief   To substitute the message type if the message is coming from external
 *  \param  *Fr            The frame received
 *  \param  *Machine       The state machine
 *  \param  *MsgType       the message type for the state machine
 *  \return TRUE if the substitution is successful, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN APMsgTypeSubst(
    IN PRTMP_ADAPTER pAd,
    IN PFRAME_802_11 pFrame, 
    OUT INT *Machine, 
    OUT INT *MsgType) 
{
    USHORT Seq;
    UCHAR  EAPType;
    BOOLEAN     Return = FALSE;
#ifdef WSC_AP_SUPPORT
	UCHAR EAPCode;
    PMAC_TABLE_ENTRY pEntry;
#endif // WSC_AP_SUPPORT //

//TODO:
// only PROBE_REQ can be broadcast, all others must be unicast-to-me && is_mybssid; otherwise, 
// ignore this frame

    // wpa EAPOL PACKET
    if (pFrame->Hdr.FC.Type == BTYPE_DATA) 
    {    
#ifdef WSC_AP_SUPPORT    
        //WSC EAPOL PACKET        
        pEntry = MacTableLookup(pAd, pFrame->Hdr.Addr2);
        if ((pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx != WSC_INIT_ENTRY_APIDX) &&
            pEntry && 
            (pEntry->ValidAsCLI) && 
            (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE))
        {
            *Machine = WSC_STATE_MACHINE;
            EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
            EAPCode = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 4);
            Return = WscMsgTypeSubst(EAPType, EAPCode, MsgType);
        }
        if (!Return)
        {
#endif // WSC_AP_SUPPORT //
        *Machine = AP_WPA_STATE_MACHINE;
        EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
            Return = APWpaMsgTypeSubst(EAPType, (INT *) MsgType);
#ifdef WSC_AP_SUPPORT            
        }
#endif // WSC_AP_SUPPORT //
        return Return;
    }
    
    if (pFrame->Hdr.FC.Type != BTYPE_MGMT)
        return FALSE;
    
    switch (pFrame->Hdr.FC.SubType) 
    {
        case SUBTYPE_ASSOC_REQ:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_ASSOC_REQ;
            DBGPRINT_RAW(RT_DEBUG_INFO,("SUBTYPE_ASSOC_REQ\n"));
            break;
//      case SUBTYPE_ASSOC_RSP:
//          *Machine = AP_ASSOC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_ASSOC_RSP;
//          break;
        case SUBTYPE_REASSOC_REQ:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_REASSOC_REQ;
            break;
//      case SUBTYPE_REASSOC_RSP:
//          *Machine = AP_ASSOC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_REASSOC_RSP;
//          break;
        case SUBTYPE_PROBE_REQ:
            *Machine = AP_SYNC_STATE_MACHINE;              
            *MsgType = APMT2_PEER_PROBE_REQ;
            break;
// test for 40Mhz intolerant
#if 0
		case SUBTYPE_PROBE_RSP:
          *Machine = AP_SYNC_STATE_MACHINE;
          *MsgType = APMT2_PEER_PROBE_RSP;
          break;
#endif
        case SUBTYPE_BEACON:
            *Machine = AP_SYNC_STATE_MACHINE;
            *MsgType = APMT2_PEER_BEACON;
            break;
//      case SUBTYPE_ATIM:
//          *Machine = AP_SYNC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_ATIM;
//          break;
        case SUBTYPE_DISASSOC:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_DISASSOC_REQ;
            break;
        case SUBTYPE_AUTH:
            // get the sequence number from payload 24 Mac Header + 2 bytes algorithm
            NdisMoveMemory(&Seq, &pFrame->Octet[2], sizeof(USHORT));
            DBGPRINT(RT_DEBUG_INFO,("AUTH seq=%d Octet=%02x %02x %02x %02x %02x %02x %02x %02x\n", Seq,
                pFrame->Octet[0], pFrame->Octet[1], pFrame->Octet[2], pFrame->Octet[3], 
                pFrame->Octet[4], pFrame->Octet[5], pFrame->Octet[6], pFrame->Octet[7]));
            if (Seq == 1 || Seq == 3) 
            {
                *Machine = AP_AUTH_RSP_STATE_MACHINE;
                *MsgType = APMT2_PEER_AUTH_ODD;
            } 
//          else if (Seq == 2 || Seq == 4) 
//          {
//              *Machine = AP_AUTH_STATE_MACHINE;
//              *MsgType = APMT2_PEER_AUTH_EVEN;
//          } 
            else 
            {
                DBGPRINT(RT_DEBUG_TRACE,("wrong AUTH seq=%d Octet=%02x %02x %02x %02x %02x %02x %02x %02x\n", Seq,
                    pFrame->Octet[0], pFrame->Octet[1], pFrame->Octet[2], pFrame->Octet[3], 
                    pFrame->Octet[4], pFrame->Octet[5], pFrame->Octet[6], pFrame->Octet[7]));
                return FALSE;
            }
            break;
        case SUBTYPE_DEAUTH:
            *Machine = AP_AUTH_RSP_STATE_MACHINE;
            *MsgType = APMT2_PEER_DEAUTH;
            break;
	case SUBTYPE_ACTION:
		*Machine = ACTION_STATE_MACHINE;
		//  Sometimes Sta will return with category bytes with MSB = 1, if they receive catogory out of their support
		if ((pFrame->Octet[0]&0x7F) > MAX_PEER_CATE_MSG) 
		{
			*MsgType = MT2_ACT_INVALID;
		} 
		else
		{
			*MsgType = (pFrame->Octet[0]&0x7F);
		} 
		break;
        default:
            return FALSE;
            break;
    }

    return TRUE;
}

#if 0 // move to mlme.c 
/*
    ========================================================================
    Routine Description:
        Set/reset MAC registers according to bPiggyBack parameter
        
    Arguments:
        pAd         - Adapter pointer
        bPiggyBack  - Enable / Disable Piggy-Back
        
    Return Value:
        None
        
    ========================================================================
*/
VOID RTMPSetPiggyBack(
    IN PRTMP_ADAPTER    pAd,
    IN BOOLEAN          bPiggyBack)
{
	TX_LINK_CFG_STRUC  TxLinkCfg;
    
	RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
//	printk("TX_LINK_CFG = %08x\n", TxLinkCfg.word);
	TxLinkCfg.field.TxCFAckEn = bPiggyBack;
	RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);
}
#endif


/*
    ========================================================================
    Routine Description:
        Periodic evaluate antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APAsicEvaluateRxAnt(
	IN PRTMP_ADAPTER	pAd)
{
	UCHAR	BBPR3 = 0;
	ULONG	TxTotalCnt;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	DBGPRINT(RT_DEBUG_INFO, ("AsicEvaluateRxAnt : RealRxPath=%d \n", pAd->Mlme.RealRxPath));
	
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
	BBPR3 &= (~0x18);
	if(pAd->Antenna.field.RxPath == 3)
	{
		BBPR3 |= (0x10);
	}
	else if(pAd->Antenna.field.RxPath == 2)
	{
		BBPR3 |= (0x8);
	}
	else if(pAd->Antenna.field.RxPath == 1)
	{
		BBPR3 |= (0x0);
	}
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);

	TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					pAd->RalinkCounters.OneSecTxRetryOkCount + 
					pAd->RalinkCounters.OneSecTxFailCount;

	if (TxTotalCnt > 50)
	{
		RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 20);
		pAd->Mlme.bLowThroughput = FALSE;
	}
	else
	{
		RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 300);
		pAd->Mlme.bLowThroughput = TRUE;
	}
}

/*
    ========================================================================
    Routine Description:
        After evaluation, check antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APAsicRxAntEvalTimeout(
	PRTMP_ADAPTER	pAd) 
{
	UCHAR			BBPR3 = 0;
	CHAR			larger = -127, rssi0, rssi1, rssi2;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	// if the traffic is low, use average rssi as the criteria
	if (pAd->Mlme.bLowThroughput == TRUE)
	{
		rssi0 = pAd->ApCfg.RssiSample.LastRssi0;
		rssi1 = pAd->ApCfg.RssiSample.LastRssi1;
		rssi2 = pAd->ApCfg.RssiSample.LastRssi2;
	}
	else
	{
		rssi0 = pAd->ApCfg.RssiSample.AvgRssi0;
		rssi1 = pAd->ApCfg.RssiSample.AvgRssi1;
		rssi2 = pAd->ApCfg.RssiSample.AvgRssi2;
	}

	if(pAd->Antenna.field.RxPath == 3)
	{
		larger = max(rssi0, rssi1);

		if (larger > (rssi2 + 20))
			pAd->Mlme.RealRxPath = 2;
		else
			pAd->Mlme.RealRxPath = 3;
	}
	// Disable the below to fix 1T/2R issue. It's suggested by Rory at 2007/7/11.
#if 0	
	else if(pAd->Antenna.field.RxPath == 2)
	{
		if (rssi0 > (rssi1 + 20))
			pAd->Mlme.RealRxPath = 1;
		else
			pAd->Mlme.RealRxPath = 2;
	}
#endif

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
	BBPR3 &= (~0x18);
	if(pAd->Mlme.RealRxPath == 3)
	{
		BBPR3 |= (0x10);
	}
	else if(pAd->Mlme.RealRxPath == 2)
	{
		BBPR3 |= (0x8);
	}
	else if(pAd->Mlme.RealRxPath == 1)
	{
		BBPR3 |= (0x0);
	}
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);

	DBGPRINT(RT_DEBUG_INFO, ("APAsicRxAntEvalTimeout : RealRxPath=%d, AvgRssi0=%d, AvgRssi1=%d, AvgRssi2=%d \n",
		pAd->Mlme.RealRxPath, rssi0, rssi1, rssi2));
}

/*
	==========================================================================
	Description: BBP TUNING
		dynamic tune BBP R66 to find a balance between sensibility and 
		noise isolation

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID AsicBbpTuning1(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR	R66, R66UpperBound = 0x30, R66LowerBound = 0x30; /* default bound */
	CHAR	Rssi;

/* Disable BBP Tuning when ATE is running. */
#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	/* This algorithm is trying to compensate the effect of board (system)
		noise issue. */


	/* 2860C did not support Fase CCA, therefore can't tune */
	if (pAd->BbpTuning.bEnable == FALSE)
		return;
	/* End of if */

	/* get current R66 value, BbpWriteLatch[] be will updated in
		RTMP_BBP_IO_WRITE8_BY_REG_ID() */
	R66 = pAd->BbpWriteLatch[66];


	/* find the maximum RSSI from the three receive antennas */
	Rssi = RTMPMaxRssi(pAd, (CHAR)pAd->ApCfg.RssiSample.AvgRssi0,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi1,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi2);

	/* get R66LowerBound & R66UpperBound based on RSSI as below figure */
	/* (20 & 40MHz)
		R66
		0x48	|------------------------
				|						|
				|						|
				|						|
		0x38	|------------------------------->
						-70			-80		-90 RSSI
	 */
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		/* we are in connection state */
#if 0
		if (pAd->CommonCfg.BBPCurrentBW == BW_20)
		{
			/* BW=20MHz */
			if (Rssi <= RSSI_FOR_MID_SENSIBILITY) /* <= -90 */
			{
				/* weak RSSI, use largest gain */

				/* GET_LNA_GAIN is BLNAGain (bg channel <=14),
					ALNAGain0 (<=64), ALNAGain1 (<=128), or ALNAGain2 (>128)
					got from EEPROM tunning in Mass Production for different board */
				if (R66 != (0x26 + GET_LNA_GAIN(pAd)))
				{
					R66 = (0x26 + GET_LNA_GAIN(pAd));
					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
				} /* End of if */

				DBGPRINT(RT_DEBUG_TRACE, ("weak RSSI=%d, CCA=%ld, BW=%d, fixed R66 at 0x%x\n", 
						 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
				return;
			}
			else if (Rssi >= RSSI_FOR_MID_LOW_SENSIBILITY) /* >= -80 */
			{
				/* adjustable steps = 0x18 / 4 = 6, one step is 0x4 */
				R66UpperBound = (0x26 + GET_LNA_GAIN(pAd) + 0x18);
				R66LowerBound = (0x26 + GET_LNA_GAIN(pAd));
			}

			else
			{
				/* adjustable steps = 0x8 / 4 = 2, one step is 0x4 */
				R66UpperBound = (0x26 + GET_LNA_GAIN(pAd) + 0x8);
				R66LowerBound = (0x26 + GET_LNA_GAIN(pAd));
			} /* End of if */
		}
		else
#endif
		{
			/* BW=40MHz */
			if (Rssi <= RSSI_FOR_MID_LOW_SENSIBILITY) /* <= -80 */
			{
				/* weak RSSI, use largest gain */

				if (R66 != (0x2E + GET_LNA_GAIN(pAd)))
				{
					R66 = (0x2E + GET_LNA_GAIN(pAd) + 0x08);
					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
				} /* End of if */

				DBGPRINT(RT_DEBUG_TRACE, ("weak RSSI=%d, CCA=%d, BW=%d, fixed R66 at 0x%x\n", 
						 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
				return;
			}
			else
			{
				/* adjustable steps = 0x10 / 4 = 4, one step is 0x4 */
				R66UpperBound = (0x2E + GET_LNA_GAIN(pAd) + 0x18);
				R66LowerBound = (0x2E + GET_LNA_GAIN(pAd) + 0x08);
			} /* End of if */
		} /* End of if */
	} /* End of if */

	if (pAd->MACVersion == 0x28600100)
	{
		/* no tuning for RT2860C */
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66UpperBound);

		DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, R66= 0x%x, R66LowerBound=0x%x, R66UpperBound=0x%x (Ver.C)\n",
				 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66, R66LowerBound, R66UpperBound));
	}
	else
	{
		/* do tuning for RT2860D */
		/* FalseCCA UpperBound threshold=512, FalseCCA LowerBound threshold=100 */

		/* R66 is currenly in dynamic tuning range,
			keep dynamic tuning based on False CCA counter */
		if ((pAd->RalinkCounters.OneSecFalseCCACnt > pAd->BbpTuning.FalseCcaUpperThreshold) &&
			(R66 < R66UpperBound))
		{
			/*  1. False CCA Count is too much (>512);
				2. R66 is not largest (larger R66 lower signal strength) */
			R66 += pAd->BbpTuning.R66Delta; /* 4 */

			if (R66 >= R66UpperBound)
				R66 = R66UpperBound;
			/* End of if */
			
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, ++R66= 0x%x\n",
				Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
		}
		else if ((pAd->RalinkCounters.OneSecFalseCCACnt < pAd->BbpTuning.FalseCcaLowerThreshold) &&
				 (R66 > R66LowerBound))
		{
			/*  1. False CCA Count is lower (<100);
				2. R66 is not lowest */
			R66 -= pAd->BbpTuning.R66Delta;

			if (R66 <= R66LowerBound)
				R66 = R66LowerBound;
			/* End of if */
			
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, --R66= 0x%x\n",
				Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
		}
		else
		{
			/* R66 is largest or lowest or normal False CCA Count */
			if (R66 >= R66UpperBound)
			{
				R66 = R66UpperBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */
			
			if (R66 <= R66LowerBound)
			{
				R66 = R66LowerBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */

			if (pAd->Mlme.OneSecPeriodicRound % 4 == 0)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, R66= 0x%x, R66LowerBound=0x%x, R66UpperBound=0x%x\n",
					Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66, R66LowerBound, R66UpperBound));
			} /* End of if */
		} /* End of if */
	} /* End of if */
} /* End of AsicBbpTuning1 */


VOID AsicBbpTuning2(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR	R66;
	CHAR	Rssi;

/* Disable BBP Tuning when ATE is running. */
#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	/* This algorithm is trying to compensate the effect of board (system)
		noise issue. */


	/* 2860C did not support Fase CCA, therefore can't tune */
	if (pAd->BbpTuning.bEnable == FALSE)
		return;
	/* End of if */

	/* get current R66 value, BbpWriteLatch[] be will updated in
		RTMP_BBP_IO_WRITE8_BY_REG_ID() */
	R66 = pAd->BbpWriteLatch[66];


	/* find the maximum RSSI from the three receive antennas */
	Rssi = RTMPMaxRssi(pAd, (CHAR)pAd->ApCfg.RssiSample.AvgRssi0,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi1,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi2);


	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		if (Rssi > RSSI_FOR_MID_LOW_SENSIBILITY) /* -80 */
		{
            if (R66 != (0x2E + GET_LNA_GAIN(pAd) + 0x18))
            {
				R66 = (0x2E + GET_LNA_GAIN(pAd)) + 0x18;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
            } /* End of if */

            DBGPRINT(RT_DEBUG_TRACE,
					("strong RSSI=%d, GET_LNA_GAIN=0x%x, CCA=%d, BW=%d, fixed R66 at 0x%x\n",
					Rssi, GET_LNA_GAIN(pAd), pAd->RalinkCounters.OneSecFalseCCACnt,
					pAd->CommonCfg.BBPCurrentBW, R66));
            return;
		}
		else
		{
			/* weak RSSI, use largest gain */
			if (R66 != (0x2E + GET_LNA_GAIN(pAd) + 0x08))
			{
				R66 = (0x2E + GET_LNA_GAIN(pAd)) + 0x08;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */

			DBGPRINT(RT_DEBUG_TRACE,
					("weak RSSI=%d, GET_LNA_GAIN=0x%x, CCA=%d, BW=%d, fixed R66 at 0x%x\n", 
					Rssi, GET_LNA_GAIN(pAd), pAd->RalinkCounters.OneSecFalseCCACnt,
					pAd->CommonCfg.BBPCurrentBW, R66));
			return;
		} /* End of if */
	} /* End of if */
} /* End of AsicBbpTuning2 */

/* End of ap_mlme.c */
