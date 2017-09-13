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
    ap_uapsd.c

    Abstract:
    WMM UAPSD Module

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Sample Lin  11-07-2006      created
*/

#define MODULE_WMM_UAPSD
#include "rt_config.h"

#ifdef UAPSD_AP_SUPPORT
#include "ap_uapsd.h"




/*
========================================================================
Routine Description:
    UAPSD Module Init.

Arguments:
    ad_p            Pointer to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_Init(
    IN  PRTMP_ADAPTER       ad_p)
{
    /* allocate a lock resource */
//    NdisAllocateSpinLock(&ad_p->UAPSDEOSPLock);
} /* End of UAPSD_Init */


/*
========================================================================
Routine Description:
    UAPSD Module Release.

Arguments:
    ad_p            Pointer to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_Release(
    IN  PRTMP_ADAPTER       ad_p)
{
    /* free the lock resource */
//    NdisFreeSpinLock(&ad_p->UAPSDEOSPLock);
} /* End of UAPSD_Release */


/*
========================================================================
Routine Description:
    Deliver all queued packets.

Arguments:
    ad_p            Pointer to our adapter
    *entry_p        STATION

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_AllPacketDeliver(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p)
{
    QUEUE_HEADER *que_apsd_p;
    PQUEUE_ENTRY que_entry_p;
    UCHAR que_id_list[WMM_NUM_OF_AC] = { QID_AC_BE, QID_AC_BK,
                                         QID_AC_VI, QID_AC_VO };
    LONG ac_id, que_id; /* must be LONG, can not be ULONG */


    /* check if the EOSP frame is yet transmitted out */
    if (entry_p->pUAPSDEOSPFrame != NULL)
    {
        /* queue the EOSP frame to SW queue to be transmitted */
        que_id = RTMP_GET_PACKET_UAPSD_QUE_ID(
                        QUEUE_ENTRY_TO_PACKET(entry_p->pUAPSDEOSPFrame));

        if (que_id > QID_AC_VO)
        {
            /* should not be here, only for sanity */
            que_id = QID_AC_BE;
        } /* End of if */

        InsertTailQueue(&ad_p->TxSwQueue[que_id],
                        entry_p->pUAPSDEOSPFrame);

        entry_p->pUAPSDEOSPFrame = NULL;
        entry_p->UAPSDTxNum = 0;
    } /* End of if */

    /* deliver ALL U-APSD packets from AC3 to AC0 (AC0 to AC3 is also ok) */
    for(ac_id=(WMM_NUM_OF_AC-1); ac_id>=0; ac_id--)
    {
        que_apsd_p = &(entry_p->UAPSDQueue[ac_id]);
        que_id = que_id_list[ac_id];

        while(que_apsd_p->Head)
        {
            que_entry_p = RemoveHeadQueue(que_apsd_p);
            InsertTailQueue(&ad_p->TxSwQueue[que_id], que_entry_p);
        } /* End of while */
    } /* End of for */
} /* End of UAPSD_AllPacketDeliver */


/*
========================================================================
Routine Description:
    Parse the UAPSD field in WMM element in (re)association request frame.

Arguments:
    ad_p            Pointer to our adapter
    *entry_p        STATION
    *elm_p          QoS information field

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_AssocParse(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  UCHAR               *elm_p)
{
    PQBSS_STA_INFO_PARM  qos_info_p;


    /* check if the station enables UAPSD function */
    if ((entry_p) && (ad_p->CommonCfg.bAPSDCapable))
    {
        /* backup its UAPSD parameters */
        qos_info_p = (PQBSS_STA_INFO_PARM) elm_p;

        entry_p->MaxSPLength = qos_info_p->MaxSPLength;
        entry_p->bAPSDCapablePerAC[QID_AC_BE] = qos_info_p->UAPSD_AC_BE;
        entry_p->bAPSDCapablePerAC[QID_AC_BK] = qos_info_p->UAPSD_AC_BK;
        entry_p->bAPSDCapablePerAC[QID_AC_VI] = qos_info_p->UAPSD_AC_VI;
        entry_p->bAPSDCapablePerAC[QID_AC_VO] = qos_info_p->UAPSD_AC_VO;

        if ((entry_p->bAPSDCapablePerAC[QID_AC_BE] == 0) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_BK] == 0) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_VI] == 0) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_VO] == 0))
        {
            CLIENT_STATUS_CLEAR_FLAG(entry_p, fCLIENT_STATUS_APSD_CAPABLE);
        }
        else
        {
            CLIENT_STATUS_SET_FLAG(entry_p, fCLIENT_STATUS_APSD_CAPABLE);
        } /* End of if */

        if ((entry_p->bAPSDCapablePerAC[QID_AC_BE] == 1) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_BK] == 1) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_VI] == 1) &&
            (entry_p->bAPSDCapablePerAC[QID_AC_VO] == 1))
        {
            /* all AC are U-APSD */
            entry_p->bAPSDAllAC = 1;
        }
        else
        {
            /* at least one AC is not U-APSD */
            entry_p->bAPSDAllAC = 0;
        } /* End of if */
    } /* End of if */
} /* End of UAPSD_AssocParse */


/*
========================================================================
Routine Description:
    Enqueue a UAPSD packet.

Arguments:
    ad_p            Pointer to our adapter
    *entry_p        STATION
    packet_p        UAPSD dnlink packet
    ac_id           UAPSD AC ID (0 ~ 3)

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_PacketEnqueue(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UINT32              ac_id)
{
    /* 1. the STATION is UAPSD STATION;
       2. AC ID is legal;
       3. the AC is UAPSD AC. */
    /* so we queue the packet to its UAPSD queue */

    /* [0] ~ [3], QueIdx base is QID_AC_BE */
    QUEUE_HEADER *uapsd_que_p = &(entry_p->UAPSDQueue[ac_id]);


    /* check if current queued UAPSD packet number is too much */
    if (uapsd_que_p->Number >= MAX_PACKETS_IN_UAPSD_QUEUE)
    {
        /* too much queued pkts, free (discard) the tx packet */
        RELEASE_NDIS_PACKET(ad_p, packet_p, NDIS_STATUS_FAILURE);

        DBGPRINT(RT_DEBUG_TRACE,
                 ("too many frames (=%ld) in WCID(%d) UAPSD queue AC%d, "
                  "drop this one!\n",
                  uapsd_que_p->Number,
                  RTMP_GET_PACKET_WCID(packet_p),
                  ac_id));
    }
    else
    {
        /* queue the tx packet to the U-APSD queue of the AC */
        NdisAcquireSpinLock(&ad_p->MacTabLock);
        InsertTailQueue(uapsd_que_p, PACKET_TO_QUEUE_ENTRY(packet_p));
        NdisReleaseSpinLock(&ad_p->MacTabLock);
    } /* End of if */
} /* End of UAPSD_PacketEnqueue */


/*
========================================================================
Routine Description:
    Handle QoS Null Frame Tx Done interrupt.

Arguments:
    ad_p            Pointer to our adapter
    packet_p        Completed TX packet
    dst_mac_p       Destinated MAC address

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_QoSNullTxDoneHandle(
    IN  PRTMP_ADAPTER       ad_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UCHAR               *dst_mac_p)
{
    HEADER_802_11  *header_p;
    MAC_TABLE_ENTRY  *entry_p;
    INT_SOURCE_CSR_STRUC  intsrc;


    /* sanity check */
    if (packet_p == NULL)
        return;
    /* End of if */

    /* check if the packet is a U-APSD packet, must be QoS Null frame */
    if (RTMP_GET_PACKET_UAPSD_Flag(packet_p) != TRUE)
        return;
    /* End of if */

    /* check if the packet sub type is QoS Null */
	if (dst_mac_p == NULL)
		return;
	/* End of if */
    header_p = (HEADER_802_11 *)dst_mac_p;

    if ((header_p != NULL) &&
        (header_p->FC.SubType == SUBTYPE_QOS_NULL))
    {
        /* currently, QoS Null type is only used in UAPSD mechanism
           and no any UAPSD data packet exists */
		/* find the destination STATION */
        entry_p = MacTableLookup(ad_p, header_p->Addr1);

        if ((entry_p != NULL) && (entry_p->bAPSDFlagSPStart != 0))
        {
			/* SP is started for the station */

            if (entry_p->pUAPSDEOSPFrame != NULL)
            {
				/* SP will be closed, should not have EOSP frame */
				/* if exists, release it */
                RELEASE_NDIS_PACKET(ad_p,
                                    QUEUE_ENTRY_TO_PACKET(entry_p->pUAPSDEOSPFrame),
                                    NDIS_STATUS_FAILURE);
                entry_p->pUAPSDEOSPFrame = NULL;
            } /* End of if */

            entry_p->UAPSDTxNum = 0;

            /* check if rx done interrupt exists */
            RTMP_IO_READ32(ad_p, INT_SOURCE_CSR, &intsrc.word);

            if (intsrc.field.RxDone == 0)
            {
                /* no any new uplink packet is received */
                entry_p->bAPSDFlagSPStart = 0;
                entry_p->bAPSDFlagEOSPOK = 0;
            }
            else
			{
				/* a new uplink packet is received so check if the uplink
				   packet is transmitted from the station and close SP
				   in RxDone() */
                entry_p->bAPSDFlagEOSPOK = 1;
            } /* End of if */
        } /* End of if */
    } /* End of if */
} /* End of UAPSD_QoSNullTxDoneHandle */


/*
========================================================================
Routine Description:
    Maintenance our UAPSD PS queue.  Release all queued packet if timeout.

Arguments:
    ad_p            Pointer to our adapter
    *entry_p        STATION

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_QueueMaintenance(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p)
{
    QUEUE_HEADER *que_p;
    ULONG ac_id;
    UCHAR flg_uapsd_pkt;


    /* init */
    que_p = entry_p->UAPSDQueue;
    flg_uapsd_pkt = 0;

    /* check if more than one U-APSD packets exists */
    for(ac_id=0; ac_id<WMM_NUM_OF_AC; ac_id++)
    {
        if (que_p[ac_id].Head != NULL)
        {
            /* at least one U-APSD packets exists so we need to check if
               queued U-APSD packets are timeout */
            flg_uapsd_pkt = 1;
            break;
        } /* End of if */
    } /* End of for */

    /* check if any queued UAPSD packet exists */
    if (flg_uapsd_pkt)
    {
		USHORT	IdleCount;
		
        entry_p->UAPSDQIdleCount ++;

		if (ad_p->CommonCfg.bWiFiTest)
			IdleCount = 15;		/* 15 seconds for WMM test */
		else
			IdleCount = 5;		/* normal timeout value */
#if 0
#ifdef WIFI_TEST
		/* use more timeout */
        if (entry_p->UAPSDQIdleCount > 15) /* 15 seconds for WMM test */
#else
        if (entry_p->UAPSDQIdleCount > 5) /* normal timeout value */
#endif // WIFI_TEST //
#endif
		if (entry_p->UAPSDQIdleCount > IdleCount)
        {
            /* UAPSDQIdleCount will be 0 after trigger frame is received */
            /* clear all U-APSD packets */
            for(ac_id=0; ac_id<WMM_NUM_OF_AC; ac_id++)
                APCleanupPsQueue(ad_p, &que_p[ac_id]);
            /* End of for */

            /* free the EOSP frame */
            entry_p->UAPSDTxNum = 0;

            if (entry_p->pUAPSDEOSPFrame != NULL)
            {
                RELEASE_NDIS_PACKET(ad_p,
                                    QUEUE_ENTRY_TO_PACKET(entry_p->pUAPSDEOSPFrame),
                                    NDIS_STATUS_FAILURE);
                entry_p->pUAPSDEOSPFrame = NULL;
            } /* End of if */

            entry_p->bAPSDFlagEOSPOK = 0;
            entry_p->bAPSDFlagSPStart = 0;

            /* clear idle counter */
            entry_p->UAPSDQIdleCount = 0;
        } /* End of if */
    }
    else
	{
		/* clear idle counter */
        entry_p->UAPSDQIdleCount = 0;
    } /* End of if (flg_uapsd_pkt) */
} /* End of UAPSD_QueueMaintenance */


/*
========================================================================
Routine Description:
    Close current Service Period.

Arguments:
    ad_p            Pointer to our adapter

Return Value:
    None

Note:
    When we receive EOSP frame tx done interrupt and a uplink packet
    from the station simultaneously, we will regard it as a new trigger
    frame because the packet is received when EOSP frame tx done interrupt.

    We can not sure the uplink packet is sent after old SP or in the old SP.
    So we must close the old SP in receive done ISR to avoid the problem.
========================================================================
*/
VOID UAPSD_SP_CloseInRVDone(
    IN  PRTMP_ADAPTER       ad_p)
{
    UINT32 i;


    /* sanity check */
    if (ad_p->CommonCfg.bAPSDCapable != TRUE)
        return;
    /* End of if */

    /* check for all CLIENT's UAPSD Service Period */
    for(i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
    {
        MAC_TABLE_ENTRY *sta_p = &ad_p->MacTab.Content[i];


        /* check if SP is started and EOSP is transmitted ok */
        if ((sta_p->bAPSDFlagSPStart != 0) &&
            (sta_p->bAPSDFlagEOSPOK != 0))
        {
			/* 1. SP is started;
			   2. EOSP frame is sent ok. */

            /* we close current SP for the STATION so we can receive new
               trigger frame from the STATION again */
            sta_p->bAPSDFlagSPStart = 0;
            sta_p->bAPSDFlagEOSPOK = 0;
        } /* End of if */
    } /* End of for */
} /* End of UAPSD_SP_CloseInRVDone */


/*
========================================================================
Routine Description:
    Check if we need to close current SP.

Arguments:
    ad_p            Pointer to our adapter
    packet_p        Completed TX packet
    dst_mac_p       Destinated MAC address

Return Value:
    None

Note:
    We need to call the function in TxDone ISR.
========================================================================
*/
VOID UAPSD_SP_PacketCheck(
    IN  PRTMP_ADAPTER       ad_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UCHAR               *dst_mac_p)
{
    MAC_TABLE_ENTRY *entry_p;
    HEADER_802_11 *header_p;
    USHORT que_id;


    /* sanity check */
    if (packet_p == NULL)
        return;
    /* End of if */

    /* NOTE: no aggregation function in U-APSD so dont care about
             pTxD->pNextSkb */

    /* check if the packet is a U-APSD packet */
    if (RTMP_GET_PACKET_UAPSD_Flag(packet_p) == FALSE)
        return;
    /* End of if */

    /* check if all U-APSD packets have been transmitted except
       the EOSP packet because we must sure the EOSP packet is
       transmitted at last */

    /* get RA STATION entry */
    entry_p = NULL;
	if (RTMP_GET_PACKET_UAPSD_QUE_ID(packet_p) == UAPSD_QOS_NULL_QUE_ID)
	{
		/* currently, our QoS Null frame is sent through MMRequest(),
		   in the function, only SDPtr0 is used, SDPtr1 is not used,
		   so dst_mac_p must be got from skb->data + TXWI_SIZE */
		dst_mac_p = GET_OS_PKT_DATAPTR(packet_p) + TXWI_SIZE;
	} /* End of if */

    header_p = (HEADER_802_11 *)(dst_mac_p);

    if (header_p != NULL)
    {
 	    if (header_p->FC.SubType == SUBTYPE_QOS_NULL)
		{
			/* the packet is a QoS NULL frame */
			UAPSD_QoSNullTxDoneHandle(ad_p, packet_p, dst_mac_p);
			return;
		} /* End of if */

        /* Addr1 = receiver address */
        entry_p = MacTableLookup(ad_p, header_p->Addr1);
    } /* End of if */

    if (entry_p != NULL)
    {
        /* Note: UAPSDTxNum does NOT include the EOSP packet */

        if (entry_p->UAPSDTxNum > 0)
        {
            /* some UAPSD packets are not yet transmitted */

            if (entry_p->UAPSDTxNum == 1)
            {
                /* this is the last UAPSD packet */
                if (entry_p->pUAPSDEOSPFrame != NULL)
                {
                    /* transmit the EOSP frame */
                    PNDIS_PACKET pkt_p;


                    pkt_p = QUEUE_ENTRY_TO_PACKET(entry_p->pUAPSDEOSPFrame);
                    que_id = RTMP_GET_PACKET_UAPSD_QUE_ID(pkt_p);

                    if (que_id > QID_AC_VO)
                    {
                        /* should not be here, only for sanity */
                        que_id = QID_AC_BE;
                    } /* End of if */

                    InsertTailQueue(&ad_p->TxSwQueue[que_id],
                                    entry_p->pUAPSDEOSPFrame);

                    entry_p->pUAPSDEOSPFrame = NULL;

                    /* the EOSP frame will be put into ASIC to tx
                       in RTMPHandleTxRingDmaDoneInterrupt(),
                       not the function */
                } /* End of if */
            } /* End of if */

            /* a UAPSD frame is transmitted so decrease the counter */
            entry_p->UAPSDTxNum --;
        }
        else
        {
            /* UAPSDTxNum == 0 so the packet is the EOSP packet */

            if (entry_p->bAPSDFlagSPStart != 0)
            {
                INT_SOURCE_CSR_STRUC intsrc;

                /* activate RX Done handle thread */

                /* maybe some uplink packets are received between
                   EOSP frame start transmission and end
                   transmssion, we must forward them first or we
                   will regard them as new trigger frames */

                /* we will clear all STATION bAPSDFlagSPStart flag
                   in RX done handler when U-APSD function is
                   enabled */

                /* I dont want to use another flag to check if do
                   the job because I also need a spin lock to
                   protect the flag, the protection will cause TX
                   DONE & RX DONE relation */

                /* 1: means EOSP is sent to the peer so we can close
                      current SP */

                /* we can not guarantee RTMPHandleRxDoneInterrupt()
                   will be called before
                   RTMPHandleTxRingDmaDoneInterrupt() in RTMPIsr()*/
                RTMP_IO_READ32(ad_p, INT_SOURCE_CSR, &intsrc.word);

                if (intsrc.field.RxDone == 0)
                {
                    /* no any received packet exists so no any
                       uplink packet exists */
                    entry_p->bAPSDFlagSPStart = 0;
                    entry_p->bAPSDFlagEOSPOK = 0;
                }
                else
                {
                    /* a received packet exists we will handle it
                       in RTMPIsr(), dont worry */
                    /* but we only handle max 16 received packets
                       in RTMPHandleRxDoneInterrupt so risk exists*/
                    entry_p->bAPSDFlagEOSPOK = 1;
                } /* End of if */
            } /* End of if */
        } /* End of if */
    } /* End of if (entry_p != NULL) */
} /* End of UAPSD_SP_PacketCheck */


/*
========================================================================
Routine Description:
    Handle UAPSD Trigger Frame.

Arguments:
    ad_p            Pointer to our adapter
    *entry_p        the source STATION
    up_of_frame     the UP of the trigger frame

Return Value:
    None

Note:
========================================================================
*/
VOID UAPSD_TriggerFrameHandle(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  UCHAR               up_of_frame)
{
    QUEUE_HEADER  *ac_ps_que_p;
    QUEUE_HEADER  *ac_sw_que_p, *last_ac_sw_que_p;
	PQUEUE_ENTRY  qued_entry_p;
    PNDIS_PACKET  qued_pkt_p;

    ULONG    tf_ac_id;
    ULONG    tx_pkt_num, sp_max_len;
    /* AC ID          = VO > VI > BK > BE */
    /* so we need to change BE & BK */
    /* => AC priority = VO > VI > BE > BK */
    ULONG    ac_pri[WMM_NUM_OF_AC] = { 1, 0, 2, 3 };
    ULONG    sp_len[4] = { 0, 2, 4, 6 }; /* 0: deliver all U-APSD packets */
    UCHAR    que_id_list[WMM_NUM_OF_AC] = { QID_AC_BE, QID_AC_BK,
                                            QID_AC_VI, QID_AC_VO };
    BOOLEAN  flg_que_empty;
    BOOLEAN  flg_null_snd;
    USHORT   aid, ac_id, que_id;
    LONG     priority; /* must be signed, can not use unsigned */

//    ULONG    flg_irq;


    /* Sanity check for Service Period of the STATION */
        if (entry_p->bAPSDFlagSPStart != 0)
        {
            /* WMM Specification V1.1 3.6.1.5
               A Trigger Frame received by the WMM AP from a WMM STA that
               already has an USP underway shall not trigger the start of a new
               USP. */

            /* current SP for the STATION is not yet ended so the packet is
               normal DATA packet */
            return;
        } /* End of if */

        if (entry_p->MaxSPLength >= 4)
        {
            /* fatal error, should be 0 ~ 3 so reset it to 0 */
            entry_p->MaxSPLength = 0;
        } /* End of if */


    /* Sanity Check for UAPSD condition */
        if (up_of_frame >= 8)
            up_of_frame = 1;
        /* End of if */

        tf_ac_id = MapUserPriorityToAccessCategory[up_of_frame];

#ifndef WMM_ACM_SUPPORT
        if ((entry_p->bAPSDCapablePerAC[tf_ac_id] == 0) ||
            (ad_p->ApCfg.BssEdcaParm.bACM[tf_ac_id] == 1))
#else
        if (entry_p->bAPSDCapablePerAC[tf_ac_id] == 0)
#endif // WMM_ACM_SUPPORT //
        {
            /* WMM Specification V1.1 Page 4
               Trigger Frame: A QoS Data or QoS Null frame from a WMM STA in
               Power Save Mode associated with an AC the WMM STA has configured
               to be a trigger-enabled AC.

               A QoS Data or QoS Null frame that indicates transition to/from
               Power Save Mode is not considered to be a Trigger Frame and the
               AP shall not respond with a QoS Null frame. */

            /* WMM Specification V1.1 3.6.0.9
               Transmission of a Trigger Frame is not implicitly allowed by
               admission of a downlink TS.

               If the Trigger Frame maps to an AC that has ACM=1, then the WMM
               STA must establish a suitable uplink TS before sending Trigger
               Frames. */

            /* ERROR! the AC does not belong to a trigger-enabled AC or
                      the ACM of the AC is set */
            return;
        } /* End of if */


    /* Enqueue U-APSD packets to AC software queues */
        /* protect TxSwQueue0 & McastPsQueue because use them in
           interrupt context */
//        RTMP_IRQ_LOCK(flg_irq);

        /* init */
        flg_que_empty = TRUE;
        tx_pkt_num = 0;
        sp_max_len = sp_len[entry_p->MaxSPLength];
        ac_sw_que_p = NULL;
        last_ac_sw_que_p = NULL;
        qued_pkt_p = NULL;

        /* from highest priority AC3 --> AC2 --> AC0 --> lowest priority AC1 */
        for(priority=(WMM_NUM_OF_AC-1); priority>=0; priority--)
        {
            ac_id = ac_pri[priority];

            /* check if the AC is U-APSD */
            if (entry_p->bAPSDCapablePerAC[ac_id] == 0)
                continue;
            /* End of for */

            /* NOTE: get U-APSD queue pointer here to speed up, do NOT use
                     entry_p->UAPSDQueue[ac_id] throughout codes because
                     compiler will compile it to many assembly codes */
            ac_ps_que_p = &entry_p->UAPSDQueue[ac_id];

            /* check if any U-APSD packet is queued for the AC */
            if (ac_ps_que_p->Head == NULL)
                continue;
            /* End of if */

            /* at least one U-APSD packet exists here */

            /* get AC software queue */
            que_id = que_id_list[ac_id];
            ac_sw_que_p = &ad_p->TxSwQueue[que_id];

            /* put U-APSD packets to the AC software queue */
            while(ac_ps_que_p->Head)
            {
                /* check if Max SP Length != 0 */
                if (sp_max_len != 0)
                {
                    /* WMM Specification V1.1 3.6.1.7
                       At each USP for a WMM STA, the WMM AP shall attempt to
                       transmit at least one MSDU or MMPDU, but no more than the
                       value encoded in the Max SP Length field in the QoS Info
                       Field of a WMM Information Element from delivery-enabled
                       ACs, that are destined for the WMM STA. */
                    if (tx_pkt_num >= sp_max_len)
                    {
                        /* some queued U-APSD packets still exists so we will
                           not clear MoreData bit of the packet */
                        flg_que_empty = FALSE;
                        break;
                    } /* End of if */
                } /* End of if */

                /* count U-APSD packet number */
                tx_pkt_num ++;

                /* queue last U-APSD packet */
                if (qued_pkt_p != NULL)
                {
                    /* enqueue U-APSD packet to transmission software queue */

                    /* WMM Specification V1.1 3.6.1.7
                       Each buffered frame shall be delivered using the access
                       parameters of its AC. */
                    InsertTailQueue(last_ac_sw_que_p, qued_pkt_p);
                } /* End of if */

                /* get the U-APSD packet */
                qued_entry_p = RemoveHeadQueue(ac_ps_que_p);
				qued_pkt_p = QUEUE_ENTRY_TO_PACKET(qued_entry_p);

                if (qued_pkt_p != NULL)
                {
                    /* WMM Specification V1.1 3.6.1.7
                       The More Data bit (b13) of the directed MSDU or MMPDU
                       associated with delivery-enabled ACs and destined for
                       that WMM STA indicates that more frames are buffered for
                       the delivery-enabled ACs. */
                    RTMP_SET_PACKET_MOREDATA(qued_pkt_p, TRUE);

                    /* set U-APSD flag & its software queue ID */
                    RTMP_SET_PACKET_UAPSD(qued_pkt_p, TRUE, que_id);
                } /* End of if */

                /* backup its software queue pointer */
                last_ac_sw_que_p = ac_sw_que_p;
            } /* End of while */

            if (flg_que_empty == FALSE)
            {
                /* flg_que_empty will be FALSE only when
                   tx_pkt_num >= sp_max_len */
                break;
            } /* End of if */
        } /* End of for */


        /* no need to protect EOSP handle code because we will be here
           only when last SP is ended */
        flg_null_snd = FALSE;

        if (tx_pkt_num >= 1)
        {
            if (flg_que_empty == TRUE)
            {
                /* no any more queued U-APSD packet so clear More Data bit of
                   the last frame */
                RTMP_SET_PACKET_MOREDATA(qued_pkt_p, FALSE);
            } /* End of if */
        } /* End of if */

        entry_p->bAPSDFlagSPStart = 1; /* set the SP start flag */
        entry_p->bAPSDFlagEOSPOK = 0;

        if (tx_pkt_num <= 1)
        {
            /* if no data needs to tx, respond with QosNull for the trigger
               frame */
            DBGPRINT(RT_DEBUG_TRACE,
                     ("No data, send a Qos-Null frame with ESOP bit on and "
                      "UP=%d to end USP\n", up_of_frame));

            entry_p->pUAPSDEOSPFrame = NULL;
            entry_p->UAPSDTxNum = 0;

            if (tx_pkt_num <= 0)
                flg_null_snd = TRUE;
            else
            {
                /* only one packet so send it directly */
                RTMP_SET_PACKET_EOSP(qued_pkt_p, TRUE);
                InsertTailQueue(last_ac_sw_que_p, qued_pkt_p);
            } /* End of if */

            /* we will send the QoS Null frame below and we will hande the
               QoS Null tx done in RTMPFreeTXDUponTxDmaDone() */
        }
        else
        {
            /* more than two U-APSD packets */

            /* NOTE: EOSP bit != !MoreData bit because Max SP Length,
                     we can not use MoreData bit to decide EOSP bit */

            /* backup the EOSP frame and
               we will transmit the EOSP frame in RTMPFreeTXDUponTxDmaDone() */
            RTMP_SET_PACKET_EOSP(qued_pkt_p, TRUE);

            entry_p->pUAPSDEOSPFrame = (PQUEUE_ENTRY)qued_pkt_p;
            entry_p->UAPSDTxNum = tx_pkt_num-1; /* skip the EOSP frame */
        } /* End of if */


    /* Clear corresponding TIM bit */
        /* get its AID for the station */
        aid = entry_p->Aid;

        if ((entry_p->bAPSDAllAC == 1) && (flg_que_empty == 1))
        {
            /* all AC are U-APSD and no any U-APSD packet is queued, set TIM */

            /* clear TIM bit */
            if ((aid > 0) && (aid < MAX_LEN_OF_MAC_TABLE))
            {
				WLAN_MR_TIM_BIT_CLEAR(ad_p, entry_p->apidx, aid);
            } /* End of if */
        } /* End of if */

        /* reset idle timeout here whenever a trigger frame is received */
        entry_p->UAPSDQIdleCount = 0;

//        RTMP_IRQ_UNLOCK(flg_irq);


    /* Check if NULL Frame is needed to be transmitted */
        /* it will be crashed, when spin locked in kernel 2.6 */
        if (flg_null_snd)
        {
            /* bQosNull = bEOSP = TRUE = 1 */
            /* use management queue to tx QoS Null frame to avoid delay */
            ApEnqueueNullFrame(ad_p, entry_p->Addr, entry_p->CurrTxRate,
                               aid, entry_p->apidx, TRUE, TRUE, up_of_frame);
        } /* End of if */


    /* Dequeue outgoing frames from TxSwQueue0..3 queue and process it */

        /* TODO: 2004-12-27 it's not a good idea to handle "More Data" bit here.
           because the RTMPDeQueue process doesn't guarantee to de-queue the
           desired MSDU from the corresponding TxSwQueue/PsQueue when QOS
           in-used. We should consider "HardTransmt" this MPDU */
        /* using MGMT queue or things like that */

        RTMPDeQueuePacket(ad_p, FALSE, MAX_TX_PROCESS);
} /* End of UAPSD_TriggerFrameHandle */


#endif // UAPSD_AP_SUPPORT //

/* End of ap_uapsd.c */
