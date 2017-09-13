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
     sync.c
     
     Abstract:
     Synchronization state machine related services
     
     Revision History:
     Who         When          What
     --------    ----------    ----------------------------------------------
     John Chang  08-04-2003    created for 11g soft-AP
     
 */

#include "rt_config.h"
#include "ap_autoChSel.h"

#define OBSS_BEACON_RSSI_THRESHOLD		(-85)

void build_ext_channel_switch_ie(
	IN PRTMP_ADAPTER pAd,
	IN HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE *pIE);

/*
	==========================================================================
	Description:
		The sync state machine, 
	Parameters:
		Sm - pointer to the state machine
	Note:
		the state machine looks like the following

							AP_SYNC_IDLE
	APMT2_PEER_PROBE_REQ	peer_probe_req_action
	==========================================================================
 */
VOID APSyncStateMachineInit(
	IN PRTMP_ADAPTER pAd, 
	IN STATE_MACHINE *Sm, 
	OUT STATE_MACHINE_FUNC Trans[])
{
	StateMachineInit(Sm, (STATE_MACHINE_FUNC *)Trans, AP_MAX_SYNC_STATE, AP_MAX_SYNC_MSG, (STATE_MACHINE_FUNC)Drop, AP_SYNC_IDLE, AP_SYNC_MACHINE_BASE);

	StateMachineSetAction(Sm, AP_SYNC_IDLE, APMT2_PEER_PROBE_REQ, (STATE_MACHINE_FUNC)APPeerProbeReqAction);
	StateMachineSetAction(Sm, AP_SYNC_IDLE, APMT2_PEER_BEACON, (STATE_MACHINE_FUNC)APPeerBeaconAction);
	StateMachineSetAction(Sm, AP_SYNC_IDLE, APMT2_MLME_SCAN_REQ, (STATE_MACHINE_FUNC)APMlmeScanReqAction);

	// scan_listen state
	StateMachineSetAction(Sm, AP_SCAN_LISTEN, APMT2_MLME_SCAN_REQ, (STATE_MACHINE_FUNC)APInvalidStateWhenScan);
	StateMachineSetAction(Sm, AP_SCAN_LISTEN, APMT2_PEER_BEACON, (STATE_MACHINE_FUNC)APPeerBeaconAtScanAction);
	StateMachineSetAction(Sm, AP_SCAN_LISTEN, APMT2_PEER_PROBE_RSP, (STATE_MACHINE_FUNC)APPeerBeaconAtScanAction);
	StateMachineSetAction(Sm, AP_SCAN_LISTEN, APMT2_SCAN_TIMEOUT, (STATE_MACHINE_FUNC)APScanTimeoutAction);
	StateMachineSetAction(Sm, AP_SCAN_LISTEN, APMT2_MLME_SCAN_CNCL, (STATE_MACHINE_FUNC)APScanCnclAction);

	RTMPInitTimer(pAd, &pAd->MlmeAux.ScanTimer, GET_TIMER_FUNCTION(APScanTimeout), pAd, FALSE);
}

/* 
    ==========================================================================
    Description:
        Scan timeout handler, executed in timer thread
    ==========================================================================
 */
VOID APScanTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)FunctionContext;

	DBGPRINT(RT_DEBUG_TRACE, ("AP SYNC - Scan Timeout \n"));
	MlmeEnqueue(pAd, AP_SYNC_STATE_MACHINE, APMT2_SCAN_TIMEOUT, 0, NULL);
	RT28XX_MLME_HANDLER(pAd);
}

/* 
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID APInvalidStateWhenScan(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	DBGPRINT(RT_DEBUG_TRACE, ("AYNC - InvalidStateWhenScan(state=%ld). Reset SYNC machine\n", pAd->Mlme.ApSyncMachine.CurrState));
}

/* 
    ==========================================================================
    Description:
        Scan timeout procedure. basically add channel index by 1 and rescan
    ==========================================================================
 */
VOID APScanTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	pAd->MlmeAux.Channel = NextChannel(pAd, pAd->MlmeAux.Channel);
	ScanNextChannel(pAd);
}


/*
	==========================================================================
	Description:
		Process the received ProbeRequest from clients
	Parameters:
		Elem - msg containing the ProbeReq frame
	==========================================================================
 */
VOID APPeerProbeReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR         Addr2[MAC_ADDR_LEN];
	CHAR          Ssid[MAX_LEN_OF_SSID];
	UCHAR         SsidLen; //, Rates[MAX_LEN_OF_SUPPORTED_RATES], RatesLen;
	HEADER_802_11 ProbeRspHdr;
	NDIS_STATUS   NStatus;
	PUCHAR        pOutBuffer = NULL;
	ULONG         FrameLen = 0, TmpLen;
	LARGE_INTEGER FakeTimestamp;
	UCHAR         DsLen = 1;//, IbssLen = 2, TimLen=1, 
				  //BitmapControl=0, VirtualBitmap=0;
	UCHAR   ErpIeLen = 1;
	UCHAR         apidx = 0;
	UCHAR   RSNIe=IE_WPA, RSNIe2=IE_WPA2;//, RSN_Len=22;


#ifdef WSC_AP_SUPPORT
    UCHAR		  Addr3[MAC_ADDR_LEN];
    PFRAME_802_11 pFrame = (PFRAME_802_11)Elem->Msg;

	COPY_MAC_ADDR(Addr3, pFrame->Hdr.Addr3);
#endif // WSC_AP_SUPPORT //

	// if in bridge mode, no need to reply probe req.
	//if (apidx == BSS0 && pAd->WdsTab.Mode == WDS_BRIDGE_MODE) /* all MBssid shouldn't reply probe-req while WDS_BRIDGE_MODE. */
	if (pAd->WdsTab.Mode == WDS_BRIDGE_MODE)
		return;

	if (! APPeerProbeReqSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, Ssid, &SsidLen))
		return;

	for(apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{
		if ((pAd->ApCfg.MBSSID[apidx].MSSIDDev != NULL) &&
			!(pAd->ApCfg.MBSSID[apidx].MSSIDDev->flags & IFF_UP))
		{
			/* the interface is down, so we can not send probe response */
			continue;
		} /* End of if */

		if (((SsidLen == 0) && (! pAd->ApCfg.MBSSID[apidx].bHideSsid)) || 
#ifdef WSC_AP_SUPPORT
            /* buffalo WPS testbed STA send ProbrRequest ssid length = 32 and ssid are not AP , but DA are AP. for WPS test send ProbeResponse */
			((SsidLen == 32) && MAC_ADDR_EQUAL(Addr3, pAd->ApCfg.MBSSID[apidx].Bssid) && (pAd->ApCfg.MBSSID[apidx].bHideSsid == 0)) ||
#endif // WSC_AP_SUPPORT //
		((SsidLen == pAd->ApCfg.MBSSID[apidx].SsidLen) && NdisEqualMemory(Ssid, pAd->ApCfg.MBSSID[apidx].Ssid, (ULONG) SsidLen)))
			;
		else
			continue; /* check next BSS */

		// allocate and send out ProbeRsp frame
		NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
		if (NStatus != NDIS_STATUS_SUCCESS)
			return;

		DBGPRINT(RT_DEBUG_INFO, ("SYNC - Send PROBE_RSP to %02x:%02x:%02x:%02x:%02x:%02x...\n", Addr2[0],Addr2[1],Addr2[2],Addr2[3],Addr2[4],Addr2[5] ));

		MgtMacHeaderInit(pAd, &ProbeRspHdr, SUBTYPE_PROBE_RSP, 0, Addr2, pAd->ApCfg.MBSSID[apidx].Bssid);

		 if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA) || 
			(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPAPSK))
			RSNIe = IE_WPA;
		else if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2) || 
			(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2PSK))
			RSNIe = IE_WPA2;

		MakeOutgoingFrame(pOutBuffer,                 &FrameLen, 
						  sizeof(HEADER_802_11),      &ProbeRspHdr, 
						  TIMESTAMP_LEN,              &FakeTimestamp,
						  2,                          &pAd->CommonCfg.BeaconPeriod,
						  2,                          &pAd->ApCfg.MBSSID[apidx].CapabilityInfo,
						  1,                          &SsidIe, 
						  1,                          &pAd->ApCfg.MBSSID[apidx].SsidLen, 
						  pAd->ApCfg.MBSSID[apidx].SsidLen,     pAd->ApCfg.MBSSID[apidx].Ssid,
						  1,                          &SupRateIe, 
						  1,                          &pAd->CommonCfg.SupRateLen,
						  pAd->CommonCfg.SupRateLen,      pAd->CommonCfg.SupRate, 
						  1,                          &DsIe, 
						  1,                          &DsLen, 
						  1,                          &pAd->CommonCfg.Channel,
						  END_OF_ARGS);

		if (pAd->CommonCfg.ExtRateLen)
		{
			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen, 
							  1,                        &ErpIe,
							  1,                        &ErpIeLen,
							  1,                        &pAd->ApCfg.ErpIeContent,
							  1,                        &ExtRateIe,
							  1,                        &pAd->CommonCfg.ExtRateLen,
							  pAd->CommonCfg.ExtRateLen,    pAd->CommonCfg.ExtRate,
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}


		// add Channel switch announcement IE
		if ((pAd->CommonCfg.Channel > 14)
			&& (pAd->CommonCfg.bIEEE80211H == 1)
			&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
		{
			UCHAR CSAIe=IE_CHANNEL_SWITCH_ANNOUNCEMENT;
			UCHAR CSALen=3;
			UCHAR CSAMode=1;

			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen,
							  1,                        &CSAIe,
							  1,                        &CSALen,
							  1,                        &CSAMode,
							  1,                        &pAd->CommonCfg.Channel,
							  1,                        &pAd->CommonCfg.RadarDetect.CSCount,
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}

		if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
		{
			ULONG TmpLen;
			UCHAR	HtLen, AddHtLen, NewExtLen;
#ifdef RT_BIG_ENDIAN
			HT_CAPABILITY_IE HtCapabilityTmp;
			ADD_HT_INFO_IE	addHTInfoTmp;
#endif

   			if (pAd->CommonCfg.bExtChannelSwitchAnnouncement && (pAd->CommonCfg.Channel > 14))
			{
				HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE	HtExtChannelSwitchIe;

				build_ext_channel_switch_ie(pAd, &HtExtChannelSwitchIe);
				MakeOutgoingFrame(pOutBuffer + FrameLen,             &TmpLen,
								  sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE),	&HtExtChannelSwitchIe,
								  END_OF_ARGS);
				FrameLen += TmpLen;
			}


			HtLen = sizeof(pAd->CommonCfg.HtCapability);
			AddHtLen = sizeof(pAd->CommonCfg.AddHTInfo);
			NewExtLen = 1;
			//New extension channel offset IE is included in Beacon, Probe Rsp or channel Switch Announcement Frame
#ifndef RT_BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,            &TmpLen,
							  1,                                &HtCapIe,
							  1,                                &HtLen,
							 sizeof(HT_CAPABILITY_IE),          &pAd->CommonCfg.HtCapability, 
							  1,                                &AddHtInfoIe,
							  1,                                &AddHtLen,
							 sizeof(ADD_HT_INFO_IE),          &pAd->CommonCfg.AddHTInfo, 
							  1,                                &NewExtChanIe,
							  1,                                &NewExtLen,
							 sizeof(NEW_EXT_CHAN_IE),          &pAd->CommonCfg.NewExtChanOffset, 
							  END_OF_ARGS);
#else
			NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

			NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, AddHtLen);
			*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
			*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

			MakeOutgoingFrame(pOutBuffer + FrameLen,         &TmpLen,
								1,                           &HtCapIe,
								1,                           &HtLen,
								HtLen,                       &HtCapabilityTmp, 
								1,                           &AddHtInfoIe,
								1,                           &AddHtLen,
								AddHtLen,                    &addHTInfoTmp,
								1,                           &NewExtChanIe,
								1,                           &NewExtLen,
								sizeof(NEW_EXT_CHAN_IE),     &pAd->CommonCfg.NewExtChanOffset, 
								END_OF_ARGS);

#endif
			FrameLen += TmpLen;			

#if 0
			if (pAd->bBroadComHT == TRUE)
			{
				UCHAR	epigram_ie_len;
				UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
				UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


				epigram_ie_len = HtLen + 4;
				MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
							  1,                                &WpaIe,
							  1,                                &epigram_ie_len,
							  4,                                &BROADCOM_HTC[0],
							  HtLen,          					&pAd->CommonCfg.HtCapability, 
							  END_OF_ARGS);


				FrameLen += TmpLen;

				epigram_ie_len = AddHtLen + 4;
				MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
							  1,                                &WpaIe,
							  1,                                &epigram_ie_len,
							  4,                                &BROADCOM_AHTINFO[0],
							  AddHtLen, 						&pAd->CommonCfg.AddHTInfo, 
							  END_OF_ARGS);

				FrameLen += TmpLen;
			}
#endif

		}


		// Append RSN_IE when  WPA OR WPAPSK, 
		if (pAd->ApCfg.MBSSID[apidx].AuthMode < Ndis802_11AuthModeWPA)
			; // enough information
		else if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || 
			(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
		{
			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen, 
							  1,                        &RSNIe,
							  1,                        &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
							  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],  pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
							  1,                        &RSNIe2,
							  1,                        &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],
							  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],  pAd->ApCfg.MBSSID[apidx].RSN_IE[1],
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}
		else
		{
			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen, 
							  1,                        &RSNIe,
							  1,                        &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
							  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],  pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}

		// add WMM IE here
		if (pAd->ApCfg.MBSSID[apidx].bWmmCapable)
		{
			UCHAR i;
			UCHAR WmeParmIe[26] = {IE_VENDOR_SPECIFIC, 24, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0, 0}; 
			WmeParmIe[8] = pAd->ApCfg.BssEdcaParm.EdcaUpdateCount & 0x0f;
#ifdef UAPSD_AP_SUPPORT
            UAPSD_MR_IE_FILL(WmeParmIe[8], pAd);
#endif // UAPSD_AP_SUPPORT //
			for (i=QID_AC_BE; i<=QID_AC_VO; i++)
			{
				WmeParmIe[10+ (i*4)] = (i << 5)                                         +     // b5-6 is ACI
									   ((UCHAR)pAd->ApCfg.BssEdcaParm.bACM[i] << 4)     +     // b4 is ACM
									   (pAd->ApCfg.BssEdcaParm.Aifsn[i] & 0x0f);              // b0-3 is AIFSN
				WmeParmIe[11+ (i*4)] = (pAd->ApCfg.BssEdcaParm.Cwmax[i] << 4)           +     // b5-8 is CWMAX
									   (pAd->ApCfg.BssEdcaParm.Cwmin[i] & 0x0f);              // b0-3 is CWMIN
				WmeParmIe[12+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] & 0xff);        // low byte of TXOP
				WmeParmIe[13+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] >> 8);          // high byte of TXOP
			}

			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen,
							  26,                       WmeParmIe,
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}


#ifdef DOT11N_DRAFT3
	 	// P802.11n_D3.03
	 	// 7.3.2.60 Overlapping BSS Scan Parameters IE
	 	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) && 
			(pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == 1))
	 	{
			OVERLAP_BSS_SCAN_IE  OverlapScanParam;
			ULONG	TmpLen, ScanIELen;
			UCHAR	OverlapScanIE;

			OverlapScanIE = IE_OVERLAPBSS_SCAN_PARM;
			ScanIELen = 14;
			OverlapScanParam.ScanPassiveDwell = cpu2le16(pAd->CommonCfg.Dot11OBssScanPassiveDwell);
			OverlapScanParam.ScanActiveDwell = cpu2le16(pAd->CommonCfg.Dot11OBssScanActiveDwell);
			OverlapScanParam.TriggerScanInt = cpu2le16(pAd->CommonCfg.Dot11BssWidthTriggerScanInt);
			OverlapScanParam.PassiveTalPerChannel = cpu2le16(pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel);
			OverlapScanParam.ActiveTalPerChannel = cpu2le16(pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel);
			OverlapScanParam.DelayFactor = cpu2le16(pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor);
			OverlapScanParam.ScanActThre = cpu2le16(pAd->CommonCfg.Dot11OBssScanActivityThre);
			
			MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
								1,			&OverlapScanIE,
								1,			&ScanIELen,
								ScanIELen,	&OverlapScanParam,
								END_OF_ARGS);
			
			FrameLen += TmpLen;
	 	}
#endif // DOT11N_DRAFT3 //

		// P802.11n_D1.10 
		// 7.3.2.27 Extended Capabilities IE
		// HT Information Exchange Support
		if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
		{
			ULONG TmpLen;
			UCHAR ExtCapIe[3] = {IE_EXT_CAPABILITY, 1, 0x01};
#ifdef DOT11N_DRAFT3		
			EXT_CAP_INFO_ELEMENT	extCapInfo;

			NdisZeroMemory(&extCapInfo, sizeof(EXT_CAP_INFO_ELEMENT));
			extCapInfo.BssCoexistMgmtSupport = 1;
			NdisMoveMemory(&ExtCapIe[3], &extCapInfo, 1);
#endif // DOT11N_DRAFT3 //
			MakeOutgoingFrame(pOutBuffer+FrameLen, &TmpLen,
								3,                  ExtCapIe,
								END_OF_ARGS);
			FrameLen += TmpLen;

		}

		// add Ralink-specific IE here - Byte0.b0=1 for aggregation, Byte0.b1=1 for piggy-back
{
	ULONG TmpLen;
	UCHAR RalinkSpecificIe[9] = {IE_VENDOR_SPECIFIC, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00};
	if (pAd->CommonCfg.bAggregationCapable)
		RalinkSpecificIe[5] |= 0x1;
	if (pAd->CommonCfg.bPiggyBackCapable)
		RalinkSpecificIe[5] |= 0x2;
	if (pAd->CommonCfg.bRdg)
		RalinkSpecificIe[5] |= 0x4;

	MakeOutgoingFrame(pOutBuffer+FrameLen,		 &TmpLen,
						9,						 RalinkSpecificIe,
						END_OF_ARGS);
	FrameLen += TmpLen;
}

		// add Channel switch announcement IE
		if ((pAd->CommonCfg.Channel > 14)
			&& (pAd->CommonCfg.bIEEE80211H == 1)
			&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
		{
			UCHAR CSAIe=IE_CHANNEL_SWITCH_ANNOUNCEMENT;
			UCHAR CSALen=3;
			UCHAR CSAMode=1;

			MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen,
							  1,                        &CSAIe,
							  1,                        &CSALen,
							  1,                        &CSAMode,
							  1,                        &pAd->CommonCfg.Channel,
							  1,                        &pAd->CommonCfg.RadarDetect.CSCount,
							  END_OF_ARGS);
			FrameLen += TmpLen;

   			if (pAd->CommonCfg.bExtChannelSwitchAnnouncement)
			{
				HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE	HtExtChannelSwitchIe;

				build_ext_channel_switch_ie(pAd, &HtExtChannelSwitchIe);
				MakeOutgoingFrame(pOutBuffer + FrameLen,             &TmpLen,
								  sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE),	&HtExtChannelSwitchIe,
								  END_OF_ARGS);
			}
			FrameLen += TmpLen;
		}

	    // add country IE, power constraint IE
		if (pAd->CommonCfg.bCountryFlag)
		{
		    ULONG TmpLen2=0;
		    UCHAR TmpFrame[256];
		    UCHAR CountryIe = IE_COUNTRY;
		    UCHAR MaxTxPower=16;

			// Only 802.11a APs that comply with 802.11h are required to include a Power Constrint Element(IE=32) 
			// in beacons and probe response frames
			if (pAd->CommonCfg.Channel > 14 && pAd->CommonCfg.bIEEE80211H == TRUE)
			{
		        // prepare power constraint IE
		        MakeOutgoingFrame(pOutBuffer+FrameLen,    &TmpLen,
		                          3,                 	PowerConstraintIE,
		                          END_OF_ARGS);
		        FrameLen += TmpLen;
			}

		    NdisZeroMemory(TmpFrame, sizeof(TmpFrame));

			// prepare channel information
		    MakeOutgoingFrame(TmpFrame+TmpLen2,     &TmpLen,
		                          1,                 	&pAd->ChannelList[0].Channel,
		                          1,                 	&pAd->ChannelListNum,
		                          1,                 	&MaxTxPower,
		                          END_OF_ARGS);
		    TmpLen2 += TmpLen;

		    // need to do the padding bit check, and concatenate it
		    if ((TmpLen2%2) == 0)
		    {
		       	UCHAR	TmpLen3 = TmpLen2+4;
			    MakeOutgoingFrame(pOutBuffer+FrameLen,  &TmpLen,
			                         1,                 	&CountryIe,
			                          1,                 	&TmpLen3,
			                          3,                 	pAd->CommonCfg.CountryCode,
			                          TmpLen2+1,				TmpFrame,
			                          END_OF_ARGS);
		    } 
		    else
		    {
		       	UCHAR	TmpLen3 = TmpLen2+3;
			    MakeOutgoingFrame(pOutBuffer+FrameLen,  &TmpLen,
			                          1,                 	&CountryIe,
			                          1,                 	&TmpLen3,
			                          3,                 	pAd->CommonCfg.CountryCode,
			                          TmpLen2,				TmpFrame,
			                          END_OF_ARGS);
		    }
		    FrameLen += TmpLen;
		}// Country IE -

		if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
		{
			ULONG TmpLen;
			UCHAR	HtLen, AddHtLen;//, NewExtLen;
#ifdef RT_BIG_ENDIAN
			HT_CAPABILITY_IE HtCapabilityTmp;
			ADD_HT_INFO_IE	addHTInfoTmp;
#endif
			HtLen = sizeof(pAd->CommonCfg.HtCapability);
			AddHtLen = sizeof(pAd->CommonCfg.AddHTInfo);

		if (pAd->bBroadComHT == TRUE)
		{
			UCHAR	epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = HtLen + 4;
#ifndef RT_BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
							  1,                                &epigram_ie_len,
							  4,                                &BROADCOM_HTC[0],
							  HtLen,          					&pAd->CommonCfg.HtCapability, 
							  END_OF_ARGS);
#else
				NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
				*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
				*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

				MakeOutgoingFrame(pOutBuffer + FrameLen,         &TmpLen,
								1,                               &WpaIe,
								1,                               &epigram_ie_len,
								4,                               &BROADCOM_HTC[0], 
								HtLen,                           &HtCapabilityTmp,
								END_OF_ARGS);
#endif

				FrameLen += TmpLen;

				epigram_ie_len = AddHtLen + 4;
#ifndef RT_BIG_ENDIAN
				MakeOutgoingFrame(pOutBuffer + FrameLen,          &TmpLen,
								  1,                              &WpaIe,
								  1,                              &epigram_ie_len,
								  4,                              &BROADCOM_AHTINFO[0],
								  AddHtLen, 					  &pAd->CommonCfg.AddHTInfo, 
								  END_OF_ARGS);
#else
				NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, AddHtLen);
				*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
				*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

				MakeOutgoingFrame(pOutBuffer + FrameLen,         &TmpLen,
								1,                               &WpaIe,
								1,                               &epigram_ie_len,
								4,                               &BROADCOM_AHTINFO[0],
								AddHtLen,                        &addHTInfoTmp,
							  END_OF_ARGS);
#endif

				FrameLen += TmpLen;
			}
		}

#ifdef WSC_AP_SUPPORT
        // add Simple Config Information Element
        if ((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode > WSC_DISABLE) && (pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.ValueLen))
        {
    		ULONG WscTmpLen = 0;
    		MakeOutgoingFrame(pOutBuffer+FrameLen,                                  &WscTmpLen,
    						  pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.ValueLen,   pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.Value,
                              END_OF_ARGS);
    		FrameLen += WscTmpLen;
        }
#endif // WSC_AP_SUPPORT //

		// 802.11n 11.1.3.2.2 active scanning. sending probe response with MCS rate is 
		MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, pOutBuffer);
	}
}

/* 
	==========================================================================
	Description:
		parse the received BEACON

	NOTE:
		The only thing AP cares about received BEACON frames is to decide 
		if there's any overlapped legacy BSS condition (OLBC).
		If OLBC happened, this AP should set the ERP->Use_Protection bit in its 
		outgoing BEACON. The result is to tell all its clients to use RTS/CTS
		or CTS-to-self protection to protect B/G mixed traffic
	==========================================================================
 */


typedef struct 
{
	ULONG	count;
	UCHAR	bssid[MAC_ADDR_LEN];
} BSSIDENTRY;

#if 0 // sample take off, no use
#define BSSID_MAX		32
BSSIDENTRY bssentry[BSSID_MAX];


BOOLEAN isSameChannel(UCHAR *Bssid)
{
	CHAR idx;

	for (idx=0; idx<BSSID_MAX; idx++)
	{
		if (memcmp(bssentry[idx].bssid, Bssid,  MAC_ADDR_LEN) == 0) 
		{           
			return TRUE;
		}
	}

	return FALSE;
}
#endif


VOID APPeerBeaconAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR         Bssid[MAC_ADDR_LEN], Addr2[MAC_ADDR_LEN];
	CHAR          Ssid[MAX_LEN_OF_SSID];
	UCHAR         SsidLen, MessageToMe=0, BssType, Channel;
	UCHAR         Rates[MAX_LEN_OF_SUPPORTED_RATES];
	PUCHAR        pRates = NULL;
	UCHAR         RatesLen, DtimCount=0, DtimPeriod=0, BcastFlag=0;
	USHORT        CapabilityInfo, BeaconPeriod;
	LARGE_INTEGER TimeStamp;
	//BOOLEAN       ExtendedRateIeExist;
	BOOLEAN       LegacyBssExist;
	UCHAR         Erp;

	UCHAR NewChannel;
	CF_PARM CfParm;
	USHORT AtimWin;
	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES], ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR SupRateLen, ExtRateLen;
	UCHAR CkipFlag;
	UCHAR AironetCellPowerLimit;
	EDCA_PARM EdcaParm;
	QBSS_LOAD_PARM QbssLoad;
	QOS_CAPABILITY_PARM QosCapability;
    ULONG RalinkIe;
	UCHAR VarIE[MAX_VIE_LEN];		// Total VIE length = MAX_VIE_LEN - -5
    USHORT LenVIE;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	HT_CAPABILITY_IE HtCapability;
	ADD_HT_INFO_IE AddHtInfo;	// AP might use this additional ht info IE 
	UCHAR HtCapabilityLen;
	UCHAR AddHtInfoLen;
	UCHAR NewExtChannelOffset = 0xff;
	UCHAR MaxSupportedRate = 0;



	RTMPZeroMemory(&HtCapability, sizeof(HT_CAPABILITY_IE));
	RTMPZeroMemory(&AddHtInfo, sizeof(ADD_HT_INFO_IE));

	// Init Variable IE structure
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	pRates = (PUCHAR)Rates;

	if (PeerBeaconAndProbeRspSanity(pAd, 
								Elem->Msg, 
								Elem->MsgLen, 
								Elem->Channel,
								Addr2, 
								Bssid, 
								Ssid, 
								&SsidLen, 
								&BssType, 
								&BeaconPeriod, 
								&Channel, 
								&NewChannel,
								&TimeStamp, 
								&CfParm, 
								&AtimWin, 
								&CapabilityInfo, 
								&Erp,
								&DtimCount, 
								&DtimPeriod, 
								&BcastFlag, 
								&MessageToMe, 
								SupRate,
								&SupRateLen,
								ExtRate,
								&ExtRateLen,
								&CkipFlag,
								&AironetCellPowerLimit,
								&EdcaParm,
								&QbssLoad,
								&QosCapability,
								&RalinkIe,
								&HtCapabilityLen,
								&HtCapability,
								&AddHtInfoLen,
								&AddHtInfo,
								&NewExtChannelOffset,
								&LenVIE,
								pVIE))
	{
		CHAR RealRssi;

		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0),
								ConvertToRssi(pAd, Elem->Rssi1, RSSI_1),
								ConvertToRssi(pAd, Elem->Rssi2, RSSI_2));


		if (Channel == pAd->ApCfg.AutoChannel_Channel)
		{
			AutoChBssInsertEntry(pAd, Bssid, Ssid, SsidLen, Channel, RealRssi);

			if ((RealRssi + pAd->BbpRssiToDbmDelta) > pAd->ApCfg.AutoChannel_MaxRssi)
				pAd->ApCfg.AutoChannel_MaxRssi = RealRssi + pAd->BbpRssiToDbmDelta;
		}


		// ignore BEACON not in this channel
		if (Channel != pAd->CommonCfg.Channel
#ifdef DOT11N_DRAFT3
			&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif // DOT11N_DRAFT3 //
			)
			return;

#ifdef IDS_SUPPORT
		// Conflict SSID detection 
		RTMPConflictSsidDetection(pAd, Ssid, SsidLen, Elem->Rssi0, Elem->Rssi1, Elem->Rssi2);
#endif // IDS_SUPPORT //

#if 0 // sample take off, no use
{
		CHAR idx, victim;

		victim = -1;


		for (idx=0; idx<BSSID_MAX; idx++)
		{
			if (memcmp(bssentry[idx].bssid, Bssid,  MAC_ADDR_LEN) == 0) 
			{           
				victim = idx;
				break;
			}
		}

		// not found 
		if (victim == -1)
		{
			victim = 0;
			for (idx=1; idx<BSSID_MAX; idx++)
			{
				if (bssentry[idx].count < bssentry[victim].count)
				{
					victim = idx;
				}
			}
		}
		COPY_MAC_ADDR(bssentry[victim].bssid, Bssid);
		bssentry[victim].count++;

		//DBGPRINT(RT_DEBUG_TRACE, ("%d -- %02x:%02x:%02x:%02x:%02x:%02x is a legacy BSS (%d) \n", 
		//					victim ,Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5], Channel));
}
#endif
			

		// 40Mhz BSS Width Trigger events
		// Intolerant devices
		if ((RealRssi > OBSS_BEACON_RSSI_THRESHOLD) && (HtCapability.HtCapInfo.Forty_Mhz_Intolerant)) // || (HtCapabilityLen == 0)))
		{
			Handle_BSS_Width_Trigger_Events(pAd);					
		}


#if 1
		if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40) 
#ifdef DOT11N_DRAFT3
			&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif // DOT11N_DRAFT3 //
		   ) 
		{
			if (pAd->CommonCfg.Channel<=14)
			{
				if (((pAd->CommonCfg.CentralChannel+2) != Channel) && 
					((pAd->CommonCfg.CentralChannel-2) != Channel))
				{	
					//DBGPRINT(RT_DEBUG_TRACE, ("%02x:%02x:%02x:%02x:%02x:%02x is a legacy BSS (%d) \n", 
					//Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5], Channel));
					return;
				}
			}
			else
			{
		if (Channel != pAd->CommonCfg.Channel)
			return;
			}
		}
#endif
                SupportRate(SupRate, SupRateLen, ExtRate, ExtRateLen, &pRates, &RatesLen, &MaxSupportedRate);
		
                if ((Erp & 0x01) || (RatesLen <= 4))
			LegacyBssExist = TRUE;
		else
			LegacyBssExist = FALSE;

		if (LegacyBssExist && pAd->CommonCfg.DisableOLBCDetect == 0)
		{
			pAd->ApCfg.LastOLBCDetectTime = pAd->Mlme.Now32;
			DBGPRINT(RT_DEBUG_INFO, ("%02x:%02x:%02x:%02x:%02x:%02x is a legacy BSS (rate# =%d, ERP=%d), set Use_Protection bit\n", 
				Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5], RatesLen, Erp));
		}            

		if ((pAd->CommonCfg.bHTProtect)
			&& (HtCapabilityLen == 0) && (RealRssi > OBSS_BEACON_RSSI_THRESHOLD))
		{
			DBGPRINT(RT_DEBUG_INFO, ("(RSSI:%d) %02x:%02x:%02x:%02x:%02x:%02x is a legacy BSS (rate# =%d, ERP=%d), set Use_Protection bit\n", 
				RealRssi, Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5], RatesLen, Erp));
			pAd->ApCfg.LastNoneHTOLBCDetectTime = pAd->Mlme.Now32;
		}

#ifdef APCLI_SUPPORT
#if 1
		if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
		{
			PMAC_TABLE_ENTRY pEntry = NULL;

			pEntry = &pAd->MacTab.Content[Elem->Wcid];
			
			if (pEntry && pEntry->ValidAsApCli && (pEntry->MatchAPCLITabIdx < MAX_APCLI_NUM))
				pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].ApCliRcvBeaconTime = pAd->Mlme.Now32;
		}
#else
		do
		{
			PMAC_TABLE_ENTRY pEntry = NULL;

			// check BEACON does in ApCli TABLE.
			pEntry = ApCliTableLookUp(pAd, Addr2);
			if (pEntry)
			{				
				pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].ApCliRcvBeaconTime = pAd->Mlme.Now32;
			}
		} while(FALSE);
#endif		
#endif // APCLI_SUPPORT //

#ifdef WDS_SUPPORT
		do
		{
			PMAC_TABLE_ENTRY pEntry;
			BOOLEAN bWmmCapable;

			// check BEACON does in WDS TABLE.
			pEntry = WdsTableLookup(pAd, Addr2, FALSE);
			bWmmCapable = EdcaParm.bValid ? TRUE : FALSE;

			if (pEntry)
			{
				WdsPeerBeaconProc(pAd, pEntry, CapabilityInfo, MaxSupportedRate, RatesLen, bWmmCapable, RalinkIe, &HtCapability, HtCapabilityLen);
			}
		} while(FALSE);
#endif // WDS_SUPPORT //

#ifdef DOT11N_DRAFT3
		if (pAd->CommonCfg.bOverlapScanning == TRUE)
		{
			CHAR		index, secChIdx;
			BOOLEAN		found = FALSE;

			for (index = 0; index < pAd->ChannelListNum; index++)
			{
				// found the effected channel, mark that.
				if(pAd->ChannelList[index].Channel == Channel)
				{
					secChIdx = -1;
					if (HtCapabilityLen > 0 && AddHtInfoLen > 0)
					{	// This is a 11n AP.
						pAd->ChannelList[index].bEffectedChannel |= 2; 	// 2 for 11N 20/40MHz AP with primary channel set as this channel.
						if (AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW)
						{
							if (Channel > 14)
								secChIdx = ((index > 0) ? (index - 1) : -1);
							else
								secChIdx = ((index >= 4) ? (index - 4) : -1);
						}
						else if (AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE)
						{
							if (Channel > 14)
								secChIdx = (((index+1) < pAd->ChannelListNum) ? (index + 1) : -1);
							else
								secChIdx = (((index+4) < pAd->ChannelListNum) ? (index + 4) : -1);
						}

						if (secChIdx >=0)
							pAd->ChannelList[secChIdx].bEffectedChannel |= 1;
					}
					else
					{
						// This is a legacy AP.
						pAd->ChannelList[index].bEffectedChannel |= 4; // 1 for legacy AP.
					}

					found = TRUE;
				}
			}
		}
#endif // DOT11N_DRAFT3 //

	}
	// sanity check fail, ignore this frame
}

/* 
    ==========================================================================
    Description:
        MLME SCAN req state machine procedure
    ==========================================================================
 */
VOID APMlmeScanReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN        Cancelled;
	UCHAR          Ssid[MAX_LEN_OF_SSID], SsidLen, ScanType, BssType, BBPValue = 0;
	ULONG          Now;

	// Suspend MSDU transmission here
	RTMPSuspendMsduTransmission(pAd);

	// first check the parameter sanity
	if (MlmeScanReqSanity(pAd, Elem->Msg, Elem->MsgLen, &BssType, Ssid, &SsidLen, &ScanType))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("AP SYNC - MlmeScanReqAction\n"));
		Now = jiffies;
		pAd->ApCfg.LastScanTime = Now;

		RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);

		// record desired BSS parameters
		pAd->MlmeAux.BssType = BssType;
		pAd->MlmeAux.ScanType = ScanType;
		pAd->MlmeAux.SsidLen = SsidLen;
		NdisMoveMemory(pAd->MlmeAux.Ssid, Ssid, SsidLen);

		// start from the first channel
		pAd->MlmeAux.Channel = FirstChannel(pAd);

		// Let BBP register at 20MHz to do scan		
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &BBPValue);
		BBPValue &= (~0x18);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, BBPValue);
		DBGPRINT(RT_DEBUG_TRACE, ("SYNC - BBP R4 to 20MHz.l\n"));

		ScanNextChannel(pAd);
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("AP SYNC - MlmeScanReqAction() sanity check fail. BUG!!!\n"));
		pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
	}
}

/* 
    ==========================================================================
    Description:
        peer sends beacon back when scanning
    ==========================================================================
 */
VOID APPeerBeaconAtScanAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
    UCHAR           Bssid[MAC_ADDR_LEN], Addr2[MAC_ADDR_LEN];
    UCHAR           Ssid[MAX_LEN_OF_SSID], BssType, Channel, NewChannel,  
                    SsidLen, DtimCount, DtimPeriod, BcastFlag, MessageToMe;
    CF_PARM         CfParm;
    USHORT          BeaconPeriod, AtimWin, CapabilityInfo;
    PFRAME_802_11   pFrame;
    LARGE_INTEGER   TimeStamp;
    UCHAR           Erp;
	UCHAR         	SupRate[MAX_LEN_OF_SUPPORTED_RATES], ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR		  	SupRateLen, ExtRateLen;
	UCHAR			CkipFlag;
	UCHAR			AironetCellPowerLimit;
	EDCA_PARM       EdcaParm;
	QBSS_LOAD_PARM  QbssLoad;
	QOS_CAPABILITY_PARM QosCapability;
    ULONG           RalinkIe;
	UCHAR						VarIE[MAX_VIE_LEN];		// Total VIE length = MAX_VIE_LEN - -5
    USHORT          LenVIE;
	NDIS_802_11_VARIABLE_IEs	*pVIE = NULL;
	HT_CAPABILITY_IE	HtCapability;
	ADD_HT_INFO_IE	AddHtInfo;	// AP might use this additional ht info IE 
	UCHAR			HtCapabilityLen;
	UCHAR			AddHtInfoLen;
	UCHAR			NewExtChannelOffset = 0xff;


	SsidLen = 0;

	RTMPZeroMemory(&HtCapability, sizeof(HT_CAPABILITY_IE));
	RTMPZeroMemory(&AddHtInfo, sizeof(ADD_HT_INFO_IE));

    pFrame = (PFRAME_802_11) Elem->Msg;
	// Init Variable IE structure
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	
	
	if (PeerBeaconAndProbeRspSanity(pAd, 
								Elem->Msg, 
								Elem->MsgLen,
								Elem->Channel,
								Addr2, 
								Bssid, 
								Ssid, 
								&SsidLen, 
								&BssType, 
								&BeaconPeriod, 
								&Channel, 
								&NewChannel,
								&TimeStamp, 
								&CfParm, 
								&AtimWin, 
								&CapabilityInfo, 
								&Erp,
								&DtimCount, 
								&DtimPeriod, 
								&BcastFlag, 
								&MessageToMe, 
								SupRate,
								&SupRateLen,
								ExtRate,
								&ExtRateLen,
								&CkipFlag,
								&AironetCellPowerLimit,
								&EdcaParm,
								&QbssLoad,
								&QosCapability,
								&RalinkIe,
								&HtCapabilityLen,
								&HtCapability,
								&AddHtInfoLen,
								&AddHtInfo,
								&NewExtChannelOffset,
								&LenVIE,
								pVIE))
    {
		ULONG Idx;
		CHAR  Rssi = -127;
        CHAR  RealRssi;

		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0), ConvertToRssi(pAd, Elem->Rssi1, RSSI_1), ConvertToRssi(pAd, Elem->Rssi2, RSSI_2));


		DBGPRINT(RT_DEBUG_INFO, ("Channel = %d, RealRssi = %d\n", Channel, RealRssi));

   		if ((RealRssi > OBSS_BEACON_RSSI_THRESHOLD) && (HtCapability.HtCapInfo.Forty_Mhz_Intolerant)) // || (HtCapabilityLen == 0)))
		{
			Handle_BSS_Width_Trigger_Events(pAd);					
		}

#ifdef IDS_SUPPORT
		// Conflict SSID detection		
		if (Channel == pAd->CommonCfg.Channel)		
			RTMPConflictSsidDetection(pAd, Ssid, SsidLen, Elem->Rssi0, Elem->Rssi1, Elem->Rssi2);
#endif // IDS_SUPPORT //

		// This correct im-proper RSSI indication during SITE SURVEY issue.
		// Always report bigger RSSI during SCANNING when receiving multiple BEACONs from the same AP. 
		// This case happens because BEACONs come from adjacent channels, so RSSI become weaker as we 
		// switch to more far away channels.
        Idx = BssTableSearch(&pAd->ScanTab, Bssid, Channel);
		if (Idx != BSS_NOT_FOUND) 
            Rssi = pAd->ScanTab.BssEntry[Idx].Rssi;

		DBGPRINT(RT_DEBUG_INFO, ("(RSSI:%d) %02x:%02x:%02x:%02x:%02x:%02x is a [legacy] BSS\n", 
				Rssi, Bssid[0], Bssid[1], Bssid[2], Bssid[3], Bssid[4], Bssid[5]));

        // TODO: 2005-03-04 dirty patch. we should change all RSSI related variables to SIGNED SHORT for easy/efficient reading and calaulation
		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0), ConvertToRssi(pAd, Elem->Rssi1, RSSI_1), ConvertToRssi(pAd, Elem->Rssi2, RSSI_2));
        if ((RealRssi + pAd->BbpRssiToDbmDelta) > Rssi)
            Rssi = RealRssi + pAd->BbpRssiToDbmDelta;

		Idx = BssTableSetEntry(pAd, &pAd->ScanTab, Bssid, Ssid, SsidLen, BssType, BeaconPeriod,
				&CfParm, AtimWin, CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,  &HtCapability,
				&AddHtInfo, HtCapabilityLen, AddHtInfoLen, NewExtChannelOffset, Channel, Rssi, TimeStamp, CkipFlag, 
				&EdcaParm, &QosCapability, &QbssLoad, LenVIE, pVIE);
		if (Idx != BSS_NOT_FOUND)
		{
			NdisMoveMemory(pAd->ScanTab.BssEntry[Idx].PTSF, &Elem->Msg[24], 4);
			NdisMoveMemory(&pAd->ScanTab.BssEntry[Idx].TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
			NdisMoveMemory(&pAd->ScanTab.BssEntry[Idx].TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
		}
	}
	// sanity check fail, ignored
}

/* 
    ==========================================================================
    Description:
        MLME Cancel the SCAN req state machine procedure
    ==========================================================================
 */
VOID APScanCnclAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN Cancelled;

	RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);
	pAd->MlmeAux.Channel = 0;
	ScanNextChannel(pAd);

	return;
}

/* 
    ==========================================================================
    Description:
        Site survey entry point

    NOTE:
    ==========================================================================
*/
VOID ApSiteSurvey(
	IN   PRTMP_ADAPTER   pAd)
{
    MLME_SCAN_REQ_STRUCT    ScanReq;
    CHAR                    BroadSsid[MAX_LEN_OF_SSID];

    AsicDisableSync(pAd);

    BssTableInit(&pAd->ScanTab);
    BroadSsid[0] = '\0';
    pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;

    ScanReq.SsidLen = 0;
    NdisMoveMemory(ScanReq.Ssid, BroadSsid, ScanReq.SsidLen);
    ScanReq.BssType = BSS_ANY;
    ScanReq.ScanType = SCAN_PASSIVE;
    
    MlmeEnqueue(pAd, AP_SYNC_STATE_MACHINE, APMT2_MLME_SCAN_REQ, sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq);
    RT28XX_MLME_HANDLER(pAd);
}

VOID SupportRate(
	IN PUCHAR SupRate,
	IN UCHAR SupRateLen,
	IN PUCHAR ExtRate,
	IN UCHAR ExtRateLen,
	OUT PUCHAR *ppRates,
	OUT PUCHAR RatesLen,
	OUT PUCHAR pMaxSupportRate)
{
	INT i;

	*pMaxSupportRate = 0;

	if ((SupRateLen <= MAX_LEN_OF_SUPPORTED_RATES) && (SupRateLen > 0))
	{
		NdisMoveMemory(*ppRates, SupRate, SupRateLen);
		*RatesLen = SupRateLen;
	}
	else
	{
		// HT rate not ready yet. return true temporarily. rt2860c
		//DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n"));
		*RatesLen = 8;
		*(*ppRates + 0) = 0x82;
		*(*ppRates + 1) = 0x84;
		*(*ppRates + 2) = 0x8b;
		*(*ppRates + 3) = 0x96;
		*(*ppRates + 4) = 0x12;
		*(*ppRates + 5) = 0x24;
		*(*ppRates + 6) = 0x48;
		*(*ppRates + 7) = 0x6c;
		DBGPRINT(RT_DEBUG_TRACE, ("SUPP_RATES., Len=%d\n", SupRateLen));
	}

	if (ExtRateLen + *RatesLen <= MAX_LEN_OF_SUPPORTED_RATES)
	{
		NdisMoveMemory((*ppRates + (ULONG)*RatesLen), ExtRate, ExtRateLen);
		*RatesLen = (*RatesLen) + ExtRateLen;
	}
	else
	{
		NdisMoveMemory((*ppRates + (ULONG)*RatesLen), ExtRate, MAX_LEN_OF_SUPPORTED_RATES - (*RatesLen));
		*RatesLen = MAX_LEN_OF_SUPPORTED_RATES;
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s - SUPP_RATES., Len=%d. Rates[0]=%x\n", __FUNCTION__, *RatesLen, **ppRates));
	DBGPRINT(RT_DEBUG_INFO, ("Rates[1]=%x %x %x %x %x %x %x\n",
			*(*ppRates+1), *(*ppRates+2), *(*ppRates+3), *(*ppRates+4), *(*ppRates+5), *(*ppRates+6), *(*ppRates+7)));

	for (i = 0; i < *RatesLen; i++)
	{
		if(*pMaxSupportRate < (*(*ppRates + i) & 0x7f))
			*pMaxSupportRate = (*(*ppRates + i) & 0x7f);
	}

	return;
}

/* Regulatory classes in the USA */

typedef struct
{
	UCHAR	regclass;		// regulatory class
	UCHAR	spacing;		// 0: 20Mhz, 1: 40Mhz
	UCHAR	channelset[16];	// max 15 channels, use 0 as terminator 
} REG_CLASS;

REG_CLASS reg_class[] =
{
	{  1, 0, {36, 40, 44, 48, 0}},
	{  2, 0, {52, 56, 60, 64, 0}},
	{  3, 0, {149, 153, 157, 161, 0}},
	{  4, 0, {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 0}},
	{  5, 0, {165, 0}},
	{ 22, 1, {36, 44, 0}},
	{ 23, 1, {52, 60, 0}},
	{ 24, 1, {100, 108, 116, 124, 132, 0}},
	{ 25, 1, {149, 157, 0}},
	{ 26, 1, {149, 157, 0}},
	{ 27, 1, {40, 48, 0}},
	{ 28, 1, {56, 64, 0}},
	{ 29, 1, {104, 112, 120, 128, 136, 0}},
	{ 30, 1, {153, 161, 0}},
	{ 31, 1, {153, 161, 0}},
	{ 32, 1, {1, 2, 3, 4, 5, 6, 7, 0}},
	{ 33, 1, {5, 6, 7, 8, 9, 10, 11, 0}},
	{ 0,  0, {0}}			// end
};


UCHAR get_regulatory_class(
	IN PRTMP_ADAPTER pAd)
{
	int i=0;
	UCHAR regclass = 0;

	do
	{
		if (reg_class[i].spacing == pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth)
		{
			int j=0;

			do
			{
				if (reg_class[i].channelset[j] == pAd->CommonCfg.Channel)
				{
					regclass = reg_class[i].regclass;
					break;
				}
				j++;
			} while (reg_class[i].channelset[j] != 0);
		}
		i++;
	} while (reg_class[i].regclass != 0);

	ASSERT(regclass);

	return regclass; 
}


void build_ext_channel_switch_ie(
	IN PRTMP_ADAPTER pAd,
	IN HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE *pIE)
{

	pIE->ID = IE_EXT_CHANNEL_SWITCH_ANNOUNCEMENT;
	pIE->Length = 4;
	pIE->ChannelSwitchMode = 1;	//no further frames
	pIE->NewRegClass = get_regulatory_class(pAd);
	pIE->NewChannelNum = pAd->CommonCfg.Channel;
    pIE->ChannelSwitchCount = pAd->CommonCfg.RadarDetect.CSCount;
}

BOOLEAN ApScanRunning(
		IN PRTMP_ADAPTER pAd)
{
	return (pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN) ? TRUE : FALSE;
}

