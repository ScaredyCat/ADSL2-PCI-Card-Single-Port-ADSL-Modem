// CallDlgList.h: interface for the CCallDlgList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CALLDLGLIST_H__4DDA0540_B8CA_4741_AB0F_8DAD1DD572D1__INCLUDED_)
#define AFX_CALLDLGLIST_H__4DDA0540_B8CA_4741_AB0F_8DAD1DD572D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CALLDLG_MAX_COUNT	4

#define CALLDLG_STATE_INBOUND		1	// incoming call
#define CALLDLG_STATE_OUTBOUND		2	// outgoing call
#define CALLDLG_STATE_CONNECTED		3	// connected call
#define CALLDLG_STATE_LOCAL_HOLD	4	// hold by local
#define CALLDLG_STATE_REMOTE_HOLD	5	// hold by remote

#define CALLDLG_TYPE_CALLIN		1
#define CALLDLG_TYPE_CALLOUT	2
	

struct CALL_DLG_INFO
{
	CALL_DLG_INFO() : dlgHandle(-1), state(0), type(0), bConnected(0), bRinging(0), bConference(0) {}
	int dlgHandle;
	CString telno;
	CString name;
	CString photoPath;
	CString company;
	int state;
	int type;
	BOOL bConnected;
	BOOL bRinging;
	BOOL bConference;
	CTime connTime;
	COleDateTime startTime;
};

class CCallDlgList  
{

public:
	int GetCount();
	void SetState( int dlg, int state );
	int GetState( int dlg );
	int GetType( int dlg );
	
	int GetActiveDlgHandle();
	int GetDlgHandleByState( int state);
	int GetDlgHandleByIndex( int index);
	
	COleDateTime GetStartTime( int dlg );
	void SetConnectTime( int dlg, CTime t );
	CTime GetConnectTime( int dlg );
	void SetFlagConference(int dlg, BOOL bVal);
	BOOL GetFlagConference( int dlg );
	CALL_DLG_INFO* GetInfoIdx( int idx );
	void _TraceContent();
	void Remove( int dlg);
	void SetInfo( int dlg, CALL_DLG_INFO &cdinfo );
	CALL_DLG_INFO * GetInfo( int dlg );
	BOOL Find( int dlg );
	int Add( int dlg, LPCTSTR tel, int state, int type);
	CCallDlgList();
	virtual ~CCallDlgList();

private:
	CALL_DLG_INFO dlgInfo[CALLDLG_MAX_COUNT];
};

#endif // !defined(AFX_CALLDLGLIST_H__4DDA0540_B8CA_4741_AB0F_8DAD1DD572D1__INCLUDED_)
