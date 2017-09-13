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
     ap_sanity.c
     
     Abstract:
     Handle association related requests either from WSTA or from local MLME
     
     Revision History:
     Who         When          What
     --------    ----------    ----------------------------------------------
     John Chang  08-14-2003    created for 11g soft-AP
     John Chang  12-30-2004    merge with STA driver for RT2600
*/

#include "rt_config.h"

extern UCHAR	CISCO_OUI[];

extern UCHAR	WPA_OUI[];
extern UCHAR	RSN_OUI[];
extern UCHAR   WME_INFO_ELEM[];
extern UCHAR   WME_PARM_ELEM[];
extern UCHAR	Ccx2QosInfo[];
extern UCHAR   RALINK_OUI[];

extern UCHAR 	BROADCOM_OUI[]; 

#ifdef WSC_AP_SUPPORT
extern UCHAR    WPS_OUI[];

typedef struct wsc_ie_probreq_data
{
	UCHAR	ssid[32];
	UCHAR	macAddr[6];
	UCHAR	data[2];
} WSC_IE_PROBREQ_DATA;
#endif // WSC_AP_SUPPORT //

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */

BOOLEAN PeerAssocReqCmmSanity(
    IN PRTMP_ADAPTER pAd, 
	IN BOOLEAN isReassoc,
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2,
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pListenInterval, 
    OUT PUCHAR pApAddr,
    OUT UCHAR *pSsidLen,
    OUT char *Ssid,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *RSN,
    OUT UCHAR *pRSNLen,
    OUT BOOLEAN *pbWmmCapable,
#ifdef WSC_AP_SUPPORT
    OUT BOOLEAN *pWscCapable,
#endif // WSC_AP_SUPPORT //
    OUT ULONG  *pRalinkIe,
#ifdef DOT11N_DRAFT3
    OUT EXT_CAP_INFO_ELEMENT *pExtCapInfo,
#endif // DOT11N_DRAFT3 //
    OUT UCHAR		 *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE *pHtCapability)
{
    CHAR         *Ptr;
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;
    PEID_STRUCT  eid_ptr;
    UCHAR        Sanity=0;
    UCHAR           WPA1_OUI[4]={0x00,0x50,0xF2,0x01};
    UCHAR           WPA2_OUI[3]={0x00,0x0F,0xAC};
    MAC_TABLE_ENTRY *pEntry = (MAC_TABLE_ENTRY*)NULL;

    // to prevent caller from using garbage output value
	*pSsidLen     = 0;
    *pRatesLen    = 0;
    *pRSNLen      = 0;
    *pbWmmCapable = FALSE;
    *pRalinkIe    = 0;
    *pHtCapabilityLen= 0;	

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);

	pEntry = MacTableLookup(pAd, pAddr2);

	//ASSERT(pEntry);

	if (pEntry == NULL)
	{
		return FALSE;
	}

    Ptr = Fr->Octet;

    NdisMoveMemory(pCapabilityInfo, &Fr->Octet[0], 2);
    NdisMoveMemory(pListenInterval, &Fr->Octet[2], 2);

    if (isReassoc) 
	{
		NdisMoveMemory(pApAddr, &Fr->Octet[4], 6);
		eid_ptr = (PEID_STRUCT) &Fr->Octet[10];
	}
	else
	{
    eid_ptr = (PEID_STRUCT) &Fr->Octet[4];
	}


    // get variable fields from payload and advance the pointer
    while (((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((UCHAR*)Fr + MsgLen))
    {
        switch(eid_ptr->Eid)
        {
            case IE_SSID:
		 if (((Sanity&0x1) == 1))
		 	break;

                if((eid_ptr->Len <= MAX_LEN_OF_SSID))
                {
                    Sanity |= 0x01;
                    NdisMoveMemory(Ssid, eid_ptr->Octet, eid_ptr->Len);
                    *pSsidLen = eid_ptr->Len;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - SsidLen = %d  \n", *pSsidLen ));
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SSID\n"));
                    return FALSE;
                }
                break;
                
            case IE_SUPP_RATES:
                if ((eid_ptr->Len <= MAX_LEN_OF_SUPPORTED_RATES) && (eid_ptr->Len > 0))
                {
                    Sanity |= 0x02;
                    NdisMoveMemory(Rates, eid_ptr->Octet, eid_ptr->Len);
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - IE_SUPP_RATES., Len=%d. Rates[0]=%x\n",eid_ptr->Len, Rates[0]));
                    DBGPRINT(RT_DEBUG_TRACE, ("Rates[1]=%x %x %x %x %x %x %x\n", Rates[1], Rates[2], Rates[3], Rates[4], Rates[5], Rates[6], Rates[7]));
                    *pRatesLen = eid_ptr->Len;
                }
                else
                {
                	// HT rate not ready yet. return true temporarily. rt2860c
                    //DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n"));
                    Sanity |= 0x02;
                    *pRatesLen = 8;
					Rates[0] = 0x82;
					Rates[1] = 0x84;
					Rates[2] = 0x8b;
					Rates[3] = 0x96;
					Rates[4] = 0x12;
					Rates[5] = 0x24;
					Rates[6] = 0x48;
					Rates[7] = 0x6c;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES., Len=%d\n",eid_ptr->Len));
                    //return FALSE;
                }
                break;
                
            case IE_EXT_SUPP_RATES:
                if (eid_ptr->Len + *pRatesLen <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(&Rates[*pRatesLen], eid_ptr->Octet, eid_ptr->Len);
                    *pRatesLen = (*pRatesLen) + eid_ptr->Len;
                }
                else
                {
                    NdisMoveMemory(&Rates[*pRatesLen], eid_ptr->Octet, MAX_LEN_OF_SUPPORTED_RATES - (*pRatesLen));
                    *pRatesLen = MAX_LEN_OF_SUPPORTED_RATES;
                }
                break;
                
            case IE_HT_CAP:
			if (eid_ptr->Len >= sizeof(HT_CAPABILITY_IE))
			{
				NdisMoveMemory(pHtCapability, eid_ptr->Octet, SIZE_HT_CAP_IE);

				*(USHORT *)(&pHtCapability->HtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));
				*(USHORT *)(&pHtCapability->ExtHtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));

				*pHtCapabilityLen = SIZE_HT_CAP_IE;
				Sanity |= 0x10;
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_HT_CAP\n"));
			}
			else
			{
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - wrong IE_HT_CAP.eid_ptr->Len = %d\n", eid_ptr->Len));
			}
				
		break;
#ifdef DOT11N_DRAFT3
		case IE_EXT_CAPABILITY:
			if (eid_ptr->Len >= sizeof(EXT_CAP_INFO_ELEMENT))
			{
				NdisMoveMemory(pExtCapInfo, eid_ptr->Octet, 1);
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_EXT_CAPABILITY!\n"));
			}

			break;
#endif // DOT11N_DRAFT3 //

            case IE_WPA:    // same as IE_VENDOR_SPECIFIC
            case IE_WPA2:
#ifdef WSC_AP_SUPPORT                
                if (NdisEqualMemory(eid_ptr->Octet, WPS_OUI, 4))
                {
                    *pWscCapable = TRUE;
                    break;
                }
#endif // WSC_AP_SUPPORT //
				/* Handle Atheros and Broadcom draft 11n STAs */
				if (NdisEqualMemory(eid_ptr->Octet, BROADCOM_OUI, 3))
				{
					switch (eid_ptr->Octet[3])
					{
						case 0x33: 
							if ((eid_ptr->Len-4) == sizeof(HT_CAPABILITY_IE))
							{
								NdisMoveMemory(pHtCapability, &eid_ptr->Octet[4], SIZE_HT_CAP_IE);

								*(USHORT *)(&pHtCapability->HtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));
								*(USHORT *)(&pHtCapability->ExtHtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));

								*pHtCapabilityLen = SIZE_HT_CAP_IE;
							}
							break;
						
						default:
							// ignore other cases 
							break;
					}
				}

                if (NdisEqualMemory(eid_ptr->Octet, RALINK_OUI, 3) && (eid_ptr->Len == 7))
                {
                    //*pRalinkIe = eid_ptr->Octet[3];
					if (eid_ptr->Octet[3] != 0)
                    	*pRalinkIe = eid_ptr->Octet[3];
        			else
        				*pRalinkIe = 0xf0000000; // Set to non-zero value (can't set bit0-2) to represent this is Ralink Chip. So at linkup, we will set ralinkchip flag.
                    break;
                }
                
                // WMM_IE
                if (NdisEqualMemory(eid_ptr->Octet, WME_INFO_ELEM, 6) && (eid_ptr->Len == 7))
                {
                    *pbWmmCapable = TRUE;

#ifdef UAPSD_AP_SUPPORT
                    UAPSD_AssocParse(pAd, pEntry, &eid_ptr->Octet[6]);
#endif // UAPSD_AP_SUPPORT //

                    break;
                }

                if (pAd->ApCfg.MBSSID[pEntry->apidx].AuthMode < Ndis802_11AuthModeWPA)
                    break;
                
                // If this IE did not begins with 00:0x50:0xf2:0x01,  it would be proprietary.  So we ignore
                if (!NdisEqualMemory(eid_ptr->Octet, WPA1_OUI, sizeof(WPA1_OUI))
                    && !NdisEqualMemory(&eid_ptr->Octet[2], WPA2_OUI, sizeof(WPA2_OUI)))
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("Not RSN IE, maybe WMM IE!!!\n"));
                    break;                          
                }
                
                if ((eid_ptr->Len <= MAX_LEN_OF_RSNIE) && (eid_ptr->Len > MIN_LEN_OF_RSNIE))
                {
                    if (!pEntry)
                        return FALSE;

                    DBGPRINT(RT_DEBUG_TRACE, ("Receive IE_WPA : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                        eid_ptr->Octet[0],eid_ptr->Octet[1],eid_ptr->Octet[2],eid_ptr->Octet[3],
                        eid_ptr->Octet[4],eid_ptr->Octet[5],eid_ptr->Octet[6],eid_ptr->Octet[7]));
                    
                    //*pRSNLen=eid_ptr->Len;
                    if (!RTMPCheckMcast(pAd, eid_ptr, pEntry))
                    {
						// send wireless event - for RSN IE sanity check fail
						if (pAd->CommonCfg.bWirelessEvent)
							RTMPSendWirelessEvent(pAd, IW_RSNIE_SANITY_FAIL_EVENT_FLAG, pAddr2, 0, 0); 
			
                        DBGPRINT(RT_DEBUG_TRACE, ("RTMPCheckMcast FAILED !!!\n"));
                        MlmeDeAuthAction( pAd, pEntry, REASON_MCIPHER_NOT_VALID);
                        return FALSE;
                    }                        
                    if (!RTMPCheckUcast(pAd, eid_ptr, pEntry))
                    {
						// send wireless event - for RSN IE sanity check fail
						if (pAd->CommonCfg.bWirelessEvent)
							RTMPSendWirelessEvent(pAd, IW_RSNIE_SANITY_FAIL_EVENT_FLAG, pAddr2, 0, 0);

                        DBGPRINT(RT_DEBUG_TRACE, ("RTMPCheckUcast FAILED !!!\n"));
                        MlmeDeAuthAction( pAd, pEntry, REASON_UCIPHER_NOT_VALID);
                        return FALSE;
                    }                        
                    if (!RTMPCheckAUTH(pAd, eid_ptr, pEntry))
                    {
						// send wireless event - for RSN IE sanity check fail
						if (pAd->CommonCfg.bWirelessEvent)
							RTMPSendWirelessEvent(pAd, IW_RSNIE_SANITY_FAIL_EVENT_FLAG, pAddr2, 0, 0);

                        DBGPRINT(RT_DEBUG_TRACE, ("RTMPCheckAUTH FAILED !!!\n"));
                        MlmeDeAuthAction( pAd, pEntry, REASON_INVALID_IE);
                        return FALSE;
                    }                        
                    
					// Copy whole RSNIE context
                    //NdisMoveMemory(RSN, eid_ptr->Octet, eid_ptr->Len);
                    NdisMoveMemory(RSN, eid_ptr, eid_ptr->Len + 2);
					*pRSNLen=eid_ptr->Len + 2;

                    // for debug
                    {
                        UCHAR CipherAlg = CIPHER_NONE;
                        if (pEntry->WepStatus == Ndis802_11Encryption1Enabled)
                            CipherAlg = CIPHER_WEP64;
                        else if (pEntry->WepStatus == Ndis802_11Encryption2Enabled)
                            CipherAlg = CIPHER_TKIP;
                        else if (pEntry->WepStatus == Ndis802_11Encryption3Enabled)
                            CipherAlg = CIPHER_AES;
                        DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity(AID#%d WepStatus=%s)\n", pEntry->Aid, CipherName[CipherAlg]));
                    }
                }
                else
                {
                    *pRSNLen=0;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - missing IE_WPA(%d)\n",eid_ptr->Len));
                    return FALSE;
                }               
                break;          
            default:
                break;
        }
        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);        
    }

	if ((Sanity&0x3) != 0x03)	 
    {
        DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - missing mandatory field\n"));
        return FALSE;
    }
    else
    {
        DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - success\n"));
        return TRUE;
    }
}


#if 0
BOOLEAN PeerAssocReqSanity(
    IN  PRTMP_ADAPTER   pAd, 
    IN  VOID *Msg, 
    IN  ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pListenInterval, 
    OUT UCHAR *pSsidLen,
    OUT char *Ssid,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *RSN,
    OUT UCHAR *pRSNLen,
    OUT BOOLEAN *pbWmmCapable,
    OUT ULONG *pRalinkIe,
    OUT UCHAR		 *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE	*pHtCapability) 
{
	UCHAR	ApAddr[MAC_ADDR_LEN];
                
	return PeerAssocReqCmmSanity(pAd, 0, Msg, MsgLen, pAddr2, pCapabilityInfo, pListenInterval,
									 ApAddr, pSsidLen, Ssid, pRatesLen, Rates, RSN, pRSNLen, pbWmmCapable, 
									 pRalinkIe, pHtCapabilityLen, pHtCapability);
                    }                        

BOOLEAN PeerReassocReqSanity(
    IN  PRTMP_ADAPTER   pAd, 
    IN  VOID *Msg, 
    IN  ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pListenInterval, 
    OUT PUCHAR pApAddr,
    OUT UCHAR *pSsidLen,
    OUT char *Ssid,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *RSN,
    OUT UCHAR *pRSNLen,
    OUT BOOLEAN *pbWmmCapable,
    OUT ULONG *pRalinkIe,
    OUT UCHAR		 *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE	*pHtCapability) 
                {
   	return PeerAssocReqCmmSanity(pAd, 1, Msg, MsgLen, pAddr2, pCapabilityInfo, pListenInterval,
									 pApAddr, pSsidLen, Ssid, pRatesLen, Rates, RSN, pRSNLen, pbWmmCapable, 
									 pRalinkIe, pHtCapabilityLen, pHtCapability);
                
    }
#endif


/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerDisassocReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *Reason) 
{
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);
    NdisMoveMemory(Reason, &Fr->Octet[0], 2);

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerDeauthReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *Reason) 
{
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);
    NdisMoveMemory(Reason, &Fr->Octet[0], 2);

    return TRUE;
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN APPeerAuthSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr1, 
    OUT PUCHAR pAddr2, 
    OUT USHORT *Alg, 
    OUT USHORT *Seq, 
    OUT USHORT *Status, 
    CHAR *ChlgText) 
{
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;

	COPY_MAC_ADDR(pAddr1,  &Fr->Hdr.Addr1);		// BSSID 
    COPY_MAC_ADDR(pAddr2,  &Fr->Hdr.Addr2);		// SA
    NdisMoveMemory(Alg,    &Fr->Octet[0], 2);
    NdisMoveMemory(Seq,    &Fr->Octet[2], 2);
    NdisMoveMemory(Status, &Fr->Octet[4], 2);

    if (*Alg == AUTH_MODE_OPEN) 
    {
        if (*Seq == 1 || *Seq == 2) 
        {
            return TRUE;
        } 
        else 
        {
            DBGPRINT(RT_DEBUG_TRACE, ("APPeerAuthSanity fail - wrong Seg# (=%d)\n", *Seq));
            return FALSE;
        }
    } 
    else if (*Alg == AUTH_MODE_KEY) 
    {
        if (*Seq == 1 || *Seq == 4) 
        {
            return TRUE;
        } 
        else if (*Seq == 2 || *Seq == 3) 
        {
            NdisMoveMemory(ChlgText, &Fr->Octet[8], CIPHER_TEXT_LEN);
            return TRUE;
        } 
        else 
        {
            DBGPRINT(RT_DEBUG_TRACE, ("APPeerAuthSanity fail - wrong Seg# (=%d)\n", *Seq));
            return FALSE;
        }
    } 
    else 
    {
        DBGPRINT(RT_DEBUG_TRACE, ("APPeerAuthSanity fail - wrong algorithm (=%d)\n", *Alg));
        return FALSE;
    }
}

/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN APPeerProbeReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2,
    OUT CHAR Ssid[], 
    OUT UCHAR *SsidLen) 
{
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;
#ifdef WSC_AP_SUPPORT
    CHAR          *Ptr;
    UCHAR		eid =0, eid_len = 0, *eid_data;
    UCHAR         apidx = MAIN_MBSSID;
	UINT		WpsOui = 0, total_ie_len = 0;
#endif // WSC_AP_SUPPORT //

    // to prevent caller from using garbage output value
    *SsidLen = 0;

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);

    if (Fr->Octet[0] != IE_SSID || Fr->Octet[1] > MAX_LEN_OF_SSID) 
    {
        DBGPRINT(RT_DEBUG_TRACE, ("APPeerProbeReqSanity fail - wrong SSID IE\n"));
        return FALSE;
    } 
    
    *SsidLen = Fr->Octet[1];
    NdisMoveMemory(Ssid, &Fr->Octet[2], *SsidLen);

#ifdef WSC_AP_SUPPORT
    Ptr = Fr->Octet;
    eid = Ptr[0];
    eid_len = Ptr[1];
	total_ie_len = eid_len + 2;
	eid_data = Ptr+2;
    
    // get variable fields from payload and advance the pointer
	while((eid_data + eid_len) <= ((UCHAR*)Fr + MsgLen))
    {
        switch(eid)
        {
	        case IE_VENDOR_SPECIFIC:
				if (eid_len <= 4)
					break;
				NdisMoveMemory(&WpsOui, eid_data, sizeof(UINT));
                if (be2cpu32(WpsOui) == WSC_OUI)
                {
                    if (pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode != WSC_DISABLE)
    				{	
    					int bufLen = 0;
    					PUCHAR pBuf = NULL;
    					WSC_IE_PROBREQ_DATA	*pprobreq = NULL;
						WSC_IE				*pWscIE;
						PUCHAR				pData = NULL;
						UCHAR				Len = 0, i;
						CHAR				Index = 0;
						USHORT				DevicePasswordID;
						PWSC_CTRL			pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;

						pData = eid_data + 4;
						Len = eid_len - 4;
						while (Len > 0)
						{
							WSC_IE	WscIE;
							NdisMoveMemory(&WscIE, pData, sizeof(WSC_IE));
							// Check for WSC IEs
							pWscIE = &WscIE;

							// Check for device password ID, PBC = 0x0004
							if (be2cpu16(pWscIE->Type) == WSC_ID_DEVICE_PWD_ID)
							{
								// Found device password ID
								NdisMoveMemory(&DevicePasswordID, pData + 4, sizeof(DevicePasswordID));
								DevicePasswordID = be2cpu16(DevicePasswordID);
								DBGPRINT(RT_DEBUG_TRACE, ("APPeerProbeReqSanity : DevicePasswordID = 0x%04x\n", DevicePasswordID));
								if (DevicePasswordID == DEV_PASS_ID_PBC)	// Check for PBC value
								{
									if (pWscControl->WscStaPbcProbeInfo.WscPBCStaProbeCount < MAX_PBC_STA_TABLE_SIZE)
									{
										Index = 0;
										if (pWscControl->WscStaPbcProbeInfo.WscPBCStaProbeCount > 0)
										{
											for (i = 0; i < pWscControl->WscStaPbcProbeInfo.WscPBCStaProbeCount; i++)
											{
												if (NdisEqualMemory(Fr->Hdr.Addr2, pWscControl->WscStaPbcProbeInfo.StaMacAddr[Index], MAC_ADDR_LEN))
												{
													Index = -1;
													pWscControl->WscStaPbcProbeInfo.ReciveTime[Index] = jiffies;
													break;
												}
											}
										}
										if (Index == 0)
										{
											Index = pWscControl->WscStaPbcProbeInfo.WscPBCStaProbeCount;
											pWscControl->WscStaPbcProbeInfo.WscPBCStaProbeCount++;
										}
									}
									else
									{
										Index = -1;
										for (i = 0; i < MAX_PBC_STA_TABLE_SIZE; i++)
										{
											if (((jiffies - pWscControl->WscStaPbcProbeInfo.ReciveTime[i]) > 120*OS_HZ) &&
												(!NdisEqualMemory(Fr->Hdr.Addr2, pWscControl->WscStaPbcProbeInfo.StaMacAddr[i], MAC_ADDR_LEN)))
											{
												Index = i;
												break;
											}
										}										
									}
									if (Index >= 0)
									{
										pWscControl->WscStaPbcProbeInfo.ReciveTime[Index] = jiffies;
										NdisMoveMemory(pWscControl->WscStaPbcProbeInfo.StaMacAddr[Index], Fr->Hdr.Addr2, MAC_ADDR_LEN);
									}
									break;
								}
							}
							
							// Set the offset and look for PBC information
							// Since Type and Length are both short type, we need to offset 4, not 2
							pData += (be2cpu16(pWscIE->Length) + 4);
							Len   -= (be2cpu16(pWscIE->Length) + 4);
						}

						if ((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode & WSC_PROXY) != WSC_DISABLE)
						{

	    					bufLen = sizeof(WSC_IE_PROBREQ_DATA) + eid_len;
    						pBuf = kmalloc(bufLen, MEM_ALLOC_FLAG);
    						if(pBuf == NULL)
    							break;

                			//Send WSC probe req to UPnP
                    		memset(pBuf, 0 ,  bufLen);
    	                	pprobreq = (WSC_IE_PROBREQ_DATA*)pBuf;
    						if (32 >= *SsidLen)	//Well, I think that it must be TRUE!
    						{
    							NdisMoveMemory(pprobreq->ssid, Ssid, *SsidLen);			// SSID
    							NdisMoveMemory(pprobreq->macAddr, Fr->Hdr.Addr2, 6);	// Mac address
                        	    pprobreq->data[0] = 221; 									// element ID
	    	                    pprobreq->data[1] = eid_len;							// element Length
	    						NdisMoveMemory((pBuf+sizeof(WSC_IE_PROBREQ_DATA)), eid_data, eid_len);	// (WscProbeReqData)
    							WscSendUPnPMessage(pAd, WSC_OPCODE_UPNP_MGMT, WSC_UPNP_MGMT_SUB_PROBE_REQ, pBuf, bufLen, 0, 0, &Fr->Hdr.Addr2[0]);
                			}
                        	if (pBuf)
                        	    kfree(pBuf);
						}
                		break;
    				}
                    break;
                }                
            default:
            	break;
        }
		eid = Ptr[total_ie_len];
    	eid_len = Ptr[total_ie_len + 1];
		eid_data = Ptr + total_ie_len + 2;
		total_ie_len += (eid_len + 2);
	}
#endif // WSC_AP_SUPPORT //

    return TRUE;
}


/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN APPeerBeaconAndProbeRspSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT PUCHAR pBssid, 
    OUT CHAR Ssid[], 
    OUT UCHAR *SsidLen, 
    OUT UCHAR *BssType, 
    OUT USHORT *BeaconPeriod, 
    OUT UCHAR *Channel, 
    OUT LARGE_INTEGER *Timestamp, 
    OUT USHORT *CapabilityInfo, 
    OUT UCHAR Rate[], 
    OUT UCHAR *RateLen,
    OUT BOOLEAN *ExtendedRateIeExist,
    OUT UCHAR *Erp)
{
    CHAR                *Ptr;
    PFRAME_802_11       Fr;
    PEID_STRUCT         eid_ptr;
    UCHAR               SubType;
    UCHAR               Sanity;

    // to prevent caller from using garbage output value
    *RateLen = 0;
    *ExtendedRateIeExist = FALSE;
    *Erp = 0;
    
    // Add for 3 necessary EID field check
    Sanity = 0;
    
    Fr = (PFRAME_802_11)Msg;
    
    // get subtype from header
    SubType = (UCHAR)Fr->Hdr.FC.SubType;

    // get Addr2 and BSSID from header
    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);
    COPY_MAC_ADDR(pBssid, &Fr->Hdr.Addr3);
    
    Ptr = Fr->Octet;
    
    // get timestamp from payload and advance the pointer
    NdisMoveMemory(Timestamp, Ptr, TIMESTAMP_LEN);
    Ptr += TIMESTAMP_LEN;

    // get beacon interval from payload and advance the pointer
    NdisMoveMemory(BeaconPeriod, Ptr, 2);
    Ptr += 2;

    // get capability info from payload and advance the pointer
    NdisMoveMemory(CapabilityInfo, Ptr, 2);
    Ptr += 2;
    if (CAP_IS_ESS_ON(*CapabilityInfo)) 
    {
        *BssType = BSS_INFRA;
    } 
    else 
    {
        *BssType = BSS_ADHOC;
    }

    eid_ptr = (PEID_STRUCT) Ptr;

    // get variable fields from payload and advance the pointer
    while(((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((UCHAR*)Fr + MsgLen))
    {
        switch(eid_ptr->Eid)
        {
            case IE_SSID:
                if(eid_ptr->Len <= MAX_LEN_OF_SSID)
                {
                    NdisMoveMemory(Ssid, eid_ptr->Octet, eid_ptr->Len);
                    *SsidLen = eid_ptr->Len;
                    Sanity |= 0x1;
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("APPeerBeaconAndProbeRspSanity - wrong IE_SSID (len=%d)\n",eid_ptr->Len));
                    return FALSE;
                }
                break;

            case IE_SUPP_RATES:
                if(eid_ptr->Len <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(Rate, eid_ptr->Octet, eid_ptr->Len);
                    *RateLen = eid_ptr->Len;
                    Sanity |= 0x2;
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("APPeerBeaconAndProbeRspSanity - wrong IE_SUPP_RATES (len=%d)\n",eid_ptr->Len));
                    return FALSE;
                }
                break;

            case IE_DS_PARM:
                if(eid_ptr->Len == 1)
                {
                    *Channel = *eid_ptr->Octet;                    
                    Sanity |= 0x4;
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("APPeerBeaconAndProbeRspSanity - wrong IE_DS_PARM (len=%d)\n",eid_ptr->Len));
                    return FALSE;
                }
                break;

            case IE_FH_PARM:
            case IE_CF_PARM:
            case IE_IBSS_PARM:
            case IE_TIM:
            case IE_WPA:
                break;

            case IE_EXT_SUPP_RATES:
                // concatenate all extended rates to Rates[] and RateLen
                *ExtendedRateIeExist = TRUE;
                if (eid_ptr->Len + *RateLen <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(&Rate[*RateLen], eid_ptr->Octet, eid_ptr->Len);
                    *RateLen = (*RateLen) + eid_ptr->Len;
                }
                else
                {
                    NdisMoveMemory(&Rate[*RateLen], eid_ptr->Octet, MAX_LEN_OF_SUPPORTED_RATES - (*RateLen));
                    *RateLen = MAX_LEN_OF_SUPPORTED_RATES;
                }
                break;

            case IE_ERP:
                if (eid_ptr->Len == 1)
                {
                    *Erp = (UCHAR)eid_ptr->Octet[0];
                }
                break;
                
            default:
                DBGPRINT(RT_DEBUG_INFO, ("APPeerBeaconAndProbeRspSanity - unrecognized EID = %d\n", eid_ptr->Eid));
                break;
        }
        
        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);        
    }

	// For some 11a AP. it did not have the channel EID, patch here
	if (pAd->LatchRfRegs.Channel > 14)
	{
		*Channel = pAd->LatchRfRegs.Channel;					 
		Sanity |= 0x4;		
	}

	if ((Sanity&0x7) != 0x7)
    {
        DBGPRINT(RT_DEBUG_WARN, ("APPeerBeaconAndProbeRspSanity - missing mandatory field)\n"));
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

