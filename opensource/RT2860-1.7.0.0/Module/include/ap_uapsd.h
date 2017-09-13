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
    ap_uapsd.h

    Abstract:
    WMM UAPSD Module

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Sample Lin  11-07-2006      created
*/

#ifndef MODULE_WMM_UAPSD

#define UAPSD_EXTERN    extern

/* Public Marco list */
/* init some parameters in packet structure for QoS Null frame;
   purpose: is for management frame tx done use */
#define UAPSD_MR_QOS_NULL_HANDLE(pAd, pData, pPacket) 								\
    {   PHEADER_802_11 header_p = (PHEADER_802_11)pData; 							\
		MAC_TABLE_ENTRY *entry_p;													\
        if (header_p->FC.SubType == SUBTYPE_QOS_NULL) {								\
            RTMP_SET_PACKET_QOS_NULL(pPacket);										\
			entry_p = MacTableLookup(pAd, header_p->Addr1);							\
			if (entry_p != NULL) { RTMP_SET_PACKET_WCID(pPacket, entry_p->Aid); } }	\
		else { RTMP_SET_PACKET_NON_QOS_NULL(pPacket); } }

/* init MAC entry UAPSD parameters;
   purpose: initialize UAPSD PS queue and control parameters */
#define UAPSD_MR_ENTRY_INIT(pEntry) \
    {   USHORT ac_id; \
        for(ac_id=0; ac_id<WMM_NUM_OF_AC; ac_id++) \
            InitializeQueueHeader(&pEntry->UAPSDQueue[ac_id]); \
        pEntry->UAPSDTxNum = 0; \
        pEntry->pUAPSDEOSPFrame = NULL; \
        pEntry->bAPSDFlagSPStart = 0; \
        pEntry->bAPSDFlagEOSPOK = 0; \
        pEntry->MaxSPLength = 0; }

/* reset MAC entry UAPSD parameters;
   purpose: clean all UAPSD PS queue; release the EOSP frame if exists;
            reset control parameters */
#define UAPSD_MR_ENTRY_RESET(pEntry) \
    {   MAC_TABLE_ENTRY *sta_p; \
        ULONG ac_id; \
        sta_p = pEntry; \
        /* clear all U-APSD queues */ \
        for(ac_id=0; ac_id<WMM_NUM_OF_AC; ac_id++) \
            APCleanupPsQueue(pAd, &sta_p->UAPSDQueue[ac_id]); \
        /* clear EOSP frame */ \
        sta_p->UAPSDTxNum = 0; \
        if (sta_p->pUAPSDEOSPFrame != NULL) { \
            RELEASE_NDIS_PACKET(pAd, \
                                QUEUE_ENTRY_TO_PACKET(sta_p->pUAPSDEOSPFrame), \
                                NDIS_STATUS_FAILURE); \
            sta_p->pUAPSDEOSPFrame = NULL; } \
        sta_p->bAPSDFlagSPStart = 0; \
        sta_p->bAPSDFlagEOSPOK = 0; }

/* enable or disable UAPSD flag in WMM element in beacon frame;
   purpose: set UAPSD enable/disable bit */
#define UAPSD_MR_IE_FILL(qos_ctrl_field, pAd) \
        qos_ctrl_field |= (pAd->CommonCfg.bAPSDCapable) ? 0x80 : 0x00;

/* check if we do NOT need to control TIM bit for the station;
   note: we control TIM bit only when all AC are UAPSD AC */
#define UAPSD_MR_IS_NOT_TIM_BIT_NEEDED_HANDLED(pMacEntry, QueIdx) \
        (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_APSD_CAPABLE) && \
         (!pMacEntry->bAPSDCapablePerAC[QID_AC_VO] || \
          !pMacEntry->bAPSDCapablePerAC[QID_AC_VI] || \
          !pMacEntry->bAPSDCapablePerAC[QID_AC_BE] || \
          !pMacEntry->bAPSDCapablePerAC[QID_AC_BK]) && \
         pMacEntry->bAPSDCapablePerAC[QueIdx])

/* check if the AC is UAPSD AC */
#define UAPSD_MR_IS_UAPSD_AC(pMacEntry, ac_id) \
        (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_APSD_CAPABLE) && \
         ((0 <= ac_id) && (ac_id < WMM_NUM_OF_AC)) && /* 0 ~ 3 */ \
         pMacEntry->bAPSDCapablePerAC[ac_id])

/* check if all AC are UAPSD AC */
#define UAPSD_MR_IS_ALL_AC_UAPSD(isActive, pMacEntry) \
        ((isActive == FALSE) && (pMacEntry->bAPSDAllAC == 1))

#else

#define UAPSD_EXTERN
#define UAPSD_QOS_NULL_QUE_ID		0x7f

#endif // MODULE_WMM_UAPSD //


/* max UAPSD buffer queue size */
#define MAX_PACKETS_IN_UAPSD_QUEUE	16  /* for each AC = 16*4 = 64 */


/* Public function list */
UAPSD_EXTERN VOID UAPSD_Init(
    IN  PRTMP_ADAPTER       ad_p);

UAPSD_EXTERN VOID UAPSD_Release(
    IN  PRTMP_ADAPTER       ad_p);

UAPSD_EXTERN VOID UAPSD_SP_Close(
    IN  PRTMP_ADAPTER       ad_p,
	IN	MAC_TABLE_ENTRY		*entry_p);

UAPSD_EXTERN VOID UAPSD_AllPacketDeliver(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p);

UAPSD_EXTERN VOID UAPSD_AssocParse(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  UCHAR               *elm_p);

UAPSD_EXTERN VOID UAPSD_PacketEnqueue(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UINT32              ac_id);

UAPSD_EXTERN VOID UAPSD_QoSNullTxDoneHandle(
    IN  PRTMP_ADAPTER       ad_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UCHAR               *dst_mac_p);

UAPSD_EXTERN VOID UAPSD_QueueMaintenance(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p);

UAPSD_EXTERN VOID UAPSD_SP_CloseInRVDone(
    IN  PRTMP_ADAPTER       ad_p);

UAPSD_EXTERN VOID UAPSD_SP_PacketCheck(
    IN  PRTMP_ADAPTER       ad_p,
    IN  PNDIS_PACKET        packet_p,
    IN  UCHAR               *dst_mac_p);

UAPSD_EXTERN VOID UAPSD_TriggerFrameHandle(
    IN  PRTMP_ADAPTER       ad_p,
    IN  MAC_TABLE_ENTRY     *entry_p,
    IN  UCHAR               up_of_frame);


/* End of ap_uapsd.h */
