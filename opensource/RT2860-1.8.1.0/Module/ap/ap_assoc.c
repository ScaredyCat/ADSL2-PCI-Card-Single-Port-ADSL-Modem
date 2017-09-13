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
    assoc.c
 
    Abstract:
    Handle association related requests either from WSTA or from local MLME
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"

static void ap_assoc_info_debugshow(
	IN	PRTMP_ADAPTER		pAd,
	IN	BOOLEAN				isReassoc,
	IN	MAC_TABLE_ENTRY 	*pEntry,
	IN  UCHAR				HTCapability_Len,
	IN	HT_CAPABILITY_IE	*pHTCapability);


/*  
    ==========================================================================
    Description: 
        association state machine init, including state transition and timer init
    Parameters: 
        S - pointer to the association state machine
    Note:
        The state machine looks like the following 
        
                                    AP_ASSOC_IDLE             
        APMT2_MLME_DISASSOC_REQ    mlme_disassoc_req_action 
        APMT2_PEER_DISASSOC_REQ    peer_disassoc_action     
        APMT2_PEER_ASSOC_REQ       drop                     
        APMT2_PEER_REASSOC_REQ     drop                     
        APMT2_CLS3ERR              cls3err_action           
    ==========================================================================
 */
VOID APAssocStateMachineInit(
    IN  PRTMP_ADAPTER   pAd, 
    IN  STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]) 
{
    StateMachineInit(S, (STATE_MACHINE_FUNC*)Trans, AP_MAX_ASSOC_STATE, AP_MAX_ASSOC_MSG, (STATE_MACHINE_FUNC)Drop, AP_ASSOC_IDLE, AP_ASSOC_MACHINE_BASE);

    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_MLME_DISASSOC_REQ, (STATE_MACHINE_FUNC)APMlmeDisassocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_DISASSOC_REQ, (STATE_MACHINE_FUNC)APPeerDisassocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_ASSOC_REQ,    (STATE_MACHINE_FUNC)APPeerAssocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_REASSOC_REQ,  (STATE_MACHINE_FUNC)APPeerReassocReqAction);
//  StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_CLS3ERR,           APCls3errAction);
}

/* Layer 2 Update frame to switch/bridge */
/* For any Layer2 devices, e.g., bridges, switches and other APs, the frame
   can update their forwarding tables with the correct port to reach the new
   location of the STA */
typedef struct PACKED _RT_IAPP_L2_UPDATE_FRAME {

    UCHAR   DA[ETH_ALEN]; /* broadcast MAC address */
    UCHAR   SA[ETH_ALEN]; /* the MAC address of the STA that has just associated
                             or reassociated */
    USHORT  Len;          /* 8 octets */
    UCHAR   DSAP;         /* null */
    UCHAR   SSAP;         /* null */
    UCHAR   Control;      /* reference to IEEE Std 802.2 */
    UCHAR   XIDInfo[3];   /* reference to IEEE Std 802.2 */
} RT_IAPP_L2_UPDATE_FRAME, *PRT_IAPP_L2_UPDATE_FRAME;


/*
 ========================================================================
 Routine Description:
    Send Leyer 2 Update Frame to update forwarding table in Layer 2 devices.

 Arguments:
    *mac_p - the STATION MAC address pointer

 Return Value:
    TRUE - send successfully
    FAIL - send fail

 Note:
 ========================================================================
*/
static BOOLEAN IAPP_L2_Update_Frame_Send(
	IN PRTMP_ADAPTER	pAd,
    IN UINT8 *mac_p,
    IN INT  bssid)
{
    RT_IAPP_L2_UPDATE_FRAME  frame_body;

    INT size = sizeof(RT_IAPP_L2_UPDATE_FRAME);
#ifdef UCOS
	struct net_pkt_blk *skb = net_pkt_alloc(NET_BUF_1024);
#else
	struct sk_buff *skb = dev_alloc_skb(size+2);
#endif
	if (!skb)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Error! Can't allocate a skb.\n"));
		return FALSE;
}
    /* init the update frame body */
    memset(&frame_body, 0, size);

    memset(frame_body.DA, 0xFF, ETH_ALEN);
    memcpy(frame_body.SA, mac_p, ETH_ALEN);

    frame_body.Len      = htons(ETH_ALEN);
    frame_body.DSAP     = 0;
    frame_body.SSAP     = 0x01;
    frame_body.Control  = 0xAF;

    frame_body.XIDInfo[0] = 0x81;
    frame_body.XIDInfo[1] = 1;
    frame_body.XIDInfo[2] = 1 << 1;

    GET_OS_PKT_NETDEV(skb) = get_netdev_from_bssid(pAd, bssid);
#ifdef UCOS    
	net_pkt_put(skb, size);    
	memcpy(skb->data, &frame_body, size);
#else
    skb_reserve(skb, 2);
    memcpy(skb_put(skb, size), &frame_body, size);    
#endif 

    // UCOS: update the built-in bridge, too (don't use gmac.xmit())
    announce_802_3_packet(pAd, skb);

    return TRUE;
} /* End of IAPP_L2_Update_Frame_Send */

/*
 ========================================================================
 Routine Description:
    Send Leyer 2 Frame to notify 802.1x daemon to disconnect 
    a specific client.

 Arguments:

 Return Value:
    TRUE - send successfully
    FAIL - send fail

 Note:
 ========================================================================
*/
BOOLEAN IEEE8021X_L2_Disconnect_Frame_Send(
    IN  PRTMP_ADAPTER	pAd,
    IN  MAC_TABLE_ENTRY *pEntry)
{	
	INT				apidx = MAIN_MBSSID;		
	UCHAR 			Header802_3[14];
	UCHAR 			RalinkIe[9] = {221, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00}; 	
								   
    if((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE))
	{		
		INT size = sizeof(Header802_3) + sizeof(RalinkIe);
#ifdef UCOS
	struct net_pkt_blk *skb = net_pkt_alloc(NET_BUF_1024);
#else
	struct sk_buff *skb = dev_alloc_skb(size+2);
#endif

	apidx = pEntry->apidx; 
	MAKE_802_3_HEADER(Header802_3, pAd->ApCfg.MBSSID[apidx].Bssid, pEntry->Addr, EAPOL); 

	if (!skb)
{
		DBGPRINT(RT_DEBUG_ERROR, ("Error! Can't allocate a skb.\n"));
		return FALSE;
}
   	GET_OS_PKT_NETDEV(skb) = get_netdev_from_bssid(pAd, apidx);
				    	
#ifdef UCOS    
		
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb), Header802_3, LENGTH_802_3);	
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb) + LENGTH_802_3, RalinkIe, sizeof(RalinkIe));
	
	 	net_pkt_put(GET_OS_PKT_TYPE(skb), size);		 		
#else
		skb_reserve(skb, 2);	// 16 byte align the IP header
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb), Header802_3, LENGTH_802_3);	
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb) + LENGTH_802_3, RalinkIe, sizeof(RalinkIe));
	
 		skb_put(GET_OS_PKT_TYPE(skb), size);		        
#endif 

		DBGPRINT(RT_DEBUG_TRACE, ("Notify 8021.x daemon to remove this sta(%02x:%02x:%02x:%02x:%02x:%02x)\n", PRINT_MAC(pEntry->Addr)));

    // UCOS: update the built-in bridge, too (don't use gmac.xmit())
    announce_802_3_packet(pAd, skb);

	}	

	return TRUE;
}	    


/*
 ========================================================================
 Routine Description:
    Send Leyer 2 Frame to trigger 802.1x EAP state machine.     

 Arguments:

 Return Value:
    TRUE - send successfully
    FAIL - send fail

 Note:
 ========================================================================
*/
BOOLEAN IEEE8021X_L2_Trigger_Frame_Send(
    IN  PRTMP_ADAPTER	pAd,
    IN  MAC_TABLE_ENTRY *pEntry)
{	
	INT				apidx = MAIN_MBSSID;		
	UCHAR 			Header802_3[14];
	UCHAR 			eapol_start_1x_hdr[4] = {0x01, 0x01, 0x00, 0x00}; 	
								   
    if((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2) || (pAd->ApCfg.MBSSID[apidx].IEEE8021X == TRUE))
	{		
		INT size = sizeof(Header802_3) + sizeof(eapol_start_1x_hdr);
#ifdef UCOS
		struct net_pkt_blk *skb = net_pkt_alloc(NET_BUF_1024);
#else
		struct sk_buff *skb = dev_alloc_skb(size+2);
#endif

		apidx = pEntry->apidx; 
		MAKE_802_3_HEADER(Header802_3, pAd->ApCfg.MBSSID[apidx].Bssid, pEntry->Addr, EAPOL); 

		if (!skb)
		{
				DBGPRINT(RT_DEBUG_ERROR, ("Error! Can't allocate a skb.\n"));
				return FALSE;
		}
	   	GET_OS_PKT_NETDEV(skb) = get_netdev_from_bssid(pAd, apidx);
					    	
#ifdef UCOS    
			
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb), Header802_3, LENGTH_802_3);	
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb) + LENGTH_802_3, eapol_start_1x_hdr, sizeof(eapol_start_1x_hdr));
	
	 	net_pkt_put(GET_OS_PKT_TYPE(skb), size);		 		
#else
		skb_reserve(skb, 2);	// 16 byte align the IP header
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb), Header802_3, LENGTH_802_3);	
		NdisMoveMemory(GET_OS_PKT_DATAPTR(skb) + LENGTH_802_3, eapol_start_1x_hdr, sizeof(eapol_start_1x_hdr));
	
 		skb_put(GET_OS_PKT_TYPE(skb), size);		        
#endif 

		DBGPRINT(RT_DEBUG_TRACE, ("Notify 8021.x daemon to trigger EAP-SM for this sta(%02x:%02x:%02x:%02x:%02x:%02x)\n", PRINT_MAC(pEntry->Addr)));

	    // UCOS: update the built-in bridge, too (don't use gmac.xmit())
	    announce_802_3_packet(pAd, skb);

	}	

	return TRUE;
}	    


VOID ap_cmm_peer_assoc_req_action(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem,
	IN BOOLEAN isReassoc) 
{
    UCHAR           ApAddr[MAC_ADDR_LEN], Addr2[MAC_ADDR_LEN];
    HEADER_802_11   AssocRspHdr;
    USHORT          ListenInterval;
    USHORT          CapabilityInfo;
    USHORT          CapabilityInfoForAssocResp;
    USHORT          StatusCode = 0;
    USHORT          Aid;
    PUCHAR          pOutBuffer = NULL;
    NDIS_STATUS     NStatus;
    ULONG           FrameLen = 0;
    char            Ssid[MAX_LEN_OF_SSID];
	UCHAR			SsidLen;//, HtLen, AddHtLen;
    UCHAR           SupportedRatesLen;
    UCHAR           SupportedRates[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR           MaxSupportedRate = 0;
    UCHAR           i;
    MAC_TABLE_ENTRY *pEntry;
    UCHAR           RSNIE_Len;
    UCHAR           RSN_IE[MAX_LEN_OF_RSNIE];
    BOOLEAN         bWmmCapable;
    ULONG           RalinkIe;
	HT_CAPABILITY_IE		HTCapability;
	UCHAR			HTCapability_Len;
#ifdef DBG
	UCHAR			*sAssoc = isReassoc ? "ReASSOC" : "ASSOC";
#endif // DBG //
	UCHAR			SubType;
#ifdef WSC_AP_SUPPORT
    BOOLEAN          bWscCapable = FALSE;
#endif // WSC_AP_SUPPORT //
	BOOLEAN 		 bACLReject = FALSE;

	RTMPZeroMemory(&HTCapability, sizeof(HT_CAPABILITY_IE));
    // 1. frame sanity check
#ifdef WSC_AP_SUPPORT
    if (! PeerAssocReqCmmSanity(pAd, isReassoc, Elem->Msg, Elem->MsgLen, Addr2, 
						&CapabilityInfo, &ListenInterval, ApAddr, &SsidLen, &Ssid[0], 
						&SupportedRatesLen, &SupportedRates[0],RSN_IE, 
						&RSNIE_Len, &bWmmCapable, &bWscCapable, &RalinkIe, &HTCapability_Len, &HTCapability)) 
#else // WSC_AP_SUPPORT //
	if (! PeerAssocReqCmmSanity(pAd, isReassoc, Elem->Msg, Elem->MsgLen, Addr2, 
						&CapabilityInfo, &ListenInterval, ApAddr, &SsidLen, &Ssid[0], 
						&SupportedRatesLen, &SupportedRates[0],RSN_IE, 
						&RSNIE_Len, &bWmmCapable, &RalinkIe, &HTCapability_Len, &HTCapability)) 
#endif // WSC_AP_SUPPORT //
        return;

	pEntry = MacTableLookup(pAd, Addr2);
    if (!pEntry) {
		printk("NoAuth MAC - %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(Addr2));
		return;
	}
    
	// clear the previous Pairwise key table
    if(pEntry->Aid != 0 && 
		(pEntry->WepStatus >= Ndis802_11Encryption2Enabled || pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X))
    {
		NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));  

		// clear this entry as no-security mode
		AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, pEntry->Aid);

		// Notify 802.1x daemon to clear this sta info
		IEEE8021X_L2_Disconnect_Frame_Send(pAd, pEntry);
    }
#ifdef WSC_AP_SUPPORT
    // since sta has been left, ap should receive EapolStart and EapRspId again.
    pEntry->Receive_EapolStart_EapRspId = 0;
    // only support WSC in ra0 now, 2006.11.10
    if (pEntry->apidx == MAIN_MBSSID)
    {
        if (MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr))
        {
            BOOLEAN Cancelled;
            pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;
            memset(pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr, 0, MAC_ADDR_LEN);
            RTMPCancelTimer(&pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EapolTimer, &Cancelled);
            pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EapolTimerRunning = FALSE;
            DBGPRINT(RT_DEBUG_TRACE, ("Reset EntryApIdx to WSC_INIT_ENTRY_APIDX.\n"));
        }
    }

    pEntry->bWscCapable = bWscCapable;
    if ((RSNIE_Len == 0) && 
        (pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode != WSC_DISABLE))
        pEntry->bWscCapable = TRUE;
#endif // WSC_AP_SUPPORT //

    // for hidden SSID sake, SSID in AssociateRequest should be fully verified    
	if ((SsidLen != pAd->ApCfg.MBSSID[pEntry->apidx].SsidLen) || (NdisEqualMemory(Ssid, pAd->ApCfg.MBSSID[pEntry->apidx].Ssid, SsidLen)==0))
        return;
        
    // set a flag for sending Assoc-Fail response to unwanted STA later.
    if (! ApCheckAccessControlList(pAd, Addr2, pEntry->apidx))
		bACLReject = TRUE;

	DBGPRINT(RT_DEBUG_TRACE, ("%s - MBSS(%d), receive %s request from %02x:%02x:%02x:%02x:%02x:%02x\n", 
							  sAssoc, pEntry->apidx, sAssoc, PRINT_MAC(Addr2)));
    
    // supported rates array may not be sorted. sort it and find the maximum rate
    for (i=0; i<SupportedRatesLen; i++)
    {
        if (MaxSupportedRate < (SupportedRates[i] & 0x7f)) 
            MaxSupportedRate = SupportedRates[i] & 0x7f;
    }            
    
    // 2. qualify this STA's auth_asoc status in the MAC table, decide StatusCode
	StatusCode = APBuildAssociation(pAd, pEntry, CapabilityInfo, MaxSupportedRate, RSN_IE, &RSNIE_Len, bWmmCapable, RalinkIe, &HTCapability, HTCapability_Len, &Aid);

#if 0 // handle it in ap::MacTableMaintenance
	// Check write TXOP != 0.
	if ((pAd->CommonCfg.bRdg==TRUE) &&  (RalinkIe != 0)  &&  (pAd->MacTab.Size == 1))
	{
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
		AsicEnableRDG(pAd);
	}
	else
	{
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
		AsicDisableRDG(pAd);
	}
#endif

	pEntry->RateLen = SupportedRatesLen;

    // 3. send Association Response
    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS) 
        return;
        
    DBGPRINT(RT_DEBUG_TRACE, ("%s - Send %s response (Status=%d)...\n", sAssoc, sAssoc, StatusCode));
    Aid |= 0xc000; // 2 most significant bits should be ON

	SubType = isReassoc ? SUBTYPE_REASSOC_RSP : SUBTYPE_ASSOC_RSP;

	CapabilityInfoForAssocResp = pAd->ApCfg.MBSSID[pEntry->apidx].CapabilityInfo; //use AP's cability 
#ifdef WSC_AP_SUPPORT
	if ((pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE) && 
        (CapabilityInfo & 0x0010))
	{
		CapabilityInfoForAssocResp |= 0x0010;
	}
#endif // WSC_AP_SUPPORT //
	
	// fail in ACL checking => send an AUTH-Fail resp.
	if (bACLReject == TRUE)
	{
	    MgtMacHeaderInit(pAd, &AssocRspHdr, SubType, 0, Addr2, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);
		StatusCode = MLME_UNSPECIFY_FAIL;
	    MakeOutgoingFrame(pOutBuffer,               &FrameLen,
	                      sizeof(HEADER_802_11),    &AssocRspHdr,
	                      2,                        &CapabilityInfo,
	                      2,                        &StatusCode,
	                      2,                        &Aid,
	                      1,                        &SupRateIe,
	                      1,                        &pAd->CommonCfg.SupRateLen,
	                      pAd->CommonCfg.SupRateLen,    pAd->CommonCfg.SupRate,
	                      END_OF_ARGS);
		MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, (PVOID) pOutBuffer);
		return;
	}

    MgtMacHeaderInit(pAd, &AssocRspHdr, SubType, 0, Addr2, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);

    MakeOutgoingFrame(pOutBuffer,               &FrameLen,
                      sizeof(HEADER_802_11),    &AssocRspHdr,
                      2,                        &CapabilityInfoForAssocResp,
                      2,                        &StatusCode,
                      2,                        &Aid,
                      1,                        &SupRateIe,
                      1,                        &pAd->CommonCfg.SupRateLen,
                      pAd->CommonCfg.SupRateLen,    pAd->CommonCfg.SupRate,
                      END_OF_ARGS);

    if (pAd->CommonCfg.ExtRateLen)
    {
    	// The ERPIE should not be included in association response, so remove it.
//        UCHAR ErpIeLen = 1;
        ULONG TmpLen;
        MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen,
//                          1,                        &ErpIe,
//                          1,                        &ErpIeLen,
//                          1,                        &pAd->ApCfg.ErpIeContent,
                          1,                        &ExtRateIe,
                          1,                        &pAd->CommonCfg.ExtRateLen,
                          pAd->CommonCfg.ExtRateLen,    pAd->CommonCfg.ExtRate,
                          END_OF_ARGS);
        FrameLen += TmpLen;
    }                                                                          // add WMM IE here

    // add WMM IE here
    if (pAd->ApCfg.MBSSID[pEntry->apidx].bWmmCapable && CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE))
    {
        ULONG TmpLen;
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

	// HT capability in AssocRsp frame.
	if ((HTCapability_Len > 0) && (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR HtLen1;
		//UCHAR SupportedMCSSet[16];
		//UCHAR mode, bitmask;
		HT_CAPABILITY_IE HtCapabilityRsp;

#if 0
		RTMPZeroMemory(SupportedMCSSet, sizeof(SupportedMCSSet));

		// Find the supported MCSSet
		for (i = 0; i <= 76; i++)
		{	
			mode = i/8;	
			bitmask = (1<<(i-(mode*8)));
			if ((pAd->CommonCfg.DesiredHtPhy.MCSSet[mode] & bitmask) && (HTCapability.MCSSet[mode] & bitmask))
			{
				SupportedMCSSet[mode] |= bitmask;						
			}					
		}
		DBGPRINT(RT_DEBUG_TRACE, ("Supported MCSSet %02x %02x %02x %02x %02x\n", SupportedMCSSet[0], SupportedMCSSet[1], SupportedMCSSet[2], SupportedMCSSet[3], SupportedMCSSet[4]));	
#endif
	
		NdisMoveMemory(&HtCapabilityRsp, &pAd->CommonCfg.HtCapability, HTCapability_Len);
		//NdisMoveMemory(&HtCapabilityRsp.MCSSet[0], &SupportedMCSSet[0], sizeof(SupportedMCSSet));		
#if 0
		HtCapabilityRsp.HtCapInfo.LSIGTxopProSup = HTCapability.HtCapInfo.LSIGTxopProSup & pAd->CommonCfg.HtCapability.HtCapInfo.LSIGTxopProSup;
		HtCapabilityRsp.HtCapInfo.ShortGIfor40 = HTCapability.HtCapInfo.ShortGIfor40 & pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor40;
		HtCapabilityRsp.HtCapInfo.ShortGIfor20 = HTCapability.HtCapInfo.ShortGIfor20 & pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor20;
		HtCapabilityRsp.HtCapInfo.GF = HTCapability.HtCapInfo.GF & pAd->CommonCfg.HtCapability.HtCapInfo.GF;
		HtCapabilityRsp.HtCapInfo.ChannelWidth = HTCapability.HtCapInfo.ChannelWidth & pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth;
#endif		

#ifdef BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
#endif
		// add HT Capability IE 
		HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);

#ifndef BIG_ENDIAN
		MakeOutgoingFrame(pOutBuffer+FrameLen,			&TmpLen,
											1,			&HtCapIe,
											1,			&HTCapability_Len,
							HTCapability_Len,			&HtCapabilityRsp,
											1,			&AddHtInfoIe,
											1,			&HtLen1,
										HtLen1,			&pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
		NdisMoveMemory(&HtCapabilityTmp, &HtCapabilityRsp, HTCapability_Len);
		*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
		*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

		NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
		*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
		*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

		MakeOutgoingFrame(pOutBuffer + FrameLen,         &TmpLen,
							1,                           &HtCapIe,
							1,                           &HTCapability_Len,
							HTCapability_Len,            &HtCapabilityTmp, 
							1,                           &AddHtInfoIe,
							1,                           &HtLen1,
							HtLen1,                      &addHTInfoTmp,
							END_OF_ARGS);
#endif
		FrameLen += TmpLen;

		if ((RalinkIe) == 0 || (pAd->bBroadComHT == TRUE))
		{
			UCHAR epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = HTCapability_Len + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  HTCapability_Len,            		&HtCapabilityRsp, 
						  END_OF_ARGS);
#else
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  HTCapability_Len,            		&HtCapabilityTmp, 
						  END_OF_ARGS);
#endif

			FrameLen += TmpLen;
			epigram_ie_len = HtLen1 + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&addHTInfoTmp, 
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
	MakeOutgoingFrame(pOutBuffer+FrameLen,		 &TmpLen,
						9,						 RalinkSpecificIe,
						END_OF_ARGS);
	FrameLen += TmpLen;
}
  
	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, (PVOID) pOutBuffer);

	// set up BA session
	if (StatusCode == MLME_SUCCESS)
	{
#ifdef IAPP_SUPPORT
		//PFRAME_802_11 Fr = (PFRAME_802_11)Elem->Msg;
		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

		/* send association ok message to IAPPD */
		//pObj->RTSignal.Sig = SIG_ASSOCIATION;
		//pObj->RTSignal.Sequence = Fr->Hdr.Sequence;
		//pObj->ioctl_if = pEntry->apidx;
		//NdisMoveMemory(pObj->RTSignal.MacAddr, pEntry->Addr, MAC_ADDR_LEN);

		IAPP_L2_Update_Frame_Send(pAd, pEntry->Addr, pEntry->apidx);
		DBGPRINT(RT_DEBUG_TRACE, ("####### Send L2 Frame Mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
								  pEntry->Addr[0],
								  pEntry->Addr[1],
								  pEntry->Addr[2],
								  pEntry->Addr[3],
								  pEntry->Addr[4],
								  pEntry->Addr[5]));

		SendSingalToDaemon(SIGUSR2, pObj->IappPid);
#endif // IAPP_SUPPORT //

		ap_assoc_info_debugshow(pAd, isReassoc, pEntry, HTCapability_Len, &HTCapability);

		// send wireless event - for association
		if (pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_ASSOC_EVENT_FLAG, pEntry->Addr, 0, 0);			
    	
		/* clear txBA bitmap */
		pEntry->TXBAbitmap = 0;
		if (pEntry->MaxHTPhyMode.field.MODE >= MODE_HTMIX)
		{  
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			if ((pAd->CommonCfg.Channel <=14) && pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset && (HTCapability.HtCapInfo.ChannelWidth==BW_40))
			{
				SendBeaconRequest(pAd, pEntry->Aid);
			}
			//BAOriSessionSetUp(pAd, pEntry, 0, 0, 3000, FALSE);		
		}

		// enqueue a EAPOL_START message to trigger EAP state machine doing the authentication
	    if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
    	{
#ifdef WSC_AP_SUPPORT
            /*
                In WPA-PSK mode,
                If Association Request of station has RSN/SSN, WPS AP Must Not send EAP-Request/Identity to station 
                no matter WPS AP does receive EAPoL-Start from STA or not.
                Marvell WPS test bed(v2.1.1.5) will send AssocReq with WPS IE and RSN/SSN IE.
            */
            if (pEntry->bWscCapable || (RSNIE_Len == 0))
            {
                RTMPSetTimer(&pEntry->EnqueueEapolStartTimerForWsc, WSC_EAPOL_START_TIME_OUT);
				pEntry->EnqueueEapolStartTimerForWscRunning = TRUE;
                DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - IF(ra%d) This is a WPS Client.\n\n", pEntry->apidx));
                return;
            }
            else
            {
                pEntry->bWscCapable = FALSE;
                pEntry->Receive_EapolStart_EapRspId = (WSC_ENTRY_GET_EAPOL_START | WSC_ENTRY_GET_EAP_RSP_ID);
                // This STA is not a WPS STA
                pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;
                NdisZeroMemory(pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryAddr, 6);
            }
#endif // WSC_AP_SUPPORT //

			// Enqueue a EAPOL-start message with the pEntry for WPAPSK State Machine
			if ((pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE)
#ifdef WSC_AP_SUPPORT
				&& !pEntry->bWscCapable
#endif // WSC_AP_SUPPORT //
				)
			{
        		// Enqueue a EAPOL-start message with the pEntry
        		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_PSK;	
        		RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);				
			}
    	}
		else if (isReassoc && (pEntry->AuthMode == Ndis802_11AuthModeWPA2) && (RSNIE_Len == 40/*38*/))
		{// Key cache
	    	INT	CacheIdx;
	    	
	    	if (((CacheIdx = RTMPSearchPMKIDCache(pAd, pEntry->apidx, pEntry->Addr)) != -1) 
					&& (RTMPEqualMemory((RSN_IE + 24/*22*/), &pAd->ApCfg.MBSSID[pEntry->apidx].PMKIDCache.BSSIDInfo[CacheIdx].PMKID, LEN_PMKID)))
	    	{
		    	// Enqueue a EAPOL-start message with the pEntry for WPAPSK State Machine
				if ((pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE)
#ifdef WSC_AP_SUPPORT
					&& !pEntry->bWscCapable
#endif // WSC_AP_SUPPORT //
					)
				{
		    		// Enqueue a EAPOL-start message with the pEntry
    	    		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_PSK;	
        			RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);				
				}				

		    	pEntry->PMKID_CacheIdx = CacheIdx;
		    	DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - 2.PMKID matched and start key cache algorithm\n"));
	    	}
	    	else
	    	{
	    		pEntry->PMKID_CacheIdx = ENTRY_NOT_FOUND;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - 2.PMKID not found \n"));
				//DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - 2.Recv PMKID=%02x:%02x:%02x:%02x:%02x:%02x\n", *(RSN_IE+22),*(RSN_IE+23),*(RSN_IE+24),*(RSN_IE+25),*(RSN_IE+26),*(RSN_IE+27)));
	    	}
#ifdef WSC_AP_SUPPORT
            if ((pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode != WSC_DISABLE) &&
                (pEntry->apidx == MAIN_MBSSID) && 
                MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr))
            {
                if (!pEntry->EnqueueEapolStartTimerForWscRunning)
                {
                    pEntry->EnqueueEapolStartTimerForWscRunning = TRUE;
                    RTMPSetTimer(&pEntry->EnqueueEapolStartTimerForWsc, WSC_EAPOL_START_TIME_OUT);
                }
            }
#endif // WSC_AP_SUPPORT //
	    }
		else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || 
				 (pEntry->AuthMode == Ndis802_11AuthModeWPA2) || 
				 (
#ifdef WSC_AP_SUPPORT
					(!pEntry->bWscCapable) && 
#endif // WSC_AP_SUPPORT //
					(pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X)))
		{
			// Enqueue a EAPOL-start message to trigger EAP SM
			if (pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE)				
			{        		
        		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_1X;	
        		RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);				
			}
		}		
#ifdef WSC_AP_SUPPORT
        else
        {
            /*
                In WEP mode (no 802.1X indicated in beacon),
                Preferably, EAP-Request/Identity should not be sent unless STA first sends EAPoL-Start.
                Therefore, when WPS AP is configured as static WEP mode, driver would not enqueue EAPoL-Start.
            */
            if ((pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode != WSC_DISABLE) &&
                (pAd->ApCfg.MBSSID[MAIN_MBSSID].IEEE8021X || (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus != Ndis802_11WEPEnabled)) &&
                (pEntry->apidx == MAIN_MBSSID) && 
                MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EntryAddr))
            {
                if (!pEntry->EnqueueEapolStartTimerForWscRunning)
                {
                    pEntry->EnqueueEapolStartTimerForWscRunning = TRUE;
                    RTMPSetTimer(&pEntry->EnqueueEapolStartTimerForWsc, WSC_EAPOL_START_TIME_OUT);
                }
            }
        }
#endif // WSC_AP_SUPPORT //
	}

	
}



/*
    ==========================================================================
    Description:
        peer assoc req handling procedure
    Parameters:
        Adapter - Adapter pointer
        Elem - MLME Queue Element
    Pre:
        the station has been authenticated and the following information is stored
    Post  :
        -# An association response frame is generated and sent to the air
    ==========================================================================
 */
VOID APPeerAssocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
	ap_cmm_peer_assoc_req_action(pAd, Elem, 0);
}

/*
    ==========================================================================
    Description:
        mlme reassoc req handling procedure
    Parameters:
        Elem - 
    Pre:
        -# SSID  (Adapter->ApCfg.ssid[])
        -# BSSID (AP address, Adapter->ApCfg.bssid)
        -# Supported rates (Adapter->ApCfg.supported_rates[])
        -# Supported rates length (Adapter->ApCfg.supported_rates_len)
        -# Tx power (Adapter->ApCfg.tx_power)
    ==========================================================================
 */
VOID APPeerReassocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
	ap_cmm_peer_assoc_req_action(pAd, Elem, 1);
}

/*
    ==========================================================================
    Description:
        left part of IEEE 802.11/1999 p.374 
    Parameters:
        Elem - MLME message containing the received frame
    ==========================================================================
 */
VOID APPeerDisassocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
    UCHAR         Addr2[MAC_ADDR_LEN];
    USHORT        Reason;
    MAC_TABLE_ENTRY       *pEntry;

	DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - 1 receive DIS-ASSOC request \n"));
    if (! PeerDisassocReqSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, &Reason)) 
        return;

    DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - receive DIS-ASSOC request from %02x:%02x:%02x:%02x:%02x:%02x, reason=%d\n", Addr2[0],Addr2[1],Addr2[2],Addr2[3],Addr2[4],Addr2[5],Reason));
    
	pEntry = MacTableLookup(pAd, Addr2);

	if (pEntry == NULL)	
		return;
		
	if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
    {
		// Notify 802.1x daemon to clear this sta info
		IEEE8021X_L2_Disconnect_Frame_Send(pAd, pEntry);
	
		// send wireless event - for disassociation
		if (pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, Addr2, 0, 0); 
		
        ApLogEvent(pAd, Addr2, EVENT_DISASSOCIATED);
		MacTableDeleteEntry(pAd, Elem->Wcid, Addr2);
    }
}

/*
    ==========================================================================
    Description:
        Upper layer orders to disassoc s STA
    Parameters:
        Elem -
    ==========================================================================
 */
VOID APMlmeDisassocReqAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
    MLME_DISASSOC_REQ_STRUCT *DisassocReq;
    HEADER_802_11        DisassocHdr;
    PUCHAR                pOutBuffer = NULL;
    ULONG                 FrameLen = 0;
    NDIS_STATUS           NStatus;
    MAC_TABLE_ENTRY       *pEntry;

    DisassocReq = (MLME_DISASSOC_REQ_STRUCT *)(Elem->Msg);

	pEntry = MacTableLookup(pAd, DisassocReq->Addr);

	if (pEntry == NULL)
	{
		return;
	}

	ASSERT(pEntry->Aid == Elem->Wcid);

	if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
    {
		// send wireless event - for disassocation
		if (pAd->CommonCfg.bWirelessEvent)
			RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, DisassocReq->Addr, 0, 0);  
	
        ApLogEvent(pAd, DisassocReq->Addr, EVENT_DISASSOCIATED);
		MacTableDeleteEntry(pAd, Elem->Wcid, DisassocReq->Addr);
    }

    // 2. send out a DISASSOC request frame
    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS) 
        return;
    
    DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - MLME disassociates %02x:%02x:%02x:%02x:%02x:%02x; Send DISASSOC request\n",
        DisassocReq->Addr[0],DisassocReq->Addr[1],DisassocReq->Addr[2],
        DisassocReq->Addr[3],DisassocReq->Addr[4],DisassocReq->Addr[5]));
    MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, DisassocReq->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);
    MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
                      sizeof(HEADER_802_11), &DisassocHdr, 
                      2,                     &DisassocReq->Reason, 
                      END_OF_ARGS);
    MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
    MlmeFreeMemory(pAd, pOutBuffer);
}


/*
    ==========================================================================
    Description:
        right part of IEEE 802.11/1999 page 374
    Note: 
        This event should never cause ASSOC state machine perform state
        transition, and has no relationship with CNTL machine. So we separate
        this routine as a service outside of ASSOC state transition table.
    ==========================================================================
 */
VOID APCls3errAction(
    IN PRTMP_ADAPTER pAd, 
	IN 	ULONG Wcid,
    IN	PHEADER_802_11	pHeader) 
{
    HEADER_802_11         DisassocHdr;
    PUCHAR                pOutBuffer = NULL;
    ULONG                 FrameLen = 0;
    NDIS_STATUS           NStatus;
    USHORT                Reason = REASON_CLS3ERR;
    MAC_TABLE_ENTRY       *pEntry = NULL;

#if 0
    pEntry = MacTableLookup(pAd, pHeader->Addr2);
#else	
	if (Wcid < MAX_LEN_OF_MAC_TABLE)
	{
      pEntry = &(pAd->MacTab.Content[Wcid]);
	}
#endif	

    if (pEntry)
    {
        //ApLogEvent(pAd, pAddr, EVENT_DISASSOCIATED);
		MacTableDeleteEntry(pAd, pEntry->Aid, pHeader->Addr2);
    }
    
    // 2. send out a DISASSOC request frame
    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS) 
        return;
    
    DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - Class 3 Error, Send DISASSOC frame to %02x:%02x:%02x:%02x:%02x:%02x\n",
    		PRINT_MAC(pHeader->Addr2)));
    MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pHeader->Addr2, pHeader->Addr1);
    MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
                      sizeof(HEADER_802_11), &DisassocHdr, 
                      2,                     &Reason, 
                      END_OF_ARGS);
    MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
    MlmeFreeMemory(pAd, pOutBuffer);
}
 
/*
    ==========================================================================
    Description:
       assign a new AID to the newly associated/re-associated STA and
       decide its MaxSupportedRate and CurrTxRate. Both rates should not
       exceed AP's capapbility
    Return:
       MLME_SUCCESS - association successfully built
       others - association failed due to resource issue
    ==========================================================================
 */
USHORT APBuildAssociation(
    IN PRTMP_ADAPTER pAd,
    IN MAC_TABLE_ENTRY *pEntry,
    IN USHORT        CapabilityInfo,
    IN UCHAR         MaxSupportedRateIn500Kbps,
    IN UCHAR         *RSN,
    IN UCHAR         *pRSNLen,
    IN BOOLEAN       bWmmCapable,
    IN ULONG         ClientRalinkIe,
	IN HT_CAPABILITY_IE		*pHtCapability,
	IN UCHAR		 HtCapabilityLen,
    OUT USHORT       *pAid)
{
    USHORT           StatusCode = MLME_SUCCESS;
    UCHAR            MaxSupportedRate = RATE_11;
//	UCHAR 		CipherAlg;
//	UCHAR 		KeyIdx;

    switch (MaxSupportedRateIn500Kbps)
    {
        case 108: MaxSupportedRate = RATE_54;   break;
        case 96:  MaxSupportedRate = RATE_48;   break;
        case 72:  MaxSupportedRate = RATE_36;   break;
        case 48:  MaxSupportedRate = RATE_24;   break;
        case 36:  MaxSupportedRate = RATE_18;   break;
        case 24:  MaxSupportedRate = RATE_12;   break;
        case 18:  MaxSupportedRate = RATE_9;    break;
        case 12:  MaxSupportedRate = RATE_6;    break;
        case 22:  MaxSupportedRate = RATE_11;   break;
        case 11:  MaxSupportedRate = RATE_5_5;  break;
        case 4:   MaxSupportedRate = RATE_2;    break;
        case 2:   MaxSupportedRate = RATE_1;    break;
        default:  MaxSupportedRate = RATE_11;   break;
    }

    if ((pAd->CommonCfg.PhyMode == PHY_11G) && (MaxSupportedRate < RATE_FIRST_OFDM_RATE))
        return MLME_ASSOC_REJ_DATA_RATE;

	// 11n only
	if (((pAd->CommonCfg.PhyMode == PHY_11N_2_4G) || (pAd->CommonCfg.PhyMode == PHY_11N_5G))&& (HtCapabilityLen == 0))
		return MLME_ASSOC_REJ_DATA_RATE;

    if (!pEntry)
        return MLME_UNSPECIFY_FAIL;

    if (pEntry && ((pEntry->Sst == SST_AUTH) || (pEntry->Sst == SST_ASSOC)))
    {
        // TODO:
        // should qualify other parameters, for example - capablity, supported rates, listen interval, ... etc
        // to decide the Status Code
        //*pAid = APAssignAid(pAd, pEntry);
        //pEntry->Aid = *pAid;
        *pAid = pEntry->Aid;
		pEntry->NoDataIdleCount = 0;
        
        NdisMoveMemory(pEntry->RSN_IE, RSN, *pRSNLen);
        pEntry->RSNIE_Len = *pRSNLen;
        DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - RSNIE_Len = 0x%x, RSNIE = 0x%02x:0x%02x:0x%02x:0x%02x\n", 
            pEntry->RSNIE_Len, pEntry->RSN_IE[0], pEntry->RSN_IE[1], pEntry->RSN_IE[2], pEntry->RSN_IE[3]));

		DBGPRINT(RT_DEBUG_INFO, ("ASSOC - MaxSupportedRate = %x, pAd->CommonCfg.MaxTxRate = %x\n", 
			 MaxSupportedRate, pAd->CommonCfg.MaxTxRate));
		if (*pAid == 0)
			StatusCode = MLME_ASSOC_REJ_UNABLE_HANDLE_STA;
		else if ((pEntry->RSNIE_Len == 0) && (pAd->ApCfg.MBSSID[pEntry->apidx].AuthMode >= Ndis802_11AuthModeWPA))
        {      
#ifdef WSC_AP_SUPPORT
            if (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE && 
                pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx == WSC_INIT_ENTRY_APIDX)
            {
                WscInitEntryFunc(pEntry);
                pEntry->Sst = SST_ASSOC;
                StatusCode = MLME_SUCCESS;
                // In WPA or 802.1x mode, the port is not secured.
    			if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) || (pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))	
    				pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
    			else
    				pEntry->PortSecured = WPA_802_1X_PORT_SECURED;

                if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
    			{
    				pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
    				pEntry->WpaState = AS_INITPSK;
    			}
    			else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2)
    					|| (pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))
    			{
    				pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
    				pEntry->WpaState = AS_AUTHENTICATION;
    			}
            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - WSC_STATE_MACHINE for this STA is OFF.<WscConfMode = %d, EntryApIdx =%d>\n", 
                                         pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode,
                                         pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx));
                StatusCode = MLME_ASSOC_DENY_OUT_SCOPE;
            }
#else  // WSC_AP_SUPPORT //
			StatusCode = MLME_ASSOC_DENY_OUT_SCOPE;
#endif // WSC_AP_SUPPORT //
        }
		else
		{
			// Update auth, wep, legacy transmit rate setting . 
			pEntry->Sst = SST_ASSOC;

			// patch for Nintendo DS support rate bug - it only support tx rate 1 and 2 
			// For this client, AP always transmits low rate(1Mbps) packets.
            if ((((pEntry->Addr[0]==0x00) && (pEntry->Addr[1]==0x09) && (pEntry->Addr[2]==0xBF)) || 
            	 ((pEntry->Addr[0]==0x00) && (pEntry->Addr[1]==0x16) && (pEntry->Addr[2]==0x56))) 
            		&& MaxSupportedRate == RATE_11)
            { 
            	DBGPRINT(RT_DEBUG_TRACE, ("==>Assoc-Req from Nintendo DS client.\n"));
            	MaxSupportedRate = RATE_1;
            }
			
			pEntry->MaxSupportedRate = min(pAd->CommonCfg.MaxTxRate, MaxSupportedRate);
			if (pEntry->MaxSupportedRate < RATE_FIRST_OFDM_RATE)
			{
				pEntry->MaxHTPhyMode.field.MODE = MODE_CCK;
				pEntry->MaxHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
				pEntry->MinHTPhyMode.field.MODE = MODE_CCK;
				pEntry->MinHTPhyMode.field.MCS = pEntry->MaxSupportedRate;
				pEntry->HTPhyMode.field.MODE = MODE_CCK;
				pEntry->HTPhyMode.field.MCS = pEntry->MaxSupportedRate;
			}
			else
			{
				pEntry->MaxHTPhyMode.field.MODE = MODE_OFDM;
				pEntry->MaxHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
				pEntry->MinHTPhyMode.field.MODE = MODE_OFDM;
				pEntry->MinHTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
				pEntry->HTPhyMode.field.MODE = MODE_OFDM;
				pEntry->HTPhyMode.field.MCS = OfdmRateToRxwiMCS[pEntry->MaxSupportedRate];
			}
			pEntry->CapabilityInfo = CapabilityInfo;
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
			{
				pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
				pEntry->WpaState = AS_INITPSK;
			}
			else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2)
					|| (pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))
			{
				pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
				pEntry->WpaState = AS_AUTHENTICATION;
			}
            
			if (bWmmCapable)
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}
			else
			{
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			}

			//if (ClientRalinkIe & 0x00000004)
			if (ClientRalinkIe != 0x0)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
			else
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
			

			// Ralink proprietary Piggyback and Aggregation support for legacy RT61 chip
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
#ifdef AGGREGATION_SUPPORT
			if ((pAd->CommonCfg.bAggregationCapable) && (ClientRalinkIe & 0x00000001))
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC -RaAggregate= 1\n"));
			}
#endif // AGGREGATION_SUPPORT //
#ifdef PIGGYBACK_SUPPORT
			if ((pAd->CommonCfg.bPiggyBackCapable) && (ClientRalinkIe & 0x00000002))
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC -PiggyBack= 1\n"));
			}
#endif // PIGGYBACK_SUPPORT //

#if 0
			// If dynamic rate switching is enabled, starts from a more reliable rate to 
			// increase STA's DHCP succeed rate
			if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
			{
				pEntry->CurrTxRate = min(pEntry->MaxSupportedRate, RATE_11); 
			}
			else
#endif				  
			// In WPA or 802.1x mode, the port is not secured, otherwise is secued.
			if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) || (pAd->ApCfg.MBSSID[pEntry->apidx].IEEE8021X == TRUE))	
				pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
			else
				pEntry->PortSecured = WPA_802_1X_PORT_SECURED;

			// If this Entry supports 802.11n, upgrade to HT rate. 
			if ((HtCapabilityLen != 0) && (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
			{
				UCHAR	j, bitmask; //k,bitmask;
				CHAR    i;

				if ((pHtCapability->HtCapInfo.GF) && (pAd->CommonCfg.DesiredHtPhy.GF))
				{
					pEntry->MaxHTPhyMode.field.MODE = MODE_HTGREENFIELD;
				}
				else
				{	
					pEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
					pAd->MacTab.fAnyStationNonGF = TRUE;
					pAd->CommonCfg.AddHTInfo.AddHtInfo2.NonGfPresent = 1;
				}

				// 40Mhz BSS Width Trigger events
				if (pHtCapability->HtCapInfo.Forty_Mhz_Intolerant)
				{
					Handle_BSS_Width_Trigger_Events(pAd);					
				}

				if ((pHtCapability->HtCapInfo.ChannelWidth) && (pAd->CommonCfg.DesiredHtPhy.ChannelWidth))
				{
					pEntry->MaxHTPhyMode.field.BW= BW_40;
					pEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor40)&(pHtCapability->HtCapInfo.ShortGIfor40));
				}
				else
				{	
					pEntry->MaxHTPhyMode.field.BW = BW_20;
					pEntry->MaxHTPhyMode.field.ShortGI = ((pAd->CommonCfg.DesiredHtPhy.ShortGIfor20)&(pHtCapability->HtCapInfo.ShortGIfor20));
					pAd->MacTab.fAnyStation20Only = TRUE;
				}
				
				// find max fixed rate
				for (i=15; i>=0; i--)
				{	
					j = i/8;	
					bitmask = (1<<(i-(j*8)));
					if ((pAd->ApCfg.MBSSID[pEntry->apidx].DesiredHtPhyInfo.MCSSet[j] & bitmask) && (pHtCapability->MCSSet[j] & bitmask))
					{
						pEntry->MaxHTPhyMode.field.MCS = i;
						break;
					}
					if (i==0)
						break;
				}

				 
				if (pAd->ApCfg.MBSSID[pEntry->apidx].DesiredTransmitSetting.field.MCS != MCS_AUTO)
				{

					printk("@@@ IF-ra%d DesiredTransmitSetting.field.MCS = %d\n", pEntry->apidx,
						pAd->ApCfg.MBSSID[pEntry->apidx].DesiredTransmitSetting.field.MCS);
					if (pAd->ApCfg.MBSSID[pEntry->apidx].DesiredTransmitSetting.field.MCS == 32)
					{
						// Fix MCS as HT Duplicated Mode
						pEntry->MaxHTPhyMode.field.BW = 1;
						pEntry->MaxHTPhyMode.field.MODE = MODE_HTMIX;
						pEntry->MaxHTPhyMode.field.STBC = 0;
						pEntry->MaxHTPhyMode.field.ShortGI = 0;
						pEntry->MaxHTPhyMode.field.MCS = 32;
					}
					else if (pEntry->MaxHTPhyMode.field.MCS > pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.MCS)
					{
						// STA supports fixed MCS 
						pEntry->MaxHTPhyMode.field.MCS = pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.MCS;
					}
				}

				pEntry->MaxHTPhyMode.field.STBC = (pHtCapability->HtCapInfo.RxSTBC & (pAd->CommonCfg.DesiredHtPhy.TxSTBC));
				pEntry->MpduDensity = pHtCapability->HtCapParm.MpduDensity;
				pEntry->MaxRAmpduFactor = pHtCapability->HtCapParm.MaxRAmpduFactor;
				pEntry->MmpsMode = (UCHAR)pHtCapability->HtCapInfo.MimoPs;
				pEntry->AMsduSize = (UCHAR)pHtCapability->HtCapInfo.AMsduSize;				
				pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

				if (pAd->CommonCfg.DesiredHtPhy.AmsduEnable)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED);
				if (pHtCapability->HtCapInfo.ShortGIfor20)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI20_CAPABLE);
				if (pHtCapability->HtCapInfo.ShortGIfor40)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI40_CAPABLE);
				if (pHtCapability->HtCapInfo.TxSTBC)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_TxSTBC_CAPABLE);
				if (pHtCapability->HtCapInfo.RxSTBC)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RxSTBC_CAPABLE);
				if (pHtCapability->ExtHtCapInfo.PlusHTC)				
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_HTC_CAPABLE);
				if (pAd->CommonCfg.bRdg && pHtCapability->ExtHtCapInfo.RDGSupport)				
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RDG_CAPABLE);	
				if (pHtCapability->ExtHtCapInfo.MCSFeedback == 0x03)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_MCSFEEDBACK_CAPABLE);		
			}
			else
			{
				pAd->MacTab.fAnyStationIsLegacy = TRUE;
			}				

			pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

			NdisMoveMemory(&pEntry->HTCapability, pHtCapability, sizeof(HT_CAPABILITY_IE));

			pEntry->CurrTxRate = pEntry->MaxSupportedRate;
			
			// Set asic auto fall back
			if (pAd->ApCfg.MBSSID[pEntry->apidx].bAutoTxRateSwitch == TRUE)
			{
				PUCHAR					pTable;
				UCHAR					TableSize = 0;
				
				APMlmeSelectTxRateTable(pAd, pEntry, &pTable, &TableSize, &pEntry->CurrTxRateIndex);
				// don't need to update these register
				//AsicUpdateAutoFallBackTable(pAd, pTable);

				pEntry->bAutoTxRateSwitch = TRUE;
			}
			else
			{
				pEntry->HTPhyMode.field.MODE	= pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.MODE;
				pEntry->HTPhyMode.field.MCS	= pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.MCS;
				pEntry->bAutoTxRateSwitch = FALSE;
				
				// If the legacy mode is set, overwrite the transmit setting of this entry.  			
				RTMPUpdateLegacyTxSetting((UCHAR)pAd->ApCfg.MBSSID[pEntry->apidx].DesiredTransmitSetting.field.FixedTxMode, pEntry);

				// Use STA Capability
#if 0
				pEntry->HTPhyMode.field.BW		= pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.BW;
				pEntry->HTPhyMode.field.STBC	= pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.STBC;
				pEntry->HTPhyMode.field.ShortGI	= pAd->ApCfg.MBSSID[pEntry->apidx].HTPhyMode.field.ShortGI;
#endif
			}


			if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
				ApLogEvent(pAd, pEntry->Addr, EVENT_ASSOCIATED);
			
			APUpdateCapabilityAndErpIe(pAd);
			APUpdateOperationMode(pAd);
	   
			pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;
			
#ifdef WSC_AP_SUPPORT
            if ((pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE) && 
                (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx == WSC_INIT_ENTRY_APIDX))
            {
                WscInitEntryFunc(pEntry);
            }
            else
                DBGPRINT(RT_DEBUG_TRACE, ("**) WSC_STATE_MACHINE for this STA is OFF.<WscConfMode = %d, EntryApIdx =%d>\n", 
                                         pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode,
                                         pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx));
#endif // WSC_AP_SUPPORT //
            
			StatusCode = MLME_SUCCESS;
		}
	}
	else // CLASS 3 error should have been handled beforehand; here should be MAC table full
		StatusCode = MLME_ASSOC_REJ_UNABLE_HANDLE_STA;

    return StatusCode;
}

static void ap_assoc_info_debugshow(
	IN	PRTMP_ADAPTER		pAd,
	IN	BOOLEAN				isReassoc,
	IN	MAC_TABLE_ENTRY 	*pEntry,
	IN  UCHAR				HTCapability_Len,
	IN	HT_CAPABILITY_IE	*pHTCapability)
{
#ifdef DBG
	PUCHAR	sAssoc = isReassoc ? "ReASSOC" : "ASSOC";
#endif // DBG //
	HT_CAP_INFO			*pHTCap;
	HT_CAP_PARM			*pHTCapParm;
	EXT_HT_CAP_INFO		*pExtHT;



	DBGPRINT(RT_DEBUG_TRACE, ("%s - \n\tAssign AID=%d to STA %02x:%02x:%02x:%02x:%02x:%02x\n", 
		sAssoc, pEntry->Aid, PRINT_MAC(pEntry->Addr)));
		
	//DBGPRINT(RT_DEBUG_TRACE, (HTCapability_Len ? "%s - 11n HT STA\n" : "%s - legacy STA\n", sAssoc));

	if (HTCapability_Len && (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{	
		ASSERT(pHTCapability);

		pHTCap = &pHTCapability->HtCapInfo;
		pHTCapParm = &pHTCapability->HtCapParm;
		pExtHT = &pHTCapability->ExtHtCapInfo;

		DBGPRINT(RT_DEBUG_TRACE, ("%s - 11n HT STA\n", sAssoc));
		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t AdvCode(%d), BW(%d), MIMOPS(%d), GF(%d), ShortGI_20(%d), ShortGI_40(%d)\n", 
			pHTCap->AdvCoding, pHTCap->ChannelWidth, pHTCap->MimoPs, pHTCap->GF,
			pHTCap->ShortGIfor20, pHTCap->ShortGIfor40));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t TxSTBC(%d), RxSTBC(%d), DelayedBA(%d), A-MSDU(%d), CCK_40(%d)\n", 
			pHTCap->TxSTBC, pHTCap->RxSTBC, pHTCap->DelayedBA, pHTCap->AMsduSize, pHTCap->CCKmodein40));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t PSMP(%d), Forty_Mhz_Intolerant(%d), L-SIG(%d)\n", 
			pHTCap->PSMP, pHTCap->Forty_Mhz_Intolerant, pHTCap->LSIGTxopProSup));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Parm Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t MaxRx A-MPDU Factor(%d), MPDU Density(%d)\n",
			pHTCapParm->MaxRAmpduFactor, pHTCapParm->MpduDensity));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT MCS set: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t %02x %02x %02x %02x %02x\n", pHTCapability->MCSSet[0], 
			pHTCapability->MCSSet[1], pHTCapability->MCSSet[2],
			pHTCapability->MCSSet[3], pHTCapability->MCSSet[4]));

        DBGPRINT(RT_DEBUG_TRACE, ("\tExt HT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t PCO(%d), TransTime(%d), MCSFeedback(%d), +HTC(%d), RDG(%d)\n",
			pExtHT->Pco, pExtHT->TranTime, pExtHT->MCSFeedback, pExtHT->PlusHTC, pExtHT->RDGSupport));

		DBGPRINT(RT_DEBUG_TRACE, ("\n%s - MODE=%d, BW=%d, MCS=%d, ShortGI=%d, MaxRxFactor=%d, MpduDensity=%d, MIMOPS=%d, AMSDU=%d\n",
			sAssoc,
			pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, 
			pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.ShortGI,
			pEntry->MaxRAmpduFactor, pEntry->MpduDensity,
			pEntry->MmpsMode, pEntry->AMsduSize));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - legacy STA\n", sAssoc));
		DBGPRINT(RT_DEBUG_TRACE, ("\n%s - MaxSupRate=%d Mbps, CurrTxRate=%d Mbps\n", sAssoc,
								  RateIdToMbps[pEntry->MaxSupportedRate], RateIdToMbps[pEntry->CurrTxRate]));
	}

	DBGPRINT(RT_DEBUG_TRACE, ("\tAuthMode=%d, WepStatus=%d, WpaState=%d, GroupKeyWepStatus=%d\n",
		pEntry->AuthMode, pEntry->WepStatus, pEntry->WpaState, pAd->ApCfg.MBSSID[pEntry->apidx].GroupKeyWepStatus));

	DBGPRINT(RT_DEBUG_TRACE, ("\tWMMCapable=%d, Legacy AGGRE=%d, PiggyBack=%d, RDG=%d, TxAMSDU=%d\n", 
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE), 							  
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE), 
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RDG_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED)));

	DBGPRINT(RT_DEBUG_TRACE, ("\n%s - Update AP OperaionMode=%d , fAnyStationIsLegacy=%d, fAnyStation20Only=%d, fAnyStationNonGF=%d\n\n", 
		sAssoc, pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, pAd->MacTab.fAnyStationIsLegacy, 
		pAd->MacTab.fAnyStation20Only, pAd->MacTab.fAnyStationNonGF));
}


