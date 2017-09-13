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
    auth.c
 
    Abstract:
    Handle de-auth request from local MLME
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"

/*
    ==========================================================================
    Description:
        authenticate state machine init, including state transition and timer init
    Parameters:
        Sm - pointer to the auth state machine
    Note:
        The state machine looks like this
        
                                    AP_AUTH_REQ_IDLE           
        APMT2_MLME_DEAUTH_REQ     mlme_deauth_req_action  
    ==========================================================================
 */
void APAuthStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN STATE_MACHINE *Sm, 
    OUT STATE_MACHINE_FUNC Trans[]) 
{
    StateMachineInit(Sm, (STATE_MACHINE_FUNC *)Trans, AP_MAX_AUTH_STATE, AP_MAX_AUTH_MSG, (STATE_MACHINE_FUNC)Drop, AP_AUTH_REQ_IDLE, AP_AUTH_MACHINE_BASE);
     
    // the first column
    StateMachineSetAction(Sm, AP_AUTH_REQ_IDLE, APMT2_MLME_DEAUTH_REQ, (STATE_MACHINE_FUNC)APMlmeDeauthReqAction);
}

/*
    ==========================================================================
    Description:
        Upper Layer request to kick out a STA
    ==========================================================================
 */
VOID APMlmeDeauthReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
    MLME_DEAUTH_REQ_STRUCT *pInfo;
    HEADER_802_11 Hdr;
    PUCHAR        pOutBuffer = NULL;
    NDIS_STATUS   NStatus;
    ULONG         FrameLen = 0;
    MAC_TABLE_ENTRY *pEntry;

    pInfo = (MLME_DEAUTH_REQ_STRUCT *)Elem->Msg;

    if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
    {
		pEntry = &pAd->MacTab.Content[Elem->Wcid];

		// send wireless event - for deauthentication
		if (pEntry && pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_DEAUTH_EVENT_FLAG, pInfo->Addr, 0, 0);  

        // 1. remove this STA from MAC table
        ApLogEvent(pAd, pInfo->Addr, EVENT_DISASSOCIATED);
        MacTableDeleteEntry(pAd, Elem->Wcid, pInfo->Addr);

		pEntry = &pAd->MacTab.Content[Elem->Wcid];

        // 2. send out DE-AUTH request frame
        NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
        if (NStatus != NDIS_STATUS_SUCCESS) 
            return;

        DBGPRINT(RT_DEBUG_TRACE, ("AUTH - Send DE-AUTH req to %02x:%02x:%02x:%02x:%02x:%02x\n",
            pInfo->Addr[0],pInfo->Addr[1],pInfo->Addr[2],pInfo->Addr[3],pInfo->Addr[4],pInfo->Addr[5]));
        printk("AUTH - Send DE-AUTH req to %02x:%02x:%02x:%02x:%02x:%02x\n",
            pInfo->Addr[0],pInfo->Addr[1],pInfo->Addr[2],pInfo->Addr[3],pInfo->Addr[4],pInfo->Addr[5]);		
           		
        MgtMacHeaderInit(pAd, &Hdr, SUBTYPE_DEAUTH, 0, pInfo->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);
        MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
                          sizeof(HEADER_802_11), &Hdr, 
                          2,                     &pInfo->Reason, 
                          END_OF_ARGS);
        MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
        MlmeFreeMemory(pAd, pOutBuffer);
    }
}

/*
    ==========================================================================
    Description:
        Some STA/AP
    Note:
        This action should never trigger AUTH state transition, therefore we
        separate it from AUTH state machine, and make it as a standalone service
    ==========================================================================
 */
VOID APCls2errAction(
    IN PRTMP_ADAPTER pAd, 
	IN 	ULONG Wcid, 
    IN	PHEADER_802_11	pHeader) 
{
    HEADER_802_11 Hdr;
    PUCHAR        pOutBuffer = NULL;
    NDIS_STATUS   NStatus;
    ULONG         FrameLen = 0;
    USHORT        Reason = REASON_CLS2ERR;
    MAC_TABLE_ENTRY *pEntry = NULL;


#if 0
    pEntry = MacTableLookup(pAd, pHeader->Addr2);
#else	
	if (Wcid < MAX_LEN_OF_MAC_TABLE)
	{
      pEntry = &(pAd->MacTab.Content[Wcid]);
	}
#endif	

    if (pEntry && pEntry->ValidAsCLI)
    {
        //ApLogEvent(pAd, pAddr, EVENT_DISASSOCIATED);
        MacTableDeleteEntry(pAd, pEntry->Aid, pHeader->Addr2);
	}
	else
	{
		UCHAR   bssid[MAC_ADDR_LEN];

		NdisMoveMemory(bssid, pHeader->Addr1, MAC_ADDR_LEN);
		bssid[5] &= pAd->ApCfg.MacMask;

		if (NdisEqualMemory(pAd->CurrentAddress, bssid, MAC_ADDR_LEN) == 0)
		{
			return;
		}
	}

    	// send out DEAUTH request frame 
    	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    	if (NStatus != NDIS_STATUS_SUCCESS) 
    	    return;

    	DBGPRINT(RT_DEBUG_TRACE, ("AUTH - Class 2 error, Send DEAUTH frame to %02x:%02x:%02x:%02x:%02x:%02x\n", 
    		PRINT_MAC(pHeader->Addr2)));

    	MgtMacHeaderInit(pAd, &Hdr, SUBTYPE_DEAUTH, 0, pHeader->Addr2, pHeader->Addr1);
    	MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
    	                  sizeof(HEADER_802_11), &Hdr, 
    	                  2,                     &Reason, 
    	                  END_OF_ARGS);
    	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
    	MlmeFreeMemory(pAd, pOutBuffer);

}

