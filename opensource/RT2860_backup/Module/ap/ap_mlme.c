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
	// decide the rate table for tuning
	if (pAd->CommonCfg.TxRateTableSize > 0)
	{
		*ppTable = RateSwitchTable;
		*pTableSize = RateSwitchTable[0];
		*pInitTxRateIdx = RateSwitchTable[1];
	}
	else
	{
		if ((pEntry->RateLen == 12) && (pEntry->HTCapability.MCSSet[0] == 0xff) &&
			((pEntry->HTCapability.MCSSet[1] == 0) || (pAd->Antenna.field.TxPath == 1)))
		{// 11BGN 1S AP
			*ppTable = RateSwitchTable11BGN1S;
			*pTableSize = RateSwitchTable11BGN1S[0];
			*pInitTxRateIdx = RateSwitchTable11BGN1S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11BGN 1S STA \n"));
		}
		else if ((pEntry->RateLen == 12) && (pEntry->HTCapability.MCSSet[0] == 0xff) &&
			(pEntry->HTCapability.MCSSet[1] == 0xff) && (pAd->Antenna.field.TxPath == 2))
		{// 11BGN 2S AP
			if (pAd->LatchRfRegs.Channel <= 14)
			{
			*ppTable = RateSwitchTable11BGN2S;
			*pTableSize = RateSwitchTable11BGN2S[0];
			*pInitTxRateIdx = RateSwitchTable11BGN2S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11BGN 2S STA \n"));
		}
			else
			{
				*ppTable = RateSwitchTable11BGN2SForABand;
				*pTableSize = RateSwitchTable11BGN2SForABand[0];
				*pInitTxRateIdx = RateSwitchTable11BGN2SForABand[1];
                		DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11AN 2S STA \n"));
			}
			
		}
		else if ((pEntry->HTCapability.MCSSet[0] == 0xff) && ((pEntry->HTCapability.MCSSet[1] == 0) || (pAd->Antenna.field.TxPath == 1)))
		{// 11N 1S AP
			*ppTable = RateSwitchTable11N1S;
			*pTableSize = RateSwitchTable11N1S[0];
			*pInitTxRateIdx = RateSwitchTable11N1S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11N 1S STA \n"));
		}
		else if ((pEntry->HTCapability.MCSSet[0] == 0xff) && (pEntry->HTCapability.MCSSet[1] == 0xff) && (pAd->Antenna.field.TxPath == 2))
		{// 11N 2S AP
			if (pAd->LatchRfRegs.Channel <= 14)
			{
			*ppTable = RateSwitchTable11N2S;
			*pTableSize = RateSwitchTable11N2S[0];
			*pInitTxRateIdx = RateSwitchTable11N2S[1];
            }
			else
			{
				*ppTable = RateSwitchTable11N2SForABand;
				*pTableSize = RateSwitchTable11N2SForABand[0];
				*pInitTxRateIdx = RateSwitchTable11N2SForABand[1];
			}
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11N 2S STA \n"));
		}
		else if ((pEntry->RateLen == 4) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// B only AP
			*ppTable = RateSwitchTable11B;
			*pTableSize = RateSwitchTable11B[0];
			*pInitTxRateIdx = RateSwitchTable11B[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: B only STA \n"));
		}
		else if ((pEntry->RateLen > 8) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// B/G  mixed AP
			*ppTable = RateSwitchTable11BG;
			*pTableSize = RateSwitchTable11BG[0];
			*pInitTxRateIdx = RateSwitchTable11BG[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: B/G mixed STA \n"));
		}
		else if ((pEntry->RateLen == 8) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// G only AP
			*ppTable = RateSwitchTable11G;
			*pTableSize = RateSwitchTable11G[0];
			*pInitTxRateIdx = RateSwitchTable11G[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: A/G STA \n"));
		}
		else
		{
			*ppTable = RateSwitchTable11N2S;
			*pTableSize = RateSwitchTable11N2S[0];
			*pInitTxRateIdx = RateSwitchTable11N2S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: unkown mode \n"));
		}
	}
}

VOID APMlmeSetTxRate(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PRTMP_TX_RATE_SWITCH	pTxRate)
{
	if ((pTxRate->STBC) && (pEntry->MaxHTPhyMode.field.STBC))
		pEntry->HTPhyMode.field.STBC = STBC_USE;
	else
		pEntry->HTPhyMode.field.STBC = STBC_NONE;

	if (((pTxRate->ShortGI) && (pEntry->MaxHTPhyMode.field.ShortGI))
         || ( pAd->WIFItestbed.bShortGI &&  pEntry->MaxHTPhyMode.field.ShortGI) )
		pEntry->HTPhyMode.field.ShortGI = GI_400;
	else
		pEntry->HTPhyMode.field.ShortGI = GI_800;

	if (pTxRate->CurrMCS < MCS_AUTO)
		pEntry->HTPhyMode.field.MCS = pTxRate->CurrMCS;

	if (pTxRate->Mode <= MODE_HTGREENFIELD)
		pEntry->HTPhyMode.field.MODE = pTxRate->Mode;

	if ((pAd->WIFItestbed.bGreenField & pEntry->HTCapability.HtCapInfo.GF) && (pEntry->HTPhyMode.field.MODE == MODE_HTMIX))
	{
		// force Tx GreenField 
		pEntry->HTPhyMode.field.MODE = MODE_HTGREENFIELD;
	}

	if (pAd->CommonCfg.bRcvBSSWidthTriggerEvents)
	{
		pEntry->HTPhyMode.field.BW = BW_20;
	}

	pAd->LastTxRate = (USHORT)(pEntry->HTPhyMode.word);

	DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Aid=%d, APMlmeSetTxRate - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d \n",
		pEntry->Aid, pEntry->CurrTxRateIndex,
		pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC, pEntry->HTPhyMode.field.ShortGI,
		pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW));
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
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY			*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE, bUpgradeQuality = FALSE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, RssiOffset = 0;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;
	
#ifdef RALINK_ATE
    	if (pAd->ate.Mode != ATE_STOP)
    	{
			return;
    	}
#endif // RALINK_ATE //

    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
    
	DBGPRINT_RAW(RT_DEBUG_INFO,("==> APMlmeDynamicTxRateSwitching : Now time (%ld)\n", (jiffies/OS_HZ)));    


   	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
        pEntry = &pAd->MacTab.Content[i];

        // only associated STA counts
        if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef APCLI_SUPPORT
            &&(pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
            && ((pEntry->ValidAsWDS == FALSE) || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
            )
            continue;



		//NICUpdateFifoStaCounters(pAd);


		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
				 pAd->RalinkCounters.OneSecTxRetryOkCount + 
				 pAd->RalinkCounters.OneSecTxFailCount;

            if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else

			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;			
#endif
		}
		else
		{
		TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
			 pEntry->OneSecTxRetryOkCount + 
			 pEntry->OneSecTxFailCount;

		if (TxTotalCnt)
			TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
		}

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;

		Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		APMlmeSelectTxRateTable(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];

		if ((Rssi > -65) && (pCurrTxRate->Mode >= MODE_HTMIX))
		{
			TrainUp		= (pCurrTxRate->TrainUp + (pCurrTxRate->TrainUp >> 1));
			TrainDown	= (pCurrTxRate->TrainDown + (pCurrTxRate->TrainDown >> 1));
		}
		else
		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}

		pEntry->LastTimeTxRateChangeAction = pEntry->LastSecTxRateChangeAction;

		if (pAd->MacTab.Size == 1)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS:Aid=%d, TxRetransmit=%ld, TxFailCount=%ld, TxSuccess=%ld \n",
			pEntry->Aid, TxRetransmit, TxFailCount,	TxSuccess));
		}
		else
		{
		DBGPRINT_RAW(RT_DEBUG_INFO,("DRS:Aid=%d, OneSecTxRetryOkCount=%d, OneSecTxFailCount=%d, OneSecTxNoRetryOkCount=%d \n",
			pEntry->Aid,
			pEntry->OneSecTxRetryOkCount,
			pEntry->OneSecTxFailCount,
			pEntry->OneSecTxNoRetryOkCount));
		}

		if (pAd->CommonCfg.bAutoTxRateSwitch == FALSE)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Fixed - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d, PER=%ld%% \n\n",
				pEntry->CurrTxRateIndex, pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC,
				pEntry->HTPhyMode.field.ShortGI, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, TxErrorRatio));
			return;
		}
		else
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Before- CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, TrainUp=%d, TrainDown=%d, NextUp=%d, NextDown=%d, CurrMCS=%d, PER=%ld%%\n",
				CurrRateIdx,
				pCurrTxRate->CurrMCS,
				pCurrTxRate->STBC,
				pCurrTxRate->ShortGI,
				pCurrTxRate->Mode,
				TrainUp,
				TrainDown,
				UpRateIdx,
				DownRateIdx,
				pEntry->HTPhyMode.field.MCS,
				TxErrorRatio));
		}

		if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
			continue;

        if (TxTotalCnt <= 15)
        {
   			CHAR	idx = 0;
			UCHAR	TxRateIdx;
			UCHAR	MCS0 = 0, MCS1 = 0, MCS2 = 0, MCS3 = 0, MCS4 = 0,  MCS5 =0, MCS6 = 0, MCS7 = 0;
			UCHAR	MCS12 = 0, MCS13 = 0, MCS14 = 0, MCS15 = 0;

			// check the existence and index of each needed MCS
			while (idx < pTable[0])
			{
				pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(idx+1)*5];

				if (pCurrTxRate->CurrMCS == MCS_0)
				{
					MCS0 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_1)
				{
					MCS1 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_2)
				{
					MCS2 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_3)
				{
					MCS3 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_4)
				{
					MCS4 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_5)
				{
					MCS5 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_6)
				{
					MCS6 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_7)
				{
					MCS7 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_12)
				{
					MCS12 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_13)
				{
					MCS13 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_14)
				{
					MCS14 = idx;
				}
				else if ((pCurrTxRate->CurrMCS == MCS_15) && (pCurrTxRate->ShortGI == GI_800))	//we hope to use ShortGI as initial rate
				{
					MCS15 = idx;
				}
				
				idx ++;
			}
#if 1 // test by gary //		
			if ((pTable == RateSwitchTable11BGN2S) || (pTable == RateSwitchTable11BGN2SForABand) || (pTable == RateSwitchTable11N2S) || (pTable == RateSwitchTable11N2SForABand) || (pTable == RateSwitchTable))
			{
				RssiOffset = 2;
			}
			else
			{
				RssiOffset = 5;
			}
#endif // test by gary //
			if ((pTable == RateSwitchTable11BGN2S) || (pTable == RateSwitchTable11N2S) || (pTable == RateSwitchTable))
			{// N mode with 2 stream
				if (MCS15 && (Rssi >= (-70+RssiOffset)))
					TxRateIdx = MCS15;
				else if (MCS14 && (Rssi >= (-72+RssiOffset)))
					TxRateIdx = MCS14;
				else if (MCS13 && (Rssi >= (-76+RssiOffset)))
					TxRateIdx = MCS13;
				else if (MCS12 && (Rssi >= (-78+RssiOffset)))
					TxRateIdx = MCS12;
				else if (MCS4 && (Rssi >= (-82+RssiOffset)))
					TxRateIdx = MCS4;
				else if (MCS3 && (Rssi >= (-84+RssiOffset)))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi >= (-86+RssiOffset)))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi >= (-88+RssiOffset)))
					TxRateIdx = MCS1;
				else
					TxRateIdx = MCS0;
			}
			else if ((pTable == RateSwitchTable11BGN1S) || (pTable == RateSwitchTable11N1S))
			{// N mode with 1 stream
				if (MCS7 && (Rssi > (-72+RssiOffset)))
					TxRateIdx = MCS7;
				else if (MCS6 && (Rssi > (-74+RssiOffset)))
					TxRateIdx = MCS6;
				else if (MCS5 && (Rssi > (-77+RssiOffset)))
					TxRateIdx = MCS5;
				else if (MCS4 && (Rssi > (-79+RssiOffset)))
					TxRateIdx = MCS4;
				else if (MCS3 && (Rssi > (-81+RssiOffset)))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi > (-83+RssiOffset)))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi > (-86+RssiOffset)))
					TxRateIdx = MCS1;
			else
					TxRateIdx = MCS0;
			}
			else
			{// Legacy mode
				if (MCS7 && (Rssi > -70))
				TxRateIdx = MCS7;
				else if (MCS6 && (Rssi > -74))
					TxRateIdx = MCS6;
				else if (MCS5 && (Rssi > -78))
					TxRateIdx = MCS5;
				else if (MCS4 && (Rssi > -82))
				TxRateIdx = MCS4;
				else if (MCS4 == 0)							// for B-only mode
					TxRateIdx = MCS3;
				else if (MCS3 && (Rssi > -85))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi > -87))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi > -90))
					TxRateIdx = MCS1;
			else
				TxRateIdx = MCS0;
			}

			if (TxRateIdx != pEntry->CurrTxRateIndex)
			{
				pEntry->CurrTxRateIndex = TxRateIdx;
				pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
				APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
			}

			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
			pEntry->fLastSecAccordingRSSI = TRUE;

			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: TxTotalCnt <= 15, switch MCS according to RSSI (%d), RssiOffset=%d\n", Rssi, RssiOffset));
			continue;
        }

		if (pEntry->fLastSecAccordingRSSI == TRUE)
		{
			pEntry->fLastSecAccordingRSSI = FALSE;
			pEntry->LastSecTxRateChangeAction = 0;
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: MCS is according to RSSI, and ignore tuning this sec \n"));
			return;
		}

		do
		{
			BOOLEAN	bTrainUpDown = FALSE;
			
			pEntry->CurrTxRateStableTime ++;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				bTrainUpDown = TRUE;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			// upgrade TX quality if PER <= Rate-Up threshold
			else if (TxErrorRatio <= TrainUp)
			{
				bTrainUpDown = TRUE;
				bUpgradeQuality = TRUE;
				if (pEntry->TxQuality[CurrRateIdx])
					pEntry->TxQuality[CurrRateIdx] --;  // quality very good in CurrRate

				if (pEntry->TxRateUpPenalty)
					pEntry->TxRateUpPenalty --;
				else if (pEntry->TxQuality[UpRateIdx])
					pEntry->TxQuality[UpRateIdx] --;    // may improve next UP rate's quality
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (bTrainUpDown == TRUE)
			{
			// perform DRS - consider TxRate Down first, then rate up.
			if ((CurrRateIdx != DownRateIdx) && (pEntry->TxQuality[CurrRateIdx] >= DRS_TX_QUALITY_WORST_BOUND))
			{
				pEntry->CurrTxRateIndex = DownRateIdx;
			}
			else if ((CurrRateIdx != UpRateIdx) && (pEntry->TxQuality[UpRateIdx] <= 0))
			{
				pEntry->CurrTxRateIndex = UpRateIdx;
			}
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: ++TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;
			pEntry->LastSecTxRateChangeAction = 1; // rate UP
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			//
			// For TxRate fast train up
			// 
			if (!pAd->ApCfg.ApQuickResponeForRateUpTimerRunning)
			{				
				RTMPSetTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, 100);

				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = TRUE;
			}
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: --TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->LastSecTxRateChangeAction = 2; // rate DOWN
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;

			//
			// For TxRate fast train down
			// 
			if (!pAd->ApCfg.ApQuickResponeForRateUpTimerRunning)
			{
				RTMPSetTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, 100);

				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = TRUE;
			}
		}
		else
		{
			pEntry->LastSecTxRateChangeAction = 0; // rate no change
			bTxRateChanged = FALSE;
		}
			
		if (pAd->MacTab.Size == 1)
		{
			//test by gary 
       		//pEntry->LastTxOkCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount;
			pEntry->LastTxOkCount = TxSuccess;
		}
		else
		{
			pEntry->LastTxOkCount = pEntry->OneSecTxNoRetryOkCount;

		// reset all OneSecTx counters
		pEntry->OneSecTxRetryOkCount = 0;
		pEntry->OneSecTxFailCount = 0;
		pEntry->OneSecTxNoRetryOkCount = 0;
		}

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
		}
    }
}

/*
    ========================================================================
    Routine Description:
        AP side, Auto TxRate faster train up timer call back function.
        
    Arguments:
        SystemSpecific1         - Not used.
        FunctionContext         - Pointer to our Adapter context.
        SystemSpecific2         - Not used.
        SystemSpecific3         - Not used.
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APQuickResponeForRateUpExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
	PRTMP_ADAPTER			pAd = (PRTMP_ADAPTER)FunctionContext;
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt, TxCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY			*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, ratio;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;

	pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = FALSE;
	
    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
   	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
        pEntry = &pAd->MacTab.Content[i];

        // only associated STA counts
//        if ((pEntry->ValidAsCLI == FALSE) || (pEntry->Sst != SST_ASSOC))
        if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef APCLI_SUPPORT
			&& (pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			&& (pEntry->ValidAsWDS == FALSE || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
			)
            continue;

    	Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;


		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
            TX_STA_CNT1_STRUC		StaTx1;
			TX_STA_CNT0_STRUC		TxStaCnt0;

       		// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;

			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
				pAd->RalinkCounters.OneSecTxRetryOkCount + 
				pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else
			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;

			if (pAd->Antenna.field.TxPath > 1)
				Rssi = (pEntry->RssiSample.AvgRssi0 + pEntry->RssiSample.AvgRssi1) >> 1;
			else
				Rssi = pEntry->RssiSample.AvgRssi0;

			TxCnt = AccuTxTotalCnt;
#endif
		}
		else
		{
		TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
			 pEntry->OneSecTxRetryOkCount + 
			 pEntry->OneSecTxFailCount;

		if (TxTotalCnt)
			TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
	
			TxCnt = TxTotalCnt;	
		}

		// decide the rate table for tuning
		APMlmeSelectTxRateTable(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];

		if ((Rssi > -65) && (pCurrTxRate->Mode >= MODE_HTMIX))
		{
			TrainUp		= (pCurrTxRate->TrainUp + (pCurrTxRate->TrainUp >> 1));
			TrainDown	= (pCurrTxRate->TrainDown + (pCurrTxRate->TrainDown >> 1));
		}
		else
		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}

		if (pAd->MacTab.Size == 1)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS:Aid=%d, TxRetransmit=%ld, TxFailCount=%ld, TxSuccess=%ld \n",
			pEntry->Aid, TxRetransmit, TxFailCount,	TxSuccess));
		}
		else
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS:Aid=%d, OneSecTxRetryOkCount=%d, OneSecTxFailCount=%d, OneSecTxNoRetryOkCount=%d \n",
			pEntry->Aid,
			pEntry->OneSecTxRetryOkCount,
			pEntry->OneSecTxFailCount,
			pEntry->OneSecTxNoRetryOkCount));
		}

		if (pAd->CommonCfg.bAutoTxRateSwitch == FALSE)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: Fixed - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d, PER=%ld%% \n\n",
				pEntry->CurrTxRateIndex, pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC,
				pEntry->HTPhyMode.field.ShortGI, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, TxErrorRatio));
			return;
		}
		else
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: Before- CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, TrainUp=%d, TrainDown=%d, NextUp=%d, NextDown=%d, CurrMCS=%d, PER=%ld%%\n",
				CurrRateIdx,
				pCurrTxRate->CurrMCS,
				pCurrTxRate->STBC,
				pCurrTxRate->ShortGI,
				pCurrTxRate->Mode,
				TrainUp,
				TrainDown,
				UpRateIdx,
				DownRateIdx,
				pEntry->HTPhyMode.field.MCS,
				TxErrorRatio));
		}

		if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
			continue;

        if (/*TxTotalCnt*/ TxCnt <= 15)
        {
			NdisZeroMemory(pAd->DrsCounters.TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pAd->DrsCounters.PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				pEntry->CurrTxRateIndex = DownRateIdx;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				pEntry->CurrTxRateIndex = UpRateIdx;
			}
			
            DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: TxTotalCnt <= 15, train back to original rate \n"));
			continue;
        }

		do
		{
			ULONG		OneSecTxNoRetryOKRationCount;

			// test by gary
			//if (pEntry->LastSecTxRateChangeAction == 0)
			if (pEntry->LastTimeTxRateChangeAction == 0)
				ratio = 5;
			else
				ratio = 4;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (pAd->MacTab.Size == 1)
			{
   				//OneSecTxNoRetryOKRationCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount * ratio + (pAd->RalinkCounters.OneSecTxNoRetryOkCount >> 1);
				// test by gary
				OneSecTxNoRetryOKRationCount = (TxSuccess * ratio);
			}
			else
			{
				OneSecTxNoRetryOKRationCount = pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1);
			}

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				//if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = DownRateIdx;
					pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
					//DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) bad tx ok count (L:%ld, C:%d)\n", pEntry->LastTxOkCount, (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1))));
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Up) bad tx ok count (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
				else
				{
					//DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%d)\n", pEntry->LastTxOkCount, (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1))));
					DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				//if ((TxErrorRatio >= 50) || (TxErrorRatio >= TrainDown))
				if ((TxErrorRatio >= 50) && (TxErrorRatio >= TrainDown))
				{
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) direct train down (TxErrorRatio >= TrainDown)\n"));
				}
				//else if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				else if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = UpRateIdx;
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) bad tx ok count (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
				else
				{
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) keep rate-down (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: ++TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->TxRateUpPenalty = 0;
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: --TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;
		}
		else
		{
			bTxRateChanged = FALSE;
		}

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
		}
    }
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
