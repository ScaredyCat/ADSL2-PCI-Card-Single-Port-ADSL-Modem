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
    wpa.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Jan Lee     03-07-22        Initial
    Rory Chen   04-11-29        Add WPA2PSK
*/
#include "rt_config.h"

extern UCHAR	EAPOL[];

VOID RTMPGetTxTscFromAsic(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			apidx,
	OUT	PUCHAR			pTxTsc);

BOOLEAN APWpaMsgTypeSubst(
    IN UCHAR    EAPType,
    OUT INT   *MsgType) 
{
    switch(EAPType)
    {
        case EAPPacket:
            *MsgType = APMT2_EAPPacket;
            break;
        case EAPOLStart:
            *MsgType = APMT2_EAPOLStart;
            break;
        case EAPOLLogoff:
            *MsgType = APMT2_EAPOLLogoff;
            break;
        case EAPOLKey:
            *MsgType = APMT2_EAPOLKey;
            break;
        case EAPOLASFAlert:
            *MsgType = APMT2_EAPOLASFAlert;
            break;
        default:
            DBGPRINT(RT_DEBUG_TRACE, ("APWpaMsgTypeSubst : return FALSE; \n"));    
            return FALSE;
    }
    
    return TRUE;
}

/*  
    ==========================================================================
    Description: 
        association state machine init, including state transition and timer init
    Parameters: 
        S - pointer to the association state machine
    ==========================================================================
 */
VOID APWpaStateMachineInit(
    IN  PRTMP_ADAPTER   pAd, 
    IN  STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]) 
{
    StateMachineInit(S, (STATE_MACHINE_FUNC *)Trans, AP_MAX_WPA_PTK_STATE, AP_MAX_WPA_MSG, (STATE_MACHINE_FUNC)Drop, AP_WPA_PTK, AP_WPA_MACHINE_BASE);

    StateMachineSetAction(S, AP_WPA_PTK, APMT2_EAPPacket, (STATE_MACHINE_FUNC)APWpaEAPPacketAction);
    StateMachineSetAction(S, AP_WPA_PTK, APMT2_EAPOLStart, (STATE_MACHINE_FUNC)APWpaEAPOLStartAction);
    StateMachineSetAction(S, AP_WPA_PTK, APMT2_EAPOLLogoff, (STATE_MACHINE_FUNC)APWpaEAPOLLogoffAction);
    StateMachineSetAction(S, AP_WPA_PTK, APMT2_EAPOLKey, (STATE_MACHINE_FUNC)APWpaEAPOLKeyAction);
    StateMachineSetAction(S, AP_WPA_PTK, APMT2_EAPOLASFAlert, (STATE_MACHINE_FUNC)APWpaEAPOLASFAlertAction);
}

/*
    ==========================================================================
    Description:
        this is state machine function. 
        When receiving EAP packets which is  for 802.1x authentication use. 
        Not use in PSK case
    Return:
    ==========================================================================
*/
VOID APWpaEAPPacketAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{   
}

VOID APWpaEAPOLASFAlertAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{   
}

VOID APWpaEAPOLLogoffAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{   
}

/*
    ==========================================================================
    Description:
        Port Access Control Inquiry function. Return entry's Privacy and Wpastate.
    Return:
        pEntry 
    ==========================================================================
*/
MAC_TABLE_ENTRY *PACInquiry(
    IN  PRTMP_ADAPTER               pAd, 
    IN  ULONG                       Wcid)
{

    MAC_TABLE_ENTRY *pEntry = (MAC_TABLE_ENTRY*)NULL;
    
//    *Privacy = Ndis802_11PrivFilterAcceptAll;
//    *WpaState = AS_NOTUSE;

#if 0
    if (MAC_ADDR_IS_GROUP(pAddr))
    {// mcast & broadcast address
    } 
    else
    {// unicast address
        pEntry = MacTableLookup(pAd, pAddr);
        if (pEntry) 
        {
            *Privacy = pEntry->PrivacyFilter;
            *WpaState = pEntry->WpaState;
        } 
    }
#else
	if (Wcid < MAX_LEN_OF_MAC_TABLE)
	{
		pEntry = &(pAd->MacTab.Content[Wcid]);

		//if (pAd->MacTab.Content[Wcid].ValidAsCLI)

		ASSERT(pAd->MacTab.Content[Wcid].ValidAsCLI);
		ASSERT(pEntry->Sst == SST_ASSOC);

//	    *Privacy = pEntry->PrivacyFilter;
//        *WpaState = pEntry->WpaState;
	}		 
#endif

    return pEntry;
}

/*
    ==========================================================================
    Description:
       Check sanity of multicast cipher selector in RSN IE.
    Return:
         TRUE if match
         FALSE otherwise
    ==========================================================================
*/
BOOLEAN RTMPCheckMcast(
    IN PRTMP_ADAPTER    pAd,
    IN PEID_STRUCT      eid_ptr,
    IN MAC_TABLE_ENTRY  *pEntry)
{
	UCHAR apidx;


	ASSERT(pEntry);
	ASSERT(pEntry->apidx < pAd->ApCfg.BssidNum);

	apidx = pEntry->apidx;

    pEntry->AuthMode = pAd->ApCfg.MBSSID[apidx].AuthMode;

    if (eid_ptr->Len >= 6)
    {
        // WPA and WPA2 format not the same in RSN_IE
        if (eid_ptr->Eid == IE_WPA)
        {
            if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2)
                pEntry->AuthMode = Ndis802_11AuthModeWPA;
            else if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
                pEntry->AuthMode = Ndis802_11AuthModeWPAPSK;

            if (NdisEqualMemory(&eid_ptr->Octet[6], &pAd->ApCfg.MBSSID[apidx].RSN_IE[0][6], 4))
                return TRUE;
        }
        else if (eid_ptr->Eid == IE_WPA2)
        {
            UCHAR   IE_Idx = 0;

            // When WPA1/WPA2 mix mode, the RSN_IE is stored in different structure
            if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || 
                (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
                IE_Idx = 1;
    
            if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2)
                pEntry->AuthMode = Ndis802_11AuthModeWPA2;
            else if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
                pEntry->AuthMode = Ndis802_11AuthModeWPA2PSK;

            if (NdisEqualMemory(&eid_ptr->Octet[2], &pAd->ApCfg.MBSSID[apidx].RSN_IE[IE_Idx][2], 4))
                return TRUE;
        }
    }

    DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckMcast ==> WPAIE = %d\n", eid_ptr->Eid));

    return FALSE;
}

/*
    ==========================================================================
    Description:
       Check sanity of unicast cipher selector in RSN IE.
    Return:
         TRUE if match
         FALSE otherwise
    ==========================================================================
*/
BOOLEAN RTMPCheckUcast(
    IN PRTMP_ADAPTER    pAd,
    IN PEID_STRUCT      eid_ptr,
    IN MAC_TABLE_ENTRY	*pEntry)
{
	PUCHAR 	pStaTmp;
	USHORT	Count;
	UCHAR 	apidx;

	ASSERT(pEntry);
	ASSERT(pEntry->apidx < pAd->ApCfg.BssidNum);

	apidx = pEntry->apidx;

	pEntry->WepStatus = pAd->ApCfg.MBSSID[apidx].WepStatus;
    
	if (eid_ptr->Len < 16)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckUcast ==> WPAIE len is too short(%d) \n", eid_ptr->Len));
	    return FALSE;
	}	

	// Store STA RSN_IE capability
	pStaTmp = &eid_ptr->Octet[0];
	if(eid_ptr->Eid == IE_WPA2)
	{
		// skip Version(2),Multicast cipter(4) 2+4==6
		// point to number of unicast
        pStaTmp +=6;
	}
	else if (eid_ptr->Eid == IE_WPA)	
	{
		// skip OUI(4),Vesrion(2),Multicast cipher(4) 4+2+4==10
		// point to number of unicast
        pStaTmp += 10;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckUcast ==> Unknown WPAIE, WPAIE=%d\n", eid_ptr->Eid));
	    return FALSE;
	}

	// Store unicast cipher count
    NdisMoveMemory(&Count, pStaTmp, sizeof(USHORT));
    Count = cpu2le16(Count);		


	// pointer to unicast cipher
    pStaTmp += sizeof(USHORT);	
			
    if (eid_ptr->Len >= 16)
    {
    	if (eid_ptr->Eid == IE_WPA)
    	{
    		if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption4Enabled)
			{// multiple cipher (TKIP/CCMP)

				while (Count > 0)
				{
					// TKIP
					if (RTMPEqualMemory(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[0][12], 4))
					{
						pEntry->WepStatus = Ndis802_11Encryption2Enabled;
						return TRUE;
					}
					// AES
					else if (RTMPEqualMemory(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[0][16], 4))
					{
						pEntry->WepStatus = Ndis802_11Encryption3Enabled;
						return TRUE;
					}
					pStaTmp += 4;
					Count--;
				}
    		}
    		else
    		{// single cipher
    			while (Count > 0)
    			{
    				if (RTMPEqualMemory(pStaTmp , &pAd->ApCfg.MBSSID[apidx].RSN_IE[0][12], 4))
		            	return TRUE;

					pStaTmp += 4;
					Count--;
				}
    		}
    	}
    	else if (eid_ptr->Eid == IE_WPA2)
    	{
    		UCHAR	IE_Idx = 0;

			// When WPA1/WPA2 mix mode, the RSN_IE is stored in different structure
			if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
				IE_Idx = 1;
	
			if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption4Enabled)
			{// multiple cipher (TKIP/CCMP)

				while (Count > 0)
    			{
    				// TKIP
					if (RTMPEqualMemory(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[IE_Idx][8], 4))
					{
						pEntry->WepStatus = Ndis802_11Encryption2Enabled;
						return TRUE;
					}
					// AES
					else if (RTMPEqualMemory(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[IE_Idx][12], 4))
					{
						pEntry->WepStatus = Ndis802_11Encryption3Enabled;
						return TRUE;
					}
					pStaTmp += 4;
					Count--;
				}
			}
			else
			{// single cipher
				while (Count > 0)
    			{
					if (RTMPEqualMemory(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[IE_Idx][8], 4))
						return TRUE;

					pStaTmp += 4;
					Count--;
				}
			}
    	}
    }

    DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckUcast ==> WPAIE parsing error, WPAIE=%d\n", eid_ptr->Eid));

    return FALSE;
}


/*
    ==========================================================================
    Description:
       Check invalidity of authentication method selection in RSN IE.
    Return:
         TRUE if match
         FALSE otherwise
    ==========================================================================
*/
BOOLEAN RTMPCheckAKM(PUCHAR sta_akm, PUCHAR ap_rsn_ie, INT iswpa2)
{
	PUCHAR pTmp;
	USHORT Count;

	pTmp = ap_rsn_ie;

	if(iswpa2)
    // skip Version(2),Multicast cipter(4) 2+4==6
        pTmp +=6;
    else	
    //skip OUI(4),Vesrion(2),Multicast cipher(4) 4+2+4==10
        pTmp += 10;//point to number of unicast
	    
    NdisMoveMemory(&Count, pTmp, sizeof(USHORT));	
    Count = cpu2le16(Count);		

    pTmp   += sizeof(USHORT);//pointer to unicast cipher

    // Skip all unicast cipher suite
    while (Count > 0)
    	{
		// Skip OUI
		pTmp += 4;
		Count--;
	}

	NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
    Count = cpu2le16(Count);		

    pTmp   += sizeof(USHORT);//pointer to AKM cipher
    while (Count > 0)
    {
		//rtmp_hexdump(RT_DEBUG_TRACE,"MBSS WPA_IE AKM ",pTmp,4);
		if(RTMPEqualMemory(sta_akm,pTmp,4))
		   return TRUE;
    	else
		{
			pTmp += 4;
			Count--;
		}
    }
    return FALSE;// do not match the AKM   	

}


/*
    ==========================================================================
    Description:
       Check sanity of authentication method selector in RSN IE.
    Return:
         TRUE if match
         FALSE otherwise
    ==========================================================================
*/
BOOLEAN RTMPCheckAUTH(
    IN PRTMP_ADAPTER    pAd,
    IN PEID_STRUCT      eid_ptr,
    IN MAC_TABLE_ENTRY	*pEntry)
{
	PUCHAR pStaTmp;
	USHORT Count;	
	UCHAR 	apidx;

	ASSERT(pEntry);
	ASSERT(pEntry->apidx < pAd->ApCfg.BssidNum);

	apidx = pEntry->apidx;

	if (eid_ptr->Len < 16)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckAUTH ==> WPAIE len is too short(%d) \n", eid_ptr->Len));
	    return FALSE;
	}	

	// Store STA RSN_IE capability
	pStaTmp = &eid_ptr->Octet[0];
	if(eid_ptr->Eid == IE_WPA2)
	{
		// skip Version(2),Multicast cipter(4) 2+4==6
		// point to number of unicast
        pStaTmp +=6;
	}
	else if (eid_ptr->Eid == IE_WPA)	
	{
		// skip OUI(4),Vesrion(2),Multicast cipher(4) 4+2+4==10
		// point to number of unicast
        pStaTmp += 10;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckAUTH ==> Unknown WPAIE, WPAIE=%d\n", eid_ptr->Eid));
	    return FALSE;
	}

	// Store unicast cipher count
    NdisMoveMemory(&Count, pStaTmp, sizeof(USHORT));
    Count = cpu2le16(Count);		

	// pointer to unicast cipher
    pStaTmp += sizeof(USHORT);	

    // Skip all unicast cipher suite
    while (Count > 0)
    {
		// Skip OUI
		pStaTmp += 4;
		Count--;
	}

	// Store AKM count
	NdisMoveMemory(&Count, pStaTmp, sizeof(USHORT));
    Count = cpu2le16(Count);		

	//pointer to AKM cipher
    pStaTmp += sizeof(USHORT);			

    if (eid_ptr->Len >= 16)
    {
    	if (eid_ptr->Eid == IE_WPA)
    	{
			while (Count > 0)
			{
				if (RTMPCheckAKM(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[0][0],0))
					return TRUE;
	
				pStaTmp += 4;
				Count--;
			}
    	}
    	else if (eid_ptr->Eid == IE_WPA2)
    	{
    		UCHAR	IE_Idx = 0;

			// When WPA1/WPA2 mix mode, the RSN_IE is stored in different structure
			if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2) || (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
				IE_Idx = 1;

			while (Count > 0)
			{			
				if (RTMPCheckAKM(pStaTmp, &pAd->ApCfg.MBSSID[apidx].RSN_IE[IE_Idx][0],1))
					return TRUE;
			
				pStaTmp += 4;
				Count--;
			}
    	}
    }

    DBGPRINT(RT_DEBUG_ERROR, ("RTMPCheckAUTH ==> WPAIE parsing error, WPAIE=%d\n", eid_ptr->Eid));

    return FALSE;
}

/*
    ==========================================================================
    Description:
       Start 4-way HS when rcv EAPOL_START which may create by our driver in assoc.c
    Return:
    ==========================================================================
*/
VOID APWpaEAPOLStartAction(
    IN PRTMP_ADAPTER    pAd, 
    IN MLME_QUEUE_ELEM  *Elem) 
{   
    MAC_TABLE_ENTRY     *pEntry;
    PHEADER_802_11      pHeader;
#ifdef WIN_NDIS	
    BOOLEAN             Cancelled;
#endif

    DBGPRINT(RT_DEBUG_TRACE, ("APWpaEAPOLStartAction ===> \n"));
    
    pHeader = (PHEADER_802_11)Elem->Msg;
    
    //For normaol PSK, we enqueue an EAPOL-Start command to trigger the process.
    if (Elem->MsgLen == 6)
        pEntry = MacTableLookup(pAd, Elem->Msg);
    else
    {
        pEntry = MacTableLookup(pAd, pHeader->Addr2);
#ifdef WSC_AP_SUPPORT
        /* 
            a WSC enabled AP must ignore EAPOL-Start frames received from clients that associated to 
            the AP with an RSN IE or SSN IE indicating a WPA2-PSK/WPA-PSK authentication method in 
            the assication request.  <<from page52 in Wi-Fi Simple Config Specification version 1.0g>>
        */
        if (pEntry && 
            (pEntry->apidx == MAIN_MBSSID) &&
            (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE) &&
            ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)))
        {
            DBGPRINT(RT_DEBUG_TRACE, ("WPS enabled AP: Ignore EAPOL-Start frames received from clients.\n"));
            return;
        }
#endif // WSC_AP_SUPPORT //
    }
    
    if (pEntry) 
    {
		DBGPRINT(RT_DEBUG_TRACE, (" PortSecured(%d), WpaState(%d), AuthMode(%d), PMKID_CacheIdx(%d) \n", pEntry->PortSecured, pEntry->WpaState, pEntry->AuthMode, pEntry->PMKID_CacheIdx));

        if ((pEntry->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
			&& (pEntry->WpaState < AS_PTKSTART)
            && ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK) || ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) && (pEntry->PMKID_CacheIdx != ENTRY_NOT_FOUND))))
        {
            pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
            pEntry->WpaState = AS_INITPSK;
            pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
            NdisZeroMemory(pEntry->R_Counter, sizeof(pEntry->R_Counter));
            pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;
            
            WPAStart4WayHS(pAd, pEntry, PEER_MSG1_RETRY_EXEC_INTV);
        }
    }
}

/*
    ==========================================================================
    Description:
        Function to handle countermeasures active attack.  Init 60-sec timer if necessary.
    Return:
    ==========================================================================
*/
VOID HandleCounterMeasure(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry) 
{
    INT         i;
    BOOLEAN     Cancelled;

    if (!pEntry)
        return;

	// Todo by AlbertY - Not support currently in ApClient-link
	if (pEntry->ValidAsApCli)
		return;

	// if entry not set key done, ignore this RX MIC ERROR
	if ((pEntry->WpaState < AS_PTKINITDONE) || (pEntry->GTKState != REKEY_ESTABLISHED))
		return;

	DBGPRINT(RT_DEBUG_TRACE, ("HandleCounterMeasure ===> \n"));

    // record which entry causes this MIC error, if this entry sends disauth/disassoc, AP doesn't need to log the CM
    pEntry->CMTimerRunning = TRUE;
    pAd->ApCfg.MICFailureCounter++;
    
	// send wireless event - for MIC error
	if (pAd->CommonCfg.bWirelessEvent)
		RTMPSendWirelessEvent(pAd, IW_MIC_ERROR_EVENT_FLAG, pEntry->Addr, 0, 0); 
	
    if (pAd->ApCfg.CMTimerRunning == TRUE)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("Receive CM Attack Twice within 60 seconds ====>>> \n"));
        
		// send wireless event - for counter measures
		if (pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_COUNTER_MEASURES_EVENT_FLAG, pEntry->Addr, 0, 0); 
		
        // renew GTK
		GenRandom(pAd, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid, pAd->ApCfg.MBSSID[pEntry->apidx].GNonce);
        ApLogEvent(pAd, pEntry->Addr, EVENT_COUNTER_M);

		// Cancel CounterMeasure Timer
#ifdef	WIN_NDIS
        NdisMCancelTimer(&pAd->ApCfg.CounterMeasureTimer, &Cancelled);
#else	
		RTMPCancelTimer(&pAd->ApCfg.CounterMeasureTimer, &Cancelled);
#endif
		pAd->ApCfg.CMTimerRunning = FALSE;

        for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
        {
            // happened twice within 60 sec,  AP SENDS disaccociate all associated STAs.  All STA's transition to State 2
            if (pAd->MacTab.Content[i].ValidAsCLI == TRUE)
            {
                DisAssocAction(pAd, &pAd->MacTab.Content[i], REASON_MIC_FAILURE);
            }
        }
        
        // Further,  ban all Class 3 DATA transportation for  a period 0f 60 sec
        // disallow new association , too
        pAd->ApCfg.BANClass3Data = TRUE;        

        // check how many entry left...  should be zero
        //pAd->ApCfg.MBSSID[pEntry->apidx].GKeyDoneStations = pAd->MacTab.Size;
        //DBGPRINT(RT_DEBUG_TRACE, ("GKeyDoneStations=%d \n", pAd->ApCfg.MBSSID[pEntry->apidx].GKeyDoneStations));
    }

#ifdef	WIN_NDIS
    NdisMSetTimer(&pAd->ApCfg.CounterMeasureTimer, 60*MLME_TASK_EXEC_INTV);
#else
	RTMPSetTimer(&pAd->ApCfg.CounterMeasureTimer, 60 * MLME_TASK_EXEC_INTV * MLME_TASK_EXEC_MULTIPLE);
#endif
    pAd->ApCfg.CMTimerRunning = TRUE;
    pAd->ApCfg.PrevaMICFailTime = pAd->ApCfg.aMICFailTime;
#ifdef	WIN_NDIS
    NdisGetCurrentSystemTime(&pAd->ApCfg.aMICFailTime);
#else
	RTMP_GetCurrentSystemTime(&pAd->ApCfg.aMICFailTime);
#endif
}

/*
    ==========================================================================
    Description:
        This is state machine function. 
        When receiving EAPOL packets which is  for 802.1x key management. 
        Use both in WPA, and WPAPSK case. 
        In this function, further dispatch to different functions according to the received packet.  3 categories are : 
          1.  normal 4-way pairwisekey and 2-way groupkey handshake
          2.  MIC error (Countermeasures attack)  report packet from STA.
          3.  Request for pairwise/group key update from STA
    Return:
    ==========================================================================
*/
VOID APWpaEAPOLKeyAction(
    IN PRTMP_ADAPTER    pAd, 
    IN MLME_QUEUE_ELEM  *Elem) 
{
#ifdef WIN_NDIS
    BOOLEAN             Cancelled;
#endif
	INT					i;
    MAC_TABLE_ENTRY     *pEntry;
    PHEADER_802_11      pHeader;
    PEAPOL_PACKET       pEapol_packet;
	UCHAR				apidx = 0;
	KEY_INFO			peerKeyInfo;

    DBGPRINT(RT_DEBUG_TRACE, ("APWpaEAPOLKeyAction ===>\n"));

    pHeader = (PHEADER_802_11)Elem->Msg;
    pEapol_packet = (PEAPOL_PACKET)&Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];

	NdisZeroMemory((PUCHAR)&peerKeyInfo, sizeof(peerKeyInfo));
	NdisMoveMemory((PUCHAR)&peerKeyInfo, (PUCHAR)&pEapol_packet->KeyDesc.KeyInfo, sizeof(KEY_INFO));

#if 1
	hex_dump("Received Eapol frame: ", (unsigned char *)pEapol_packet, (Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H));
#endif

	*((USHORT *)&peerKeyInfo) = cpu2le16(*((USHORT *)&peerKeyInfo));

    do
    {
        pEntry = MacTableLookup(pAd, pHeader->Addr2);

		if (!pEntry || ((!pEntry->ValidAsCLI) && (!pEntry->ValidAsApCli)))		
            break;

		if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
				break;		

			DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPoL-Key frame from STA %02X-%02X-%02X-%02X-%02X-%02X\n", PRINT_MAC(pEntry->Addr)));

        if (((pEapol_packet->ProVer != EAPOL_VER) && (pEapol_packet->ProVer != EAPOL_VER2)) || ((pEapol_packet->KeyDesc.Type != WPA1_KEY_DESC) && (pEapol_packet->KeyDesc.Type != WPA2_KEY_DESC)))
        {
            DBGPRINT(RT_DEBUG_ERROR, ("Key descripter does not match with WPA rule\n"));
            break;
        }

        if ((pEntry->WepStatus == Ndis802_11Encryption2Enabled) && (peerKeyInfo.KeyDescVer != DESC_TYPE_TKIP))
        {
            DBGPRINT(RT_DEBUG_ERROR, ("Key descripter version not match(TKIP) \n"));
            break;
        }
        else if ((pEntry->WepStatus == Ndis802_11Encryption3Enabled) && (peerKeyInfo.KeyDescVer != DESC_TYPE_AES))
        {
            DBGPRINT(RT_DEBUG_ERROR, ("Key descripter version not match(AES) \n"));
            break;
        }

        if ((pEntry->Sst == SST_ASSOC) && (pEntry->WpaState >= AS_INITPSK))
        {
#ifdef QOS_DLS_SUPPORT
	    	if ((pEntry->GTKState == REKEY_ESTABLISHED) && (peerKeyInfo.KeyMic) && (peerKeyInfo.Secure) && !(peerKeyInfo.Error))
    	    {
        		RTMPHandleSTAKey(pAd, pEntry, Elem);
	    	}
			else
#endif // QOS_DLS_SUPPORT //
			if ((peerKeyInfo.KeyMic) && (peerKeyInfo.Request) && (peerKeyInfo.Error))
            {
                // KEYMIC=1, REQUEST=1, ERROR=1
                DBGPRINT(RT_DEBUG_ERROR, ("MIC, REQUEST, ERROR  are all 1, active countermeasure \n"));
                RT28XX_HANDLE_COUNTER_MEASURE(pAd, pEntry);
            }
            else if ((peerKeyInfo.Secure) && !(peerKeyInfo.Request) && !(peerKeyInfo.Error))
            {
                // SECURE=1, REQUEST=0, ERROR=0
#ifdef APCLI_SUPPORT								
				if (pEntry->ValidAsApCli)
				{	
					// Process 1. the Pairwise-Msg3 of 4-way in WPA2 
					//		   2. the Group-Msg1 in WPA or WPA2

					if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
						ApCliPeerGroupMsg1Action(pAd, pEntry, Elem);
					else if (((pEntry->AuthMode == Ndis802_11AuthModeWPA2) || 
		      				(pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)) 
					    	 && (pEntry->WpaState >= AS_PTKINITDONE))
	                	ApCliPeerGroupMsg1Action(pAd, pEntry, Elem);
    	            else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
        	            ApCliPeerPairMsg3Action(pAd, pEntry, Elem);					
				}
				else
#endif // APCLI_SUPPORT //			
				{
					// Process 1. the Pairwise-Msg4 of 4-way in WPA2 
					//		   2. the Group-Msg2 in WPA or WPA2
					
                if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
                    PeerGroupMsg2Action(pAd, pEntry, &Elem->Msg[LENGTH_802_11], (Elem->MsgLen - LENGTH_802_11));
				else if (((pEntry->AuthMode == Ndis802_11AuthModeWPA2) || 
		      			(pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)) 
					     && (pEntry->WpaState >=AS_PTKINITDONE) && (pEntry->GTKState == REKEY_NEGOTIATING))
                	PeerGroupMsg2Action(pAd, pEntry, &Elem->Msg[LENGTH_802_11], (Elem->MsgLen - LENGTH_802_11));
                else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
                    	PeerPairMsg4Action(pAd, pEntry, Elem);
            }
            }
            else if (!(peerKeyInfo.Secure) && !(peerKeyInfo.Request) && 
					 !(peerKeyInfo.Error) && (peerKeyInfo.KeyType))
            {
            	// SECURE=0, REQUEST=0, ERROR=0, KeyType=1
#ifdef APCLI_SUPPORT								
				if (pEntry->ValidAsApCli)
				{
					// Process 1. the Pairwise-Msg1 of 4-way in WPA or WPA2 
					//		   2. the Pairwise-Msg3 in WPA
				
					if (peerKeyInfo.KeyMic == 0)
                    	ApCliPeerPairMsg1Action(pAd, pEntry, Elem);
	                else                	                	
    	                ApCliPeerPairMsg3Action(pAd, pEntry, Elem);	                
				}
				else
#endif // APCLI_SUPPORT //					
				{
                	if (pEntry->WpaState == AS_PTKSTART)
                	    PeerPairMsg2Action(pAd, pEntry, Elem);
                	else if (pEntry->WpaState == AS_PTKINIT_NEGOTIATING)
                	    PeerPairMsg4Action(pAd, pEntry, Elem);
            	}
            }
            else if ((peerKeyInfo.Request) && !(peerKeyInfo.Error))
            {
                // REQUEST=1, ERROR=0
#ifdef APCLI_SUPPORT		
				// The Authenticator shall never set this bit(Request bit).
				if (pEntry->ValidAsApCli)
					break;	// give up this frame
#endif // APCLI_SUPPORT //
				
				apidx = pEntry->apidx;	
				
                // Need to check KeyType for groupkey or pairwise key update, refer to 8021i P.114, 
                if (peerKeyInfo.KeyType == GROUPKEY)
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("REQUEST=1, ERROR=0, update group key\n"));
					
					GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GNonce);
					pAd->ApCfg.MBSSID[apidx].DefaultKeyId = (pAd->ApCfg.MBSSID[apidx].DefaultKeyId == 1) ? 2 : 1;   

					// Update group key to Asic
					{
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
                    
					for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
                    {
                    	if ((pAd->MacTab.Content[i].ValidAsCLI == TRUE) 
							&& (pAd->MacTab.Content[i].WpaState == AS_PTKINITDONE)
							&& (pAd->MacTab.Content[i].apidx == apidx))
                    	{
                        	pAd->MacTab.Content[i].GTKState = REKEY_NEGOTIATING;
                        	WPAStart2WayGroupHS(pAd, &pAd->MacTab.Content[i]);
							pAd->MacTab.Content[i].ReTryCounter = GROUP_MSG1_RETRY_TIMER_CTR;
#ifdef	WIN_NDIS
                        	NdisMCancelTimer(&pAd->MacTab.Content[i].RetryTimer, &Cancelled);
                        	NdisMSetTimer(&pAd->MacTab.Content[i].RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#else
							RTMPModTimer(&pAd->MacTab.Content[i].RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#endif
                    	}
                	}
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("REQUEST=1, ERROR= 0, update pairwise key\n"));
                    
					NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));  
					
					// clear this entry as no-security mode
					AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, pEntry->Aid); 

                    pEntry->Sst = SST_ASSOC;
                    if (pEntry->AuthMode == Ndis802_11AuthModeWPA || pEntry->AuthMode == Ndis802_11AuthModeWPA2)
                        pEntry->WpaState = AS_INITPMK;  
                    else if (pEntry->AuthMode == Ndis802_11AuthModeWPAPSK || pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)
                        pEntry->WpaState = AS_INITPSK;  
					
                    pEntry->GTKState = REKEY_NEGOTIATING;
					pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
                    pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;
					pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
					NdisZeroMemory(pEntry->R_Counter, sizeof(pEntry->R_Counter));
            
		            WPAStart4WayHS(pAd, pEntry, PEER_MSG1_RETRY_EXEC_INTV);
                }
            }
            else
            {
                //
            }
        }
    }while(FALSE);
}

/*
    ==========================================================================
    Description:
        This is a function to initilize 4-way handshake
    Return:
         
    ==========================================================================
*/
VOID WPAStart4WayHS(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN ULONG			TimeInterval) 
{
    UCHAR           Header802_3[14];
    EAPOL_PACKET 	EAPOLPKT;
	UCHAR			apidx;
#ifdef WIN_NDIS
	BOOLEAN         Cancelled;
#endif

    DBGPRINT(RT_DEBUG_TRACE, ("===> WPAStart4WayHS\n"));

    do
    {       
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS | fRTMP_ADAPTER_HALT_IN_PROGRESS))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("!!!!!!WPAStart4WayHS : The interface is closed...\n"));
			break;		
		}
	
        if ((!pEntry) || (!pEntry->ValidAsCLI))
            break;
            
		if (pEntry->apidx >= pAd->ApCfg.BssidNum)
			break;
		else
			apidx = pEntry->apidx;
            
        if ((pEntry->WpaState > AS_PTKSTART) || (pEntry->WpaState < AS_INITPMK))
        {
            DBGPRINT(RT_DEBUG_ERROR, ("Not expect calling  WPAStart4WayHS \n"));
            break;
        }
        
#ifdef WSC_AP_SUPPORT
        if (MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[apidx].WscControl.EntryAddr) &&
            pAd->ApCfg.MBSSID[apidx].WscControl.EapMsgRunning)
        {
            pEntry->WpaState = AS_NOTUSE;
	        DBGPRINT(RT_DEBUG_ERROR, ("This is a WSC-Enrollee. Not expect calling WPAStart4WayHS here \n"));
	        break;
        }
#endif // WSC_AP_SUPPORT //
        
		// Increment replay counter by 1
		ADD_ONE_To_64BIT_VAR(pEntry->R_Counter);
        
		// Randomly generate ANonce
		GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pEntry->ANonce); 		

		// Construct EAPoL message - Pairwise Msg 1
		NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
		ConstructEapolMsg(pAd,
						  pEntry->AuthMode,
						  pEntry->WepStatus, 
						  pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus, 
						  EAPOL_PAIR_MSG_1,
						  0,					// Default key index
						  pEntry->R_Counter,
						  pEntry->ANonce,
						  NULL,					// TxRSC
						  NULL, 				// PTK
						  NULL,					// GTK
						  NULL,					// RSNIE
						  0,					// RSNIE length	
						  &EAPOLPKT);

		// If PMKID match in WPA2-enterprise mode, fill PMKID into Key data field and update PMK here	
		if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) && (pEntry->PMKID_CacheIdx != ENTRY_NOT_FOUND))
		{
				// Fill in value for KDE 
			EAPOLPKT.KeyDesc.KeyData[0] = 0xDD;
	        EAPOLPKT.KeyDesc.KeyData[2] = 0x00;
	        EAPOLPKT.KeyDesc.KeyData[3] = 0x0F;
	        EAPOLPKT.KeyDesc.KeyData[4] = 0xAC;
	        EAPOLPKT.KeyDesc.KeyData[5] = 0x04;

			NdisMoveMemory(&EAPOLPKT.KeyDesc.KeyData[6], &pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[pEntry->PMKID_CacheIdx].PMKID, LEN_PMKID);
        	NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMK, &pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[pEntry->PMKID_CacheIdx].PMK, PMK_LEN);
        		
        	EAPOLPKT.KeyDesc.KeyData[1] = 0x14;// 4+LEN_PMKID
        	EAPOLPKT.KeyDesc.KeyDataLen[1] += 6 + LEN_PMKID;
        	EAPOLPKT.Body_Len[1] += 6 + LEN_PMKID;			
		}
			
		// Make outgoing frame
        MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.MBSSID[apidx].Bssid, EAPOL);            
        APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, TRUE);

		// Trigger Retry Timer
#ifdef WIN_NDIS
        NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
        NdisMSetTimer(&pEntry->RetryTimer, TimeInterval);
#else
        RTMPModTimer(&pEntry->RetryTimer, TimeInterval);		
#endif

		// Update State
        pEntry->WpaState = AS_PTKSTART;
    
    DBGPRINT(RT_DEBUG_TRACE, ("<=== WPAStart4WayHS, WpaState= %d \n", pEntry->WpaState));
    }while(FALSE);
        
}

/*
    ==========================================================================
    Description:
        When receiving the second packet of 4-way pairwisekey handshake.
    Return:
    ==========================================================================
*/
VOID PeerPairMsg2Action(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{   
    UCHAR				SNonce[LEN_KEY_DESC_NONCE];
	UCHAR				PTK[80];
	UCHAR				RSNIE[MAX_LEN_OF_RSNIE];
	UCHAR				RSNIE_Len;	
    BOOLEAN             Cancelled;
    PHEADER_802_11      pHeader;
	EAPOL_PACKET        EAPOLPKT;	
	PEAPOL_PACKET       pMsg2;
	UINT            	MsgLen;
    UCHAR               Header802_3[LENGTH_802_3];
	UCHAR 				TxTsc[6];
	UCHAR				apidx;

    DBGPRINT(RT_DEBUG_TRACE, ("PeerPairMsg2Action Start...\n"));

    if ((!pEntry) || (!pEntry->ValidAsCLI))
        return;
        
    if (Elem->MsgLen < (LENGTH_802_11 + LENGTH_802_1_H + LENGTH_EAPOL_H + sizeof(KEY_DESCRIPTER) - MAX_LEN_OF_RSNIE - 2))
        return;

    // check Entry in valid State
    if (pEntry->WpaState < AS_PTKSTART)
        return;

	if (pEntry->apidx >= pAd->ApCfg.BssidNum)
        return;
	else
	apidx = pEntry->apidx;

    // pointer to 802.11 header
	pHeader = (PHEADER_802_11)Elem->Msg;

	// skip 802.11_header(24-byte) and LLC_header(8) 
	pMsg2 = (PEAPOL_PACKET)&Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];       
	MsgLen = Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H;

	// store the received EAPoL packet
	NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
    NdisMoveMemory(&EAPOLPKT, pMsg2, MsgLen);
        
	// Derive PTK
	NdisMoveMemory(SNonce, pMsg2->KeyDesc.KeyNonce, LEN_KEY_DESC_NONCE);
    
	WpaCountPTK(pAd, 
				pAd->ApCfg.MBSSID[apidx].PMK,  
				pEntry->ANonce, 
				pAd->ApCfg.MBSSID[apidx].Bssid, 
				SNonce, 
				pEntry->Addr, 
				PTK, 
				LEN_PTK); 		

    NdisMoveMemory(pEntry->PTK, PTK, LEN_PTK);

	// Sanity Check peer Pairwise message 2 - Replay Counter, MIC, RSNIE
	if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, MsgLen, EAPOL_PAIR_MSG_2, pMsg2->KeyDesc.KeyMic, pEntry))
		return;

    do
    {
        // delete retry timer
#ifdef	WIN_NDIS
        NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
#else
		RTMPCancelTimer(&pEntry->RetryTimer, &Cancelled);
#endif
		// Change state
        pEntry->WpaState = AS_PTKINIT_NEGOTIATING;

		// Increment replay counter by 1
		ADD_ONE_To_64BIT_VAR(pEntry->R_Counter);
          
		// Specify corresponding RSNIE   	    	
		NdisZeroMemory(RSNIE, MAX_LEN_OF_RSNIE);
		RSNIE_Len = 0;
        if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
        {
			RSNIE_Len = pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0];
            NdisMoveMemory(RSNIE, &pAd->ApCfg.MBSSID[apidx].RSN_IE[0], RSNIE_Len);	
        }
        else
        {
            if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK) ||
				(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA1WPA2)) 
            {
                RSNIE_Len = pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1];
                NdisMoveMemory(RSNIE, pAd->ApCfg.MBSSID[apidx].RSN_IE[1], RSNIE_Len);
            }
            else
            {
                RSNIE_Len = pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0];
                NdisMoveMemory(RSNIE, &pAd->ApCfg.MBSSID[apidx].RSN_IE[0], RSNIE_Len);
            }
        }

		// Get Group TxTsc form Asic
		RTMPGetTxTscFromAsic(pAd, apidx, TxTsc);

		// Construct EAPoL message - Pairwise Msg 3
		NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
		ConstructEapolMsg(pAd,
						  pEntry->AuthMode,
						  pEntry->WepStatus, 
						  pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus,
						  EAPOL_PAIR_MSG_3,
						  pAd->ApCfg.MBSSID[apidx].DefaultKeyId,
						  pEntry->R_Counter,
						  pEntry->ANonce,
						  TxTsc,
						  pEntry->PTK,
						  pAd->ApCfg.MBSSID[apidx].GTK,
						  RSNIE,
						  RSNIE_Len,
						  &EAPOLPKT);
            
        // Make outgoing frame
        MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.MBSSID[apidx].Bssid, EAPOL);
            
        APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, TRUE);

        pEntry->ReTryCounter = PEER_MSG3_RETRY_TIMER_CTR;
#ifdef	WIN_NDIS
        NdisMSetTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#else
		RTMPSetTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#endif
        
		// Update State
        pEntry->WpaState = AS_PTKINIT_NEGOTIATING;
    }while(FALSE);

	DBGPRINT(RT_DEBUG_TRACE, ("PeerPairMsg2Action End.\n"));
}

/*
    ==========================================================================
    Description:
        When receiving the last packet of 4-way pairwisekey handshake.
        Initilize 2-way groupkey handshake following.
    Return:
    ==========================================================================
*/
VOID PeerPairMsg4Action(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{
    UCHAR           	PMK_key[20], digest[80];
	PEAPOL_PACKET   	pMsg4;    
    PHEADER_802_11      pHeader;
    EAPOL_PACKET        EAPOLPKT;
    UINT            	MsgLen;
    BOOLEAN             Cancelled;

    DBGPRINT(RT_DEBUG_TRACE, ("===> PeerPairMsg4Action\n"));

    do
    {
        if ((!pEntry) || (!pEntry->ValidAsCLI))
            break;
		
        if (Elem->MsgLen < (LENGTH_802_11 + LENGTH_802_1_H + LENGTH_EAPOL_H + sizeof(KEY_DESCRIPTER) - MAX_LEN_OF_RSNIE - 2 ) )
            break;

        if (pEntry->WpaState < AS_PTKINIT_NEGOTIATING)
            break;

        // pointer to 802.11 header
        pHeader = (PHEADER_802_11)Elem->Msg;

		// skip 802.11_header(24-byte) and LLC_header(8) 
		pMsg4 = (PEAPOL_PACKET)&Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H]; 
		MsgLen = Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H;

		// store the received EAPoL packet
		NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
    	NdisMoveMemory(&EAPOLPKT, pMsg4, MsgLen);

        // Sanity Check peer Pairwise message 4 - Replay Counter, MIC
		if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, MsgLen, EAPOL_PAIR_MSG_4, pMsg4->KeyDesc.KeyMic, pEntry))
			break;

        // 3. uses the MLME.SETKEYS.request to configure PTK into MAC
        NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));   

		// reset IVEIV in Asic 
		AsicUpdateWCIDIVEIV(pAd, pEntry->Aid, 0, 0); 

        pEntry->PairwiseKey.KeyLen = LEN_TKIP_EK;
        NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[32], LEN_TKIP_EK);
        NdisMoveMemory(pEntry->PairwiseKey.RxMic, &pEntry->PTK[TKIP_AP_RXMICK_OFFSET], LEN_TKIP_RXMICK);
        NdisMoveMemory(pEntry->PairwiseKey.TxMic, &pEntry->PTK[TKIP_AP_TXMICK_OFFSET], LEN_TKIP_TXMICK);

		// Set pairwise key to Asic
        {
            pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
            if (pEntry->WepStatus == Ndis802_11Encryption2Enabled)
                pEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
            else if (pEntry->WepStatus == Ndis802_11Encryption3Enabled)
                pEntry->PairwiseKey.CipherAlg = CIPHER_AES;

			// Add Pair-wise key to Asic
            AsicAddPairwiseKeyEntry(
                pAd, 
                pEntry->Addr, 
                (UCHAR)pEntry->Aid, 
                &pEntry->PairwiseKey);

			// update WCID attribute table and IVEIV table for this entry
			RTMPAddWcidAttributeEntry(
				pAd, 
				pEntry->apidx, 
				0, 
				pEntry->PairwiseKey.CipherAlg,
				pEntry);
        }
        
        // 4. upgrade state
        pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
        pEntry->WpaState = AS_PTKINITDONE;
		pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
        pAd->ApCfg.MBSSID[pEntry->apidx].PortSecured = WPA_802_1X_PORT_SECURED;
        ApLogEvent(pAd, pEntry->Addr, EVENT_ASSOCIATED);  // 2005-02-14 log association only after 802.1x successful

#ifdef WSC_AP_SUPPORT
        if (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE)
            WscInformFromWPA(pEntry);
#endif // WSC_AP_SUPPORT //

		if (pEntry->AuthMode == Ndis802_11AuthModeWPA2 || pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK)
		{
			pEntry->GTKState = REKEY_ESTABLISHED;

#ifdef	WIN_NDIS        
        	NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
#else
			RTMPCancelTimer(&pEntry->RetryTimer, &Cancelled);
#endif

			if (pEntry->AuthMode == Ndis802_11AuthModeWPA2)
        	{
				// Calculate PMKID, refer to IEEE 802.11i-2004 8.5.1.2
				NdisMoveMemory(&PMK_key[0], "PMK Name", 8);
				NdisMoveMemory(&PMK_key[8], pAd->ApCfg.MBSSID[pEntry->apidx].Bssid, MAC_ADDR_LEN);
				NdisMoveMemory(&PMK_key[14], pEntry->Addr, MAC_ADDR_LEN);
				HMAC_SHA1(PMK_key, 20, pAd->ApCfg.MBSSID[pEntry->apidx].PMK, PMK_LEN, digest);
				RTMPAddPMKIDCache(pAd, pEntry->apidx, pEntry->Addr, digest, pAd->ApCfg.MBSSID[pEntry->apidx].PMK);
				DBGPRINT(RT_DEBUG_TRACE, ("Calc PMKID=%02x:%02x:%02x:%02x:%02x:%02x\n", digest[0],digest[1],digest[2],digest[3],digest[4],digest[5]));
        	}

			// send wireless event - for set key done WPA2
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_SET_KEY_DONE_WPA2_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 
	 
	        DBGPRINT(RT_DEBUG_TRACE, ("AP SETKEYS DONE - WPA2, AuthMode(%d)=%s, WepStatus(%d)=%s, GroupWepStatus(%d)=%s\n\n", pEntry->AuthMode, GetAuthMode(pEntry->AuthMode), pEntry->WepStatus, GetEncryptType(pEntry->WepStatus), pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus, GetEncryptType(pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus)));

		}
		else
		{

        	// 5. init Group 2-way handshake if necessary.
	        WPAStart2WayGroupHS(pAd, pEntry);

        	pEntry->ReTryCounter = GROUP_MSG1_RETRY_TIMER_CTR;
#ifdef	WIN_NDIS
        	NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
        	NdisMSetTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#else
			RTMPModTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#endif
		}
    }while(FALSE);
    
}


/*
    ==========================================================================
    Description:
        This is a function to send the first packet of 2-way groupkey handshake
    Return:
         
    ==========================================================================
*/
VOID WPAStart2WayGroupHS(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry) 
{
    UCHAR               Header802_3[14];
	UCHAR   			TxTsc[6]; 
    EAPOL_PACKET    EAPOLPKT;
	UCHAR				apidx;
    
	DBGPRINT(RT_DEBUG_TRACE, ("===> WPAStart2WayGroupHS\n"));

    if ((!pEntry) || (!pEntry->ValidAsCLI))
        return;
            
	if (pEntry->apidx >= pAd->ApCfg.BssidNum)
		return;
    else
		apidx = pEntry->apidx;	

    do
    {
        // Increment replay counter by 1
		ADD_ONE_To_64BIT_VAR(pEntry->R_Counter);
		
		// Get Group TxTsc form Asic
		RTMPGetTxTscFromAsic(pAd, apidx, TxTsc);

		// Construct EAPoL message - Group Msg 1
		NdisZeroMemory(&EAPOLPKT, sizeof(EAPOL_PACKET));
		ConstructEapolMsg(pAd,
						  pEntry->AuthMode,
						  pEntry->WepStatus, 
						  pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus, 
						  EAPOL_GROUP_MSG_1,
						  pAd->ApCfg.MBSSID[apidx].DefaultKeyId,
						  pEntry->R_Counter,
						  pAd->ApCfg.MBSSID[apidx].GNonce,
						  TxTsc,
						  pEntry->PTK,
						  pAd->ApCfg.MBSSID[apidx].GTK,
						  NULL,
						  0,
						  &EAPOLPKT);

		// Make outgoing frame
        MAKE_802_3_HEADER(Header802_3, pEntry->Addr, pAd->ApCfg.MBSSID[apidx].Bssid, EAPOL);            
        APToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&EAPOLPKT, EAPOLPKT.Body_Len[1] + 4, FALSE);



    }while (FALSE);

    DBGPRINT(RT_DEBUG_TRACE, ("Send out Group Message 1 \n"));
        
    return;
}
     

/*
    ==========================================================================
    Description:
        When receiving the last packet of 2-way groupkey handshake.
    Return:
    ==========================================================================
*/
VOID PeerGroupMsg2Action(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN VOID             *Msg,
    IN UINT             MsgLen) 
{
    UINT            Len;
    PUCHAR          pData;
    BOOLEAN         Cancelled;
	PEAPOL_PACKET       pMsg2;
    EAPOL_PACKET        EAPOLPKT;

    do
    {
        if ((!pEntry) || (!pEntry->ValidAsCLI))
            break;
            
        if (MsgLen < (LENGTH_802_1_H + LENGTH_EAPOL_H + sizeof(KEY_DESCRIPTER) - MAX_LEN_OF_RSNIE - 2))
            break;
            
        if (pEntry->WpaState != AS_PTKINITDONE)
            break;

        DBGPRINT(RT_DEBUG_TRACE, ("PeerGroupMsg2Action ====> %x %x %x %x %x %x \n",pEntry->Addr[0], pEntry->Addr[1],
            pEntry->Addr[2],pEntry->Addr[3],pEntry->Addr[4],pEntry->Addr[5]));

        
        pData = (PUCHAR)Msg;
		pMsg2 = (PEAPOL_PACKET) (pData + LENGTH_802_1_H);
        Len = MsgLen - LENGTH_802_1_H;

		// store the received EAPoL packet
		NdisZeroMemory((PUCHAR)&EAPOLPKT, sizeof(EAPOLPKT));
		NdisMoveMemory(&EAPOLPKT, pMsg2, Len);  

		// Sanity Check peer group message 2 - Replay Counter, MIC
		if (!PeerWpaMessageSanity(pAd, &EAPOLPKT, Len, EAPOL_GROUP_MSG_2, pMsg2->KeyDesc.KeyMic, pEntry))
            break;

        // 3.  upgrade state
#ifdef	WIN_NDIS
        NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
#else
		RTMPCancelTimer(&pEntry->RetryTimer, &Cancelled);
#endif
        pEntry->GTKState = REKEY_ESTABLISHED;
        
		if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
		{
			// send wireless event - for set key done WPA2
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_SET_KEY_DONE_WPA2_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 

			DBGPRINT(RT_DEBUG_TRACE, ("AP SETKEYS DONE - WPA2, AuthMode(%d)=%s, WepStatus(%d)=%s, GroupWepStatus(%d)=%s\n\n", pEntry->AuthMode, GetAuthMode(pEntry->AuthMode), pEntry->WepStatus, GetEncryptType(pEntry->WepStatus), pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus, GetEncryptType(pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus)));
		}
		else
		{
			// send wireless event - for set key done WPA
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_SET_KEY_DONE_WPA1_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 

        	DBGPRINT(RT_DEBUG_TRACE, ("AP SETKEYS DONE - WPA1, AuthMode(%d)=%s, WepStatus(%d)=%s, GroupWepStatus(%d)=%s\n\n", pEntry->AuthMode, GetAuthMode(pEntry->AuthMode), pEntry->WepStatus, GetEncryptType(pEntry->WepStatus), pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus, GetEncryptType(pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus)));
		}	
    }while(FALSE);  
}


/*
    ==========================================================================
    Description:
        countermeasures active attack timer execution
    Return:
    ==========================================================================
*/
VOID CMTimerExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
    UINT            i,j=0;
    PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER)FunctionContext;
        
    pAd->ApCfg.BANClass3Data = FALSE;
    for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
    {
        if ((pAd->MacTab.Content[i].ValidAsCLI == TRUE) && (pAd->MacTab.Content[i].CMTimerRunning == TRUE))
        {
            pAd->MacTab.Content[i].CMTimerRunning =FALSE;
            j++;
        }
    }
    if (j > 1)
        DBGPRINT(RT_DEBUG_ERROR, ("Find more than one entry which generated MIC Fail ..  \n"));

    pAd->ApCfg.CMTimerRunning = FALSE;
}
    
VOID EnqueueStartForPSKExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
	MAC_TABLE_ENTRY     *pEntry = (PMAC_TABLE_ENTRY) FunctionContext;

	if ((pEntry) && (pEntry->ValidAsCLI == TRUE))
	{
		PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pEntry->pAd;

		switch (pEntry->EnqueueEapolStartTimerRunning)
		{
			case EAPOL_START_PSK:								
				DBGPRINT(RT_DEBUG_TRACE, ("Enqueue EAPoL-Start-PSK for sta(%02x:%02x:%02x:%02x:%02x:%02x) \n", PRINT_MAC(pEntry->Addr)));

				MlmeEnqueue(pAd, AP_WPA_STATE_MACHINE, APMT2_EAPOLStart, 6, &pEntry->Addr);		
				break;

			case EAPOL_START_1X:							
				DBGPRINT(RT_DEBUG_TRACE, ("Enqueue EAPoL-Start-1X for sta(%02x:%02x:%02x:%02x:%02x:%02x) \n", PRINT_MAC(pEntry->Addr)));

				IEEE8021X_L2_Trigger_Frame_Send(pAd, pEntry);						
				break;

			default:
				break;
			
		}

		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_DISABLE;
		
	}				
}

	
VOID WPARetryExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
    MAC_TABLE_ENTRY     *pEntry = (MAC_TABLE_ENTRY *)FunctionContext;

    if ((pEntry) && (pEntry->ValidAsCLI == TRUE))
    {
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pEntry->pAd;
        
        pEntry->ReTryCounter++;
        DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec---> ReTryCounter=%d, WpaState=%d \n", pEntry->ReTryCounter, pEntry->WpaState));

        switch (pEntry->AuthMode)
        {
			case Ndis802_11AuthModeWPA:
            case Ndis802_11AuthModeWPAPSK:
			case Ndis802_11AuthModeWPA2:
            case Ndis802_11AuthModeWPA2PSK:
				// 1. GTK already retried, give up and disconnect client.
                if (pEntry->ReTryCounter > (GROUP_MSG1_RETRY_TIMER_CTR + 1))
                {    
                	// send wireless event - for group key handshaking timeout
					if (pAd->CommonCfg.bWirelessEvent)
						RTMPSendWirelessEvent(pAd, IW_GROUP_HS_TIMEOUT_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 
					
                    DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec::Group Key HS exceed retry count, Disassociate client, pEntry->ReTryCounter %d\n", pEntry->ReTryCounter));
                    DisAssocAction(pAd, pEntry, REASON_GROUP_KEY_HS_TIMEOUT);
                }
				// 2. Retry GTK.
                else if (pEntry->ReTryCounter > GROUP_MSG1_RETRY_TIMER_CTR)
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec::ReTry 2-way group-key Handshake \n"));
                    if (pEntry->GTKState == REKEY_NEGOTIATING)
                    {
                        WPAStart2WayGroupHS(pAd, pEntry);
#ifdef	WIN_NDIS
                        NdisMSetTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#else
						RTMPSetTimer(&pEntry->RetryTimer, PEER_MSG3_RETRY_EXEC_INTV);
#endif
                    }
                }
				// 3. 4-way message 1 retried more than three times. Disconnect client
                else if (pEntry->ReTryCounter > (PEER_MSG1_RETRY_TIMER_CTR + 3))
                {
					// send wireless event - for pairwise key handshaking timeout
					if (pAd->CommonCfg.bWirelessEvent)
						RTMPSendWirelessEvent(pAd, IW_PAIRWISE_HS_TIMEOUT_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0);

                    DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec::MSG1 timeout, pEntry->ReTryCounter = %d\n", pEntry->ReTryCounter));
                    DisAssocAction(pAd, pEntry, REASON_4_WAY_TIMEOUT);
                }
				// 4. Retry 4 way message 1, the last try, the timeout is 3 sec for EAPOL-Start
                else if (pEntry->ReTryCounter == (PEER_MSG1_RETRY_TIMER_CTR + 3))                
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec::Retry MSG1, the last try\n"));
                    WPAStart4WayHS(pAd , pEntry, PEER_MSG3_RETRY_EXEC_INTV);
                }
				// 4. Retry 4 way message 1
                else if (pEntry->ReTryCounter < (PEER_MSG1_RETRY_TIMER_CTR + 3))
                {
                    if ((pEntry->WpaState == AS_PTKSTART) || (pEntry->WpaState == AS_INITPSK) || (pEntry->WpaState == AS_INITPMK))
                    {
                        DBGPRINT(RT_DEBUG_TRACE, ("WPARetryExec::ReTry MSG1 of 4-way Handshake\n"));
                        WPAStart4WayHS(pAd, pEntry, PEER_MSG1_RETRY_EXEC_INTV);
                    }
                }
                break;

            default:
                break;
        }
    }
}

#if 0 	// replaced by WPAStart2WayGroupHS
/*
    ==========================================================================
    Description:
        Only for sending the first packet of 2-way groupkey handshake
    Return:
    ==========================================================================
*/
NDIS_STATUS APWpaHardTransmit(
    IN PRTMP_ADAPTER    pAd,
    IN MAC_TABLE_ENTRY  *pEntry)

#endif // 0 //

VOID DisAssocAction(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN USHORT           Reason)
{
    PUCHAR          pOutBuffer = NULL;
    ULONG           FrameLen = 0;
    HEADER_802_11   DisassocHdr;
    NDIS_STATUS     NStatus;

    if (pEntry)
    {
        //  send out a DISASSOC request frame
        NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
        if (NStatus != NDIS_STATUS_SUCCESS)
            return;

		// send wireless event - for send disassication 
		if (pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 

        DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - Send DISASSOC Reason = %d frame to %x %x %x %x %x %x \n",Reason,pEntry->Addr[0],
            pEntry->Addr[1],pEntry->Addr[2],pEntry->Addr[3],pEntry->Addr[4],pEntry->Addr[5]));
        
        MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pEntry->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);
        MakeOutgoingFrame(pOutBuffer,               &FrameLen, 
                          sizeof(HEADER_802_11),    &DisassocHdr,
                          2,                        &Reason,
                          END_OF_ARGS);
        MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
        MlmeFreeMemory(pAd, pOutBuffer);
    
        // ApLogEvent(pAd, pEntry->Addr, EVENT_DISASSOCIATED);
        MacTableDeleteEntry(pAd, pEntry->Aid, pEntry->Addr);
    }
}

/*
    ==========================================================================
    Description:
        Timer execution function for periodically updating group key.
    Return:
    ==========================================================================
*/  
VOID GREKEYPeriodicExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
    UINT            i;
    ULONG           temp_counter = 0;
    //BOOLEAN         Cancelled;  
    PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER)FunctionContext;
	UCHAR			apidx;

        
    DBGPRINT(RT_DEBUG_INFO, ("GROUP REKEY PeriodicExec ==>> \n"));
    
	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
    {
		if (pAd->ApCfg.MBSSID[apidx].AuthMode < Ndis802_11AuthModeWPA)
		{
			continue;
    }
    
    if ((pAd->ApCfg.WPAREKEY.ReKeyMethod == TIME_REKEY) && (pAd->ApCfg.REKEYCOUNTER < 0xffffffff))
        temp_counter = (++pAd->ApCfg.REKEYCOUNTER);
    // REKEYCOUNTER is incremented every MCAST packets transmitted, 
    // But the unit of Rekeyinterval is 1K packets
    else if (pAd->ApCfg.WPAREKEY.ReKeyMethod == PKT_REKEY)
        temp_counter = pAd->ApCfg.REKEYCOUNTER/1000;
    else
    {
			continue;
    }
    
    if (temp_counter > (pAd->ApCfg.WPAREKEY.ReKeyInterval))
    {
        pAd->ApCfg.REKEYCOUNTER = 0;
        DBGPRINT(RT_DEBUG_TRACE, ("Rekey Interval Excess, GKeyDoneStations=%d\n", pAd->MacTab.Size));
        
        // take turn updating different groupkey index, 
		if ((pAd->MacTab.Size) > 0)
        {
				USHORT	Wcid;

				// change key index
				pAd->ApCfg.MBSSID[apidx].DefaultKeyId = (pAd->ApCfg.MBSSID[apidx].DefaultKeyId == 1) ? 2 : 1;         
	
				// Generate GNonce randomly
				GenRandom(pAd, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].GNonce);

				// initialize IVEIV in Asic			  
				GET_GroupKey_WCID(Wcid, apidx);
				AsicUpdateWCIDIVEIV(pAd, Wcid, 0, 0);
				NdisZeroMemory(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].TxTsc, sizeof(pAd->SharedKey[apidx][pAd->ApCfg.MBSSID[apidx].DefaultKeyId].TxTsc));
				
				// Count GTK
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

				// Process 2-way handshaking
            for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
            {
					MAC_TABLE_ENTRY  *pEntry;

					pEntry = &pAd->MacTab.Content[i];
					if ((pEntry->ValidAsCLI == TRUE) && (pEntry->WpaState == AS_PTKINITDONE) &&
						(pEntry->apidx == apidx))
                {
						pEntry->GTKState = REKEY_NEGOTIATING;
						
                    	WPAStart2WayGroupHS(pAd, &pAd->MacTab.Content[i]);
                    DBGPRINT(RT_DEBUG_TRACE, ("Rekey interval excess, Update Group Key for  %x %x %x  %x %x %x , DefaultKeyId= %x \n",\
												  pEntry->Addr[0],pEntry->Addr[1],\
												  pEntry->Addr[2],pEntry->Addr[3],\
												  pEntry->Addr[4],pEntry->Addr[5],\
												  pAd->ApCfg.MBSSID[apidx].DefaultKeyId));
					}
                }
            }
        }
    }
}

VOID CountGTK(
    IN  UCHAR   *GMK,
    IN  UCHAR   *GNonce,
    IN  UCHAR   *AA,
    OUT UCHAR   *output,
    IN  UINT    len)
{
    UCHAR   concatenation[76];
    UINT    CurrPos=0;
    UCHAR   Prefix[19];
    UCHAR   temp[80];   

    NdisMoveMemory(&concatenation[CurrPos], AA, 6);
    CurrPos += 6;

    NdisMoveMemory(&concatenation[CurrPos], GNonce , 32);
    CurrPos += 32;

    Prefix[0] = 'G';
    Prefix[1] = 'r';
    Prefix[2] = 'o';
    Prefix[3] = 'u';
    Prefix[4] = 'p';
    Prefix[5] = ' ';
    Prefix[6] = 'k';
    Prefix[7] = 'e';
    Prefix[8] = 'y';
    Prefix[9] = ' ';
    Prefix[10] = 'e';
    Prefix[11] = 'x';
    Prefix[12] = 'p';
    Prefix[13] = 'a';
    Prefix[14] = 'n';
    Prefix[15] = 's';
    Prefix[16] = 'i';
    Prefix[17] = 'o';
    Prefix[18] = 'n';

    PRF(GMK, PMK_LEN, Prefix,  19, concatenation, 38 , temp, len);
    NdisMoveMemory(output, temp, len);
}

/*
    ========================================================================

    Routine Description:
        Sending EAP Req. frame to station in authenticating state.
        These frames come from Authenticator deamon.

    Arguments:
        pAdapter        Pointer to our adapter
        pPacket     Pointer to outgoing EAP frame body + 8023 Header
        Len             length of pPacket
        
    Return Value:
        None
    ========================================================================
*/
VOID WpaSend(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  PUCHAR          pPacket,
    IN  ULONG           Len)
{
    PEAP_HDR        	pEapHdr;
    UCHAR         		Addr[MAC_ADDR_LEN];
	UCHAR				Header802_3[LENGTH_802_3];
    MAC_TABLE_ENTRY 	*pEntry;
	PUCHAR				pData;
    
    DBGPRINT(RT_DEBUG_INFO, ("WpaSend ==> Len=%ld\n",Len));

    NdisMoveMemory(Addr, pPacket, 6);
	NdisMoveMemory(Header802_3, pPacket, LENGTH_802_3);
    pEapHdr = (EAP_HDR*)(pPacket + LENGTH_802_3);
	pData = (pPacket + LENGTH_802_3);
	
    if ((pEntry = MacTableLookup(pAdapter, Addr)) == NULL)
    {	
		DBGPRINT(RT_DEBUG_INFO, ("WpaSend - No such MAC - %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(Addr)));
		return;
    }

	// Send EAP frame to STA
    if (((pEntry->AuthMode >= Ndis802_11AuthModeWPA) && (pEapHdr->ProType != EAPOLKey)) ||
        (pAdapter->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))
		APToWirelessSta(pAdapter, pEntry, Header802_3, LENGTH_802_3, pData, Len - LENGTH_802_3, TRUE);		
	
	// After receiving EAP_SUCCESS, trigger state machine
    if (RTMPEqualMemory((pPacket+12), EAPOL, 2))
    {
        switch (pEapHdr->code)
        {
            case EAP_CODE_SUCCESS:
                if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) && (pEapHdr->ProType != EAPOLKey))
                {
                    DBGPRINT(RT_DEBUG_TRACE,("Send EAP_CODE_SUCCESS\n\n"));
                    if (pEntry->Sst == SST_ASSOC)
                    {
                        pEntry->WpaState = AS_INITPMK;
						// Only set the expire and counters	                    
	                    pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;
	                    WPAStart4WayHS(pAdapter, pEntry, PEER_MSG1_RETRY_EXEC_INTV);
#if 0						
#ifdef WIN_NDIS
			            NdisMCancelTimer(&pEntry->RetryTimer, &Cancelled);
            			NdisMSetTimer(&pEntry->RetryTimer, PEER_MSG1_RETRY_EXEC_INTV);
#else
			            RTMPModTimer(&pEntry->RetryTimer, PEER_MSG1_RETRY_EXEC_INTV);
#endif
#endif
                    }
                }
                else
                {
                    pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
                    pEntry->WpaState = AS_PTKINITDONE;
                    pAdapter->ApCfg.MBSSID[pEntry->apidx].PortSecured = WPA_802_1X_PORT_SECURED;
                    pEntry->PortSecured = WPA_802_1X_PORT_SECURED;
#ifdef WSC_AP_SUPPORT
                    if (pAdapter->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE)
                        WscInformFromWPA(pEntry);
#endif // WSC_AP_SUPPORT //
                    DBGPRINT(RT_DEBUG_TRACE,("IEEE8021X-WEP : Send EAP_CODE_SUCCESS\n\n"));
                }
                break;

            case EAP_CODE_FAILURE:
                break;

            default:
                break;    
        }
    }
    else     
    {
        DBGPRINT(RT_DEBUG_TRACE, ("Send Deauth, Reason : REASON_NO_LONGER_VALID\n"));
        DisAssocAction(pAdapter, pEntry, REASON_NO_LONGER_VALID);
    }
}    

VOID    APToWirelessSta(
    IN  PRTMP_ADAPTER   pAd,
    IN  MAC_TABLE_ENTRY  	*pEntry,
    IN  PUCHAR          pHeader802_3,
    IN  UINT            HdrLen,
    IN  PUCHAR          pData,
    IN  UINT            	DataLen,
    IN	BOOLEAN				bClearFrame)
{
    PNDIS_PACKET    pPacket;
    NDIS_STATUS     Status;

	if ((!pEntry) || ((!pEntry->ValidAsCLI) && (!pEntry->ValidAsApCli)))
		return;
	
    do {
        // build a NDIS packet
        Status = RTMPAllocateNdisPacket(pAd, &pPacket, pHeader802_3, HdrLen, pData, DataLen);
        if (Status != NDIS_STATUS_SUCCESS)
            break;

        DBGPRINT(RT_DEBUG_INFO,("APToWirelessSta - len=%d, AID=%d, DA=%02x:%02x:%02x:%02x:%02x:%02x\n", 
            HdrLen+DataLen, pEntry->Aid, pHeader802_3[0],pHeader802_3[1],pHeader802_3[2],pHeader802_3[3],pHeader802_3[4],pHeader802_3[5]));

			if (bClearFrame)
				RTMP_SET_PACKET_CLEAR_EAP_FRAME(pPacket, 1);
			else
				RTMP_SET_PACKET_CLEAR_EAP_FRAME(pPacket, 0);	
		
#ifdef APCLI_SUPPORT
		if (pEntry->ValidAsApCli)
		{				
			RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);
			RTMP_SET_PACKET_MOREDATA(pPacket, FALSE);
			RTMP_SET_PACKET_NET_DEVICE_APCLI(pPacket, pEntry->MatchAPCLITabIdx);
			RTMP_SET_PACKET_WCID(pPacket, pEntry->Aid); // to ApClient links.
		}
		else
#endif // APCLI_SUPPORT //
		{
			RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);

			RTMP_SET_PACKET_NET_DEVICE_MBSSID(pPacket, MAIN_MBSSID);	// set a default value
			if(pEntry->apidx != 0)
        		RTMP_SET_PACKET_NET_DEVICE_MBSSID(pPacket, pEntry->apidx);
		
        	RTMP_SET_PACKET_WCID(pPacket, (UCHAR)pEntry->Aid);
			RTMP_SET_PACKET_MOREDATA(pPacket, FALSE);
		}

        // send out the packet
        APSendPacket(pAd, pPacket); // send to one of TX queue
        RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS);     // Dequeue outgoing frames from TxSwQueue0..3 queue and process it

    } while (FALSE);
}


VOID RTMPAddPMKIDCache(
	IN  PRTMP_ADAPTER   		pAd,
	IN	INT						apidx,
	IN	PUCHAR				pAddr,
	IN	UCHAR					*PMKID,
	IN	UCHAR					*PMK)
{
	INT	i, chcheidx;

	// Update PMKID status
	if ((chcheidx = RTMPSearchPMKIDCache(pAd, apidx, pAddr)) != -1)
	{
		pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[chcheidx].RefreshTime = jiffies;
		NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[chcheidx].PMKID, PMKID, LEN_PMKID);
		NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[chcheidx].PMK, PMK, PMK_LEN);
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddPMKIDCache update %02x:%02x:%02x:%02x:%02x:%02x cache(%d) from IF(ra%d)\n", 
           	pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], chcheidx, apidx));
		
		return;
	}

	// Add a new PMKID
	for (i = 0; i < MAX_PMKID_COUNT; i++)
	{
		if (!pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].Valid)
		{
			pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].Valid = TRUE;
			pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].RefreshTime = jiffies;
			COPY_MAC_ADDR(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].MAC, pAddr);
			NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].PMKID, PMKID, LEN_PMKID);
			NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].PMK, PMK, PMK_LEN);
			DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddPMKIDCache add %02x:%02x:%02x:%02x:%02x:%02x cache(%d) from IF(ra%d)\n", 
            	pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], i, apidx));
			break;
		}
	}
 
	if (i == MAX_PMKID_COUNT)
	{
		ULONG	timestamp = 0, idx = 0;

		DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddPMKIDCache(IF(%d) Cache full\n", apidx));
		for (i = 0; i < MAX_PMKID_COUNT; i++)
		{
			if (pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].Valid)
			{
				if (((timestamp == 0) && (idx == 0)) || ((timestamp != 0) && timestamp < pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].RefreshTime))
				{
					timestamp = pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].RefreshTime;
					idx = i;
				}
			}
		}
		pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].Valid = TRUE;
		pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].RefreshTime = jiffies;
		COPY_MAC_ADDR(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].MAC, pAddr);
		NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].PMKID, PMKID, LEN_PMKID);
		NdisMoveMemory(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].PMK, PMK, PMK_LEN);
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddPMKIDCache add %02x:%02x:%02x:%02x:%02x:%02x cache(%ld) from IF(ra%d)\n", 
           	pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], idx, apidx));
	}
}

INT RTMPSearchPMKIDCache(
	IN  PRTMP_ADAPTER   pAd,
	IN	INT				apidx,
	IN	PUCHAR		pAddr)
{
	INT	i = 0;
	
	for (i = 0; i < MAX_PMKID_COUNT; i++)
	{
		if ((pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].Valid)
			&& MAC_ADDR_EQUAL(&pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[i].MAC, pAddr))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("RTMPSearchPMKIDCache %02x:%02x:%02x:%02x:%02x:%02x cache(%d) from IF(ra%d)\n", 
            	pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], i, apidx));
			break;
		}
	}

	if (i == MAX_PMKID_COUNT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPSearchPMKIDCache - IF(%d) not found\n", apidx));
		return -1;
	}

	return i;
}

VOID RTMPDeletePMKIDCache(
	IN  PRTMP_ADAPTER   pAd,
	IN	INT				apidx,
	IN  INT				idx)
{
	if (pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].Valid)
	{
		pAd->ApCfg.MBSSID[apidx].PMKIDCache.BSSIDInfo[idx].Valid = FALSE;
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPDeletePMKIDCache(IF(%d), del PMKID CacheIdx=%d\n", apidx, idx));
	}
}

VOID RTMPMaintainPMKIDCache(
	IN  PRTMP_ADAPTER   pAd)
{
	INT	i, j;
	
	for (i = 0; i < MAX_MBSSID_NUM; i++)
	{
		for (j = 0; j < MAX_PMKID_COUNT; j++)
		{
			if ((pAd->ApCfg.MBSSID[i].PMKIDCache.BSSIDInfo[j].Valid)
				&& ((jiffies - pAd->ApCfg.MBSSID[i].PMKIDCache.BSSIDInfo[j].RefreshTime) >= pAd->ApCfg.PMKCachePeriod))
			{
				RTMPDeletePMKIDCache(pAd, i, j);
			}
		}
	}
}

VOID RTMPGetTxTscFromAsic(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			apidx,
	OUT	PUCHAR			pTxTsc)
{
	USHORT			Wcid;
	USHORT			offset;
	UCHAR			IvEiv[8];
	int				i;

	// Get apidx for this BSSID
	GET_GroupKey_WCID(Wcid, apidx);	

	// Read IVEIV from Asic
	offset = MAC_IVEIV_TABLE_BASE + (Wcid * HW_IVEIV_ENTRY_SIZE);
	NdisZeroMemory(IvEiv, 8);
	NdisZeroMemory(pTxTsc, 6);
			
	for (i=0 ; i < 8; i++)
		RTMP_IO_READ8(pAd, offset+i, &IvEiv[i]); 

	// Record current TxTsc	
	if (pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
	{	// AES				
		*pTxTsc 	= IvEiv[0];
		*(pTxTsc+1) = IvEiv[1];
		*(pTxTsc+2) = IvEiv[4];
		*(pTxTsc+3) = IvEiv[5];
		*(pTxTsc+4) = IvEiv[6];
		*(pTxTsc+5) = IvEiv[7];					
	}
	else
	{	// TKIP
		*pTxTsc 	= IvEiv[2];
		*(pTxTsc+1) = IvEiv[0];
		*(pTxTsc+2) = IvEiv[4];
		*(pTxTsc+3) = IvEiv[5];
		*(pTxTsc+4) = IvEiv[6];
		*(pTxTsc+5) = IvEiv[7];	
	}
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPGetTxTscFromAsic : WCID(%d) TxTsc 0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x \n", 
									Wcid, *pTxTsc, *(pTxTsc+1), *(pTxTsc+2), *(pTxTsc+3), *(pTxTsc+4), *(pTxTsc+5)));
			

}

#if 0
VOID GetSmall(
    IN  PVOID   pSrc1,
    IN  PVOID   pSrc2,
    OUT PUCHAR  pOut,
    IN  ULONG   Length)
{
    PUCHAR  pMem1;
    PUCHAR  pMem2;
    ULONG   Index = 0;
    pMem1 = (PUCHAR) pSrc1;
    pMem2 = (PUCHAR) pSrc2;

    for (Index = 0; Index < Length; Index++)
    {
        if (pMem1[Index] != pMem2[Index])
        {
            if (pMem1[Index] > pMem2[Index])        
                NdisMoveMemory(pOut, pSrc2, Length);
            else
                NdisMoveMemory(pOut, pSrc1, Length);             

            break;
        }
    }
}

VOID GetLarge(
    IN  PVOID   pSrc1,
    IN  PVOID   pSrc2,
    OUT PUCHAR  pOut,
    IN  ULONG   Length)
{
    PUCHAR  pMem1;
    PUCHAR  pMem2;
    ULONG   Index = 0;
    pMem1 = (PUCHAR) pSrc1;
    pMem2 = (PUCHAR) pSrc2;

    for (Index = 0; Index < Length; Index++)
    {
        if (pMem1[Index] != pMem2[Index])
        {
            if (pMem1[Index] > pMem2[Index])        
                NdisMoveMemory(pOut, pSrc1, Length);
            else
                NdisMoveMemory(pOut, pSrc2, Length);             

            break;
        }
    }
}
VOID CountPTK(
    IN UCHAR    *PMK,
    IN UCHAR    *ANonce,
    IN UCHAR    *AA,
    IN UCHAR    *SNonce,
    IN UCHAR    *SA,
    OUT UCHAR   *output,
    IN UINT     len)
{   
    UCHAR   concatenation[76];
    UINT    CurrPos=0;
    UCHAR   temp[32];
    UCHAR   Prefix[] = {'P', 'a', 'i', 'r', 'w', 'i', 's', 'e', ' ', 'k', 'e', 'y', ' ', 
                        'e', 'x', 'p', 'a', 'n', 's', 'i', 'o', 'n'};

    NdisZeroMemory(temp, sizeof(temp));

    GetSmall(SA, AA, temp, 6);
    NdisMoveMemory(concatenation, temp, 6);
    CurrPos += 6;

    GetLarge(SA, AA, temp, 6);
    NdisMoveMemory(&concatenation[CurrPos], temp, 6);
    CurrPos += 6;

    GetSmall(ANonce, SNonce, temp, 32);
    NdisMoveMemory(&concatenation[CurrPos], temp, 32);
    CurrPos += 32;

    GetLarge(ANonce, SNonce, temp, 32);
    NdisMoveMemory(&concatenation[CurrPos], temp, 32);
    CurrPos += 32;
    
	hex_dump("concatenation=", concatenation, 76);
	
    PRF(PMK, PMK_LEN, Prefix, 22, concatenation, 76 , output, len);
}
#endif // 0 //

#ifdef QOS_DLS_SUPPORT
VOID RTMPHandleSTAKey(
    IN PRTMP_ADAPTER    pAd, 
    IN PMAC_TABLE_ENTRY	pEntry,
    IN MLME_QUEUE_ELEM  *Elem) 
{
	extern UCHAR		OUI_WPA2_WEP40[];
	ULONG				FrameLen = 0;
	PUCHAR				pOutBuffer = NULL;
	UCHAR				Header802_3[14];
	EAPOL_PACKET		Packet, EAPOLPKT;
	PEAPOL_PACKET		pSTAKey;
	PHEADER_802_11		pHeader;
	UCHAR				Offset = 0;
	ULONG				MICMsgLen;
	UCHAR				DA[MAC_ADDR_LEN];
	UCHAR				Key_Data[512];
	UCHAR				key_length;
	UCHAR				mic[LEN_KEY_DESC_MIC];
	UCHAR				digest[80];
	UCHAR				temp[64];
	PMAC_TABLE_ENTRY	pDaEntry;

    DBGPRINT(RT_DEBUG_TRACE, ("==> RTMPHandleSTAKey\n"));

    if (!pEntry)
		return;
	
	if ((pEntry->WpaState != AS_PTKINITDONE))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("Not expect calling STAKey hand shaking here"));
        return;
    }

    pHeader = (PHEADER_802_11) Elem->Msg;

	// QoS control field (2B) is took off
//    if (pHeader->FC.SubType & 0x08)
//        Offset += 2;
    
    pSTAKey = (PEAPOL_PACKET)&Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H + Offset];	

    // Check Replay Counter
    if (!RTMPEqualMemory(pSTAKey->KeyDesc.ReplayCounter, pEntry->R_Counter, LEN_KEY_DESC_REPLAY))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("Replay Counter Different in STAKey handshake!! \n"));
        DBGPRINT(RT_DEBUG_ERROR, ("Receive : %d %d %d %d  \n",
				pSTAKey->KeyDesc.ReplayCounter[0],
				pSTAKey->KeyDesc.ReplayCounter[1],
				pSTAKey->KeyDesc.ReplayCounter[2],
				pSTAKey->KeyDesc.ReplayCounter[3]));
        DBGPRINT(RT_DEBUG_ERROR, ("Current : %d %d %d %d  \n",
				pEntry->R_Counter[4],pEntry->R_Counter[5],
				pEntry->R_Counter[6],pEntry->R_Counter[7]));
        return;
    }

    // Check MIC, if not valid, discard silently
    NdisMoveMemory(DA, &pSTAKey->KeyDesc.KeyData[6], MAC_ADDR_LEN);
	if (pSTAKey->KeyDesc.KeyInfo.KeyMic && pSTAKey->KeyDesc.KeyInfo.Secure && pSTAKey->KeyDesc.KeyInfo.Request)
	{
		pEntry->bDlsInit = TRUE;
		DBGPRINT(RT_DEBUG_TRACE, ("STAKey Initiator: %02x:%02x:%02x:%02x:%02x:%02x\n",
			pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2], pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]));
	}
    MICMsgLen = pSTAKey->Body_Len[1] | ((pSTAKey->Body_Len[0]<<8) && 0xff00);
    MICMsgLen += LENGTH_EAPOL_H;
    if (MICMsgLen > (Elem->MsgLen - LENGTH_802_11 - LENGTH_802_1_H))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("Receive wrong format EAPOL packets \n"));
        return;        
    }

	// This is proprietary DLS protocol, it will be adhered when spec. is finished.
	NdisZeroMemory(temp, 64);
	NdisZeroMemory(pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, sizeof(pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK));
	NdisMoveMemory(temp, "IEEE802.11 WIRELESS ACCESS POINT", 32);

	WpaCountPTK(pAd, temp, temp, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid, temp,
				pEntry->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_PTK);
	DBGPRINT(RT_DEBUG_TRACE, ("PTK-%x %x %x %x %x %x %x %x \n",
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[0],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[1],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[2],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[3],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[4],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[5],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[6],
			pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[7]));

    NdisMoveMemory((PUCHAR)&EAPOLPKT, pSTAKey, MICMsgLen);
    NdisZeroMemory(EAPOLPKT.KeyDesc.KeyMic, LEN_KEY_DESC_MIC);
    if (pEntry->WepStatus == Ndis802_11Encryption2Enabled)
    {
        hmac_md5(pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_EAP_MICK, (PUCHAR)&EAPOLPKT, MICMsgLen, mic);
    }
    else
    {
        HMAC_SHA1((PUCHAR)&EAPOLPKT,  MICMsgLen, pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_EAP_MICK, mic);
    }
    if (!RTMPEqualMemory(pSTAKey->KeyDesc.KeyMic, mic, LEN_KEY_DESC_MIC))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("MIC Different in STAKey handshake!! \n"));
        return;
    }
    else
        DBGPRINT(RT_DEBUG_TRACE, ("MIC VALID in STAKey handshake!! \n"));

	// Receive init STA's STAKey Message-2, and terminate the handshake
	if (pEntry->bDlsInit && !pSTAKey->KeyDesc.KeyInfo.Request)
	{
		pEntry->bDlsInit = FALSE;
		DBGPRINT(RT_DEBUG_TRACE, ("Receive init STA's STAKey Message-2, STAKey handshake finished \n"));
		return;
	}

	// Receive init STA's STAKey Message-2, and terminate the handshake
	if (RTMPEqualMemory(&pSTAKey->KeyDesc.KeyData[2], OUI_WPA2_WEP40, 3))
	{
		DBGPRINT(RT_DEBUG_WARN, ("Receive a STAKey message which not support currently, just drop it \n"));
		return;
	}
	
    do
    {
    	pDaEntry = MacTableLookup(pAd, DA);
    	if (!pDaEntry)
    		break;

    	if ((pDaEntry->WpaState != AS_PTKINITDONE))
	    {
	        DBGPRINT(RT_DEBUG_ERROR, ("Not expect calling STAKey hand shaking here \n"));
	        break;
	    }
    	
        pOutBuffer = kmalloc(MAX_LEN_OF_EAP_HS, MEM_ALLOC_FLAG);
        if(pOutBuffer == NULL)
            break;

        MAKE_802_3_HEADER(Header802_3, pDaEntry->Addr, pAd->ApCfg.MBSSID[pDaEntry->apidx].Bssid, EAPOL);

        // Increment replay counter by 1  
        ADD_ONE_To_64BIT_VAR(pDaEntry->R_Counter);

        // 0. init Packet and Fill header
        NdisZeroMemory(&Packet, sizeof(Packet));

        Packet.ProVer = EAPOL_VER;
        Packet.ProType = EAPOLKey;
        Packet.Body_Len[1] = 0x5f;
        
        // 1. Fill replay counter
//        NdisMoveMemory(pDaEntry->R_Counter, pAd->ApCfg.R_Counter, sizeof(pDaEntry->R_Counter));
        NdisMoveMemory(Packet.KeyDesc.ReplayCounter, pDaEntry->R_Counter, sizeof(pDaEntry->R_Counter));
        
        // 2. Fill key version, keyinfo, key len
        Packet.KeyDesc.KeyInfo.KeyDescVer= GROUP_KEY;
        Packet.KeyDesc.KeyInfo.KeyType	= GROUPKEY;
        Packet.KeyDesc.KeyInfo.Install	= 1;
        Packet.KeyDesc.KeyInfo.KeyAck	= 1;
        Packet.KeyDesc.KeyInfo.KeyMic	= 1;
        Packet.KeyDesc.KeyInfo.Secure	= 1;
        Packet.KeyDesc.KeyInfo.EKD_DL	= 1;
		DBGPRINT(RT_DEBUG_TRACE, ("STAKey handshake for peer STA %02x:%02x:%02x:%02x:%02x:%02x\n",
			DA[0], DA[1], DA[2], DA[3], DA[4], DA[5]));
        
        if ((pDaEntry->AuthMode == Ndis802_11AuthModeWPA) || (pDaEntry->AuthMode == Ndis802_11AuthModeWPAPSK))
        {
        	Packet.KeyDesc.Type = WPA1_KEY_DESC;

        	DBGPRINT(RT_DEBUG_TRACE, ("pDaEntry->AuthMode == Ndis802_11AuthModeWPA/WPAPSK\n"));
        }
        else if ((pDaEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pDaEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
        {
        	Packet.KeyDesc.Type = WPA2_KEY_DESC;
        	Packet.KeyDesc.KeyDataLen[1] = 0;

        	DBGPRINT(RT_DEBUG_TRACE, ("pDaEntry->AuthMode == Ndis802_11AuthModeWPA2/WPA2PSK\n"));
        }

        Packet.KeyDesc.KeyLength[1] = LEN_TKIP_KEY;
        Packet.KeyDesc.KeyDataLen[1] = LEN_TKIP_KEY;
        Packet.KeyDesc.KeyInfo.KeyDescVer = DESC_TYPE_TKIP;
        if (pDaEntry->WepStatus == Ndis802_11Encryption3Enabled)
        {
            Packet.KeyDesc.KeyLength[1] = LEN_AES_KEY;
            Packet.KeyDesc.KeyDataLen[1] = LEN_AES_KEY;
            Packet.KeyDesc.KeyInfo.KeyDescVer = DESC_TYPE_AES;
        }

		// Key Data Encapsulation format, use Ralink OUI to distinguish proprietary and standard.
    	Key_Data[0] = 0xDD;
		Key_Data[1] = 0x00;		// Length (This field will be filled later)
    	Key_Data[2] = 0x00;		// OUI
    	Key_Data[3] = 0x0C;		// OUI
    	Key_Data[4] = 0x43;		// OUI
    	Key_Data[5] = 0x02;		// Data Type (STAKey Key Data Encryption)

		// STAKey Data Encapsulation format
    	Key_Data[6] = 0x00;		//Reserved
		Key_Data[7] = 0x00;		//Reserved

		// STAKey MAC address
		NdisMoveMemory(&Key_Data[8], pEntry->Addr, MAC_ADDR_LEN);		// initiator MAC address

		// STAKey (Handle the difference between TKIP and AES-CCMP)
		if (pDaEntry->WepStatus == Ndis802_11Encryption3Enabled)
        {
        	Key_Data[1] = 0x1E;	// 4+2+6+16(OUI+Reserved+STAKey_MAC_Addr+STAKey)
        	NdisMoveMemory(&Key_Data[14], pEntry->PairwiseKey.Key, LEN_AES_KEY);
		}
		else
		{
			Key_Data[1] = 0x2E;	// 4+2+6+32(OUI+Reserved+STAKey_MAC_Addr+STAKey)
			NdisMoveMemory(&Key_Data[14], pEntry->PairwiseKey.Key, LEN_TKIP_EK);
			NdisMoveMemory(&Key_Data[14+LEN_TKIP_EK], pEntry->PairwiseKey.TxMic, LEN_TKIP_TXMICK);
			NdisMoveMemory(&Key_Data[14+LEN_TKIP_EK+LEN_TKIP_TXMICK], pEntry->PairwiseKey.RxMic, LEN_TKIP_RXMICK);
		}

		key_length = Key_Data[1];
		Packet.Body_Len[1] = key_length + 0x5f;

		// This is proprietary DLS protocol, it will be adhered when spec. is finished.
		NdisZeroMemory(temp, 64);
		NdisMoveMemory(temp, "IEEE802.11 WIRELESS ACCESS POINT", 32);
		WpaCountPTK(pAd, temp, temp, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid, temp, DA, pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_PTK);
		
		DBGPRINT(RT_DEBUG_TRACE, ("PTK-0-%x %x %x %x %x %x %x %x \n",
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[0],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[1],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[2],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[3],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[4],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[5],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[6],
				pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK[7]));

       	NdisMoveMemory(Packet.KeyDesc.KeyData, Key_Data, key_length);
		NdisZeroMemory(mic, sizeof(mic));

		MakeOutgoingFrame(pOutBuffer,			&FrameLen,
                        Packet.Body_Len[1] + 4,	&Packet,
                        END_OF_ARGS);
	    
		// Calculate MIC
        if (pDaEntry->WepStatus == Ndis802_11Encryption3Enabled)
        {
            HMAC_SHA1(pOutBuffer, FrameLen, pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_EAP_MICK, digest);
            NdisMoveMemory(Packet.KeyDesc.KeyMic, digest, LEN_KEY_DESC_MIC);
	    }
        else
        {
            hmac_md5(pAd->ApCfg.MBSSID[pEntry->apidx].DlsPTK, LEN_EAP_MICK, pOutBuffer, FrameLen, mic);
            NdisMoveMemory(Packet.KeyDesc.KeyMic, mic, LEN_KEY_DESC_MIC);
        }

#ifdef BIG_ENDIAN
		{
			USHORT	tmpKeyinfo;

			NdisMoveMemory(&tmpKeyinfo, &Packet.KeyDesc.KeyInfo, sizeof(USHORT)); 
			tmpKeyinfo = SWAP16(tmpKeyinfo);
			NdisMoveMemory(&Packet.KeyDesc.KeyInfo, &tmpKeyinfo, sizeof(USHORT)); 
		}
#endif

        // Fill frame
        MakeOutgoingFrame(pOutBuffer,				&FrameLen,
                            sizeof(Header802_3),	Header802_3,
                            Packet.Body_Len[1] + 4,	&Packet,
                            END_OF_ARGS);

        APToWirelessSta(pAd, pDaEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&Packet, Packet.Body_Len[1] + 4, FALSE);

        kfree(pOutBuffer);
    }while(FALSE);
    
    DBGPRINT(RT_DEBUG_TRACE, ("<== RTMPHandleSTAKey: FrameLen=%ld\n", FrameLen));
}
#endif // QOS_DLS_SUPPORT //
