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
    ap_qload.c
 
    Abstract:
    Provide information on the current STA population and traffic levels
	in the QBSS.

	This attribute is available only at a QAP. This attribute, when TRUE,
	indicates that the QAP implementation is capable of generating and
	transmitting the QBSS load element in the Beacon and Probe Response frames.
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Sample Lin	01-21-2008    Created

 */

#include "rt_config.h"


typedef struct PACKED {

	UINT8 element_id;
	UINT8 length;

	/* the total number of STAs currently associated with this QBSS */
	UINT16 station_count;

	/*	defined as the percentage of time, nomalized to 255, the QAP sensed the
		medium busy, as indicated by either the physical or virtual carrier
		sense mechanism.
		This percentage is computed using the formula:
			((channel busy time / (dot11ChannelUtilizationBeaconIntervals *
			dot11BeaconPeriod * 1024)) * 255) */
	UINT8 chan_util;

	/*	specifies the remaining amount of medium time available via explicit
		admission control, in units of 32 microsecond periods per 1 second.
		The field is helpful for roaming non-AP QSTAs to select a QAP that is
		likely to accept future admission control requests, but it does not
		represent a guarantee that the HC will admit these requests. */
	UINT16 aval_adm_cap;

} ELM_QBSS_LOAD;

#define ELM_QBSS_LOAD_ID					11
#define ELM_QBSS_LOAD_LEN					5




/* --------------------------------- Public -------------------------------- */

/*
========================================================================
Routine Description:
	Initialize ASIC Channel Busy Calculation mechanism.

Arguments:
	ad_p				- WLAN control block pointer

Return Value:
	None

Note:
	Init Condition: WMM must be enabled.
========================================================================
*/
VOID QBSS_LoadInit(
 	IN		RTMP_ADAPTER	*pAd)
{
	UINT32 i;


	/* check whether any BSS enables WMM feature */
	for(i=0; i<pAd->ApCfg.BssidNum; i++)
	{
		if (pAd->ApCfg.MBSSID[i].bWmmCapable)
		{
			pAd->flg_qload_enable = TRUE;
			break;
		} /* End of if */
	} /* End of for */

	if (pAd->flg_qload_enable == TRUE)
	{
		/* Count EIFS, NAV, RX busy, TX busy as channel busy and
			enable Channel statistic timer (bit 0) */
		/* if bit 0 == 0, the function will be disabled */
		RTMP_IO_WRITE32(pAd, CH_TIME_CFG, 0x0000001F);

		/* default value is 50, please reference to IEEE802.11e 2005 Annex D */
		pAd->qload_chan_util_beacon_int = 50;
	}
	else
	{
		/* no any WMM is enabled */
		RTMP_IO_WRITE32(pAd, CH_TIME_CFG, 0x00000000);
	} /* End of if */
} /* End of QBSS_LoadInit */


/*
========================================================================
Routine Description:
	Append the QBSS Load element to the beacon frame.

Arguments:
	ad_p				- WLAN control block pointer
	*buf_p				- the beacon or probe response frame

Return Value:
	the element total length

Note:
	Append Condition: You must check whether WMM is enabled before the
	function is using.
========================================================================
*/
UINT32 QBSS_LoadElementAppend(
 	IN		RTMP_ADAPTER	*pAd,
	OUT		UINT8			*buf_p)
{
	ELM_QBSS_LOAD load, *load_p = &load;
	ULONG elm_len;


	/* check whether channel busy time calculation is enabled */
	if (pAd->flg_qload_enable == 0)
		return 0;
	/* End of if */

	/* init */
	load_p->element_id = ELM_QBSS_LOAD_ID;
	load_p->length = ELM_QBSS_LOAD_LEN;

	load_p->station_count = MacTableAssocStaNumGet(pAd);
	load_p->chan_util = pAd->qload_chan_util;

	/* because no ACM is supported, the available bandwidth is 1 sec */
	load_p->aval_adm_cap = 0x7a12; /* 0x7a12 * 32us = 1 second */

	/* copy the element to the frame */
    MakeOutgoingFrame(	buf_p,					&elm_len,
						sizeof(ELM_QBSS_LOAD),	load_p,
						END_OF_ARGS);

	return elm_len;
} /* End of QBSS_LoadElementAppend */


/*
========================================================================
Routine Description:
	Update Channel Utilization.

Arguments:
	ad_p				- WLAN control block pointer

Return Value:
	None

Note:
	Only be used in TBTT handler.
========================================================================
*/
VOID QBSS_LoadUpdate(
 	IN		RTMP_ADAPTER	*pAd)
{
	UINT32 chan_util_nu, chan_util_de;
	UINT32 csr_time = 0;


	/* check whether channel busy time calculation is enabled */
	if (pAd->flg_qload_enable == 0)
		return;
	/* End of if */

	/* accumulate channel busy time */
	RTMP_IO_READ32(pAd, CH_BUSY_STA, &csr_time);
	pAd->qload_chan_util_total += csr_time;

	/* update new channel utilization */
	if (++pAd->qload_chan_util_beacon_cnt >= pAd->qload_chan_util_beacon_int)
	{
		chan_util_nu = (pAd->qload_chan_util_total*255);
		chan_util_de = (pAd->qload_chan_util_beacon_int*\
						pAd->CommonCfg.BeaconPeriod)<<10;
		pAd->qload_chan_util = (UINT8)(chan_util_nu/chan_util_de);

		/* re-accumulate channel busy time */
		pAd->qload_chan_util_beacon_cnt = 0;
		pAd->qload_chan_util_total = 0;
	} /* End of if */
} /* End of QBSS_LoadUpdate */

/* End of ap_qload.c */
