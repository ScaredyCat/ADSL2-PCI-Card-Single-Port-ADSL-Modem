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

#include "stdafx.h"
#include <common/cm_def.h>
#include <uacore/ua_core.h>

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

void DebugMsg2UI(const char* buf);
void DebugMsg(const char* buf);
void InitDebug(HWND hWIN);
void ExitDebug();

void UpdateCallList(const char* callid,
		    const char* uri,
		    const char* status);

DWORD AddrCStrToDW(CString StrAddr);

CString AddrDWToCStr(DWORD DWAddr);

char* strDupFromTCHAR(const TCHAR* wtemp);

class MyCString : public CString {
	char* buf;

public:
	MyCString() : CString() { buf = NULL; }
	MyCString(const CString& stringSrc ) : CString( stringSrc ) { buf = NULL; }
	MyCString( TCHAR ch, int nRepeat = 1 ) : CString( ch, nRepeat ) { buf = NULL; }
	MyCString( LPCSTR lpsz ) : CString( lpsz ) { buf = NULL; }
	MyCString( LPCWSTR lpsz ) : CString( lpsz ) { buf = NULL; }
	MyCString( LPCTSTR lpch, int nLength ) : CString( lpch, nLength ) { buf = NULL; }
	MyCString( const unsigned char* psz ) : CString( psz ) { buf = NULL; }
	operator LPCTSTR ();
#ifdef _WIN32_WCE
	operator LPCSTR ();
#endif
	~MyCString();
};

class CListItem
{
public:
	CListItem();
	CListItem(CString callid, CString uri, CString status);
	virtual ~CListItem();
public:
	CString m_uri;
	CString m_status;
	CString m_callid;
};

MyCString GetIP();

const char *getMyAddr();

const char **getAllMyAddr();

BOOL IsPortAvailable(unsigned short port);

MyCString UACallStateToCStr(UACallStateType stateType);

#endif //__UA_Common_h__
