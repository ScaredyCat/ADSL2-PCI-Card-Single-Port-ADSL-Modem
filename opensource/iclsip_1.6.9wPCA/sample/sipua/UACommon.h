/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* $Id: UACommon.h,v 1.8 2006/08/04 02:50:54 shinjan Exp $ */
// CallManager.h: interface for the CCallManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __UA_Common_h__
#define __UA_Common_h__

#include <common/cm_def.h>

// User defined messages
#define		WM_USER_DEBUGMSG			(WM_USER + 1)	/* Debug Message */
#define		WM_USER_CALL_EVENT			(WM_USER + 2)	/* Got incoming call */
#define		WM_USER_UPDATE_SKIN_SET		(WM_USER + 3)
#define		WM_USER_UPDATE_MESSAGE_BOX	(WM_USER + 4)
#define		WM_USER_TIMER				(WM_USER + 5)

// UI displaying mode
typedef enum {
	NOT_INIT_MODE,
	ON_HOOK_MODE,
	OFF_HOOK_MODE,
	WITH_FUN_MODE,
	RINGING_MODE,
	CALLING_MODE,
	CONNECTED_MODE
} EUIMode;

typedef enum {
	UICMD_DIAL,
	UICMD_CANCEL,
	UICMD_DROP,
	UICMD_ANSWER,
	UICMD_REJECT,
	UICMD_HOLD,
	UICMD_UNHOLD,
	UICMD_UXFER,
	UICMD_AXFER,
	UICMD_REG,
	UICMD_UNREG,
	UICMD_REGQUERY,
	UICMD_SHOWPREF,
	UICMD_SHOWVIDEO,
	UICMD_INFO,
	UICMD_MESSAGE
} UICmdID;

typedef enum {
	DLG_DIAL,
	DLG_ATTENDXFER,
	DLG_UNATTENDXFER,
	DLG_XFER,
	DLG_RTPTEST
} DialogType;

typedef enum {
	INCOMING_CALL,
	CALL_PROCEEDING,
	CALL_CONNECTED,
	CALL_DISCONNECTED,
	VIDEO_STARTED,
	VIDEO_STOP
} CallEvent;

typedef enum {
	CODEC_NONE = -1,
	CODEC_PCMU = 0,
	CODEC_GSM = 3,
	CODEC_G723 = 4,
	CODEC_PCMA = 8,
	CODEC_G729 = 18,
	CODEC_H263 = 34,
	CODEC_MPEG4 = 96,
	CODEC_iLBC = 97
} CODEC_NUMBER;



#endif //__UA_Common_h__
