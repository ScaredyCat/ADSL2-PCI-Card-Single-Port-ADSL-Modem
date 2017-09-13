/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2005, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attempt
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	apcli_mlme.h

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2006-06-26		Modify for RT61-APCli
	
*/
#ifndef __APCLI_MLME_H__
#define __APCLI_MLME_H__


#define AUTH_TIMEOUT	300         // unit: msec
#define ASSOC_TIMEOUT	300         // unit: msec
//#define JOIN_TIMEOUT	2000        // unit: msec // not used in Ap-client mode, remove it
#define PROBE_TIMEOUT	1000        // unit: msec

typedef struct _APCLI_MLME_JOIN_REQ_STRUCT {
	UCHAR	Bssid[MAC_ADDR_LEN];
	UCHAR	SsidLen;
	UCHAR	Ssid[MAX_LEN_OF_SSID];
} APCLI_MLME_JOIN_REQ_STRUCT;

typedef struct _STA_CTRL_JOIN_REQ_STRUCT {
	USHORT	Status;
} APCLI_CTRL_MSG_STRUCT, *PSTA_CTRL_MSG_STRUCT;


VOID ApCliStateMachineInit(
	IN APCLI_STATE_MACHINE *S, 
	IN APCLI_STATE_MACHINE_FUNC Trans[], 
	IN ULONG StNr,
	IN ULONG MsgNr,
	IN APCLI_STATE_MACHINE_FUNC DefFunc, 
	IN ULONG InitState, 
	IN ULONG Base);

VOID ApCliCurrentStateInit(
	IN ULONG InitState,
	OUT PULONG pCurrState);

VOID ApCliStateMachineSetAction(
	IN APCLI_STATE_MACHINE *S, 
	IN ULONG St, 
	IN ULONG Msg, 
	IN APCLI_STATE_MACHINE_FUNC Func);

BOOLEAN isValidApCliIf(
	SHORT ifIndex);

VOID ApCliStateMachinePerformAction(
	IN PRTMP_ADAPTER pAd, 
	IN APCLI_STATE_MACHINE *S, 
	IN MLME_QUEUE_ELEM *Elem,
	USHORT ifIndex,
	PULONG pCurrState);

BOOLEAN ApCliMlmeEnqueue(
	IN	PRTMP_ADAPTER pAd,
	IN ULONG Machine, 
	IN ULONG MsgType, 
	IN ULONG MsgLen, 
	IN VOID *Msg,
	IN USHORT ifIndex);

VOID ApCliDrop(
    IN PRTMP_ADAPTER pAd,
    IN MLME_QUEUE_ELEM *Elem,
	PULONG pCurrState,
	USHORT ifIndex);

//
// Private routines in apcli_ctrl.c
//
VOID ApCliCtrlStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN APCLI_STATE_MACHINE *Sm,
	OUT APCLI_STATE_MACHINE_FUNC Trans[]);

//
// Private routines in apcli_sync.c
//
VOID ApCliSyncStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN APCLI_STATE_MACHINE *Sm, 
    OUT APCLI_STATE_MACHINE_FUNC Trans[]);

//
// Private routines in apcli_auth.c
//
VOID ApCliAuthStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN APCLI_STATE_MACHINE *Sm, 
    OUT APCLI_STATE_MACHINE_FUNC Trans[]);

//
// Private routines in apcli_assoc.c
//
VOID ApCliAssocStateMachineInit(
    IN PRTMP_ADAPTER pAd, 
    IN APCLI_STATE_MACHINE *Sm, 
    OUT APCLI_STATE_MACHINE_FUNC Trans[]);

#endif /* __APCLI_MLME_H__ */

