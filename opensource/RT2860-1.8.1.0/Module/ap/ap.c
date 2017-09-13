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
    soft_ap.c
 
    Abstract:
    Access Point specific routines and MAC table maintenance routines
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP

 */

#include "rt_config.h"

BOOLEAN ApCheckLongPreambleSTA(
    IN PRTMP_ADAPTER pAd);

char const *pEventText[EVENT_MAX_EVENT_TYPE] = {
	"restart access point",
	"successfully associated",
	"has disassociated",
	"has been aged-out and disassociated" ,    
	"active countermeasures",
	"has disassociated with invalid PSK password"};

static inline BOOLEAN BW40_ChannelCheck(
	IN UCHAR ch)
{
	INT i;
	BOOLEAN result = TRUE;
	UCHAR NorBW40_CH[] = {140, 165};
	UCHAR NorBW40ChNum = sizeof(NorBW40_CH) / sizeof(UCHAR);

	for (i=0; i<NorBW40ChNum; i++)
	{
		if (ch == NorBW40_CH[i])
		{
			result = FALSE;
			break;
		}
	}

	return result;
}

/*
	==========================================================================
	Description:
		Initialize AP specific data especially the NDIS packet pool that's
		used for wireless client bridging.
	==========================================================================
 */
NDIS_STATUS APInitialize(
	IN  PRTMP_ADAPTER   pAd)
{
	NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
	//UCHAR   GTK[TKIP_GTK_LENGTH];

	DBGPRINT(RT_DEBUG_TRACE, ("---> APInitialize\n"));

	// Init Group key update timer, and countermeasures timer
	RTMPInitTimer(pAd, &pAd->ApCfg.REKEYTimer, GET_TIMER_FUNCTION(GREKEYPeriodicExec), pAd,  TRUE); 
	RTMPInitTimer(pAd, &pAd->ApCfg.CounterMeasureTimer, GET_TIMER_FUNCTION(CMTimerExec), pAd, FALSE);

#ifdef IDS_SUPPORT
	// Init intrusion detection timer
	RTMPInitTimer(pAd, &pAd->ApCfg.IDSTimer, GET_TIMER_FUNCTION(RTMPIdsPeriodicExec), pAd, FALSE);
	pAd->ApCfg.IDSTimerRunning = FALSE;
#endif // IDS_SUPPORT //

#ifdef WDS_SUPPORT
	APWdsInitialize(pAd);
#endif // WDS_SUPPORT //

#ifdef IGMP_SNOOP_SUPPORT
	MulticastFilterTableInit(&pAd->pMulticastFilterTable);
#endif // IGMP_SNOOP_SUPPORT //

	NdisAllocateSpinLock(&pAd->WdsTabLock);

	DBGPRINT(RT_DEBUG_TRACE, ("<--- APInitialize\n"));
	return Status;
}

/*
	==========================================================================
	Description:
		Shutdown AP and free AP specific resources
	==========================================================================
 */
VOID APShutdown(
	IN PRTMP_ADAPTER pAd)
{
	DBGPRINT(RT_DEBUG_TRACE, ("---> APShutdown\n"));
//	if (pAd->OpMode == OPMODE_AP)
		APStop(pAd);

	MlmeRadioOff(pAd);
#ifdef RT2860
	// reset DMA Index
	RTMP_IO_WRITE32(pAd, WPDMA_RST_IDX , 0xFFFFFFFF);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe1f);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe00);
#endif // RT2860 //

#ifdef IGMP_SNOOP_SUPPORT
	MultiCastFilterTableReset(&pAd->pMulticastFilterTable);
#endif // IGMP_SNOOP_SUPPORT //

	NdisFreeSpinLock(&pAd->MacTabLock);
	NdisFreeSpinLock(&pAd->WdsTabLock);
	DBGPRINT(RT_DEBUG_TRACE, ("<--- APShutdown\n"));
}

/*
	==========================================================================
	Description:
		Start AP service. If any vital AP parameter is changed, a STOP-START
		sequence is required to disassociate all STAs.

	IRQL = DISPATCH_LEVEL.(from SetInformationHandler)
	IRQL = PASSIVE_LEVEL. (from InitializeHandler)  

	Note:
		Can't call NdisMIndicateStatus on this routine.

		RT61 is a serialized driver on Win2KXP and is a deserialized on Win9X
		Serialized callers of NdisMIndicateStatus must run at IRQL = DISPATCH_LEVEL.

	==========================================================================
 */
VOID APStartUp(
	IN PRTMP_ADAPTER pAd) 
{
	//UCHAR         GTK[TKIP_GTK_LENGTH];
//	UCHAR		BCASTADDR[6]={0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
	ULONG		offset, i;
	UINT32		Value = 0;
	BOOLEAN		bWmmCapable = FALSE;
	UCHAR		apidx;
	BOOLEAN		TxPreamble, SpectrumMgmt;
	UCHAR		BBPR1 = 0, BBPR3 = 0, byteValue = 0;

	
	DBGPRINT(RT_DEBUG_TRACE, ("===> APStartUp\n"));
		
	SetCommonHT(pAd);
	AsicDisableSync(pAd);

	TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);

	// Decide the Capability information field
    // In IEEE Std 802.1h-2003, the spectrum management bit is enabled in the 5 GHz band 
    if ((pAd->CommonCfg.Channel > 14) && pAd->CommonCfg.bIEEE80211H == TRUE)
    	SpectrumMgmt = TRUE;
	else
		SpectrumMgmt = FALSE;	
			
	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{
		if ((pAd->ApCfg.MBSSID[apidx].SsidLen <= 0) || (pAd->ApCfg.MBSSID[apidx].SsidLen > MAX_LEN_OF_SSID))
		{
			NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].Ssid, "HT_AP", 5);
			pAd->ApCfg.MBSSID[apidx].Ssid[5] = '0'+apidx;
			pAd->ApCfg.MBSSID[apidx].SsidLen = 6;			
		}

		/* re-copy the MAC to virtual interface to avoid these MAC = all zero,
		   when re-open the ra0,
		   i.e. ifconfig ra0 down, ifconfig ra0 up, ifconfig ra0 down, ifconfig up ... */
		COPY_MAC_ADDR(pAd->ApCfg.MBSSID[apidx].Bssid, pAd->CurrentAddress);
		pAd->ApCfg.MBSSID[apidx].Bssid[5] += apidx;

		if (pAd->ApCfg.MBSSID[apidx].MSSIDDev != NULL)
		{
			NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].MSSIDDev->dev_addr,
    	                   pAd->ApCfg.MBSSID[apidx].Bssid,
        	               MAC_ADDR_LEN);
		} /* End of if */

		if (pAd->ApCfg.MBSSID[apidx].bWmmCapable)
		{
        	bWmmCapable = TRUE;
		}
		
		pAd->ApCfg.MBSSID[apidx].CapabilityInfo =
			CAP_GENERATE(1, 0, (pAd->ApCfg.MBSSID[apidx].WepStatus != Ndis802_11EncryptionDisabled), TxPreamble, pAd->CommonCfg.bUseShortSlotTime, SpectrumMgmt);
		
		if (bWmmCapable == TRUE)
		{
			/* In page 38, QoS = CF-Pollable = CF-Poll Request = 0 means
			   no PC (PCF) function, dont need to set the bit */
//			pAd->ApCfg.MBSSID[apidx].CapabilityInfo |= 0x0200;
		} /* End of if */

#ifdef UAPSD_AP_SUPPORT
        if (pAd->CommonCfg.bAPSDCapable == TRUE)
		{
			/* QAPs set the APSD subfield to 1 within the Capability Information
			   field when the MIB attribute dot11APSDOptionImplemented is true
			   and set it to 0 otherwise. STAs always set this subfield to 0. */
            pAd->ApCfg.MBSSID[apidx].CapabilityInfo |= 0x0800;
        } /* End of if */
#endif // UAPSD_AP_SUPPORT //
	}

	
	COPY_MAC_ADDR(pAd->CommonCfg.Bssid, pAd->CurrentAddress);

	// Select DAC according to HT or Legacy, write to BBP R1(bit4:3)
	// In HT mode and two stream mode, both DACs are selected.
	// In legacy mode or one stream mode, DAC-0 is selected.
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) && (pAd->Antenna.field.TxPath == 2))
	{
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
		BBPR1 &= (~0x18);
		BBPR1 |= 0x10;
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
	}
	else
	{
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
		BBPR1 &= (~0x18);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
	}

	// Receiver Antenna selection, write to BBP R3(bit4:3)
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
	
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) || bWmmCapable)
	{
		// EDCA parameters used for AP's own transmission
		if (pAd->CommonCfg.APEdcaParm.bValid == FALSE)
		{
			pAd->CommonCfg.APEdcaParm.bValid = TRUE;
			pAd->CommonCfg.APEdcaParm.Aifsn[0] = 3;
			pAd->CommonCfg.APEdcaParm.Aifsn[1] = 7;
			pAd->CommonCfg.APEdcaParm.Aifsn[2] = 1;
			pAd->CommonCfg.APEdcaParm.Aifsn[3] = 1;

			pAd->CommonCfg.APEdcaParm.Cwmin[0] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmin[1] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmin[2] = 3;
			pAd->CommonCfg.APEdcaParm.Cwmin[3] = 2;

			pAd->CommonCfg.APEdcaParm.Cwmax[0] = 6;
			pAd->CommonCfg.APEdcaParm.Cwmax[1] = 10;
			pAd->CommonCfg.APEdcaParm.Cwmax[2] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmax[3] = 3;

			pAd->CommonCfg.APEdcaParm.Txop[0]  = 0;
			pAd->CommonCfg.APEdcaParm.Txop[1]  = 0;
			pAd->CommonCfg.APEdcaParm.Txop[2]  = 94;	//96;
			pAd->CommonCfg.APEdcaParm.Txop[3]  = 47;	//48;
		}
		AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);

		// EDCA parameters to be annouced in outgoing BEACON, used by WMM STA
		if (pAd->ApCfg.BssEdcaParm.bValid == FALSE)
		{
			pAd->ApCfg.BssEdcaParm.bValid = TRUE;
			pAd->ApCfg.BssEdcaParm.Aifsn[0] = 3;
			pAd->ApCfg.BssEdcaParm.Aifsn[1] = 7;
			pAd->ApCfg.BssEdcaParm.Aifsn[2] = 2;
			pAd->ApCfg.BssEdcaParm.Aifsn[3] = 2;

			pAd->ApCfg.BssEdcaParm.Cwmin[0] = 4;
			pAd->ApCfg.BssEdcaParm.Cwmin[1] = 4;
			pAd->ApCfg.BssEdcaParm.Cwmin[2] = 3;
			pAd->ApCfg.BssEdcaParm.Cwmin[3] = 2;

			pAd->ApCfg.BssEdcaParm.Cwmax[0] = 10;
			pAd->ApCfg.BssEdcaParm.Cwmax[1] = 10;
			pAd->ApCfg.BssEdcaParm.Cwmax[2] = 4;
			pAd->ApCfg.BssEdcaParm.Cwmax[3] = 3;

			pAd->ApCfg.BssEdcaParm.Txop[0]  = 0;
			pAd->ApCfg.BssEdcaParm.Txop[1]  = 0;
			pAd->ApCfg.BssEdcaParm.Txop[2]  = 94;	//96;
			pAd->ApCfg.BssEdcaParm.Txop[3]  = 47;	//48;
		}
	}
	else
		AsicSetEdcaParm(pAd, NULL);

	if (pAd->CommonCfg.PhyMode < PHY_11ABGN_MIXED)
	{
		// Patch UI
		pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = BW_20;
	}

	// init
	if (pAd->CommonCfg.bRdg)
	{	
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
		AsicEnableRDG(pAd);
	}
	else	
	{
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
		AsicDisableRDG(pAd);
	}	

	COPY_MAC_ADDR(pAd->ApCfg.MBSSID[BSS0].Bssid, pAd->CurrentAddress);
	AsicSetBssid(pAd, pAd->CurrentAddress); 
	AsicSetMcastWC(pAd);
	// In AP mode,  First WCID Table in ASIC will never be used. To prevent it's 0xff-ff-ff-ff-ff-ff, Write 0 here.
	// p.s ASIC use all 0xff as termination of WCID table search.
	RTMP_IO_WRITE32(pAd, MAC_WCID_BASE, 0x00);
	RTMP_IO_WRITE32(pAd, MAC_WCID_BASE+4, 0x0);

	RTMPPrepareRadarDetectParams(pAd);

		// reset WCID table 
		for (i=1; i<255; i++)
		{
			offset = MAC_WCID_BASE + (i * HW_WCID_ENTRY_SIZE);	
			RTMP_IO_WRITE32(pAd, offset, 0x0);
			RTMP_IO_WRITE32(pAd, offset+4, 0x0);
		}

	
	pAd->MacTab.Content[0].Addr[0] = 0x01;
	pAd->MacTab.Content[0].HTPhyMode.field.MODE = MODE_OFDM;
	pAd->MacTab.Content[0].HTPhyMode.field.MCS = 3;
	pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
	
	if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE))
	{
		pAd->CommonCfg.BBPCurrentBW = BW_40;
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel + 2;
		
		//  TX : control channel at lower 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x1);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at lower 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &byteValue);
		byteValue &= (~0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, byteValue);

		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &byteValue);
		byteValue &= (~0x18);
		byteValue |= 0x10;
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, byteValue);
		if (pAd->CommonCfg.Channel > 14)
		{ 	// request by Gary 20070208 for middle and long range A Band
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x48);
		}
		else
		{	// request by Gary 20070208 for middle and long range G Band
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x38);
		}	
		// 
		if (pAd->MACVersion == 0x28600100)
		{
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x1A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x16);
		}
		else
		{
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x12);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x10);
		}	

		DBGPRINT(RT_DEBUG_TRACE, ("ApStartUp : ExtAbove, ChannelWidth=%d, Channel=%d, ExtChanOffset=%d \n",
			pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth, pAd->CommonCfg.Channel, pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset));
	}
	else if ((pAd->CommonCfg.Channel > 2) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW))
	{
		pAd->CommonCfg.BBPCurrentBW = BW_40;
		if (pAd->CommonCfg.Channel == 14)
			pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel - 1;
		else
			pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel - 2;
		
		//  TX : control channel at upper 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value |= (0x1);		
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at upper 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &byteValue);
		byteValue |= (0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, byteValue);

		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &byteValue);
		byteValue &= (~0x18);
		byteValue |= 0x10;
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, byteValue);
		
		if (pAd->CommonCfg.Channel > 14)
		{ 	// request by Gary 20070208 for middle and long range A Band
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x48);
		}
		else
		{ 	// request by Gary 20070208 for middle and long range G band
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x38);
		}	
	
		
		if (pAd->MACVersion == 0x28600100)
		{
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x1A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x16);
		}
		else
		{	
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x12);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0A);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x10);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("ApStartUp : ExtBlow, ChannelWidth=%d, Channel=%d, ExtChanOffset=%d \n",
			pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth, pAd->CommonCfg.Channel, pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset));
	}
	else
	{
		pAd->CommonCfg.BBPCurrentBW = BW_20;
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
		
		//  TX : control channel at lower 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x1);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &byteValue);
		byteValue &= (~0x18);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, byteValue);
		
		// 20 MHz bandwidth
		if (pAd->CommonCfg.Channel > 14)
		{	 // request by Gary 20070208
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x40);
		}	
		else
		{	// request by Gary 20070208
			//RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x30);
			// request by Brian 20070306
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, 0x38);
		}	
				 
		if (pAd->MACVersion == 0x28600100)
		{
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x16);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x08);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x11);
		}
		else
		{
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x12);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0a);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x10);
		}

		DBGPRINT(RT_DEBUG_TRACE, ("ApStartUp : 20MHz, ChannelWidth=%d, Channel=%d, ExtChanOffset=%d \n",
			pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth, pAd->CommonCfg.Channel, pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset));
	}
	
	if (pAd->CommonCfg.Channel > 14)
	{	// request by Gary 20070208 for middle and long range A Band
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, 0x1D);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, 0x1D);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, 0x1D);
		//RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, 0x1D);
	}
	else
	{ 	// request by Gary 20070208 for middle and long range G band
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, 0x2D);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, 0x2D);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, 0x2D);
			//RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, 0x2D);
	}	

	// Clear BG-Protection flag
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED);	
	AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
	AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
 	MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);
	
	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		MlmeUpdateTxRates(pAd, FALSE, apidx);
		MlmeUpdateHtTxRates(pAd, apidx);
	}
	
	// Set the RadarDetect Mode as Normal, bc the APUpdateAllBeaconFram() will refer this parameter.
	pAd->CommonCfg.RadarDetect.RDMode = RD_NORMAL_MODE;
	// start sending BEACON out
	APMakeAllBssBeacon(pAd);
	APUpdateAllBeaconFrame(pAd);
	APUpdateCapabilityAndErpIe(pAd);
	APUpdateOperationMode(pAd);

	if ( (pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& RadarChannelCheck(pAd, pAd->CommonCfg.Channel))
	{
		pAd->CommonCfg.RadarDetect.RDMode = RD_SILENCE_MODE;
		pAd->CommonCfg.RadarDetect.RDCount = 0;
		pAd->CommonCfg.RadarDetect.InServiceMonitorCount = 0;
#ifdef DFS_SUPPORT
		BbpRadarDetectionStart(pAd); // start Radar detection.
#endif // DFS_SUPPORT //
	}
	else
	{
		pAd->CommonCfg.RadarDetect.RDMode = RD_NORMAL_MODE;
		AsicEnableBssSync(pAd);
#ifdef CONFIG_AP_SUPPORT
#ifdef CARRIER_DETECTION_SUPPORT
		if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
		{
			// trun on Carrier-Detection. (Carrier-Detect with CTS protection).
			CarrierDetectionStart(pAd, 1);
		}
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
	}

	// Pre-tbtt interrupt setting.
	RTMP_IO_READ32(pAd, INT_TIMER_CFG, &Value);
	Value &= 0xffff0000;
	Value |= 6 << 4; // Pre-TBTT is 6ms before TBTT interrupt. 1~10 ms is reasonable.
	RTMP_IO_WRITE32(pAd, INT_TIMER_CFG, Value);
	// Enable pre-tbtt interrupt
	RTMP_IO_READ32(pAd, INT_TIMER_EN, &Value);
	Value |=0x1;
	RTMP_IO_WRITE32(pAd, INT_TIMER_EN, Value);

	// Set LED
	RTMPSetLED(pAd, LED_LINK_UP);


	DBGPRINT(RT_DEBUG_TRACE, (" APStartUp :  Group rekey method= %ld , interval = 0x%lx\n",pAd->ApCfg.WPAREKEY.ReKeyMethod,pAd->ApCfg.WPAREKEY.ReKeyInterval));
	// Group rekey related
	if ((pAd->ApCfg.WPAREKEY.ReKeyInterval != 0) && ((pAd->ApCfg.WPAREKEY.ReKeyMethod == TIME_REKEY) || (pAd->ApCfg.WPAREKEY.ReKeyMethod == PKT_REKEY))) 
	{
		// Regularly check the timer
		if (pAd->ApCfg.REKEYTimerRunning == FALSE)
		{
#ifdef	WIN_NDIS
			NdisMSetPeriodicTimer(&pAd->ApCfg.REKEYTimer, GROUP_KEY_UPDATE_EXEC_INTV);
#endif
			RTMPSetTimer(&pAd->ApCfg.REKEYTimer, GROUP_KEY_UPDATE_EXEC_INTV);

			pAd->ApCfg.REKEYTimerRunning = TRUE;
			pAd->ApCfg.REKEYCOUNTER = 0;
		}
	}
	else
		pAd->ApCfg.REKEYTimerRunning = FALSE;

	// Init some variable
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (pAd->MacTab.Content[i].ValidAsCLI)
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}

	// Init pairwise key table, re-set all WCID entry as NO-security mode.
	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
		AsicRemovePairwiseKeyEntry(pAd, BSS0, (UCHAR)i);
		
	// Init Security variables
	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		USHORT		Wcid = 0;	

		pAd->ApCfg.MBSSID[apidx].PortSecured = WPA_802_1X_PORT_NOT_SECURED;

		if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
		{   
			pAd->ApCfg.MBSSID[apidx].DefaultKeyId = 1;
		}

		// Init TKIP Group-Key-related variables
		//GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GMK);
		//GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GNonce);		

		// initialize IVEIV in Asic			  
		GET_GroupKey_WCID(Wcid, apidx);
		AsicUpdateWCIDIVEIV(pAd, Wcid, 0, 0);

		// When WEP, TKIP or AES is enabled, set group key info to Asic
		if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11WEPEnabled)
		{
    		UCHAR	CipherAlg;
			UCHAR	idx;
    		PUCHAR	Key;    		   			

			for (idx=0; idx < SHARE_KEY_NUM; idx++)
			{
				CipherAlg = pAd->SharedKey[apidx][idx].CipherAlg;
    			Key = pAd->SharedKey[apidx][idx].Key;

				if (pAd->SharedKey[apidx][idx].KeyLen > 0)
				{
					// Set key material to Asic
    				AsicAddSharedKeyEntry(pAd, apidx, idx, CipherAlg, Key, NULL, NULL);	
		
					if (idx == pAd->ApCfg.MBSSID[apidx].DefaultKeyId)
					{
						// Update WCID attribute table and IVEIV table for this group key table  
						RTMPAddWcidAttributeEntry(pAd, apidx, idx, CipherAlg, NULL);
					}
				}
			}
    	}
		else if ((pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption2Enabled) ||
				 (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption3Enabled) ||
				 (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption4Enabled))
		{
			// Init TKIP Group-Key-related variables
			GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GMK);
			GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GNonce);		

			// Count GTK for this BSSID
			CountGTK(pAd->ApCfg.MBSSID[apidx].GMK, (UCHAR*)pAd->ApCfg.MBSSID[apidx].GNonce, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GTK, TKIP_GTK_LENGTH);
        	pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].KeyLen = LEN_TKIP_EK;
	        NdisMoveMemory(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].Key, pAd->ApCfg.MBSSID[apidx].GTK, LEN_TKIP_EK);
    	    NdisMoveMemory(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].TxMic, &pAd->ApCfg.MBSSID[apidx].GTK[16], LEN_TKIP_TXMICK);
        	NdisMoveMemory(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].RxMic, &pAd->ApCfg.MBSSID[apidx].GTK[24], LEN_TKIP_RXMICK);            

        	if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11Encryption2Enabled)
            	pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg = CIPHER_TKIP;
	        else if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
    	        pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg = CIPHER_AES;
        	else
            	pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg = CIPHER_NONE;
            
        	// install Group Key to MAC ASIC
	        AsicAddSharedKeyEntry(
    	        pAd, 
        	    apidx, 
            	pAd->ApCfg.MBSSID[apidx].DefaultKeyId, 
	            pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg, 
    	        pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].Key, 
        	    pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].TxMic, 
            	pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].RxMic);
        
			// update Group key information to ASIC
			RTMPAddWcidAttributeEntry(
				pAd, 
				apidx, 
				pAd->ApCfg.MBSSID[apidx].DefaultKeyId, 
				pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].CipherAlg,
				NULL);
		
    	}

		// Send singal to daemon to indicate driver had restarted
	    if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA) || (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2)
        	|| (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || (pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE))
        {
			POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
		
        	SendSingalToDaemon(SIGUSR1, pObj->apd_pid);
    	}

		DBGPRINT(RT_DEBUG_TRACE, ("### BSS(%d) AuthMode(%d)=%s, WepStatus(%d)=%s , AccessControlList.Policy=%ld\n", apidx, pAd->ApCfg.MBSSID[apidx].AuthMode, GetAuthMode(pAd->ApCfg.MBSSID[apidx].AuthMode), 
																  pAd->ApCfg.MBSSID[apidx].WepStatus, GetEncryptType(pAd->ApCfg.MBSSID[apidx].WepStatus), pAd->ApCfg.MBSSID[apidx].AccessControlList.Policy));
	}

	// Disable Protection first.
	AsicUpdateProtect(pAd, 0, (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), FALSE, FALSE);
#ifdef PIGGYBACK_SUPPORT
	RTMPSetPiggyBack(pAd, pAd->CommonCfg.bPiggyBackCapable);
#endif // PIGGYBACK_SUPPORT //

	ApLogEvent(pAd, pAd->CurrentAddress, EVENT_RESET_ACCESS_POINT);
	pAd->Mlme.PeriodicRound = 0;
	pAd->Mlme.OneSecPeriodicRound = 0;

	OPSTATUS_SET_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);

#ifdef WIN_NDIS
	pAd->IndicateMediaState = NdisMediaStateConnected;
#else
	RTMP_IndicateMediaState();
#endif


	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);

#ifdef WDS_SUPPORT
	// Prepare WEP key
	WdsPrepareWepKeyFromMainBss(pAd);

	// Add wds key infomation to ASIC	
	AsicUpdateWdsRxWCIDTable(pAd);
#endif // WDS_SUPPORT //

#ifdef IDS_SUPPORT
	// Start IDS timer
	if (pAd->ApCfg.IdsEnable)
	{
		if (pAd->CommonCfg.bWirelessEvent == FALSE)
			DBGPRINT(RT_DEBUG_WARN, ("!!! WARNING !!! The WirelessEvent parameter doesn't be enabled \n"));
		
		RTMPIdsStart(pAd);
	}
#endif // IDS_SUPPORT //


#ifdef MESH_SUPPORT
	if (MESH_ON(pAd))
		MeshUp(pAd);
#endif // MESH_SUPPORT //

	DBGPRINT(RT_DEBUG_TRACE, ("<=== APStartUp\n"));
}

/*
	==========================================================================
	Description:
		disassociate all STAs and stop AP service.
	Note:
	==========================================================================
 */
VOID APStop(
	IN PRTMP_ADAPTER pAd) 
{
	BOOLEAN     Cancelled;
	UINT32		Value;
	
	DBGPRINT(RT_DEBUG_TRACE, ("!!! APStop !!!\n"));

#ifdef DFS_SUPPORT
	RadarDetectionStop(pAd);
	BbpRadarDetectionStop(pAd);
#endif // DFS_SUPPORT //

#ifdef MESH_SUPPORT
	if (MESH_ON(pAd))
		MeshDown(pAd, TRUE);
#endif // MESH_SUPPORT //

	MacTableReset(pAd);

	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);

	// Disable pre-tbtt interrupt
	RTMP_IO_READ32(pAd, INT_TIMER_EN, &Value);
	Value &=0xe;
	RTMP_IO_WRITE32(pAd, INT_TIMER_EN, Value);
	// Disable piggyback
	RTMPSetPiggyBack(pAd, FALSE);

   	AsicUpdateProtect(pAd, 0,  (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		//NICDisableInterrupt(pAd);
		AsicDisableSync(pAd);
		// Set LED
		RTMPSetLED(pAd, LED_LINK_DOWN);
	}


	if(pAd->ApCfg.REKEYTimerRunning==TRUE)
	{
#ifdef WIN_NDIS
		NdisMCancelTimer(&pAd->ApCfg.REKEYTimer, &Cancelled);
#else
		RTMPCancelTimer(&pAd->ApCfg.REKEYTimer, &Cancelled);
#endif 
		pAd->ApCfg.REKEYTimerRunning=FALSE;
	}

	if (pAd->ApCfg.CMTimerRunning == TRUE)
	{
#ifdef WIN_NDIS
		NdisMCancelTimer(&pAd->ApCfg.CounterMeasureTimer, &Cancelled);
#else
		RTMPCancelTimer(&pAd->ApCfg.CounterMeasureTimer, &Cancelled);
#endif
		pAd->ApCfg.CMTimerRunning = FALSE;
	}
	
	//
	// Cancel the Timer, to make sure the timer was not queued.
	//
#ifdef WIN_NDIS
	//NdisMCancelTimer(&pAd->ApCfg.CounterMeasureTimer, &Cancelled);
	NdisMCancelTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, &Cancelled);

	pAd->IndicateMediaState = NdisMediaStateDisconnected;

	//
	// We can't IndicateStatus here if driver is on halt progress.
	//
	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
	{
		NdisMIndicateStatus(pAd->AdapterHandle, NDIS_STATUS_MEDIA_DISCONNECT, (PVOID)NULL, 0);
		NdisMIndicateStatusComplete(pAd->AdapterHandle);
	}
#else
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);

#ifdef IDS_SUPPORT
	// if necessary, cancel IDS timer
	RTMPIdsStop(pAd);
#endif // IDS_SUPPORT //

#endif
}

/*
	==========================================================================
	Description:
		This routine is used to clean up a specified power-saving queue. It's
		used whenever a wireless client is deleted.
	==========================================================================
 */
VOID APCleanupPsQueue(
	IN  PRTMP_ADAPTER   pAd,
	IN  PQUEUE_HEADER   pQueue)
{
	PQUEUE_ENTRY pEntry;
	PNDIS_PACKET pPacket;

	DBGPRINT(RT_DEBUG_TRACE, ("APCleanupPsQueue...\n"));
	while (pQueue->Head)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("APCleanupPsQueue %ld...\n",pAd->MacTab.McastPsQueue.Number ));
		pEntry = RemoveHeadQueue(pQueue);
		//pPacket = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReservedEx);
		pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
	}
}

/*
	==========================================================================
	Description:
		This routine reset the entire MAC table. All packets pending in
		the power-saving queues are freed here.
	==========================================================================
 */
VOID MacTableReset(
	IN  PRTMP_ADAPTER  pAd)
{
	int         i;
	BOOLEAN     Cancelled;
	PUCHAR      pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG       FrameLen = 0;
	HEADER_802_11 DisassocHdr;
	USHORT      Reason;
#ifdef RT2860
	unsigned long	IrqFlags;
#endif // RT2860 //
#ifdef WSC_AP_SUPPORT    
    UCHAR       apidx = MAIN_MBSSID;
#endif // WSC_AP_SUPPORT //  

	DBGPRINT(RT_DEBUG_TRACE, ("MacTableReset\n"));
	//NdisAcquireSpinLock(&pAd->MacTabLock);

	for (i=1; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
#ifdef RT2860
		RT28XX_STA_ENTRY_MAC_RESET(pAd, i);
#endif // RT2860 //
		if (pAd->MacTab.Content[i].ValidAsCLI == TRUE)
	   {
			RTMPCancelTimer(&pAd->MacTab.Content[i].RetryTimer, &Cancelled);

			// cancel EnqueueStartForPSKTimer
			RTMPCancelTimer(&pAd->MacTab.Content[i].EnqueueStartForPSKTimer, &Cancelled);
            pAd->MacTab.Content[i].EnqueueEapolStartTimerRunning = EAPOL_START_DISABLE;      

#ifdef WSC_AP_SUPPORT
            RTMPCancelTimer(&pAd->MacTab.Content[i].EnqueueEapolStartTimerForWsc, &Cancelled);
            pAd->MacTab.Content[i].EnqueueEapolStartTimerForWscRunning = FALSE;
            pAd->MacTab.Content[i].Receive_EapolStart_EapRspId = 0;
#endif // WSC_AP_SUPPORT //            
#ifdef RT2860
			RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
			if (pAd->OpMode == OPMODE_AP)
            {
				APCleanupPsQueue(pAd, &pAd->MacTab.Content[i].PsQueue);

#ifdef UAPSD_AP_SUPPORT
                UAPSD_MR_ENTRY_RESET(&pAd->MacTab.Content[i]);
#endif // UAPSD_AP_SUPPORT //
            }
			DBGPRINT(RT_DEBUG_TRACE, (" %dth Sst = %d, PsQueue.Number %ld...\n", i, pAd->MacTab.Content[i].Sst, pAd->MacTab.Content[i].PsQueue.Number ));
#ifdef RT2860
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
			// free resources of BA
			BASessionTearDownALL(pAd, i);

			pAd->MacTab.Content[i].ValidAsCLI = FALSE;


			// Before reset MacTable, send disassociation packet to client.
			if (pAd->MacTab.Content[i].Sst == SST_ASSOC)
			{
				//  send out a DISASSOC request frame
				NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
				if (NStatus != NDIS_STATUS_SUCCESS) 
				{
					DBGPRINT(RT_DEBUG_TRACE, (" MlmeAllocateMemory fail  ..\n"));
					//NdisReleaseSpinLock(&pAd->MacTabLock);
					return;
				}

				Reason = REASON_DISASSOC_INACTIVE;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - Send DISASSOC  Reason = %d frame  TO %x %x %x %x %x %x \n",Reason,pAd->MacTab.Content[i].Addr[0],
					pAd->MacTab.Content[i].Addr[1],pAd->MacTab.Content[i].Addr[2],pAd->MacTab.Content[i].Addr[3],pAd->MacTab.Content[i].Addr[4],pAd->MacTab.Content[i].Addr[5]));
				MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pAd->MacTab.Content[i].Addr, pAd->ApCfg.MBSSID[pAd->MacTab.Content[i].apidx].Bssid);
				MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11), &DisassocHdr, 2, &Reason, END_OF_ARGS);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
				MlmeFreeMemory(pAd, pOutBuffer);

				RTMPusecDelay(5000);
			}


			//AsicDelWcidTab(pAd, i);
		}
	}

#ifdef WSC_AP_SUPPORT
	for (apidx = MAIN_MBSSID; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
    	RTMPCancelTimer(&pAd->ApCfg.MBSSID[apidx].WscControl.EapolTimer, &Cancelled);
    	pAd->ApCfg.MBSSID[apidx].WscControl.EapolTimerRunning = FALSE;
    
    	pAd->ApCfg.MBSSID[apidx].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;
    	pAd->ApCfg.MBSSID[apidx].WscControl.EapMsgRunning = FALSE;
	}
#endif // WSC_AP_SUPPORT //
#ifdef RT2860
	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
	DBGPRINT(RT_DEBUG_TRACE, ("McastPsQueue.Number %ld...\n",pAd->MacTab.McastPsQueue.Number));
	if (pAd->MacTab.McastPsQueue.Number > 0)
		APCleanupPsQueue(pAd, &pAd->MacTab.McastPsQueue);
	DBGPRINT(RT_DEBUG_TRACE, ("2McastPsQueue.Number %ld...\n",pAd->MacTab.McastPsQueue.Number));

	NdisZeroMemory(&pAd->MacTab, sizeof(MAC_TABLE));
	InitializeQueueHeader(&pAd->MacTab.McastPsQueue);
	//NdisReleaseSpinLock(&pAd->MacTabLock);
#ifdef RT2860
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
	return;
}

/*
	==========================================================================
	Description:
		This routine is called by APMlmePeriodicExec() every second to check if
		1. any associated client in PSM. If yes, then TX MCAST/BCAST should be
		   out in DTIM only
		2. any client being idle for too long and should be aged-out from MAC table
		3. garbage collect PSQ
	==========================================================================
*/
VOID MacTableMaintenance(
	IN PRTMP_ADAPTER pAd)
{
	int i;
	BOOLEAN	bAllStationAsRalink = TRUE;	
	BOOLEAN	bRdgActive;
#ifdef RT2860
	unsigned long	IrqFlags;
#endif // RT2860 //

	pAd->MacTab.fAnyStationInPsm = FALSE;
	pAd->MacTab.fAnyStationNonGF = FALSE;
	pAd->MacTab.fAnyStation20Only = FALSE;
	pAd->MacTab.fAnyStationIsLegacy = FALSE;
	pAd->MacTab.fAnyStationBadAtheros = FALSE;
	pAd->MacTab.fAnyStationMIMOPSDynamic = FALSE;
	pAd->MacTab.fAnyTxOPForceDisable = FALSE;

	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
		MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];

		if (pEntry->ValidAsCLI == FALSE)
			continue;

		if (pEntry->NoDataIdleCount == 0)
			pEntry->StationKeepAliveCount = 0;

		pEntry->NoDataIdleCount ++;  

		// 0. STA failed to complete association should be removed to save MAC table space.
		if ((pEntry->Sst != SST_ASSOC) && (pEntry->NoDataIdleCount >= MAC_TABLE_ASSOC_TIMEOUT))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%02x:%02x:%02x:%02x:%02x:%02x fail to complete ASSOC in %d sec\n",
					pEntry->Addr[0],pEntry->Addr[1],pEntry->Addr[2],pEntry->Addr[3],
					pEntry->Addr[4],pEntry->Addr[5],MAC_TABLE_ASSOC_TIMEOUT));
			MacTableDeleteEntry(pAd, pEntry->Aid, pEntry->Addr);
			continue;
		}

		// 1. check if there's any associated STA in power-save mode. this affects outgoing
		//    MCAST/BCAST frames should be stored in PSQ till DtimCount=0
		if (pEntry->PsMode == PWR_SAVE)
			pAd->MacTab.fAnyStationInPsm = TRUE;

		if (pEntry->MmpsMode == MMPS_DYNAMIC)
		{
			pAd->MacTab.fAnyStationMIMOPSDynamic = TRUE;
		}

		if (pEntry->MaxHTPhyMode.field.BW == BW_20)
			pAd->MacTab.fAnyStation20Only = TRUE;

		if (pEntry->MaxHTPhyMode.field.MODE != MODE_HTGREENFIELD)
			pAd->MacTab.fAnyStationNonGF = TRUE;

		if ((pEntry->MaxHTPhyMode.field.MODE == MODE_OFDM) || (pEntry->MaxHTPhyMode.field.MODE == MODE_CCK))
		{
			pAd->MacTab.fAnyStationIsLegacy = TRUE;
		}
		
		if (pEntry->bIAmBadAtheros)
		{
			pAd->MacTab.fAnyStationBadAtheros = TRUE;
			if (pAd->CommonCfg.IOTestParm.bRTSLongProtOn == FALSE)
				AsicUpdateProtect(pAd, 8, ALLN_SETPROTECT, FALSE, pAd->MacTab.fAnyStationNonGF);

			if (pEntry->WepStatus != Ndis802_11EncryptionDisabled)
			{
				pAd->MacTab.fAnyTxOPForceDisable = TRUE;
			}
		}

		// detect the station alive status
		if ((pAd->ApCfg.MBSSID[pEntry->apidx].StationKeepAliveTime > 0) &&
			(pEntry->NoDataIdleCount >= pAd->ApCfg.MBSSID[pEntry->apidx].StationKeepAliveTime))
		{
			MULTISSID_STRUCT *pMbss = &pAd->ApCfg.MBSSID[pEntry->apidx];

			// if no any data success between ap and the station for StationKeepAliveTime,
			// try to detect whether the station is still alive
			if (pEntry->StationKeepAliveCount++ == 0)
			{
				if (pEntry->PsMode == PWR_SAVE)
				{
					// use TIM bit to detect the PS station
					WLAN_MR_TIM_BIT_SET(pAd, pEntry->apidx, pEntry->Aid);
				}
				else
				{
					// use Null or QoS Null to detect the ACTIVE station
					BOOLEAN bQosNull = FALSE;
	
					if (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE))
						bQosNull = TRUE;

		            ApEnqueueNullFrame(pAd, pEntry->Addr, pEntry->CurrTxRate,
	    	                           pEntry->Aid, pEntry->apidx, bQosNull, TRUE, 0);
				}
			}
			else
			{
				if (pEntry->StationKeepAliveCount >= pMbss->StationKeepAliveTime)
					pEntry->StationKeepAliveCount = 0;
			}
		}

		// 2. delete those MAC entry that has been idle for a long time
		if (pEntry->NoDataIdleCount >= MAC_TABLE_AGEOUT_TIME)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("ageout %02x:%02x:%02x:%02x:%02x:%02x after %d-sec silence\n",
					pEntry->Addr[0],pEntry->Addr[1],pEntry->Addr[2],pEntry->Addr[3],
					pEntry->Addr[4],pEntry->Addr[5],MAC_TABLE_AGEOUT_TIME));
			ApLogEvent(pAd, pEntry->Addr, EVENT_AGED_OUT);

			// send wireless event - for ageout 
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_AGEOUT_EVENT_FLAG, pEntry->Addr, 0, 0); 


			if (pEntry->Sst == SST_ASSOC)
			{
				PUCHAR      pOutBuffer = NULL;
				NDIS_STATUS NStatus;
				ULONG       FrameLen = 0;
				HEADER_802_11 DisassocHdr;
				USHORT      Reason;

				//  send out a DISASSOC request frame
				NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
				if (NStatus != NDIS_STATUS_SUCCESS) 
				{
					DBGPRINT(RT_DEBUG_TRACE, (" MlmeAllocateMemory fail  ..\n"));
					//NdisReleaseSpinLock(&pAd->MacTabLock);
					continue;
				}

				Reason = REASON_DISASSOC_INACTIVE;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - Send DISASSOC  Reason = %d frame  TO %x %x %x %x %x %x \n",Reason,pEntry->Addr[0],
					pEntry->Addr[1],pEntry->Addr[2],pEntry->Addr[3],pEntry->Addr[4],pEntry->Addr[5]));
				MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pEntry->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);
				MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11), &DisassocHdr, 2, &Reason, END_OF_ARGS);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
				MlmeFreeMemory(pAd, pOutBuffer);
			}

			MacTableDeleteEntry(pAd, pEntry->Aid, pEntry->Addr);
			continue;
		}

		// 3. garbage collect the PsQueue if the STA has being idle for a while
		if (pEntry->PsQueue.Head)
		{
			pEntry->PsQIdleCount ++;  
			if (pEntry->PsQIdleCount > 2) 
			{
				NdisAcquireSpinLock(&pAd->MacTabLock);
				APCleanupPsQueue(pAd, &pEntry->PsQueue);
				NdisReleaseSpinLock(&pAd->MacTabLock);
				pEntry->PsQIdleCount = 0;
			}
		}
		else
			pEntry->PsQIdleCount = 0;
	
#ifdef UAPSD_AP_SUPPORT
        UAPSD_QueueMaintenance(pAd, pEntry);
#endif // UAPSD_AP_SUPPORT //

		// check if this STA is Ralink-chipset 
		if (!CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET))
			bAllStationAsRalink = FALSE;

	}

	// If all associated STAs are Ralink-chipset, AP shall enable RDG.
	if (pAd->CommonCfg.bRdg && bAllStationAsRalink)
	{
		bRdgActive = TRUE;
	}
	else
	{
		bRdgActive = FALSE;
	}

	if (bRdgActive != RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE))
	{
		if (bRdgActive)
		{
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicEnableRDG(pAd);
		}
		else
		{
			RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicDisableRDG(pAd);
		}
	}

	if ((pAd->MacTab.fAnyStationBadAtheros == FALSE) && (pAd->CommonCfg.IOTestParm.bRTSLongProtOn == TRUE))
	{
		AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, ALLN_SETPROTECT, FALSE, pAd->MacTab.fAnyStationNonGF);
	}
#ifdef RT2860
	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
	// 4. garbage collect pAd->MacTab.McastPsQueue if backlogged MCAST/BCAST frames
	//    stale in queue. Since MCAST/BCAST frames always been sent out whenever 
	//    DtimCount==0, the only case to let them stale is surprise removal of the NIC,
	//    so that ASIC-based Tbcn interrupt stops and DtimCount dead.
	if (pAd->MacTab.McastPsQueue.Head)
	{
        UINT bss_index;

		pAd->MacTab.PsQIdleCount ++;
		if (pAd->MacTab.PsQIdleCount > 1)
		{
			/* Normally, should not be here;
			   because bc/mc packets will be moved to SwQueue when DTIM = 0 and
			   DTIM period < 2 seconds;
			   If enter here, it is the kernel bug or driver bug */

			//NdisAcquireSpinLock(&pAd->MacTabLock);
			APCleanupPsQueue(pAd, &pAd->MacTab.McastPsQueue);
			//NdisReleaseSpinLock(&pAd->MacTabLock);
			pAd->MacTab.PsQIdleCount = 0;

	        /* sanity check */
	        if (pAd->ApCfg.BssidNum > MAX_MBSSID_NUM)
	            pAd->ApCfg.BssidNum = MAX_MBSSID_NUM;
	        /* End of if */
        
	        /* clear MCAST/BCAST backlog bit for all BSS */
	        for(bss_index=BSS0; bss_index<pAd->ApCfg.BssidNum; bss_index++)
	            WLAN_MR_TIM_BCMC_CLEAR(bss_index);
	        /* End of for */
		}
	}
	else
		pAd->MacTab.PsQIdleCount = 0;
#ifdef RT2860
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif // RT2860 //
}


UINT32 MacTableAssocStaNumGet(
	IN PRTMP_ADAPTER pAd)
{
	UINT32 num = 0;
	UINT32 i;


	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
		MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];

		if (pEntry->ValidAsCLI == FALSE)
			continue;

		if (pEntry->Sst == SST_ASSOC)
			num ++;
	}

	return num;
}


/*
	==========================================================================
	Description:
		Look up a STA MAC table. Return its Sst to decide if an incoming
		frame from this STA or an outgoing frame to this STA is permitted.
	Return:
	==========================================================================
*/
MAC_TABLE_ENTRY *APSsPsInquiry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr, 
	OUT SST   *Sst, 
	OUT USHORT *Aid,
	OUT UCHAR *PsMode,
	OUT UCHAR *Rate) 
{
	MAC_TABLE_ENTRY *pEntry = NULL;
	
	if (MAC_ADDR_IS_GROUP(pAddr)) // mcast & broadcast address
	{
		*Sst        = SST_ASSOC;
		*Aid        = MCAST_WCID;	// Softap supports 1 BSSID and use WCID=0 as multicast Wcid index
		*PsMode     = PWR_ACTIVE;
		*Rate       = pAd->CommonCfg.MlmeRate; 
	} 
	else // unicast address
	{
		pEntry = MacTableLookup(pAd, pAddr);
		if (pEntry) 
		{
			*Sst        = pEntry->Sst;
			*Aid        = pEntry->Aid;
			*PsMode     = pEntry->PsMode;
			if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) && (pEntry->GTKState != REKEY_ESTABLISHED))
				*Rate   = pAd->CommonCfg.MlmeRate;
			else
			*Rate       = pEntry->CurrTxRate;
		} 
		else 
		{
			*Sst        = SST_NOT_AUTH;
			*Aid        = MCAST_WCID;
			*PsMode     = PWR_ACTIVE;
			*Rate       = pAd->CommonCfg.MlmeRate; 
		}
	}
	return pEntry;
}

/*
	==========================================================================
	Description:
		Update the station current power save mode. Calling this routine also
		prove the specified client is still alive. Otherwise AP will age-out
		this client once IdleCount exceeds a threshold.
	==========================================================================
 */
BOOLEAN APPsIndicate(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR pAddr, 
	IN ULONG Wcid, 
	IN UCHAR Psm) 
{
	MAC_TABLE_ENTRY *pEntry;
    UCHAR old_psmode;

	if (Wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		return PWR_ACTIVE;	
	}

	pEntry = &pAd->MacTab.Content[Wcid];
    old_psmode = pEntry->PsMode;
	if (RTMPEqualMemory(pEntry->Addr, pAddr, MAC_ADDR_LEN)) 
	{
		if ((pEntry->PsMode == PWR_SAVE) && (Psm == PWR_ACTIVE))
		{
#ifdef RT2860
#if 0
			// TODO: For RT2870, how to handle about the BA when STA in PS mode????
			int tid;
			for (tid=0; tid<8; tid++)
			{
				BAOriSessionTearDown(pAd, pEntry->Aid, tid, FALSE, FALSE);
			}

			pEntry->NoBADataCountDown = 16; //64;
#else
			// When sta wake up, we send BAR to refresh the BA sequence.
			SendRefreshBAR(pAd, pEntry);
#endif
#endif // RT2860 //
			DBGPRINT(RT_DEBUG_TRACE, ("APPsIndicate - %02x:%02x:%02x:%02x:%02x:%02x wakes up, act like rx PS-POLL\n", pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]));
			// sleep station awakes, move all pending frames from PSQ to TXQ if any
			APHandleRxPsPoll(pAd, pAddr, pEntry->Aid, TRUE);
		}
		else if ((pEntry->PsMode != PWR_SAVE) && (Psm == PWR_SAVE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("APPsIndicate - %02x:%02x:%02x:%02x:%02x:%02x sleeps\n", pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]));
		}

		pEntry->NoDataIdleCount = 0;
		pEntry->PsMode = Psm;
	} 
	else 
	{
		DBGPRINT(RT_DEBUG_INFO, ("APPsIndicate -[%ldth] not match %02x:%02x:%02x:%02x:%02x:%02x \n", Wcid, pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]));
		// not in table, try to learn it ???? why bother?
	}
	return old_psmode;
}

/*
	==========================================================================
	Description:
		This routine is called to log a specific event into the event table.
		The table is a QUERY-n-CLEAR array that stop at full.
	==========================================================================
 */
VOID ApLogEvent(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR   pAddr,
	IN USHORT   Event)
{
	if (pAd->EventTab.Num < MAX_NUM_OF_EVENT)
	{
		RT_802_11_EVENT_LOG *pLog = &pAd->EventTab.Log[pAd->EventTab.Num];
#ifdef WIN_NDIS
		NdisGetCurrentSystemTime(&pLog->SystemTime);
#else
		RTMP_GetCurrentSystemTime(&pLog->SystemTime);
#endif
		COPY_MAC_ADDR(pLog->Addr, pAddr);
		pLog->Event = Event;
		DBGPRINT_RAW(RT_DEBUG_TRACE,("LOG#%ld %02x:%02x:%02x:%02x:%02x:%02x %s\n",
			pAd->EventTab.Num, pAddr[0], pAddr[1], pAddr[2], 
			pAddr[3], pAddr[4], pAddr[5], pEventText[Event]));
		pAd->EventTab.Num += 1;
	}
}

/*
	==========================================================================
	Description:
		Operationg mode is as defined at 802.11n for how proteciton in this BSS operates. 
		Ap broadcast the operation mode at Additional HT Infroamtion Element Operating Mode fields.
		802.11n D1.0 might has bugs so this operating mode use  EWC MAC 1.24 definition first.

		Called when receiving my bssid beacon or beaconAtJoin to update protection mode.
		40MHz or 20MHz protection mode in HT 40/20 capabale BSS.
		As STA, this obeys the operation mode in ADDHT IE.
		As AP, update protection when setting ADDHT IE and after new STA joined.
	==========================================================================
*/
VOID APUpdateOperationMode(
	IN PRTMP_ADAPTER pAd)
{
	pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode = 0;

	if ((pAd->ApCfg.LastNoneHTOLBCDetectTime + (5 * OS_HZ)) > pAd->Mlme.Now32) // non HT BSS exist within 5 sec
	{
		pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode = 1;
    	AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, ALLN_SETPROTECT, FALSE, TRUE);
	}

   	// If I am 40MHz BSS, and there exist HT-20MHz station. 
	// Update to 2 when it's zero.  Because OperaionMode = 1 or 3 has more protection.
	if ((pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode == 0) && (pAd->MacTab.fAnyStation20Only) && (pAd->CommonCfg.DesiredHtPhy.ChannelWidth == 1))
	{
		pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode = 2;
		AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, (ALLN_SETPROTECT), TRUE, pAd->MacTab.fAnyStationNonGF);
	}
		
	if (pAd->MacTab.fAnyStationIsLegacy)
	{
		pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode = 3;
		AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, (ALLN_SETPROTECT), TRUE, pAd->MacTab.fAnyStationNonGF);
	}
	
	pAd->CommonCfg.AddHTInfo.AddHtInfo2.NonGfPresent = pAd->MacTab.fAnyStationNonGF;
}

/*
	==========================================================================
	Description:
		Update ERP IE and CapabilityInfo based on STA association status.
		The result will be auto updated into the next outgoing BEACON in next
		TBTT interrupt service routine
	==========================================================================
 */
VOID APUpdateCapabilityAndErpIe(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR  i, ErpIeContent = 0;
	BOOLEAN ShortSlotCapable = pAd->CommonCfg.bUseShortSlotTime;
	UCHAR	apidx;
	BOOLEAN	bUseBGProtection;
	BOOLEAN	LegacyBssExist;

#if 0
	if (pAd->CommonCfg.PhyMode != PHY_11BG_MIXED)
	{
		if (pAd->CommonCfg.PhyMode == PHY_11G)
		{
			pAd->ApCfg.ErpIeContent = (ErpIeContent | 0x04);
			AsicSetSlotTime(pAd, ShortSlotCapable);
		}
		return;
	}
#else
	if (pAd->CommonCfg.PhyMode == PHY_11B)
		return;
#endif

	for (i=1; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((pEntry->ValidAsCLI == FALSE) || (pEntry->Sst != SST_ASSOC))
			continue;

		// at least one 11b client associated, turn on ERP.NonERPPresent bit
		// almost all 11b client won't support "Short Slot" time, turn off for maximum compatibility
		if (pEntry->MaxSupportedRate < RATE_FIRST_OFDM_RATE)
		{
			ShortSlotCapable = FALSE;
			ErpIeContent |= 0x01;
		}

		// at least one client can't support short slot
		if ((pEntry->CapabilityInfo & 0x0400) == 0)
			ShortSlotCapable = FALSE;
	}

	// legacy BSS exist within 5 sec
	if ((pAd->ApCfg.LastOLBCDetectTime + (5 * OS_HZ)) > pAd->Mlme.Now32) 
	{
		LegacyBssExist = TRUE;

		// To patch the throughput issue in Intel certification testing 
		// when B only AP exists in control channel.
		// Disable short slot time capable
		ShortSlotCapable = FALSE;
	}
	else
	{
		LegacyBssExist = FALSE;
	}
	
	// decide ErpIR.UseProtection bit, depending on pAd->CommonCfg.UseBGProtection
	//    AUTO (0): UseProtection = 1 if any 11b STA associated
	//    ON (1): always USE protection
	//    OFF (2): always NOT USE protection
	if (pAd->CommonCfg.UseBGProtection == 0)
	{
		ErpIeContent = (ErpIeContent)? 0x03 : 0x00;
		//if ((pAd->ApCfg.LastOLBCDetectTime + (5 * OS_HZ)) > pAd->Mlme.Now32) // legacy BSS exist within 5 sec
		if (LegacyBssExist)
		{
			DBGPRINT(RT_DEBUG_INFO, ("APUpdateCapabilityAndErpIe - Legacy 802.11b BSS overlaped\n"));
			ErpIeContent |= 0x02;                                     // set Use_Protection bit
		}
	}
	else if (pAd->CommonCfg.UseBGProtection == 1)   
		ErpIeContent |= 0x02;
	else
		;

	bUseBGProtection = (pAd->CommonCfg.UseBGProtection == 1) ||    // always use
						((pAd->CommonCfg.UseBGProtection == 0) && ERP_IS_USE_PROTECTION(ErpIeContent));

	// always no BG protection in A-band. falsely happened when switching A/G band to a dual-band AP
	if (pAd->CommonCfg.Channel > 14) 
		bUseBGProtection = FALSE;

	if (bUseBGProtection != OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
	{
		if (bUseBGProtection)
		{
			OPSTATUS_SET_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED);
			AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, (OFDMSETPROTECT), FALSE, pAd->MacTab.fAnyStationNonGF);
		}
		else
		{
			OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED);
			AsicUpdateProtect(pAd, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, (OFDMSETPROTECT), TRUE, pAd->MacTab.fAnyStationNonGF);
		}
					
		DBGPRINT(RT_DEBUG_INFO, ("SYNC - AP changed B/G protection to %d\n", bUseBGProtection));
	}

	// Decide Barker Preamble bit of ERP IE
	if ((pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong) || (ApCheckLongPreambleSTA(pAd) == TRUE))
		pAd->ApCfg.ErpIeContent = (ErpIeContent | 0x04);
	else
		pAd->ApCfg.ErpIeContent = ErpIeContent;

	// Force to use ShortSlotTime at A-band
	if (pAd->CommonCfg.Channel > 14)
		ShortSlotCapable = TRUE;
	
	//
	// deicide CapabilityInfo.ShortSlotTime bit
	//
    for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
    {
		// In A-band, the ShortSlotTime bit should be ignored. 
		if (ShortSlotCapable && (pAd->CommonCfg.Channel <= 14))
    		pAd->ApCfg.MBSSID[apidx].CapabilityInfo |= 0x0400;
		else
    		pAd->ApCfg.MBSSID[apidx].CapabilityInfo &= 0xfbff;


   		if (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong)
			pAd->ApCfg.MBSSID[apidx].CapabilityInfo &= (~0x020);
		else
			pAd->ApCfg.MBSSID[apidx].CapabilityInfo |= 0x020;

		DBGPRINT(RT_DEBUG_INFO, ("APUpdateCapabilityAndErpIe - Capability= 0x%04x, ERP is 0x%02x\n", 
    		pAd->ApCfg.MBSSID[apidx].CapabilityInfo, ErpIeContent));
	}

	AsicSetSlotTime(pAd, ShortSlotCapable);

}

/*
	==========================================================================
	Description:
        Check to see the exist of long preamble STA in associated list
    ==========================================================================
 */
BOOLEAN ApCheckLongPreambleSTA(
    IN PRTMP_ADAPTER pAd)
{
    UCHAR   i;
    
    for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
    {
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((pEntry->ValidAsCLI == FALSE) || (pEntry->Sst != SST_ASSOC))
			continue;
	            
        if (!CAP_IS_SHORT_PREAMBLE_ON(pEntry->CapabilityInfo))
        {
            DBGPRINT(RT_DEBUG_INFO, ("Long preamble capable STA exist\n"));
            return TRUE;
        }
    }

    return FALSE;
}    

/*
	==========================================================================
	Description:
		Check if the specified STA pass the Access Control List checking.
		If fails to pass the checking, then no authentication nor association 
		is allowed
	Return:
		MLME_SUCCESS - this STA passes ACL checking

	==========================================================================
*/
BOOLEAN ApCheckAccessControlList(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR        pAddr,
	IN UCHAR         Apidx)
{
	BOOLEAN Result = TRUE;

    if (pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy == 0)       // ACL is disabled
        Result = TRUE;
    else
    {
        ULONG i;
        if (pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy == 1)   // ACL is a positive list
            Result = FALSE;
        else                                              // ACL is a negative list
            Result = TRUE;
        for (i=0; i<pAd->ApCfg.MBSSID[Apidx].AccessControlList.Num; i++)
        {
            if (MAC_ADDR_EQUAL(pAddr, pAd->ApCfg.MBSSID[Apidx].AccessControlList.Entry[i].Addr))
            {
                Result = !Result;
                break;
            }
        }
    }

    if (Result == FALSE)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("%02x:%02x:%02x:%02x:%02x:%02x failed in ACL checking\n",
        pAddr[0],pAddr[1],pAddr[2],pAddr[3],pAddr[4],pAddr[5]));
    }

    return Result;
}

/*
	==========================================================================
	Description:
		This routine update the current MAC table based on the current ACL.
		If ACL change causing an associated STA become un-authorized. This STA
		will be kicked out immediately.
	==========================================================================
*/
VOID ApUpdateAccessControlList(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         Apidx)
{
	USHORT   AclIdx, MacIdx;
	BOOLEAN  Matched;

	PUCHAR      pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG       FrameLen = 0;
	HEADER_802_11 DisassocHdr;
	USHORT      Reason;

	
	//Apidx = pObj->ioctl_if;
	ASSERT(Apidx <= MAX_MBSSID_NUM);
	DBGPRINT(RT_DEBUG_TRACE, ("ApUpdateAccessControlList : Apidx = %d\n", Apidx));
	
    // ACL is disabled. Do nothing about the MAC table.
    if (pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy == 0)
		return;

	for (MacIdx=0; MacIdx < MAX_LEN_OF_MAC_TABLE; MacIdx++)
	{
		if (! pAd->MacTab.Content[MacIdx].ValidAsCLI) 
			continue;

		//
		// We only need to update associations related to ACL of MBSSID[Apidx].
		//
        if (pAd->MacTab.Content[MacIdx].apidx != Apidx) 
            continue;
    
		Matched = FALSE;
        for (AclIdx = 0; AclIdx < pAd->ApCfg.MBSSID[Apidx].AccessControlList.Num; AclIdx++)
		{
            if (MAC_ADDR_EQUAL(&pAd->MacTab.Content[MacIdx].Addr, pAd->ApCfg.MBSSID[Apidx].AccessControlList.Entry[AclIdx].Addr))
			{
				Matched = TRUE;
				break;
			}
		}

        if ((Matched == FALSE) && (pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy == 1))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Apidx = %d\n", Apidx));
			DBGPRINT(RT_DEBUG_TRACE, ("pAd->ApCfg.MBSSID[%d].AccessControlList.Policy = %ld\n", Apidx,
				pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy));
			DBGPRINT(RT_DEBUG_TRACE, ("STA not on positive ACL. remove it...\n"));
			
			// Before delete the entry from MacTable, send disassociation packet to client.
			if (pAd->MacTab.Content[MacIdx].Sst == SST_ASSOC)
			{
				//  send out a DISASSOC frame
				NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
				if (NStatus != NDIS_STATUS_SUCCESS) 
				{
					DBGPRINT(RT_DEBUG_TRACE, (" MlmeAllocateMemory fail  ..\n"));
					return;
				}

				Reason = REASON_DECLINED;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - Send DISASSOC  Reason = %d frame  TO %x %x %x %x %x %x \n",Reason,pAd->MacTab.Content[MacIdx].Addr[0],
					pAd->MacTab.Content[MacIdx].Addr[1],pAd->MacTab.Content[MacIdx].Addr[2],pAd->MacTab.Content[MacIdx].Addr[3],pAd->MacTab.Content[MacIdx].Addr[4],pAd->MacTab.Content[MacIdx].Addr[5]));
				MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pAd->MacTab.Content[MacIdx].Addr, pAd->ApCfg.MBSSID[pAd->MacTab.Content[MacIdx].apidx].Bssid);
				MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11), &DisassocHdr, 2, &Reason, END_OF_ARGS);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
				MlmeFreeMemory(pAd, pOutBuffer);

				RTMPusecDelay(5000);
			}
			MacTableDeleteEntry(pAd, pAd->MacTab.Content[MacIdx].Aid, pAd->MacTab.Content[MacIdx].Addr);
		}
        else if ((Matched == TRUE) && (pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy == 2))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Apidx = %d\n", Apidx));
			DBGPRINT(RT_DEBUG_TRACE, ("pAd->ApCfg.MBSSID[%d].AccessControlList.Policy = %ld\n", Apidx,
				pAd->ApCfg.MBSSID[Apidx].AccessControlList.Policy));
			DBGPRINT(RT_DEBUG_TRACE, ("STA on negative ACL. remove it...\n"));
			
			// Before delete the entry from MacTable, send disassociation packet to client.
			if (pAd->MacTab.Content[MacIdx].Sst == SST_ASSOC)
			{
				// send out a DISASSOC frame
				NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
				if (NStatus != NDIS_STATUS_SUCCESS) 
				{
					DBGPRINT(RT_DEBUG_TRACE, (" MlmeAllocateMemory fail  ..\n"));
					return;
				}

				Reason = REASON_DECLINED;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - Send DISASSOC  Reason = %d frame  TO %x %x %x %x %x %x \n",Reason,pAd->MacTab.Content[MacIdx].Addr[0],
					pAd->MacTab.Content[MacIdx].Addr[1],pAd->MacTab.Content[MacIdx].Addr[2],pAd->MacTab.Content[MacIdx].Addr[3],pAd->MacTab.Content[MacIdx].Addr[4],pAd->MacTab.Content[MacIdx].Addr[5]));
				MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pAd->MacTab.Content[MacIdx].Addr, pAd->ApCfg.MBSSID[pAd->MacTab.Content[MacIdx].apidx].Bssid);
				MakeOutgoingFrame(pOutBuffer, &FrameLen, sizeof(HEADER_802_11), &DisassocHdr, 2, &Reason, END_OF_ARGS);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
				MlmeFreeMemory(pAd, pOutBuffer);

				RTMPusecDelay(5000);
			}
			MacTableDeleteEntry(pAd, pAd->MacTab.Content[MacIdx].Aid, pAd->MacTab.Content[MacIdx].Addr);
		}
	}
}

/* 
	==========================================================================
	Description:
		Send out a NULL frame to a specified STA at a higher TX rate. The 
		purpose is to ensure the designated client is okay to received at this
		rate.
	==========================================================================
 */
VOID ApEnqueueNullFrame(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR        pAddr,
	IN UCHAR         TxRate,
	IN UCHAR         PID,
	IN UCHAR         apidx,
    IN BOOLEAN       bQosNull,
    IN BOOLEAN       bEOSP,
    IN UCHAR         OldUP)
{
	NDIS_STATUS    NState;
	PHEADER_802_11 pNullFr;
	PUCHAR pFrame;
    ULONG		   Length;


	// since TxRate may change, we have to change Duration each time
	NState = MlmeAllocateMemory(pAd, (PUCHAR *)&pFrame);
	pNullFr = (PHEADER_802_11) pFrame;
    Length = sizeof(HEADER_802_11);

	if (NState == NDIS_STATUS_SUCCESS) 
	{
//		if ((PID & 0x3f) < WDS_PAIRWISE_KEY_OFFSET) // send to client
		{
			MgtMacHeaderInit(pAd, pNullFr, SUBTYPE_NULL_FUNC, 0, pAddr, pAd->ApCfg.MBSSID[apidx].Bssid);
			pNullFr->FC.Type = BTYPE_DATA;
			pNullFr->FC.FrDs = 1;
			pNullFr->Duration = RTMPCalcDuration(pAd, TxRate, 14);

#ifdef UAPSD_AP_SUPPORT
            if (bQosNull)
			{
                UCHAR *qos_p = ((UCHAR *)pNullFr) + Length;

				pNullFr->FC.SubType = SUBTYPE_QOS_NULL;

				/* copy QOS control bytes */
				qos_p[0] = ((bEOSP) ? (1 << 4) : 0) | OldUP;
				qos_p[1] = 0;
				Length += 2;
			} /* End of if */
#endif // UAPSD_AP_SUPPORT //

			DBGPRINT(RT_DEBUG_TRACE, ("send NULL Frame @%d Mbps to AID#%d...\n", RateIdToMbps[TxRate], PID & 0x3f));
            MiniportMMRequest(pAd, MapUserPriorityToAccessCategory[0], (PCHAR)pNullFr, Length);
		}
//#ifdef  WDS
#if 0
		else                                        // send to WDS link
		{
			UCHAR ToWhichWds = (PID & 0x3f) - WDS_PAIRWISE_KEY_OFFSET;
			NdisZeroMemory(pNullFr, LENGTH_802_11_WITH_ADDR4);
			pNullFr->FC.Type = BTYPE_DATA;
			pNullFr->FC.SubType = SUBTYPE_NULL_FUNC;
			pNullFr->FC.FrDs = 1;
			pNullFr->FC.ToDs = 1;
			COPY_MAC_ADDR(pNullFr->Addr1, pAd->WdsTab.MacTab[ToWhichWds].WdsAddr);
			COPY_MAC_ADDR(pNullFr->Addr2, pAd->CurrentAddress);
			COPY_MAC_ADDR(pNullFr->Addr3, pAd->WdsTab.MacTab[ToWhichWds].WdsAddr);
			COPY_MAC_ADDR(pNullFr->Addr3 + MAC_ADDR_LEN, pAd->CurrentAddress);
			pNullFr->Duration = RTMPCalcDuration(pAd, TxRate, 14);
			DBGPRINT(RT_DEBUG_TRACE, ("send NULL Frame @%d Mbps to WDS#%d...\n", RateIdToMbps[TxRate], ToWhichWds));
			ApSendFrame(pAd, pNullFr, LENGTH_802_11_WITH_ADDR4, TxRate, PID);
		}
#endif
	MlmeFreeMemory(pAd, pFrame);
	}
}

VOID    ApSendFrame(
	IN  PRTMP_ADAPTER   pAd,
	IN  PVOID           pBuffer,
	IN  ULONG           Length,
	IN  UCHAR           TxRate,
	IN  UCHAR           PID)
{
}

/* 
	==========================================================================
	Description:
		Send out a ACK frame to a specified STA upon receiving PS-POLL
	==========================================================================
 */
VOID ApEnqueueAckFrame(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR        pAddr,
	IN UCHAR         TxRate,
	IN UCHAR		 apidx) 
{
	NDIS_STATUS    NState;
	PHEADER_802_11  pAckFr;
	PUCHAR			pFrame;

	// since TxRate may change, we have to change Duration each time
	NState = MlmeAllocateMemory(pAd, &pFrame);
	pAckFr = (PHEADER_802_11) pFrame;
	if (NState == NDIS_STATUS_SUCCESS) 
	{
		MgtMacHeaderInit(pAd, pAckFr, SUBTYPE_ACK, 0, pAddr, pAd->ApCfg.MBSSID[apidx].Bssid);
		pAckFr->FC.Type = BTYPE_CNTL;
		MiniportMMRequest(pAd, 0, (PUCHAR)pAckFr, 10);
		MlmeFreeMemory(pAd, pFrame);
	}
}

VOID APSwitchChannel(
	IN PRTMP_ADAPTER pAd,
	IN INT Channel)
{
	INT CentralChannel;
	UCHAR byteValue = 0;
	UINT32 Value;
	
	CentralChannel = Channel;
	if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE))
	{
		CentralChannel = Channel + 2;
		//  TX : control channel at lower 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x1);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at lower 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue &= (~0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);

	}
	else if ((Channel > 2) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && (pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW))
	{
		CentralChannel = Channel - 2;
		//  TX : control channel at upper 
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value |= (0x1);		
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

		//  RX : control channel at upper 
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, 3, &byteValue);
		byteValue |= (0x20);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 3, byteValue);
	}

	AsicSwitchChannel(pAd, CentralChannel, TRUE);
}
/* 
	==========================================================================
	Description:
		This routine is called at initialization. It returns a channel number
		that complies to regulation domain and less interference with current
		enviornment.
	Return:
		ch -  channel number that
	NOTE:
		the retrun channel number is guaranteed to comply to current regulation
		domain that recorded in pAd->CommonCfg.CountryRegion
	==========================================================================
 */
UCHAR APAutoSelectChannel(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN Optimal)
{
	UCHAR cnt, ch = 0, i;
	UCHAR dirtyness[MAX_NUM_OF_CHANNELS+1], dirty;
	CHAR max_rssi[MAX_NUM_OF_CHANNELS+1];
	//UINT32 FalseCca = 0, FcsError = 0; // remove sine them never be used to determine channel.
	BOOLEAN bFindIt = FALSE;
	BOOLEAN IsABand;

	// passive scan channel 1-14. collect statistics
	NdisZeroMemory(dirtyness, MAX_NUM_OF_CHANNELS+1);
	// In the autochannel select case. AP didn't get channel yet.
	// So have no way to determine which Band AP used by channel number.

	IsABand = ((pAd->CommonCfg.PhyMode == PHY_11A)
		|| (pAd->CommonCfg.PhyMode == PHY_11ABG_MIXED)
		|| (pAd->CommonCfg.PhyMode == PHY_11ABGN_MIXED)
		|| (pAd->CommonCfg.PhyMode == PHY_11AN_MIXED)
		|| (pAd->CommonCfg.PhyMode == PHY_11AGN_MIXED)) ? TRUE : FALSE;

	if ((Optimal != TRUE) && (IsABand == TRUE))
	{
		if (pAd->CommonCfg.bIEEE80211H)
		{
			cnt = 0;
			while(TRUE)
			{
				cnt++;
				ch = pAd->ChannelList[RandomByte(pAd)%pAd->ChannelListNum].Channel;

				if (ch == 0)
					ch = FirstChannel(pAd);
				if (!RadarChannelCheck(pAd, ch))
					continue;

				for (i=0; i<pAd->ChannelListNum; i++)
				{
					if (pAd->ChannelList[i].Channel == ch)
					{
						if (pAd->ChannelList[i].RemainingTimeForUse == 0)
							bFindIt = TRUE;
						
						break;
					}
				}
				
				if (bFindIt == TRUE)
					break;

				// have no avaiable channel now. force pick first channel here.
				if (cnt == pAd->ChannelListNum)
				{
					ch = FirstChannel(pAd);
					break;
				}
			};
		}
		else
		{
			ch = pAd->ChannelList[RandomByte(pAd)%pAd->ChannelListNum].Channel;
			if (ch == 0)
				ch = FirstChannel(pAd);
		}
		DBGPRINT(RT_DEBUG_TRACE,("1.APAutoSelectChannel pick up ch#%d\n",ch));
		return ch;
	}
	else
	{
		for (i=0; i<pAd->ChannelListNum; i++)
		{
			ch = pAd->ChannelList[i].Channel;
			//APSwitchChannel(pAd, ch);
			AsicSwitchChannel(pAd, ch, TRUE);
			AsicLockChannel(pAd, ch);
			pAd->Counters8023.GoodReceives = 0;
			pAd->Counters8023.RxErrors = 0;
			pAd->ApCfg.AutoChannel_MaxRssi = -127;
			pAd->ApCfg.AutoChannel_Channel = ch;
			max_rssi[i]=0;

			OS_WAIT(200); // wait for 200 ms at each channel.

			max_rssi[i] = pAd->ApCfg.AutoChannel_MaxRssi;

			// remove sine them never be used to determine channel.
			//RTMP_IO_READ32(pAd, RX_STA_CNT1, &FalseCca);
			//FalseCca &= 0x0000ffff;
			//RTMP_IO_READ32(pAd, RX_STA_CNT0, &FcsError);
			//FcsError &= 0x0000ffff;
		}

		for (i=0; i<pAd->ChannelListNum; i++)
		{
			//if (pAd->Counters8023.GoodReceives)
			if (max_rssi[i] > -127)
			{
				INT ll;
				dirtyness[i] += 10;
				if (!IsABand)
				{
					for (ll=i; ll<(i+4); ll++)
					{
						if (ll < MAX_NUM_OF_CHANNELS)
							dirtyness[ll]++;
					}

					for (ll=i; ll>(i-4); ll--)
					{
						if (ll >= 0)
							dirtyness[ll]++;
					}
				}
			}
			//DBGPRINT(RT_DEBUG_TRACE,("Msleep at ch#%d to collect RX=%lu, RSSI=%d, CRC error =%d, False CCA =%d\n", 
			//	ch, pAd->Counters8023.GoodReceives, max_rssi[i] - pAd->BbpRssiToDbmDelta, FcsError, FalseCca));
			DBGPRINT(RT_DEBUG_TRACE,("Msleep at ch#%d to collect RX=%lu, RSSI=%d\n", 
				pAd->ChannelList[i].Channel, pAd->Counters8023.GoodReceives, max_rssi[i] - pAd->BbpRssiToDbmDelta));
		}
		pAd->ApCfg.AutoChannel_Channel = 0;

		DBGPRINT(RT_DEBUG_TRACE,("Dirtyness = "));
		for (i = 0; i < pAd->ChannelListNum; i++)
		{
			if (i!=0 && i%4 == 0) DBGPRINT(RT_DEBUG_TRACE, ("-"));
			DBGPRINT(RT_DEBUG_TRACE,("%d.", dirtyness[i]));
		}
		DBGPRINT(RT_DEBUG_TRACE, ("\n"));

		// RULE 1. pick up a good channel that no one used
		for (i = 0; i < pAd->ChannelListNum; i++)
		{
			if (dirtyness[i] == 0) break;
		}
		if (i < pAd->ChannelListNum)
		{
			DBGPRINT(RT_DEBUG_TRACE,("APAutoSelectChannel pick up ch#%d\n", pAd->ChannelList[i].Channel));
			return pAd->ChannelList[i].Channel;
		}

		// RULE 2. if not available, then co-use a channel that's no interference (dirtyness=10)
		// RULE 3. if not available, then co-use a channel that has minimum interference (dirtyness=11,12)
		for (dirty = 10; dirty <= 12; dirty++)
		{
			UCHAR candidate[MAX_NUM_OF_CHANNELS+1], candidate_num;
			UCHAR min_rssi = 255, final_channel = 0;
			
			candidate_num = 0;
			NdisZeroMemory(candidate, MAX_NUM_OF_CHANNELS+1);
			for (i = 0; i<pAd->ChannelListNum; i++)
			{
				if (dirtyness[i] == dirty) 
				{ 
					candidate[i]=1; 
					candidate_num++; 
				}
			}
			// if there's more than 1 candidate, pick up the channel with minimum RSSI
			if (candidate_num)
			{
				for (i = 0; i < pAd->ChannelListNum; i++)
				{
					if (candidate[i] && (max_rssi[i] < min_rssi))
					{
						if((pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
							&& (BW40_ChannelCheck(pAd->ChannelList[i].Channel) == FALSE))
							continue;
						final_channel = pAd->ChannelList[i].Channel;
						min_rssi = max_rssi[i];
					}
				}
				if (final_channel != 0)
				{
					DBGPRINT(RT_DEBUG_TRACE,("APAutoSelectChannel pick up ch#%d\n",final_channel));
					return final_channel;
				}
			}
		}

		// RULE 4. still not available, pick up the first channel
		ch = FirstChannel(pAd);
		DBGPRINT(RT_DEBUG_TRACE,("APAutoSelectChannel pick up ch#%d\n",ch));
		return ch;
	}
}

#define BCN_TBTT_OFFSET		64	//defer 64 us
VOID ReSyncBeaconTime(
	IN  PRTMP_ADAPTER   pAd)
{	

	UINT32  Offset;


	Offset = (pAd->TbttTickCount) % (BCN_TBTT_OFFSET);

	pAd->TbttTickCount++;

	//
	// The updated BeaconInterval Value will affect Beacon Interval after two TBTT
	// beacasue the original BeaconInterval had been loaded into next TBTT_TIMER
	// 
	if (Offset == (BCN_TBTT_OFFSET-2))
	{
		BCN_TIME_CFG_STRUC csr;
		RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);
		csr.field.BeaconInterval = (pAd->CommonCfg.BeaconPeriod << 4) - 1 ;	// ASIC register in units of 1/16 TU = 64us
		RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr.word);
	}
	else
	{
		if (Offset == (BCN_TBTT_OFFSET-1))
		{
			BCN_TIME_CFG_STRUC csr;

			RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);
			csr.field.BeaconInterval = (pAd->CommonCfg.BeaconPeriod) << 4; // ASIC register in units of 1/16 TU
			RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr.word);
		}
	}
}

