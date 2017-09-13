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
    connect.c
 
    Abstract:
    Routines to deal Link UP/DOWN and build/update BEACON frame contents
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"

UCHAR PowerConstraintIE[3] = {IE_POWER_CONSTRAINT, 1, 3};

/*
	==========================================================================
	Description:
		Used to check the necessary to send Beancon.
	return value
		0: mean no necessary.
		0: mean need to send Beacon for the service.
	==========================================================================
*/
BOOLEAN BeaconTransmitRequired(
	IN PRTMP_ADAPTER	pAd,
	IN INT				apidx)
{
#ifdef WDS_SUPPORT
	UCHAR idx;
#endif // WDS_SUPPORT //
	BOOLEAN result = TRUE;

	do
	{
#ifdef WDS_SUPPORT
		if (pAd->WdsTab.Mode == WDS_BRIDGE_MODE)
		{
			result = FALSE;
			break;
		}
#endif // WDS_SUPPORT //

#ifdef CARRIER_DETECTION_SUPPORT
		if (isCarrierDetectExist(pAd) == TRUE)
		{
			result = FALSE;
			break;
		}
#endif // CARRIER_DETECTION_SUPPORT //

		if (apidx == MAIN_MBSSID)
		{
			if ((pAd->ApCfg.MBSSID[apidx].MSSIDDev != NULL) 
				&& (pAd->ApCfg.MBSSID[apidx].MSSIDDev->flags & IFF_UP))
			{
				result = TRUE;
				break;
			}
#ifdef WDS_SUPPORT
			for (idx = 0; idx < MAX_WDS_ENTRY; idx++)
			{
				if ((pAd->WdsTab.WdsEntry[idx].dev != NULL)
					&& (pAd->WdsTab.WdsEntry[idx].dev->flags & IFF_UP))
				{
					result = TRUE;
					break;
				}
			}
#endif // WDS_SUPPORT //
		}
		else
		{
			if ((pAd->ApCfg.MBSSID[apidx].MSSIDDev != NULL) 
				&& (pAd->ApCfg.MBSSID[apidx].MSSIDDev->flags & IFF_UP))
				result = TRUE;
		}
	}
	while (FALSE);

	return result;
}

/*
	==========================================================================
	Description:
		Pre-build a BEACON frame in the shared memory
	==========================================================================
*/
VOID APMakeBssBeacon(
	IN PRTMP_ADAPTER	pAd,
	IN INT				apidx)
{
	UCHAR         DsLen = 1, SsidLen;//, TimLen = 4,
				  //BitmapControl = 0, VirtualBitmap = 0, EmptySsidLen = 0, SsidLen;
//	UCHAR         RSNIe=IE_WPA, RSNIe2=IE_WPA2;
	HEADER_802_11 BcnHdr;
	LARGE_INTEGER FakeTimestamp;
	ULONG         FrameLen = 0;
	PTXWI_STRUC    pTxWI = &pAd->BeaconTxWI;
	PUCHAR        pBeaconFrame = pAd->ApCfg.MBSSID[apidx].BeaconBuf;
	UCHAR  *ptr;
	UINT  i;
	UINT32 longValue;

	HTTRANSMIT_SETTING	BeaconTransmit;   // MGMT frame PHY rate setting when operatin at Ht rate.


	if (pAd->ApCfg.MBSSID[apidx].bHideSsid)
		SsidLen = 0;
	else
		SsidLen = pAd->ApCfg.MBSSID[apidx].SsidLen;

	MgtMacHeaderInit(pAd, &BcnHdr, SUBTYPE_BEACON, 0, BROADCAST_ADDR, pAd->ApCfg.MBSSID[apidx].Bssid);

#if 0 // remove to APUpdateBeaconFrame
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPAPSK))
		RSNIe = IE_WPA;
	else if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2PSK))
		RSNIe = IE_WPA2;
#endif // remove to APUpdateBeaconFrame
	
	// for update framelen to TxWI later.
	MakeOutgoingFrame(pBeaconFrame,                  &FrameLen,
					sizeof(HEADER_802_11),           &BcnHdr, 
					TIMESTAMP_LEN,                   &FakeTimestamp,
					2,                               &pAd->CommonCfg.BeaconPeriod,
					2,                               &pAd->ApCfg.MBSSID[apidx].CapabilityInfo,
					1,                               &SsidIe, 
					1,                               &SsidLen, 
					SsidLen,                         pAd->ApCfg.MBSSID[apidx].Ssid,
					1,                               &SupRateIe, 
					1,                               &pAd->CommonCfg.SupRateLen,
					pAd->CommonCfg.SupRateLen,       pAd->CommonCfg.SupRate, 
					1,                               &DsIe, 
					1,                               &DsLen, 
					1,                               &pAd->CommonCfg.Channel,
					END_OF_ARGS);

	if (pAd->CommonCfg.ExtRateLen)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
						1,                               &ExtRateIe, 
						1,                               &pAd->CommonCfg.ExtRateLen,
						pAd->CommonCfg.ExtRateLen,           pAd->CommonCfg.ExtRate, 
						END_OF_ARGS);
		FrameLen += TmpLen;
	}

#if 0 // remove to APUpdateBeaconFrame
	// Append RSN_IE when  WPA OR WPAPSK, 
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  1,                            &RSNIe2,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],      pAd->ApCfg.MBSSID[apidx].RSN_IE[1],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
	else if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
#endif // remove to APUpdateBeaconFrame

#if 0
	// add WMM IE here
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) || pAd->ApCfg.MBSSID[apidx].bWmmCapable)
	{
		ULONG TmpLen;
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

		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
						  26,                            WmeParmIe,
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
#endif
    // add country IE, power constraint IE
    if (pAd->CommonCfg.bCountryFlag)
    {
        ULONG TmpLen, TmpLen2=0;
        UCHAR TmpFrame[256];
        UCHAR CountryIe = IE_COUNTRY;

		// Only 802.11a APs that comply with 802.11h are required to include a Power Constrint Element(IE=32) 
		// in beacons and probe response frames
		if ((pAd->CommonCfg.Channel > 14) && pAd->CommonCfg.bIEEE80211H == TRUE)
		{
        // prepare power constraint IE
        	MakeOutgoingFrame(pBeaconFrame+FrameLen,	&TmpLen,
                          3,							PowerConstraintIE,
                          END_OF_ARGS);
        	FrameLen += TmpLen;
		}	

        NdisZeroMemory(TmpFrame, sizeof(TmpFrame));

		// prepare channel information
#ifdef EXT_BUILD_CHANNEL_LIST
		BuildBeaconChList(pAd, TmpFrame, &TmpLen2);
#else
		{
	        UCHAR MaxTxPower=16;
	        MakeOutgoingFrame(TmpFrame+TmpLen2,     &TmpLen,
	                          1,                 	&pAd->ChannelList[0].Channel,
	                          1,                 	&pAd->ChannelListNum,
	                          1,                 	&MaxTxPower,
	                          END_OF_ARGS);
	        TmpLen2 += TmpLen;
		}
#endif // EXT_BUILD_CHANNEL_LIST //

        // need to do the padding bit check, and concatenate it
        if ((TmpLen2%2) == 0)
        {
        	UCHAR	TmpLen3 = TmpLen2+4;
	        MakeOutgoingFrame(pBeaconFrame+FrameLen,&TmpLen,
	                          1,                 	&CountryIe,
	                          1,                 	&TmpLen3,
	                          3,                 	pAd->CommonCfg.CountryCode,
	                          TmpLen2+1,				TmpFrame,
	                          END_OF_ARGS);
        }
        else
        {
        	UCHAR	TmpLen3 = TmpLen2+3;
	        MakeOutgoingFrame(pBeaconFrame+FrameLen,&TmpLen,
	                          1,                 	&CountryIe,
	                          1,                 	&TmpLen3,
	                          3,                 	pAd->CommonCfg.CountryCode,
	                          TmpLen2,				TmpFrame,
	                          END_OF_ARGS);
        }
        FrameLen += TmpLen;
    }

#if 1
	// AP Channel Report
	{
		UCHAR APChannelReportIe = IE_AP_CHANNEL_REPORT;
		ULONG	TmpLen;

		// 802.11n D2.0 Annex J
		// USA
		// regulatory class 32, channel set 1~7
		// regulatory class 33, channel set 5-11

		UCHAR rclass32[]={32, 1, 2, 3, 4, 5, 6, 7};
        UCHAR rclass33[]={33, 5, 6, 7, 8, 9, 10, 11};
		UCHAR rclasslen = 8; //sizeof(rclass32);

		if (pAd->CommonCfg.PhyMode == PHY_11BGN_MIXED)
		{
			MakeOutgoingFrame(pBeaconFrame+FrameLen,&TmpLen,
							  1,                    &APChannelReportIe,
							  1,                    &rclasslen,
							  rclasslen,            rclass32,
   							  1,                    &APChannelReportIe,
							  1,                    &rclasslen,
							  rclasslen,            rclass33,
							  END_OF_ARGS);
			FrameLen += TmpLen;		
		}	
	}
#endif

#ifdef WSC_AP_SUPPORT
    // add Simple Config Information Element
    if ((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode >= 1) && (pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen))
    {
		ULONG WscTmpLen = 0;
        
		MakeOutgoingFrame(pBeaconFrame+FrameLen,                            &WscTmpLen,
						  pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen,    pAd->ApCfg.MBSSID[apidx].WscIEBeacon.Value,
                              END_OF_ARGS);
		FrameLen += WscTmpLen;		  
    }

    if ((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode != WSC_DISABLE) &&
        (pAd->ApCfg.MBSSID[apidx].IEEE8021X == FALSE) && 
        (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11WEPEnabled))
    {
        /*
            Non-WPS Windows XP and Vista PCs are unable to determine if a WEP enalbed network is static key based 
            or 802.1X based. If the legacy station gets an EAP-Rquest/Identity from the AP, it assume the WEP
            network is 802.1X enabled & will prompt the user for 802.1X credentials. If the legacy station doesn't
            receive anything after sending an EAPOL-Start, it will assume the WEP network is static key based and
            prompt user for the WEP key. <<from "WPS and Static Key WEP Networks">>
            A WPS enabled AP should include this IE in the beacon when the AP is hosting a static WEP key network.  
            The IE would be 7 bytes long with the Extended Capability field set to 0 (all bits zero)
            http://msdn.microsoft.com/library/default.asp?url=/library/en-us/randz/protocol/securing_public_wi-fi_hotspots.asp
        */
        ULONG TempLen = 0;
        UCHAR PROVISION_SERVICE_IE[7] = {0xDD, 0x05, 0x00, 0x50, 0xF2, 0x05, 0x00};
        MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TempLen,
						  7,                            PROVISION_SERVICE_IE,
                          END_OF_ARGS);
        FrameLen += TempLen;
    }
#endif // WSC_AP_SUPPORT //
    

	BeaconTransmit.word = 0;
	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, BSS0Mcast_WCID, 
		FrameLen, PID_MGMT, 0, 0,IFS_HTTXOP, FALSE, &BeaconTransmit);
	//
	// step 6. move BEACON TXD and frame content to on-chip memory
	//
	ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef BIG_ENDIAN
    RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif
	for (i=0; i<TXWI_SIZE; i+=4)  // 16-byte TXWI field
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[apidx] + i, longValue);
		ptr += 4;
	}

	// update BEACON frame content. start right after the 16-byte TXWI field.
	ptr = pAd->ApCfg.MBSSID[apidx].BeaconBuf;
#ifdef BIG_ENDIAN
    RTMPFrameEndianChange(pAd, ptr, DIR_WRITE, FALSE);
#endif
	for (i= 0; i< FrameLen; i+=4)
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[apidx] + TXWI_SIZE + i, longValue);
		ptr += 4;
	}
	pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon = (UCHAR)FrameLen; 
	pAd->ApCfg.MBSSID[apidx].CapabilityInfoLocationInBeacon = sizeof(HEADER_802_11) + TIMESTAMP_LEN + 2;

}


/*
	==========================================================================
	Description:
		Update the BEACON frame in the shared memory. Because TIM IE is variable
		length. other IEs after TIM has to shift and total frame length may change
		for each BEACON period.
	Output:
		pAd->ApCfg.MBSSID[apidx].CapabilityInfo
		pAd->ApCfg.ErpIeContent
	==========================================================================
*/
VOID APUpdateBeaconFrame(
	IN PRTMP_ADAPTER	pAd,
	IN INT				apidx) 
{
	//PTXWI_STRUC    	pTxWI = &pAd->BeaconTxWI;
	PUCHAR        	pBeaconFrame = pAd->ApCfg.MBSSID[apidx].BeaconBuf;
	UCHAR  			*ptr;
	ULONG 			FrameLen = pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon;
	ULONG 			UpdatePos = pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon;
	//ULONG			CapInfoPos = pAd->ApCfg.MBSSID[apidx].CapabilityInfoLocationInBeacon;
	UCHAR         	RSNIe=IE_WPA, RSNIe2=IE_WPA2;
	UCHAR 			ID_1B, TimFirst, TimLast, *pTim;


#if 0
	UCHAR byte0 = (UCHAR)(pAd->ApCfg.MBSSID[apidx].TimBitmap & 0x000000fe);  // skip AID#0
	UCHAR byte1 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap & 0x0000ff00) >> 8);
	UCHAR byte2 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap & 0x00ff0000) >> 16);
	UCHAR byte3 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap & 0xff000000) >> 24);
	UCHAR byte4 = (UCHAR)(pAd->ApCfg.MBSSID[apidx].TimBitmap2 & 0x000000ff);
	UCHAR byte5 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap2 & 0x0000ff00) >> 8);
	UCHAR byte6 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap2 & 0x00ff0000) >> 16);
	UCHAR byte7 = (UCHAR)((pAd->ApCfg.MBSSID[apidx].TimBitmap2 & 0xff000000) >> 24);
#endif
	UINT  i;
	HTTRANSMIT_SETTING	BeaconTransmit;   // MGMT frame PHY rate setting when operatin at Ht rate.

	//
	// step 1 - update BEACON's Capability
	//
	ptr = pBeaconFrame + pAd->ApCfg.MBSSID[apidx].CapabilityInfoLocationInBeacon;
	*ptr = (UCHAR)(pAd->ApCfg.MBSSID[apidx].CapabilityInfo & 0x00ff);
	*(ptr+1) = (UCHAR)((pAd->ApCfg.MBSSID[apidx].CapabilityInfo & 0xff00) >> 8);

	//
	// step 2 - update TIM IE
	// TODO: enlarge TIM bitmap to support up to 64 STAs
	// TODO: re-measure if RT2600 TBTT interrupt happens faster than BEACON sent out time
	//
#if 0
	if (pAd->ApCfg.DtimCount == 0)
		pAd->ApCfg.DtimCount = pAd->ApCfg.DtimPeriod - 1;
	else
		pAd->ApCfg.DtimCount -= 1;
#endif

	ptr = pBeaconFrame + pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon;
	*ptr = IE_TIM;
	*(ptr + 2) = pAd->ApCfg.DtimCount;
	*(ptr + 3) = pAd->ApCfg.DtimPeriod;

#if 0
	if (byte0 || byte1) // there's some backlog frame for AID 1-15
	{
		*(ptr + 4) = 0;      // Virtual TIM bitmap stars from AID #0
		*(ptr + 5) = byte0;
		*(ptr + 6) = byte1;
		*(ptr + 7) = byte2;
		*(ptr + 8) = byte3;
		*(ptr + 9) = byte4;
		*(ptr + 10) = byte5;
		*(ptr + 11) = byte6;
		*(ptr + 12) = byte7;
		if (byte7)      *(ptr + 1) = 11; // IE length
		else if (byte6) *(ptr + 1) = 10; // IE length
		else if (byte5) *(ptr + 1) = 9;  // IE length
		else if (byte4) *(ptr + 1) = 8;  // IE length
		else if (byte3) *(ptr + 1) = 7;  // IE length
		else if (byte2) *(ptr + 1) = 6;  // IE length
		else if (byte1) *(ptr + 1) = 5;  // IE length
		else            *(ptr + 1) = 4;  // IE length
	}
	else if (byte2 || byte3) // there's some backlogged frame for AID 16-31
	{
		*(ptr + 4) = 2;      // Virtual TIM bitmap starts from AID #16
		*(ptr + 5) = byte2;
		*(ptr + 6) = byte3;
		*(ptr + 7) = byte4;
		*(ptr + 8) = byte5;
		*(ptr + 9) = byte6;
		*(ptr + 10) = byte7;
		if (byte7)      *(ptr + 1) = 9; // IE length
		else if (byte6) *(ptr + 1) = 8; // IE length
		else if (byte5) *(ptr + 1) = 7; // IE length
		else if (byte4) *(ptr + 1) = 6; // IE length
		else if (byte3) *(ptr + 1) = 5; // IE length
		else            *(ptr + 1) = 4; // IE length
	}
	else if (byte4 || byte5) // there's some backlogged frame for AID 32-47
	{
		*(ptr + 4) = 4;      // Virtual TIM bitmap starts from AID #32
		*(ptr + 5) = byte4;
		*(ptr + 6) = byte5;
		*(ptr + 7) = byte6;
		*(ptr + 8) = byte7;
		if (byte7)      *(ptr + 1) = 7; // IE length
		else if (byte6) *(ptr + 1) = 6; // IE length
		else if (byte5) *(ptr + 1) = 5; // IE length
		else            *(ptr + 1) = 4; // IE length
	}
	else if (byte6 || byte7) // there's some backlogged frame for AID 48-63
	{
		*(ptr + 4) = 6;      // Virtual TIM bitmap starts from AID #48
		*(ptr + 5) = byte6;
		*(ptr + 6) = byte7;
		if (byte7)      *(ptr + 1) = 5; // IE length
		else            *(ptr + 1) = 4; // IE length
	}
	else // no backlogged frames
	{
		*(ptr + 1) = 4; // IE length
		*(ptr + 4) = 0;
		*(ptr + 5) = 0;
	}
#else

	/* find the smallest AID (PS mode) */
	TimFirst = 0; /* record first TIM byte != 0x00 */
	TimLast = 0;  /* record last  TIM byte != 0x00 */
	pTim = pAd->ApCfg.MBSSID[apidx].TimBitmaps;

	for(ID_1B=0; ID_1B<WLAN_MAX_NUM_OF_TIM; ID_1B++)
	{
		/* get the TIM indicating PS packets for 8 stations */
		UCHAR tim_1B = pTim[ID_1B];

		if (ID_1B == 0)
			tim_1B &= 0xfe; /* skip bit0 bc/mc */
		/* End of if */

		if (tim_1B == 0)
			continue; /* find next 1B */
		/* End of if */

		if (TimFirst == 0)
			TimFirst = ID_1B;
		/* End of if */

		TimLast = ID_1B;
	} /* End of for */

	/* fill TIM content to beacon buffer */
	if (TimFirst & 0x01)
		TimFirst --; /* find the even offset byte */
	/* End of if */

	*(ptr + 1) = 3+(TimLast-TimFirst+1); /* TIM IE length */
	*(ptr + 4) = TimFirst;

	for(i=TimFirst; i<=TimLast; i++)
		*(ptr + 5 + i - TimFirst) = pTim[i];
	/* End of for */
#endif

	// bit0 means backlogged mcast/bcast
    if (pAd->ApCfg.DtimCount == 0)
	*(ptr + 4) |= (pAd->ApCfg.MBSSID[apidx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] & 0x01); 

	// adjust BEACON length according to the new TIM
	FrameLen += (2 + *(ptr+1)); 

	// Update ERP
    if (pAd->CommonCfg.ExtRateLen)
    {
		//
        // fill ERP IE
        // 
        ptr = (UCHAR *)pBeaconFrame + FrameLen; // pTxD->DataByteCnt;
        *ptr = IE_ERP;
        *(ptr + 1) = 1;
        *(ptr + 2) = pAd->ApCfg.ErpIeContent;
		FrameLen += 3;
	}

	//
	// fill up Channel Switch Announcement Element
	//
	if ((pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
	{
		ptr = pBeaconFrame + FrameLen;
		*ptr = IE_CHANNEL_SWITCH_ANNOUNCEMENT;
		*(ptr + 1) = 3;
		*(ptr + 2) = 1;
		*(ptr + 3) = pAd->CommonCfg.Channel;
		*(ptr + 4) = (pAd->CommonCfg.RadarDetect.CSPeriod - pAd->CommonCfg.RadarDetect.CSCount - 1);
		ptr      += 5;
		FrameLen += 5;

		// Extended Channel Switch Announcement Element
		if (pAd->CommonCfg.bExtChannelSwitchAnnouncement)
		{
			HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE	HtExtChannelSwitchIe;
			build_ext_channel_switch_ie(pAd, &HtExtChannelSwitchIe);
			NdisMoveMemory(ptr, &HtExtChannelSwitchIe, sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE));
			ptr += sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE);
			FrameLen += sizeof(HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE);
		}
	}


	//
	// step 5. Update HT. Since some fields might change in the same BSS.
	//
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR HtLen, HtLen1;
		//UCHAR i;

#ifdef BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
		USHORT	b2lTmp, b2lTmp2;
#endif

		// add HT Capability IE 
		HtLen = sizeof(pAd->CommonCfg.HtCapability);
		HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);
#ifndef BIG_ENDIAN
		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
								  1,                                &HtCapIe,
								  1,                                &HtLen,
								 HtLen,          &pAd->CommonCfg.HtCapability, 
								  1,                                &AddHtInfoIe,
								  1,                                &HtLen1,
								 HtLen1,          &pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
		NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
		*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
		*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

		NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
		*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
		*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
								  1,                                &HtCapIe,
								  1,                                &HtLen,
								 HtLen,                   &HtCapabilityTmp, 
								  1,                                &AddHtInfoIe,
								  1,                                &HtLen1,
								 HtLen1,                   &addHTInfoTmp, 
						  END_OF_ARGS);
#endif
		FrameLen += TmpLen;

#if 0
		if (pAd->bBroadComHT == TRUE)
		{
			USHORT	epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = HtLen + 4;
			MakeOutgoingFrame(pBeaconFrame + FrameLen,      &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  HtLen,          					&pAd->CommonCfg.HtCapability, 
						  END_OF_ARGS);


			FrameLen += TmpLen;

			epigram_ie_len = HtLen1 + 4;
			MakeOutgoingFrame(pBeaconFrame + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);

			FrameLen += TmpLen;
		}
#endif		

	}

#if 0	// move to ap_connect.c::APMakeBssBeacon
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPAPSK))
		RSNIe = IE_WPA;
	else if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2PSK))
		RSNIe = IE_WPA2;


	// Append RSN_IE when  WPA OR WPAPSK, 
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  1,                            &RSNIe2,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],      pAd->ApCfg.MBSSID[apidx].RSN_IE[1],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
	else if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
#endif

	// P802.11n_D1.10 
	// 7.3.2.27 Extended Capabilities IE
	// HT Information Exchange Support
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR ExtCapIe[3] = {IE_EXT_CAPABILITY, 1, 0x01};
		MakeOutgoingFrame(pBeaconFrame+FrameLen, &TmpLen,
							3,                   ExtCapIe,
							END_OF_ARGS);
		FrameLen += TmpLen;
	}
 
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPAPSK))
		RSNIe = IE_WPA;
	else if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2PSK))
		RSNIe = IE_WPA2;

	// Append RSN_IE when  WPA OR WPAPSK, 
	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  1,                            &RSNIe2,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1],      pAd->ApCfg.MBSSID[apidx].RSN_IE[1],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
	else if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],
						  pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0],      pAd->ApCfg.MBSSID[apidx].RSN_IE[0],
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}

	// add WMM IE here
	if (pAd->ApCfg.MBSSID[apidx].bWmmCapable)
	{
		ULONG TmpLen;
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

		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
						  26,                            WmeParmIe,
						  END_OF_ARGS);
		FrameLen += TmpLen;

		TmpLen = QBSS_LoadElementAppend(pAd, pBeaconFrame+FrameLen);
		FrameLen += TmpLen;
	}

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR HtLen, HtLen1;
		//UCHAR i;
#ifdef BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
		USHORT	b2lTmp, b2lTmp2;
#endif
		// add HT Capability IE 
		HtLen = sizeof(pAd->CommonCfg.HtCapability);
		HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);

		if (pAd->bBroadComHT == TRUE)
		{
			UCHAR epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = HtLen + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pBeaconFrame + FrameLen,      &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  HtLen,          					&pAd->CommonCfg.HtCapability, 
						  END_OF_ARGS);
#else
			NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

			MakeOutgoingFrame(pBeaconFrame + FrameLen,       &TmpLen,
						1,                               &WpaIe,
						1,                               &epigram_ie_len,
						4,                               &BROADCOM_HTC[0], 
						HtLen,                           &HtCapabilityTmp,
						END_OF_ARGS);
#endif

			FrameLen += TmpLen;

			epigram_ie_len = HtLen1 + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pBeaconFrame + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
			NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
			*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
			*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

			MakeOutgoingFrame(pBeaconFrame + FrameLen,         &TmpLen,
							1,                             &WpaIe,
							1,                             &epigram_ie_len,
							4,                             &BROADCOM_AHTINFO[0],
							HtLen1,                        &addHTInfoTmp,
							END_OF_ARGS);
#endif
			FrameLen += TmpLen;
		}

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

	MakeOutgoingFrame(pBeaconFrame+FrameLen, &TmpLen,
						9,                   RalinkSpecificIe,
						END_OF_ARGS);
	FrameLen += TmpLen;
}

	//
	// step 6. Since FrameLen may change, update TXWI.
	//
	if (pAd->CommonCfg.Channel <= 14)
		BeaconTransmit.word = 0;
	else
		BeaconTransmit.word = 0x4000;
	
	RTMPWriteTxWI(pAd, &pAd->BeaconTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, 0xff, 
		FrameLen, PID_MGMT, QID_MGMT, 0, IFS_HTTXOP, FALSE, &BeaconTransmit);

			
	//
	// step 7. move BEACON TXWI and frame content to on-chip memory
	//
	RT28xx_APUpdateBeaconToAsic(pAd, apidx, FrameLen, UpdatePos);

}


/*
    ==========================================================================
    Description:
        Pre-build All BEACON frame in the shared memory
    ==========================================================================
*/
VOID APMakeAllBssBeacon(
    IN PRTMP_ADAPTER pAd)
{
	INT		i, j;
	UINT32	regValue;
	UCHAR	NumOfMacs;
	UCHAR	NumOfBcns;

	// before MakeBssBeacon, clear all beacon TxD's valid bit
    /* Note: can not use MAX_MBSSID_NUM here, or
			 1. when MBSS_SUPPORT is enabled;
             2. MAX_MBSSID_NUM will be 8;
			 3. if HW_BEACON_OFFSET is 0x0200,
             we will overwrite other shared memory SRAM of chip */
	/* use pAd->ApCfg.BssidNum to avoid the case is best */

	// choose the Beacon number
	NumOfBcns = pAd->ApCfg.BssidNum + MAX_MESH_NUM;

//	for(i=0; i<MAX_MBSSID_NUM; i++)
//  for(i=0; i<pAd->ApCfg.BssidNum; i++)
	for(i=0; i<NumOfBcns; i++)
	{
		for (j=0; j<TXWI_SIZE; j+=4)  // 16-bytes TXWI field
	    {
	        RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[i] + j, 0);
	    }
	}


	for(i=0; i<pAd->ApCfg.BssidNum; i++)
	{
		APMakeBssBeacon(pAd, i);
	}
	
	RTMP_IO_READ32(pAd, MAC_BSSID_DW1, &regValue);
	regValue &= 0x0000FFFF;

	
	// Note: 1.	The MAC address of Mesh and AP-Client link are different from Main BSSID.
	//		 2	If the Mesh link is included, its MAC address shall follow the last MBSSID's MAC by increasing 1.
	//		 3. If the AP-Client link is included, its MAC address shall follow the Mesh interface MAC by increasing 1.		
	NumOfMacs = pAd->ApCfg.BssidNum + MAX_MESH_NUM + MAX_APCLI_NUM;

#if 0
	if (NumOfMacs <= 1)
		regValue |= 0x0;
//#ifdef MBSS_SUPPORT
	else if (NumOfMacs <= 2)
		regValue |= (1<<18) | (1<<16);
	else if (NumOfMacs <= 4)
		regValue |= (3<<18) | (2<<16);
	else if (NumOfMacs <= 8)
		regValue |= (7<<18) | (3<<16);
//#endif /* MBSS_SUPPORT */
#else

	// set Multiple BSSID mode
	if (NumOfMacs <= 1)
	{
		pAd->ApCfg.MacMask = ~(1-1);
		;//regValue |= 0x0;
	}	
	else if (NumOfMacs <= 2)
	{
		if (pAd->PermanentAddress[5] % 2 != 0)
			DBGPRINT(RT_DEBUG_ERROR, ("The 2-BSSID mode is enabled, the BSSID byte5 MUST be the multiple of 2\n"));
		
		regValue |= (1<<16);
		pAd->ApCfg.MacMask = ~(2-1);
	}
	else if (NumOfMacs <= 4)
	{
		if (pAd->PermanentAddress[5] % 4 != 0)
			DBGPRINT(RT_DEBUG_ERROR, ("The 4-BSSID mode is enabled, the BSSID byte5 MUST be the multiple of 4\n"));

		regValue |= (2<<16);
		pAd->ApCfg.MacMask = ~(4-1);
	}
	else if (NumOfMacs <= 8)
	{
		if (pAd->PermanentAddress[5] % 8 != 0)
			DBGPRINT(RT_DEBUG_ERROR, ("The 8-BSSID mode is enabled, the BSSID byte5 MUST be the multiple of 8\n"));
	
		regValue |= (3<<16);
		pAd->ApCfg.MacMask = ~(8-1);
	}

	// set Multiple BSSID Beacon number
	if (NumOfBcns > 1)
		regValue |= ((NumOfBcns - 1)<<18);	
	
#endif
	RTMP_IO_WRITE32(pAd, MAC_BSSID_DW1, regValue);


}


/*
    ==========================================================================
    Description:
        Pre-build All BEACON frame in the shared memory
    ==========================================================================
*/
VOID APUpdateAllBeaconFrame(
    IN PRTMP_ADAPTER pAd)
{
	INT		i;
	
	if (pAd->ApCfg.DtimCount == 0)
		pAd->ApCfg.DtimCount = pAd->ApCfg.DtimPeriod - 1;
	else
		pAd->ApCfg.DtimCount -= 1;

	for(i=0; i<pAd->ApCfg.BssidNum; i++)
	{		
		APUpdateBeaconFrame(pAd, i);
	}
}


