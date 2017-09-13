// CallDlgList.cpp: implementation of the CCallDlgList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pcaua.h"
#include "CallDlgList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCallDlgList::CCallDlgList()
{
}

CCallDlgList::~CCallDlgList()
{

}

BOOL CCallDlgList::Add(int dlg, LPCTSTR tel, int state, int type )
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == -1 )
		{
			dlgInfo[i].dlgHandle=dlg;
			dlgInfo[i].telno=tel;
			dlgInfo[i].state=state;
			dlgInfo[i].type=type;
			dlgInfo[i].bConnected=FALSE;
			dlgInfo[i].bRinging=FALSE;
			dlgInfo[i].startTime=COleDateTime::GetCurrentTime();
			dlgInfo[i].bConference=FALSE;
			_TraceContent();
			return i;
		}

	}
	return -1;
}

BOOL CCallDlgList::Find(int dlg)
{
	// Arlene added : if the dlg is -1, don't find anything.
	if (dlg == -1)
		return FALSE;

	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return TRUE;
	}
	return FALSE;
}

CALL_DLG_INFO * CCallDlgList::GetInfo(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
		{
			return &dlgInfo[i];
		}
	}
	return NULL;

}

void CCallDlgList::SetInfo(int dlg, CALL_DLG_INFO &cdinfo)
{

	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
		{

			dlgInfo[i].company = cdinfo.company;
			dlgInfo[i].name = cdinfo.name;
			dlgInfo[i].telno = cdinfo.telno;
			dlgInfo[i].photoPath = cdinfo.photoPath;
		}
	}

}

void CCallDlgList::Remove(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			dlgInfo[i].dlgHandle = -1;
	}
	_TraceContent();
}

void CCallDlgList::_TraceContent()
{
	TRACE0("CCallDlgList:\n");
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		TRACE("idx:%d dlghandle=%d state=%d telno=%s, name=%s, company=%s, photoPath=%s\n",
			i, dlgInfo[i].dlgHandle, dlgInfo[i].state,
			dlgInfo[i].telno,dlgInfo[i].name,dlgInfo[i].company,dlgInfo[i].photoPath );
	}
}

CALL_DLG_INFO* CCallDlgList::GetInfoIdx(int idx)
// get call dialog info by index
{
	if( idx <0 || idx >= CALLDLG_MAX_COUNT )
		return NULL;
	if( dlgInfo[idx].dlgHandle == -1 )
		return NULL;
	else
		return &dlgInfo[idx];

}

void CCallDlgList::SetState(int dlg, int state)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			dlgInfo[i].state = state;
	}
	_TraceContent();

}

int CCallDlgList::GetCount()
// get call dlg count
{
	int nCount = 0;
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle != -1)
			nCount++;
	}

	return nCount;
}

int CCallDlgList::GetType(int dlg)
// get call type (in/out) of this dlg
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return dlgInfo[i].type;
	}
	return -1;

}

int CCallDlgList::GetActiveDlgHandle()
// get active dlg handle
// return -1 if no active call 
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle != -1 && dlgInfo[i].state==CALLDLG_STATE_CONNECTED)
			return dlgInfo[i].dlgHandle;
	}
	return -1;

}

BOOL CCallDlgList::GetFlagConference(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return dlgInfo[i].bConference;
	}
	return FALSE;

}

void CCallDlgList::SetFlagConference(int dlg, BOOL bVal)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
		{
			dlgInfo[i].bConference = bVal;
			return;
		}
	}
}

CTime CCallDlgList::GetConnectTime(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return dlgInfo[i].connTime;
	}
	return 0;
}

void CCallDlgList::SetConnectTime(int dlg, CTime t)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
		{
			dlgInfo[i].connTime = t;
			return;
		}
	}

}

int CCallDlgList::GetState(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return dlgInfo[i].state;
	}
	return -1;

}

COleDateTime CCallDlgList::GetStartTime(int dlg)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if(dlgInfo[i].dlgHandle == dlg )
			return dlgInfo[i].startTime;
	}
	COleDateTime t;
	t.ParseDateTime("2000/1/1");
	return t;
}

int CCallDlgList::GetDlgHandleByState( int state)
{
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		if( dlgInfo[i].dlgHandle != -1 && dlgInfo[i].state == state)
			return dlgInfo[i].dlgHandle;
	}
	return -1;
}

int CCallDlgList::GetDlgHandleByIndex( int index)
{
	return dlgInfo[index].dlgHandle;
}

