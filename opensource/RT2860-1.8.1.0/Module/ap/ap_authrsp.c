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
    auth_rsp.c
 
    Abstract:
    Handle auth/de-auth requests from WSTA
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"

/*
    ==========================================================================
    Description:
        authentication state machine init procedure
    Parameters:
        Sm - the state machine
    Note:
        the state machine looks like the following 
        
                                        AP_AUTH_RSP_IDLE                   
    APMT2_AUTH_CHALLENGE_TIMEOUT      auth_rsp_challenge_timeout_action    
    APMT2_PEER_AUTH_ODD               peer_auth_at_auth_rsp_idle_action 
    APMT2_PEER_DEAUTH                 peer_deauth_action         
    ==========================================================================
 */
VOID APAuthRspStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN PSTATE_MACHINE Sm, 
    IN STATE_MACHINE_FUNC Trans[]) 
{

    StateMachineInit(Sm, (STATE_MACHINE_FUNC*)Trans, AP_MAX_AUTH_RSP_STATE, AP_MAX_AUTH_RSP_MSG, (STATE_MACHINE_FUNC)Drop, AP_AUTH_RSP_IDLE, AP_AUTH_RSP_MACHINE_BASE);

    // column 1
    StateMachineSetAction(Sm, AP_AUTH_RSP_IDLE, APMT2_PEER_AUTH_ODD, (STATE_MACHINE_FUNC)APPeerAuthAtAuthRspIdleAction);
    StateMachineSetAction(Sm, AP_AUTH_RSP_IDLE, APMT2_PEER_DEAUTH, (STATE_MACHINE_FUNC)APPeerDeauthReqAction);
}


/*
    ==========================================================================
    Description:
        Process the received Authnetication frame from client
    ==========================================================================
*/
VOID APPeerAuthAtAuthRspIdleAction(
    IN PRTMP_ADAPTER pAd, 
    IN PMLME_QUEUE_ELEM Elem) 
{
    USHORT          Seq, Alg, RspReason, i, Status;
    UCHAR           Addr2[MAC_ADDR_LEN];
    PHEADER_802_11  pRcvHdr;
    HEADER_802_11   AuthHdr;
    CHAR            Chtxt[CIPHER_TEXT_LEN];
    PUCHAR          pOutBuffer = NULL;
    NDIS_STATUS     NStatus;
    ULONG           FrameLen = 0;
    MAC_TABLE_ENTRY *pEntry;
    UCHAR           ChTxtIe = 16, ChTxtLen = CIPHER_TEXT_LEN;
	UCHAR			Addr1[MAC_ADDR_LEN];
	INT				apidx = -1;

    DBGPRINT(RT_DEBUG_INFO,("AUTH_RSP - APPeerAuthAtAuthRspIdleAction\n"));
    if (! APPeerAuthSanity(pAd, Elem->Msg, Elem->MsgLen, Addr1, Addr2, &Alg, &Seq, &Status, Chtxt)) 
        return;
    
    // Find which MBSSID to be authenticate

	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{	
		if (RTMPEqualMemory(Addr1, pAd->ApCfg.MBSSID[apidx].Bssid, MAC_ADDR_LEN))
		{
			break;
		}
	}

	if (apidx < 0 || apidx >= pAd->ApCfg.BssidNum)
	{	
    	DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Bssid not found\n"));
	   	return;
	}

	pEntry = NULL;
	
	//if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE) 
	{
		pEntry = MacTableLookup(pAd, Addr2);

		if (pEntry && pEntry->ValidAsCLI)
		{
			if (pEntry->bIAmBadAtheros == TRUE)
			{
				AsicUpdateProtect(pAd, 8, ALLN_SETPROTECT, FALSE, FALSE);
				DBGPRINT(RT_DEBUG_TRACE, ("Atheros Problem. Turn on RTS/CTS!!!\n"));
				pEntry->bIAmBadAtheros = FALSE;
			}
			BASessionTearDownALL(pAd, pEntry->Aid);
			ASSERT(pEntry->Aid == Elem->Wcid);
		}
		//pEntry = &pAd->MacTab.Content[Elem->Wcid];
	}
			
    pRcvHdr = (PHEADER_802_11)(Elem->Msg);
	DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - MBSS(%d), Rcv AUTH seq#%d, Alg=%d, Status=%d from [wcid=%d]%02x:%02x:%02x:%02x:%02x:%02x\n",
		 apidx, Seq, Alg, Status, Elem->Wcid, PRINT_MAC(Addr2)));

	
	// fail in ACL checking => send an AUTH-Fail seq#2.
    if (! ApCheckAccessControlList(pAd, Addr2, apidx))
    {
		ASSERT(Seq == 1);
		ASSERT(pEntry == NULL);
		APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_UNSPECIFY_FAIL);
		DBGPRINT(RT_DEBUG_TRACE, ("Failed in ACL checking => send an AUTH seq#2 with Status code = %d\n", MLME_UNSPECIFY_FAIL));
		return;
    }
	
    if (Seq == 1) 
    {
        if ((Alg == AUTH_MODE_OPEN) && 
			(pAd->ApCfg.MBSSID[apidx].AuthMode != Ndis802_11AuthModeShared)) 
        {
            if (!pEntry)
				pEntry = MacTableInsertEntry(pAd, Addr2, apidx, TRUE);

            if (pEntry)
            {
                pEntry->AuthState = AS_AUTH_OPEN;
                pEntry->Sst = SST_AUTH; // what if it already in SST_ASSOC ???????
                APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_SUCCESS);
            }
            else
                ; // MAC table full, what should we respond ?????
        } 
        else if ((Alg == AUTH_MODE_KEY) && 
			((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeShared) || (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeAutoSwitch))) 
        {
            if (!pEntry)
				pEntry = MacTableInsertEntry(pAd, Addr2, apidx, TRUE);

            if (pEntry)
            {
                pEntry->AuthState = AS_AUTHENTICATING;
                pEntry->Sst = SST_NOT_AUTH; // what if it already in SST_ASSOC ???????

                // log this STA in AuthRspAux machine, only one STA is stored. If two STAs using
                // SHARED_KEY authentication mingled together, then the late comer will win.
                COPY_MAC_ADDR(&pAd->ApMlmeAux.Addr, Addr2);
                for(i = 0; i < CIPHER_TEXT_LEN; i++) 
                    pAd->ApMlmeAux.Challenge[i] = RandomByte(pAd);

                RspReason = 0;
                Seq++;
  
                NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
                if(NStatus != NDIS_STATUS_SUCCESS) 
                    return;  // if no memory, can't do anything

                DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Send AUTH seq#2 (Challenge)\n"));
				MgtMacHeaderInit(pAd, &AuthHdr, SUBTYPE_AUTH, 0, Addr2, pAd->ApCfg.MBSSID[apidx].Bssid);
                MakeOutgoingFrame(pOutBuffer,            &FrameLen,
                                  sizeof(HEADER_802_11), &AuthHdr,
                                  2,                     &Alg,
                                  2,                     &Seq,
                                  2,                     &RspReason,
                                  1,                     &ChTxtIe,
                                  1,                     &ChTxtLen,
                                  CIPHER_TEXT_LEN,       pAd->ApMlmeAux.Challenge,
                                  END_OF_ARGS);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
                MlmeFreeMemory(pAd, pOutBuffer);
            }
            else
                ; // MAC table full, what should we respond ????
        } 
        else 
        {
            // wrong algorithm
            APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_ALG_NOT_SUPPORT);
            DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Alg=%d, Seq=%d, AuthMode=%d\n",
				Alg, Seq, pAd->ApCfg.MBSSID[apidx].AuthMode));
        }
    } 
    else if (Seq == 3)
    {
        if (pEntry && MAC_ADDR_EQUAL(Addr2, pAd->ApMlmeAux.Addr)) 
        {
            if ((pRcvHdr->FC.Wep == 1) && NdisEqualMemory(Chtxt, pAd->ApMlmeAux.Challenge, CIPHER_TEXT_LEN)) 
            {
                // Successful
                APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_SUCCESS);
                pEntry->AuthState = AS_AUTH_KEY;
                pEntry->Sst = SST_AUTH;
            } 
            else 
            {
                // fail - wep bit is not set or challenge text is not equal
                APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_REJ_CHALLENGE_FAILURE);
				MacTableDeleteEntry(pAd, pEntry->Aid, pEntry->Addr);
                //Chtxt[127]='\0';
                //pAd->ApMlmeAux.Challenge[127]='\0';
                DBGPRINT(RT_DEBUG_TRACE, ("%s\n",((pRcvHdr->FC.Wep == 1) ? "challenge text is not equal" : "wep bit is not set")));
                //DBGPRINT(RT_DEBUG_TRACE, ("Sent Challenge = %s\n",&pAd->ApMlmeAux.Challenge[100]));
                //DBGPRINT(RT_DEBUG_TRACE, ("Rcv Challenge = %s\n",&Chtxt[100]));
            }
        } 
        else 
        {
            // fail for unknown reason. most likely is AuthRspAux machine be overwritten by another
            // STA also using SHARED_KEY authentication
            APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_UNSPECIFY_FAIL);
        }
    }
    else 
    {
        // fail - wrong sequence number
        APPeerAuthSimpleRspGenAndSend(pAd, pRcvHdr, Alg, Seq + 1, MLME_SEQ_NR_OUT_OF_SEQUENCE);
    }
}

/*
    ==========================================================================
    Description:
        Process De-authentication request frame received from client
    ==========================================================================
*/
VOID APPeerDeauthReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN PMLME_QUEUE_ELEM Elem) 
{
    UCHAR       Addr2[MAC_ADDR_LEN];
    USHORT      Reason;
    MAC_TABLE_ENTRY       *pEntry;

    DBGPRINT(RT_DEBUG_INFO,("AUTH_RSP - APPeerDeauthReqAction\n"));
    if (! PeerDeauthReqSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, &Reason)) 
        return;
	pEntry = NULL;
	//pEntry = MacTableLookup(pAd, Addr2);
	if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
    {
		pEntry = &pAd->MacTab.Content[Elem->Wcid];

		// Notify 802.1x daemon to clear this sta info
		IEEE8021X_L2_Disconnect_Frame_Send(pAd, pEntry);

		// send wireless event - for deauthentication
		if (pEntry && pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_DEAUTH_EVENT_FLAG, Addr2, 0, 0);  
		
        if (pEntry->CMTimerRunning == TRUE)
        {
            // if one who initilized Counter Measure deauth itself,  AP doesn't log the MICFailTime
            pAd->ApCfg.aMICFailTime = pAd->ApCfg.PrevaMICFailTime;
        }

        ApLogEvent(pAd, Addr2, EVENT_DISASSOCIATED);
		MacTableDeleteEntry(pAd, Elem->Wcid, Addr2);
        DBGPRINT(RT_DEBUG_TRACE,("AUTH_RSP(%d) - receive DE-AUTH. delete MAC entry %02x:%02x:%02x:%02x:%02x:%02x\n", Reason, Addr2[0],Addr2[1],Addr2[2],Addr2[3],Addr2[4],Addr2[5] ));
    }
   
}

/*
    ==========================================================================
    Description:
        Send out a Authentication (response) frame
    ==========================================================================
*/
VOID APPeerAuthSimpleRspGenAndSend(
    IN PRTMP_ADAPTER pAd, 
    IN PHEADER_802_11 pHdr, 
    IN USHORT Alg, 
    IN USHORT Seq, 
    IN USHORT StatusCode) 
{
    HEADER_802_11     AuthHdr;
    ULONG             FrameLen = 0;
    PUCHAR            pOutBuffer = NULL;
    NDIS_STATUS       NStatus;

    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS) 
        return;

    if (StatusCode == MLME_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Send AUTH response (SUCCESS)...\n"));
        MgtMacHeaderInit(pAd, &AuthHdr, SUBTYPE_AUTH, 0, pHdr->Addr2, pHdr->Addr1/*pAd->ApCfg.MBSSID[apidx].Bssid*/);
        MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
                          sizeof(HEADER_802_11), &AuthHdr, 
                          2,                     &Alg, 
                          2,                     &Seq, 
                          2,                     &StatusCode, 
                          END_OF_ARGS);
		MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
        MlmeFreeMemory(pAd, pOutBuffer);
	}
    else
    {
        // For MAC wireless client(Macintosh), need to send AUTH_RSP with Status Code (fail reason code) to reject it.
        MgtMacHeaderInit(pAd, &AuthHdr, SUBTYPE_AUTH, 0, pHdr->Addr2, pHdr->Addr1/*pAd->ApCfg.MBSSID[apidx].Bssid*/);
        MakeOutgoingFrame(pOutBuffer,                &FrameLen, 
                          sizeof(HEADER_802_11),    &AuthHdr, 
                          2,                        &Alg, 
                          2,                        &Seq, 
                          2,                        &StatusCode, 
                          END_OF_ARGS);
        MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
        MlmeFreeMemory(pAd, pOutBuffer);
        DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Peer AUTH fail (Status = %d)...\n", StatusCode));
    }
}

