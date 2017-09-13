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
	action.c
 
    Abstract:
    Handle association related requests either from WSTA or from local MLME
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
	Jan Lee		2006	  	created for rt2860
 */

#include "rt_config.h"
#include "action.h"


static VOID ReservedAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

/*  
    ==========================================================================
    Description: 
        association state machine init, including state transition and timer init
    Parameters: 
        S - pointer to the association state machine
    Note:
        The state machine looks like the following 
        
                                    ASSOC_IDLE             
        MT2_MLME_DISASSOC_REQ    mlme_disassoc_req_action 
        MT2_PEER_DISASSOC_REQ    peer_disassoc_action     
        MT2_PEER_ASSOC_REQ       drop                     
        MT2_PEER_REASSOC_REQ     drop                     
        MT2_CLS3ERR              cls3err_action           
    ==========================================================================
 */
VOID ActionStateMachineInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN  STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]) 
{
	StateMachineInit(S, (STATE_MACHINE_FUNC *)Trans, MAX_ACT_STATE, MAX_ACT_MSG, (STATE_MACHINE_FUNC)Drop, ACT_IDLE, ACT_MACHINE_BASE);

	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_SPECTRUM_CATE, (STATE_MACHINE_FUNC)PeerSpectrumAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_QOS_CATE, (STATE_MACHINE_FUNC)PeerQOSAction);

	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_DLS_CATE, (STATE_MACHINE_FUNC)ReservedAction);
#ifdef CONFIG_AP_SUPPORT
#ifdef QOS_DLS_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		StateMachineSetAction(S, ACT_IDLE, MT2_PEER_DLS_CATE, (STATE_MACHINE_FUNC)PeerDLSAction);
	}
#endif // QOS_DLS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_BA_CATE, (STATE_MACHINE_FUNC)PeerBAAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_RM_CATE, (STATE_MACHINE_FUNC)ReservedAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_BSSTRANS_CATE, (STATE_MACHINE_FUNC)ReservedAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_HT_CATE, (STATE_MACHINE_FUNC)PeerHTAction);
	
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_ADD_BA_CATE, (STATE_MACHINE_FUNC)MlmeADDBAAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_ORI_DELBA_CATE, (STATE_MACHINE_FUNC)MlmeDELBAAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_REC_DELBA_CATE, (STATE_MACHINE_FUNC)MlmeDELBAAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_QOS_CATE, (STATE_MACHINE_FUNC)MlmeQOSAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_DLS_CATE, (STATE_MACHINE_FUNC)MlmeDLSAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_ACT_INVALID, (STATE_MACHINE_FUNC)MlmeInvalidAction);
}

VOID MlmeADDBAAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 

{
	MLME_ADDBA_REQ_STRUCT *pInfo;
	UCHAR           Addr[6];
	PUCHAR         pOutBuffer = NULL;
	NDIS_STATUS     NStatus;
	ULONG		Idx;
	FRAME_ADDBA_REQ  Frame;
	ULONG		FrameLen;
	BA_ORI_ENTRY			*pBAEntry = NULL;
#ifdef CONFIG_AP_SUPPORT
	UCHAR			apidx;
#endif // CONFIG_AP_SUPPORT //

	pInfo = (MLME_ADDBA_REQ_STRUCT *)Elem->Msg;
	NdisZeroMemory(&Frame, sizeof(FRAME_ADDBA_REQ));
	
	if(MlmeAddBAReqSanity(pAd, Elem->Msg, Elem->MsgLen, Addr)) 
	{
		NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  //Get an unused nonpaged memory
		if(NStatus != NDIS_STATUS_SUCCESS) 
		{
			DBGPRINT(RT_DEBUG_TRACE,("BA - MlmeADDBAAction() allocate memory failed \n"));
			return;
		}
		// 1. find entry
		Idx = pAd->MacTab.Content[pInfo->Wcid].BAOriWcidArray[pInfo->TID];
		if (Idx == 0)
		{
			MlmeFreeMemory(pAd, pOutBuffer);
			DBGPRINT(RT_DEBUG_ERROR,("BA - MlmeADDBAAction() can't find BAOriEntry \n"));
			return;
		} 
		else
		{
			pBAEntry =&pAd->BATable.BAOriEntry[Idx];
		}
		
#ifdef MESH_SUPPORT
		if (pAd->MacTab.Content[pInfo->Wcid].ValidAsMesh)
		{
			ActHeaderInit(pAd, &Frame.Hdr, pInfo->pAddr, pAd->MeshTab.CurrentAddress, pInfo->pAddr);		
		}
		else
#endif // MESH_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
#ifdef APCLI_SUPPORT
			if (pAd->MacTab.Content[pInfo->Wcid].ValidAsApCli)
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].MatchAPCLITabIdx;
				ActHeaderInit(pAd, &Frame.Hdr, pInfo->pAddr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, pInfo->pAddr);		
			}
			else
#endif // APCLI_SUPPORT //
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].apidx;
				ActHeaderInit(pAd, &Frame.Hdr, pInfo->pAddr, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].Bssid);
			}
		}
#endif // CONFIG_AP_SUPPORT //

		Frame.Category = CATEGORY_BA;
		Frame.Action = ADDBA_REQ;
		Frame.BaParm.AMSDUSupported = 0;
		Frame.BaParm.BAPolicy = IMMED_BA;
		Frame.BaParm.TID = pInfo->TID;
		Frame.BaParm.BufSize = pInfo->BaBufSize;
		Frame.Token = pInfo->Token;
		Frame.TimeOutValue = pInfo->TimeOutValue;
		Frame.BaStartSeq.field.FragNum = 0;
		Frame.BaStartSeq.field.StartSeq = pAd->MacTab.Content[pInfo->Wcid].TxSeq[pInfo->TID];

		*(USHORT *)(&Frame.BaParm) = cpu2le16(*(USHORT *)(&Frame.BaParm));
		Frame.TimeOutValue = cpu2le16(Frame.TimeOutValue);
		Frame.BaStartSeq.word = cpu2le16(Frame.BaStartSeq.word);

		MakeOutgoingFrame(pOutBuffer,		   &FrameLen,
		              sizeof(FRAME_ADDBA_REQ), &Frame,
		              END_OF_ARGS);
		MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, pOutBuffer);
		
		DBGPRINT(RT_DEBUG_TRACE, ("BA - Send ADDBA request. StartSeq = %x,  FrameLen = %ld. BufSize = %d\n", Frame.BaStartSeq.field.StartSeq, FrameLen, Frame.BaParm.BufSize));
    }
}

/*
    ==========================================================================
    Description:
        send DELBA and delete BaEntry if any
    Parametrs:
        Elem - MLME message MLME_DELBA_REQ_STRUCT
        
	IRQL = DISPATCH_LEVEL

    ==========================================================================
 */
VOID MlmeDELBAAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
	MLME_DELBA_REQ_STRUCT *pInfo;
	PUCHAR         pOutBuffer = NULL;
	PUCHAR		   pOutBuffer2 = NULL;
	NDIS_STATUS     NStatus;
	ULONG		Idx;
	FRAME_DELBA_REQ  Frame;
	ULONG		FrameLen;
	FRAME_BAR	FrameBar;
#ifdef CONFIG_AP_SUPPORT
	UCHAR		apidx;
#endif // CONFIG_AP_SUPPORT //
	
	pInfo = (MLME_DELBA_REQ_STRUCT *)Elem->Msg;	
	// must send back DELBA 
	NdisZeroMemory(&Frame, sizeof(FRAME_DELBA_REQ));
	
	if(MlmeDelBAReqSanity(pAd, Elem->Msg, Elem->MsgLen)) 
	{
		NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  //Get an unused nonpaged memory
		if(NStatus != NDIS_STATUS_SUCCESS) 
		{
			DBGPRINT(RT_DEBUG_TRACE,("BA - MlmeDELBAAction() allocate memory failed 1. \n"));
			return;
		}

		NStatus = MlmeAllocateMemory(pAd, &pOutBuffer2);  //Get an unused nonpaged memory
		if(NStatus != NDIS_STATUS_SUCCESS) 
		{
			MlmeFreeMemory(pAd, pOutBuffer);
			DBGPRINT(RT_DEBUG_TRACE,("BA - MlmeDELBAAction() allocate memory failed 2. \n"));
			return;
		}

		// SEND BAR (Send BAR to refresh peer reordering buffer.)
		Idx = pAd->MacTab.Content[pInfo->Wcid].BAOriWcidArray[pInfo->TID];
#ifdef MESH_SUPPORT
		if (pAd->MacTab.Content[pInfo->Wcid].ValidAsMesh)
		{
			BarHeaderInit(pAd, &FrameBar, pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->MeshTab.CurrentAddress);
		}	
		else
#endif // MESH_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
#ifdef APCLI_SUPPORT
			if (pAd->MacTab.Content[pInfo->Wcid].ValidAsApCli)
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].MatchAPCLITabIdx;
				BarHeaderInit(pAd, &FrameBar, pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress);
			}	
			else
#endif // APCLI_SUPPORT //
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].apidx;
				BarHeaderInit(pAd, &FrameBar, pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->ApCfg.MBSSID[apidx].Bssid);
			}
		}
#endif // CONFIG_AP_SUPPORT //


		FrameBar.StartingSeq.field.FragNum = 0; // make sure sequence not clear in DEL funciton.
		FrameBar.StartingSeq.field.StartSeq = pAd->MacTab.Content[pInfo->Wcid].TxSeq[pInfo->TID]; // make sure sequence not clear in DEL funciton.
		FrameBar.BarControl.TID = pInfo->TID; // make sure sequence not clear in DEL funciton.
		FrameBar.BarControl.ACKPolicy = IMMED_BA; // make sure sequence not clear in DEL funciton.
		FrameBar.BarControl.Compressed = 1; // make sure sequence not clear in DEL funciton.
		FrameBar.BarControl.MTID = 0; // make sure sequence not clear in DEL funciton.

		MakeOutgoingFrame(pOutBuffer2,				&FrameLen,
					  sizeof(FRAME_BAR),	  &FrameBar,
					  END_OF_ARGS);
		MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer2, FrameLen);
		MlmeFreeMemory(pAd, pOutBuffer2);
		DBGPRINT(RT_DEBUG_TRACE,("BA - MlmeDELBAAction() . Send BAR to refresh peer reordering buffer \n"));

		// SEND DELBA FRAME
		FrameLen = 0;
#ifdef MESH_SUPPORT
		if (pAd->MacTab.Content[pInfo->Wcid].ValidAsMesh)
		{
			ActHeaderInit(pAd, &Frame.Hdr, pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->MeshTab.CurrentAddress, pAd->MacTab.Content[pInfo->Wcid].Addr);
		}
		else
#endif // MESH_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
#ifdef APCLI_SUPPORT
			if (pAd->MacTab.Content[pInfo->Wcid].ValidAsApCli)
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].MatchAPCLITabIdx;
				ActHeaderInit(pAd, &Frame.Hdr, pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, pAd->MacTab.Content[pInfo->Wcid].Addr);		
			}
			else
#endif // APCLI_SUPPORT //
			{
				apidx = pAd->MacTab.Content[pInfo->Wcid].apidx;
				ActHeaderInit(pAd, &Frame.Hdr,  pAd->MacTab.Content[pInfo->Wcid].Addr, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].Bssid);
			}
		}
#endif // CONFIG_AP_SUPPORT //
		Frame.Category = CATEGORY_BA;
		Frame.Action = DELBA;
		Frame.DelbaParm.Initiator = pInfo->Initiator;
		Frame.DelbaParm.TID = pInfo->TID;
		Frame.ReasonCode = 39; // Time Out
		*(USHORT *)(&Frame.DelbaParm) = cpu2le16(*(USHORT *)(&Frame.DelbaParm));
		Frame.ReasonCode = cpu2le16(Frame.ReasonCode);
		
		MakeOutgoingFrame(pOutBuffer,               &FrameLen,
		              sizeof(FRAME_DELBA_REQ),    &Frame,
		              END_OF_ARGS);
		MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, pOutBuffer);
		DBGPRINT(RT_DEBUG_TRACE,("BA - MlmeDELBAAction() . 3 DELBA sent. \n"));
    	}
}

VOID MlmeQOSAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
}

VOID MlmeDLSAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
}

VOID MlmeInvalidAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
	//PUCHAR		   pOutBuffer = NULL;
	//Return the receiving frame except the MSB of category filed set to 1.  7.3.1.11
}

VOID PeerSpectrumAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{

	UCHAR	Action = Elem->Msg[LENGTH_802_11+1];
	
	switch(Action)
	{
		case SPEC_CHANNEL_SWITCH:
		{
//			FRAME_SPETRUM_CS	FrameCS;
			break;
		}
	}
// unknown category field value received.
// return to source with most significant bit set to 1 if received packet's MSB = 0
}

VOID PeerQOSAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
}

#ifdef CONFIG_AP_SUPPORT
#ifdef QOS_DLS_SUPPORT
VOID PeerDLSAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR	Action = Elem->Msg[LENGTH_802_11+1];

	switch(Action)
	{
		case ACTION_DLS_REQUEST:
			PeerDlsReqAction(pAd, Elem);
			break;

		case ACTION_DLS_RESPONSE:
			PeerDlsRspAction(pAd, Elem);
			break;

		case ACTION_DLS_TEARDOWN:
			PeerDlsTearDownAction(pAd, Elem);
			break;
	}
}
#endif // QOS_DLS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

VOID PeerBAAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR	Action = Elem->Msg[LENGTH_802_11+1];
	
	switch(Action)
	{
		case ADDBA_REQ:
			PeerAddBAReqAction(pAd,Elem);
			break;
		case ADDBA_RESP:
			PeerAddBARspAction(pAd,Elem);
			break;
		case DELBA:
			PeerDelBAAction(pAd,Elem);
			break;
	}
}

static VOID ReservedAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	UCHAR Category;

	if (Elem->MsgLen <= LENGTH_802_11)
	{
		return;
	}

	Category = Elem->Msg[LENGTH_802_11];
	DBGPRINT(RT_DEBUG_TRACE,("Rcv reserved category(%d) Action Frame\n", Category));
	hex_dump("Reserved Action Frame", &Elem->Msg[0], Elem->MsgLen);
}


VOID PeerRMAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 

{

}

static VOID respond_ht_information_exchange_action(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem) 
{
	PUCHAR			pOutBuffer = NULL;
	NDIS_STATUS		NStatus;
	ULONG			FrameLen;
#ifdef CONFIG_AP_SUPPORT
	INT         	apidx;
#endif // CONFIG_AP_SUPPORT //
	FRAME_HT_INFO	HTINFOframe, *pFrame;
	UCHAR   		*pAddr;


	// 2. Always send back ADDBA Response 
	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);	 //Get an unused nonpaged memory

	if (NStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE,("ACTION - respond_ht_information_exchange_action() allocate memory failed \n"));
		return;
	}

	// get RA
	pFrame = (FRAME_HT_INFO *) &Elem->Msg[0];
	pAddr = pFrame->Hdr.Addr2;

	NdisZeroMemory(&HTINFOframe, sizeof(FRAME_HT_INFO));
	// 2-1. Prepare ADDBA Response frame.
#ifdef MESH_SUPPORT
	if (pAd->MacTab.Content[Elem->Wcid].ValidAsMesh)
	{
		ActHeaderInit(pAd, &HTINFOframe.Hdr, pAddr, pAd->MeshTab.CurrentAddress, pAddr);		
	}
	else
#endif // MESH_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef APCLI_SUPPORT
		if (pAd->MacTab.Content[Elem->Wcid].ValidAsApCli)
		{
			apidx = pAd->MacTab.Content[Elem->Wcid].MatchAPCLITabIdx;
			ActHeaderInit(pAd, &HTINFOframe.Hdr, pAddr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, pAddr);		
		}
		else
#endif // APCLI_SUPPORT //
		{
			apidx = pAd->MacTab.Content[Elem->Wcid].apidx;
			ActHeaderInit(pAd, &HTINFOframe.Hdr, pAddr, pAd->ApCfg.MBSSID[apidx].Bssid, pAd->ApCfg.MBSSID[apidx].Bssid);
		}
	}
#endif // CONFIG_AP_SUPPORT //

	HTINFOframe.Category = CATEGORY_HT;
	HTINFOframe.Action = HT_INFORMATION_EXCHANGE_ACTION;
	HTINFOframe.HT_Info.Request = 0;
	HTINFOframe.HT_Info.Forty_MHz_Intolerant = pAd->CommonCfg.HtCapability.HtCapInfo.Forty_Mhz_Intolerant;
	HTINFOframe.HT_Info.STA_Channel_Width	 = pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth;	

	MakeOutgoingFrame(pOutBuffer,					&FrameLen,
					  sizeof(FRAME_HT_INFO),	&HTINFOframe,
					  END_OF_ARGS);

	MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);
}


VOID PeerHTAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR	Action = Elem->Msg[LENGTH_802_11+1];
	
	if (Elem->Wcid >= MAX_LEN_OF_MAC_TABLE)
		return;
	
	switch(Action)
	{
		case Notify_BW_ACTION:
			DBGPRINT(RT_DEBUG_TRACE,("ACTION - HT Notify Channel bandwidth action----> \n"));
			if (Elem->Msg[LENGTH_802_11+2] == 0)	// 7.4.8.2. if value is 1, keep the same as supported channel bandwidth. 
				pAd->MacTab.Content[Elem->Wcid].HTPhyMode.field.BW = 0;
			
			break;
		case SMPS_ACTION:
			// 7.3.1.25
 			DBGPRINT(RT_DEBUG_TRACE,("ACTION - SMPS action----> \n"));
			if (((Elem->Msg[LENGTH_802_11+2]&0x1) == 0))
			{
				pAd->MacTab.Content[Elem->Wcid].MmpsMode = MMPS_ENABLE;
			}
			else if (((Elem->Msg[LENGTH_802_11+2]&0x2) == 0))
			{
				pAd->MacTab.Content[Elem->Wcid].MmpsMode = MMPS_STATIC;
			}
			else
			{			
				pAd->MacTab.Content[Elem->Wcid].MmpsMode = MMPS_DYNAMIC;
			}

			DBGPRINT(RT_DEBUG_TRACE,("Aid(%d) MIMO PS = %d\n", Elem->Wcid, pAd->MacTab.Content[Elem->Wcid].MmpsMode));
			// rt2860c : add something for smps change.
			break;
 
		case SETPCO_ACTION:
			break;
		case MIMO_CHA_MEASURE_ACTION:
			break;
		case HT_INFORMATION_EXCHANGE_ACTION:
			{			
				HT_INFORMATION_OCTET	*pHT_info;

				pHT_info = (HT_INFORMATION_OCTET *) &Elem->Msg[LENGTH_802_11+2];
    				// 7.4.8.10
    				DBGPRINT(RT_DEBUG_TRACE,("ACTION - HT Information Exchange action----> \n"));
    				if (pHT_info->Request)
    				{
    					respond_ht_information_exchange_action(pAd, Elem);
    				}
#ifdef CONFIG_AP_SUPPORT
				IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
				{
	    				if (pHT_info->Forty_MHz_Intolerant)
	    				{
	    					Handle_BSS_Width_Trigger_Events(pAd);
	    				}
				}
#endif // CONFIG_AP_SUPPORT //
			}
    		break;
			
	}
}


/*
	==========================================================================
	Description:
		Retry sending ADDBA Reqest.
		
	IRQL = DISPATCH_LEVEL
	
	Parametrs:
	p8023Header: if this is already 802.3 format, p8023Header is NULL
	
	Return	: TRUE if put into rx reordering buffer, shouldn't indicaterxhere.
				FALSE , then continue indicaterx at this moment.
	==========================================================================
 */
VOID ORIBATimerTimeout(
	IN	PRTMP_ADAPTER	pAd) 
{
	MAC_TABLE_ENTRY	*pEntry;
	INT			i, total;
//	FRAME_BAR			FrameBar;
//	ULONG			FrameLen;
//	NDIS_STATUS 	NStatus;
//	PUCHAR			pOutBuffer = NULL;
//	USHORT			Sequence;
	UCHAR			TID;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	total = pAd->MacTab.Size * NUM_OF_TID;

	for (i = 1; ((i <MAX_LEN_OF_BA_ORI_TABLE) && (total > 0)) ; i++)
	{
		if  (pAd->BATable.BAOriEntry[i].ORI_BA_Status == Originator_Done)
		{
			pEntry = &pAd->MacTab.Content[pAd->BATable.BAOriEntry[i].Wcid];
			TID = pAd->BATable.BAOriEntry[i].TID;

			ASSERT(pAd->BATable.BAOriEntry[i].Wcid < MAX_LEN_OF_MAC_TABLE);
		}
		total --;
	}
}


VOID SendRefreshBAR(
	IN	PRTMP_ADAPTER	pAd,
	IN	MAC_TABLE_ENTRY	*pEntry) 
{
	FRAME_BAR		FrameBar;
	ULONG			FrameLen;
	NDIS_STATUS 	NStatus;
	PUCHAR			pOutBuffer = NULL;
	USHORT			Sequence;
	UCHAR			i, TID;
	USHORT			idx;
	BA_ORI_ENTRY	*pBAEntry;

	for (i = 0; i <NUM_OF_TID; i++)
	{
		idx = pEntry->BAOriWcidArray[i];
		if (idx == 0)
		{
			continue;
		}
		pBAEntry = &pAd->BATable.BAOriEntry[idx];

		if  (pBAEntry->ORI_BA_Status == Originator_Done)
		{
			TID = pBAEntry->TID;

			ASSERT(pBAEntry->Wcid < MAX_LEN_OF_MAC_TABLE);

			NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  //Get an unused nonpaged memory
			if(NStatus != NDIS_STATUS_SUCCESS) 
			{
				DBGPRINT(RT_DEBUG_ERROR,("BA - MlmeADDBAAction() allocate memory failed \n"));
				return;
			}
				
			Sequence = pEntry->TxSeq[TID];
#if 0
			if (Sequence == 0)
				Sequence = MAXSEQ;
			else
				Sequence--;
#else
			//if (Sequence <= 3)
			//	Sequence = 0;
			//else
			//	Sequence -=3;
#endif

#ifdef MESH_SUPPORT
			if (pEntry->ValidAsMesh)		
				BarHeaderInit(pAd, &FrameBar, pEntry->Addr, pAd->MeshTab.CurrentAddress);			
			else
#endif // MESH_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
#ifdef APCLI_SUPPORT
				if (pEntry->ValidAsApCli)		
					BarHeaderInit(pAd, &FrameBar, pEntry->Addr, pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].CurrentAddress);			
				else
#endif // APCLI_SUPPORT //
					BarHeaderInit(pAd, &FrameBar, pEntry->Addr, pAd->ApCfg.MBSSID[pEntry->apidx].Bssid);					
			}
#endif // CONFIG_AP_SUPPORT //


			FrameBar.StartingSeq.field.FragNum = 0; // make sure sequence not clear in DEL function.
			FrameBar.StartingSeq.field.StartSeq = Sequence; // make sure sequence not clear in DEL funciton.
			FrameBar.BarControl.TID = TID; // make sure sequence not clear in DEL funciton.

			MakeOutgoingFrame(pOutBuffer,				&FrameLen,
							  sizeof(FRAME_BAR),	  &FrameBar,
							  END_OF_ARGS);
			//if (!(CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET)))
			if (1)	// Now we always send BAR.
			{
				//MiniportMMRequestUnlock(pAd, 0, pOutBuffer, FrameLen);
				MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
				DBGPRINT_RAW(RT_DEBUG_INFO, ("BA --> NoBADataCountDown  [= %d]!!!\n", pEntry->NoBADataCountDown));
				DBGPRINT(RT_DEBUG_INFO,("BA - Send BAR, Seq = 0x%x \n", Sequence));
			}
			MlmeFreeMemory(pAd, pOutBuffer);
		}
	}
}



VOID ActHeaderInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN OUT PHEADER_802_11 pHdr80211, 
    IN PUCHAR Addr1, 
    IN PUCHAR Addr2,
    IN PUCHAR Addr3) 
{
    NdisZeroMemory(pHdr80211, sizeof(HEADER_802_11));
    pHdr80211->FC.Type = BTYPE_MGMT;
    pHdr80211->FC.SubType = SUBTYPE_ACTION;

    COPY_MAC_ADDR(pHdr80211->Addr1, Addr1);
	COPY_MAC_ADDR(pHdr80211->Addr2, Addr2);
    COPY_MAC_ADDR(pHdr80211->Addr3, Addr3);
}

VOID BarHeaderInit(
	IN	PRTMP_ADAPTER	pAd, 
	IN OUT PFRAME_BAR pCntlBar, 
	IN PUCHAR pDA,
	IN PUCHAR pSA) 
{
//	USHORT	Duration;

	NdisZeroMemory(pCntlBar, sizeof(FRAME_BAR));
	pCntlBar->FC.Type = BTYPE_CNTL;
	pCntlBar->FC.SubType = SUBTYPE_BLOCK_ACK_REQ;
   	pCntlBar->BarControl.MTID = 0;
	pCntlBar->BarControl.Compressed = 1;
	pCntlBar->BarControl.ACKPolicy = 0;


	pCntlBar->Duration = 16 + RTMPCalcDuration(pAd, RATE_1, sizeof(FRAME_BA));

	COPY_MAC_ADDR(pCntlBar->Addr1, pDA);
	COPY_MAC_ADDR(pCntlBar->Addr2, pSA);
}
