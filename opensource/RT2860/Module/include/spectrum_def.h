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
	spectrum_def.h
 
    Abstract:
    Handle association related requests either from WSTA or from local MLME
 
    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
	Fonchi Wu    2008	  	   created for 802.11h
 */

#ifndef __SPECTRUM_DEF_H__
#define __SPECTRUM_DEF_H__

#define MAX_MEASURE_REQ_TAB_SIZE		3
#define MAX_HASH_MEASURE_REQ_TAB_SIZE	MAX_MEASURE_REQ_TAB_SIZE

#define MAX_TPC_REQ_TAB_SIZE			3
#define MAX_HASH_TPC_REQ_TAB_SIZE		MAX_TPC_REQ_TAB_SIZE

#define MIN_RCV_PWR				100		/* Negative value ((dBm) */

#define RM_TPC_REQ				0
#define RM_MEASURE_REQ			1

#define RM_BASIC				0
#define RM_CCA					1
#define RM_RPI_HISTOGRAM		2

#define TPC_REQ_AGE_OUT			500		/* ms */
#define MQ_REQ_AGE_OUT			500		/* ms */

#define TPC_DIALOGTOKEN_HASH_INDEX(_DialogToken)	((_DialogToken) % MAX_HASH_TPC_REQ_TAB_SIZE)
#define MQ_DIALOGTOKEN_HASH_INDEX(_DialogToken)		((_DialogToken) % MAX_MEASURE_REQ_TAB_SIZE)

typedef struct _MEASURE_REQ_ENTRY
{
	struct _MEASURE_REQ_ENTRY *pNext;
	ULONG lastTime;
	BOOLEAN	Valid;
	UINT8 DialogToken;
	UINT8 MeasureDialogToken[3];	// 0:basic measure, 1: CCA measure, 2: RPI_Histogram measure.
} MEASURE_REQ_ENTRY, *PMEASURE_REQ_ENTRY;

typedef struct _MEASURE_REQ_TAB
{
	UCHAR Size;
	PMEASURE_REQ_ENTRY Hash[MAX_HASH_MEASURE_REQ_TAB_SIZE];
	MEASURE_REQ_ENTRY Content[MAX_MEASURE_REQ_TAB_SIZE];
} MEASURE_REQ_TAB, *PMEASURE_REQ_TAB;

typedef struct _TPC_REQ_ENTRY
{
	struct _TPC_REQ_ENTRY *pNext;
	ULONG lastTime;
	BOOLEAN Valid;
	UINT8 DialogToken;
} TPC_REQ_ENTRY, *PTPC_REQ_ENTRY;

typedef struct _TPC_REQ_TAB
{
	UCHAR Size;
	PTPC_REQ_ENTRY Hash[MAX_HASH_TPC_REQ_TAB_SIZE];
	TPC_REQ_ENTRY Content[MAX_TPC_REQ_TAB_SIZE];
} TPC_REQ_TAB, *PTPC_REQ_TAB;

#endif // __SPECTRUM_DEF_H__ //

