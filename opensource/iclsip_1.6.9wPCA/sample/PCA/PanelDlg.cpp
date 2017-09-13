// CPanelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PanelDlg.h"
#include "EditItriDlg.h"
#include "EditPersonalDlg.h"
#include "PCAUADlg.h"
#include "VoiceMailLogin.h"
#include "pack.h"
#include "RepeatOptDlg.h"
#include <stdio.h>

#include "AddPersonalDlg.h"
#include "SearchDlg.h"
#include "QueryDB.h"
#include "CallLog.h"
#include "EMailDialog.h"
#include "DlgMessage.h"
#include "EnterURIDlg.h"


//add by alan for group handle
#include "GroupManageDlg.h"
#include "GroupDeleteDlg.h"
#include "GroupEditDlg.h"
#include "GroupMsgDlg.h"

#ifdef _XCAPSERVER
#include "xcapapi.h"
#include "xcap_datatype.h"
#endif

#ifdef	_PCAUA_Res_
#include "PCAUARes.h"
#endif

// delete directory ( _rmdir() )
#include <direct.h>

#include <mmsystem.h>  // for playsound


// following is include or define for curl
#ifndef HAVE_WIN32_CONFIG_H
#include "xmlrpc_config.h"
#else
#include "xmlrpc_win32_config.h"
#ifdef WIN32

#ifdef _DEBUG
#	include <crtdbg.h>
#	define new DEBUG_NEW
#	define malloc(size) _malloc_dbg( size, _NORMAL_BLOCK, __FILE__, __LINE__)
#	undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

#endif /*WIN32*/

#endif


#include "xmlrpc.h"
#include "xmlrpc_client.h"
//#include <windows.h>
#include <stdio.h>
//#include <afxtempl.h>
//#include "AFXINET.H"
//#include "afxdlgs.h"

#if defined (XMLRPCDEFAULTTRANSPORT)
#undef XMLRPCDEFAULTTRANSPORT
#define XMLRPCDEFAULTTRANSPORT "curl"
#endif 

#define NAME "XML-RPC C Test Client"

#define HOST "http://127.0.0.1:8080/RPC2"
char * g_szServer = "localhost";

#define DefaultVoiceMailIP "140.96.178.65"
#define DefaultVoiceMailPort "8000"
#define ResultLimit	100
#define ID_TIMER_VM  1001


HANDLE g_hSem;
CString *pResult;
CListCtrl *presult;
xmlrpc_bool	bDeleteOK=false;


// declare for VoiceMail Email Setting
char *cEmail=(char*) malloc(1024);
xmlrpc_int32 dwNotify = 0;
//char *cNotify=(char*) malloc(1024);
char *cSaveEmail=(char*) malloc(1024);;
xmlrpc_int32 iSaveNotify=0;
xmlrpc_bool *bSavePasswordResult;
xmlrpc_bool *bSaveEmailResult;
xmlrpc_bool *bPassAuth;

struct msg
{
	//int	sn;
	CString Caller;
	CString Date;
	CString Time;
	BOOL Read;
	CString URL;
};


CArray <msg,msg> ArrayMsg;

struct msgItri
{

	CString m_EmployId;
	CString m_DepartmentId;
	CString m_CName;
	CString m_Email;
	CString m_Ext;
};

CArray <msgItri,msgItri> ArrayMsgItri;
CString ErrorMsg;



int die_if_fault_occurred (xmlrpc_env *env)
{
    if (env->fault_occurred) 
	{ 
		CString msg;
		msg.Format("XML-RPC Fault: %s (%d)",env->fault_string, env->fault_code);
		return 0;
    }
	else
		return 1;
}

void CPanelDlg::AddMsgItem(int index, LPCSTR sCaller, LPCSTR sDate, LPCSTR sTime, BOOL bStatus, LPCSTR sURL)
{
	if (bStatus)
		m_ListVoiceMail.SetItemData(m_ListVoiceMail.AddItem(1,_T(sCaller), _T(sDate), _T(sTime),_T(sURL)),index);
	else
		m_ListVoiceMail.SetItemData(m_ListVoiceMail.AddItem(0,_T(sCaller), _T(sDate), _T(sTime),_T(sURL)),index);
		

}

static void function_delete(char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
    /* Check to see if a fault occurred. */
    if(!die_if_fault_occurred(env))
		return;
	long prevCount;
	xmlrpc_parse_value(env, result, "b", &bDeleteOK);
	int ss=22;
    //if(!die_if_fault_occurred(env))
	//	return;
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

}

static void print_state_name_callback (char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
 

	int		total = 0;
	xmlrpc_value	*array,*record;
	long prevCount;
    /* Check to see if a fault occurred. */
    if(!die_if_fault_occurred(env))
		return;

	ArrayMsg.RemoveAll(); 
	xmlrpc_parse_value(env, result, "A", &array);
    if(!die_if_fault_occurred(env))
		return;

	total=xmlrpc_array_size(env, array);
	msg Msg;
	if ( total>0 ) {

		char	*sCaller;
		char	*sURL;
		xmlrpc_bool	bRead;
		FILETIME	fTime;
		
		for (int i=0; i<total; i++) {
			record = xmlrpc_array_get_item(env, array, i);
			if ( !die_if_fault_occurred(env) )
				break;
			xmlrpc_parse_value(env, record, "{s:s,s:s,s:b,s:i,s:i,*}",
				"Caller", &sCaller,
				"URL", &sURL,
				"bRead", &bRead,
				"hTime", &(fTime.dwHighDateTime),
				"lTime", &(fTime.dwLowDateTime));
			if (!die_if_fault_occurred(env))
				break;

			CTime	callTime( (FILETIME&)fTime );
			CString	fmtTime = callTime.Format( "%H:%M:%S" );
			CString	fmtDate = callTime.Format( "%Y/%m/%d" );
			
			Msg.Caller=sCaller;
			Msg.Date=fmtDate;
			Msg.Time=fmtTime;
			Msg.Read=bRead;
			Msg.URL=sURL;
			ArrayMsg.Add(Msg);

		}
	


	}


	

	
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

	
 
}

static void search_itri (char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
 

	int		total = 0;

	char *m_EmployId,*m_DepartmentId,*m_CName,*m_Email,*m_Ext;
	xmlrpc_value	*array,*record;
	long prevCount;
    /* Check to see if a fault occurred. */
    if(!die_if_fault_occurred(env))
		return;

	ArrayMsgItri.RemoveAll(); 
	xmlrpc_parse_value(env, result, "A", &array);
    if(!die_if_fault_occurred(env))
		return;

	total=xmlrpc_array_size(env, array);
	msgItri Msg;
	if ( total>0 ) 
	{	
		size_t *str_len;
		for (int i=0; i<total; i++) 
		{
			record = xmlrpc_array_get_item(env, array, i);
			if ( !die_if_fault_occurred(env) )
				break;
			xmlrpc_parse_value(env, record, "{s:s,s:6,s:6,s:s,s:s,*}",
				"m_EmployId", &m_EmployId,
				"m_DepartmentId", &m_DepartmentId,&str_len,
				"m_CName", &m_CName,&str_len,
				"m_Email", &m_Email,
				"m_Ext",&m_Ext);
			if (!die_if_fault_occurred(env))
				break;

			Msg.m_EmployId=m_EmployId;
			Msg.m_DepartmentId=m_DepartmentId;
			Msg.m_CName=m_CName;
			Msg.m_Email=m_Email;
			Msg.m_Ext=m_Ext;
			ArrayMsgItri.Add(Msg);

		}
	


	}


	

	
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

	
 
}


static void getEmail (char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
	int aa=0;

	long prevCount;

    if(!die_if_fault_occurred(env))
		return;
	//xmlrpc_parse_value(env, result, "A", &array);
    //if(!die_if_fault_occurred(env))
	//	return;

	//record = xmlrpc_array_get_item(env, array, 0);
	//if ( !die_if_fault_occurred(env) )
	//	return;

	
	//xmlrpc_parse_value(env, record, "{s:s,s:s,*}", "email",&cEmail,"notify", &cNotify);
	xmlrpc_parse_value(env, result, "(si)", &cEmail,&dwNotify);
	if (!die_if_fault_occurred(env))
		return;

	strcpy(cSaveEmail,cEmail);
	iSaveNotify=dwNotify;
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

}

static void setPassword(char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{

	long prevCount;
	xmlrpc_bool  *bResult;

	if (!die_if_fault_occurred(env))
		return;
	xmlrpc_parse_value(env, result, "b", &bResult);
	if (!die_if_fault_occurred(env))
		return;

	bSavePasswordResult=bResult;
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

}

static void setEmail(char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
	long prevCount;
	xmlrpc_bool  *bResult;

	if (!die_if_fault_occurred(env))
		return;
	xmlrpc_parse_value(env, result, "b", &bResult);
	if (!die_if_fault_occurred(env))
		return;
	bSaveEmailResult=bResult;
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

}

//add by alan , refresh phonebook ---for xcap
void refreshPBook(CSortListCtrl	*m_ListPersonal)
{
	if(m_ListPersonal == NULL)
		return;
	m_ListPersonal->DeleteAllItems();
	
#ifdef _XCAPSERVER
//	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");	

		
	//check default group, if not exist ..create it
	int ret = isGroupExist(NULL);
	if (ret == GRP_NOT_FOUND) {
		if (!CreateContactDoc())
//			MessageBox(_T("LMS connected error!!"),_T("PhoneBook"),MB_OK|MB_ICONWARNING);
			return;
	}
	else if (ret == SERVER_ERROR) {
//		MessageBox(_T("LMS server error!!"),_T("PhoneBook"),MB_OK|MB_ICONWARNING);
		return;
	}

	int intPersonalCount=GetBuddyCount();
	
	// if we get records, then we evaluate the number of records
	CString getName,getDivision,getTel,getRemark,getPic,getGroup;
	if (intPersonalCount>0	)
	{
		BuddyList *myBuddyList = GetAllBuddyList();
		BuddyList *tmpPtr;
		if(myBuddyList!=NULL)
		{
			tmpPtr = myBuddyList;
			while(tmpPtr!=NULL)
			{
				if(tmpPtr->buddy ==NULL)
				{
					tmpPtr = tmpPtr->next;
					continue;
				}
				strcpy((LPTSTR)getName.GetBuffer(64),tmpPtr->buddy->m_displayname);
				getName.ReleaseBuffer();
				if(tmpPtr->buddy->m_division[0] != 0)
				{
					strcpy((LPTSTR)getDivision.GetBuffer(64),tmpPtr->buddy->m_division);
					getDivision.ReleaseBuffer();
				
				}
				if(tmpPtr->buddy->m_remark[0] != 0)
				{
					strcpy((LPTSTR)getRemark.GetBuffer(64),tmpPtr->buddy->m_remark);
					getRemark.ReleaseBuffer();
				
				}
				if(tmpPtr->buddy->m_telephone[0] != 0)
				{
					strcpy((LPTSTR)getTel.GetBuffer(64),tmpPtr->buddy->m_telephone);
					getTel.ReleaseBuffer();
				
				}
				strcpy((LPTSTR)getGroup.GetBuffer(64),tmpPtr->group);
				getGroup.ReleaseBuffer();
				
				
				m_ListPersonal->SetItemData(m_ListPersonal->AddItem(Presence_Offline, _T(getName), _T(getGroup),_T(PRE_OFFLINE), _T(PRE_OFFLINE), _T(getTel),_T(getDivision), _T(getRemark)),0);
				tmpPtr = tmpPtr->next;
			}
			FreeBuddyList(myBuddyList);

		}
	}


#endif
}

//#include <afxdao.h>
/////////////////////////////////////////////////////////////////////////////
// CPanelDlg dialog

CPanelDlg* g_pdlgPCAExtend = NULL;

CPanelDlg::CPanelDlg(CWnd* pParent /*=NULL*/)
	: CSkinDialog(CPanelDlg::IDD, pParent)
{
	g_pdlgPCAExtend = this;

	m_bItriSearch=false;

	//{{AFX_DATA_INIT(CPanelDlg)
	m_ITRI_EmployId=_T("");
	m_ITRI_DepartmentId=_T("");
	m_ITRI_CName=_T("");
	m_ITRI_Email=_T("");
	m_ITRI_Ext=_T("");
	m_strITRILab = _T("");
	//}}AFX_DATA_INIT
}

void CPanelDlg::DoDataExchange(CDataExchange* pDX)
{
	CSkinDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPanelDlg)
	DDX_Control(pDX, IDC_LISTSEARCH, m_ListSearch);
	DDX_Control(pDX, IDC_LISTPERSONAL, m_ListPersonal);
	DDX_Control(pDX, IDC_LISTITRI, m_ListItri);
	DDX_Control(pDX, IDC_LISTCallLog, m_ListCallLog);
	DDX_Control(pDX, IDC_LISTVoiceMail,m_ListVoiceMail);
	DDX_Text(pDX, IDC_ITRI_EmployId, m_ITRI_EmployId);
	DDX_Text(pDX, IDC_ITRI_DepartmentId, m_ITRI_DepartmentId);
	DDX_Text(pDX, IDC_ITRI_CName, m_ITRI_CName);
	DDX_Text(pDX, IDC_ITRI_Email, m_ITRI_Email);
	DDX_Text(pDX, IDC_ITRI_Ext, m_ITRI_Ext);
	DDX_CBString(pDX, IDC_ITRI_LAB, m_strITRILab);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPanelDlg, CSkinDialog)
	//{{AFX_MSG_MAP(CPanelDlg)
	ON_BN_CLICKED(IDC_BTNPERSONAL, OnBtnpersonal)
	ON_BN_CLICKED(IDC_BTNITIR, OnBtnItri)
	ON_BN_CLICKED(IDC_BTNSEARCH, OnBtnsearch)
	ON_NOTIFY(NM_DBLCLK, IDC_LISTITRI, OnDblclkListitir)
	ON_NOTIFY(NM_DBLCLK, IDC_LISTPERSONAL, OnDblclkListpersonal)
	ON_BN_CLICKED(IDC_BTN_CallLog_Profile,OnBtnCallLogProfile)
	ON_BN_CLICKED(IDC_BTN_CallLog_Detail,OnBtnCallLogDetail)
	ON_BN_CLICKED(IDC_BTNVoiceMailReceive,OnBtnVoiceMailReceive)
	ON_BN_CLICKED(IDC_BTNVoiceMailPlay,OnBtnVoiceMailPlay)
	ON_BN_CLICKED(IDC_BTNVoiceMailDelete,OnBtnVoiceMailDelete)
	ON_BN_CLICKED(IDC_BTNPERSONAL_ADD, OnBtnpersonalAdd)
	ON_BN_CLICKED(IDC_BTNPERSONAL_DEL, OnBtnpersonalDel)
	ON_BN_CLICKED(IDC_BTNVoiceMail,OnBtnVoiceMail)
	ON_NOTIFY(NM_DBLCLK, IDC_LISTSEARCH, OnDblclkListsearch)
	ON_WM_MOVING()
	ON_BN_CLICKED(IDC_BTNCallLog, OnBTNCallLog)
	ON_BN_CLICKED(IDC_BTNPhone, OnBTNPhone)
	ON_NOTIFY(NM_DBLCLK, IDC_LISTCallLog, OnDblclkLISTCallLog)
	ON_NOTIFY(NM_RCLICK, IDC_LISTCallLog, OnRclickListCallLog)
	ON_NOTIFY(NM_RCLICK, IDC_LISTPERSONAL, OnRclickListPersonal)
	ON_COMMAND(IDM_CallLogDial, OnCallLogDial)
	ON_COMMAND(IDM_CallLogPro,  OnCallLogPro)
	ON_COMMAND(IDM_CallLogErase,OnCallLogErase)
	ON_COMMAND(IDM_AddressBkAdd,OnAddressBkAdd)
	ON_COMMAND(IDM_AddressBkDelete,OnAddressBkDelete)
	ON_COMMAND(IDM_AddressBkEdit,OnAddressBkEdit)
	ON_COMMAND(IDM_AddressBkDial,OnAddressBkDial)
	ON_COMMAND(IDM_AddressBkIM,OnAddressBkIM)
	ON_COMMAND(IDM_AddressBkIMTo, OnAddressBkIMTo)
	ON_BN_CLICKED(IDC_BTNPERSONAL_Edit, OnBTNPERSONALEdit)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_ITRI_Search,OnBtnItriSearch)
	ON_BN_CLICKED(IDC_ITRI_CLEAR, OnBtnItriClear)
	ON_COMMAND(IDM_PRESENCE_STATUS_OFFLINE, OnSetPresenceStatusOffline)
	ON_COMMAND(IDM_PRESENCE_STATUS_ONLINE, OnSetPresenceStatusOnline)
	ON_COMMAND(IDM_NewGroup, OnNewGroup)
	ON_COMMAND(IDM_AddressBkBlock, OnAddressBkBlock)
	ON_COMMAND(IDM_AddressBkUnBlock, OnAddressBkUnBlock)
	ON_COMMAND(IDM_DeleteGroup, OnDeleteGroup)
	ON_COMMAND(IDM_EditGroup, OnEditGroup)
	ON_COMMAND(IDM_AddressBkOfflineMsg, OnAddressBkOfflineMsg)
	ON_COMMAND(IDM_CallLogDetail,OnBtnCallLogDetail)
	ON_COMMAND(IDM_EXPORT,AddressExport)
	ON_COMMAND(IDM_IMPORT,AddressImport)
	ON_COMMAND(IDM_AddressBkGroupMsg, OnAddressBkGroupMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelDlg message handlers

BOOL CPanelDlg::OnInitDialog()
{

	// get VoiceMail URL,Port,Account,Password from registery
	CWinApp* pApp=AfxGetApp();
	BOOL bEnableVM = pApp->GetProfileInt( "", "EnableVoiceMail", 0);

	if ( bEnableVM )
	{
		CString VMURL,VMPort,VMAccount,VMPasswd;
		VMURL = pApp->GetProfileString("","VoiceMailAddress");
		VMPort = pApp->GetProfileString("","VoiceMailPort");
		VMAccount = pApp->GetProfileString( "", "VoiceMailAccount");
		VMPasswd = pApp->GetProfileString( "", "VoiceMailPassword");

		SetConnectionURL(VMURL);
		SetConnectionPort(VMPort);
		SetAccountId(VMAccount);
		SetAccountPasswd(VMPasswd);

		// get VoiceMail receive interval
		DWORD dwValue;
		dwValue=pApp->GetProfileInt("","VoiceMailInterval",5);		// default update every 5min.

		// setup VoiceMail Timer : ID_TIMER_VM
		SetTimer(ID_TIMER_VM,dwValue*60000,NULL);

		// set iVoiceMailError record error count for connecting VoiceMail Server
		iVoiceMailError=0;
	}

	//get AP location, default in .\debug\xx.exe
	//so, we have to process it after getting location of this AP
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//process AP location
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);


	// init skin system
	CString strSkinName = projectpath;
	strSkinName += "\\skinPanel.ini";
	SetSkinFile((LPCTSTR)strSkinName);
	CSkinDialog::OnInitDialog();
	
	AddCustSkinButton( "BTN_ADD", CRect(0,0,64,18), IDB_BTN_ADD, "Add");
	AddCustSkinButton( "BTN_EDIT", CRect(0,0,64,18), IDB_BTN_EDIT, "Edit");
	AddCustSkinButton( "BTN_DELETE", CRect(0,0,64,18), IDB_BTN_DEL, "Delete");
	AddCustSkinButton( "BTN_DIAL", CRect(0,0,64,18), IDB_BTN_DIAL, "Dial");
	AddCustSkinButton( "BTN_PROFILE", CRect(0,0,64,18), IDB_BTN_PROFILE, "Profile");
	AddCustSkinButton( "BTN_DETAIL", CRect(0,0,64,18), IDB_BTN_DETAIL, "Detail");
	AddCustSkinButton( "BTN_LISTEN", CRect(0,0,64,18), IDB_BTN_LISTEN, "Listen");
	AddCustSkinButton( "BTN_Setting", CRect(0,0,64,18), IDB_BTN_SETTING, "Setting");
	AddCustSkinButton( "BTN_SEARCH", CRect(0,0,64,18), IDB_BTN_SEARCH, "Search");
	AddCustSkinButton( "BTN_CLEAR", CRect(0,0,64,18), IDB_BTN_CLEAR, "Clear");

	//Set default layout (personal page)
	StatePersonal();
	SetButtonCheck( "BTN_TAB_PERSONAL", TRUE);

	//Select back color of ListView
	COLORREF crBkColor = RGB(134,174,250);
	//Select back color of txt 
	COLORREF txtBgColor=RGB(153,204,255);


	//==============================================
	//  ListView in  Itri
	//==============================================

	//Set back color of ListView
	m_ListItri.SetBkColor(crBkColor);
	//Set back color of txt 
	m_ListItri.SetTextBkColor(crBkColor);

	//Define extension style of ListView
	(void)m_ListItri.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );

	//Define header name and length of fields
	m_ListItri.SetHeadings( _T("EmployID,62;DepartmentID,70;ChineseName,78;Extension,70;Email,170") );
	m_ListItri.LoadColumnInfo();


	// Query from database, class=1 indicate "Itri"

	// now, close this function
	/*
	CDaoDB dbItir;
	CString cs_1="SELECT * FROM personal where class=1";
	dbItir.Open(dbOpenDynaset,cs_1,0);


	// whether we get any record in database ?
	if (dbItir.GetRecordCount())
	{

		// if we get records, then we evaluate the number of records
		dbItir.MoveLast();
		int intDbCount=dbItir.GetRecordCount();
		dbItir.MoveFirst();

		// add items into ListView
		if (intDbCount>0	)
		{

			if(intDbCount==1)
				(void)m_ListItri.AddItem( _T(dbItir.m_name), _T(dbItir.m_id), _T(dbItir.m_division),_T(dbItir.m_department),_T(dbItir.m_telephone),_T(dbItir.m_remark) );
			else
			{
				(void)m_ListItri.AddItem( _T(dbItir.m_name), _T(dbItir.m_id), _T(dbItir.m_division),_T(dbItir.m_department),_T(dbItir.m_telephone),_T(dbItir.m_remark) );
	
				for (int i=0;i<(intDbCount-1);i++)
				{
					dbItir.MoveNext();
					(void)m_ListItri.AddItem( _T(dbItir.m_name), _T(dbItir.m_id), _T(dbItir.m_division),_T(dbItir.m_department),_T(dbItir.m_telephone),_T(dbItir.m_remark) );
				}
			}

		}

	}
  */

	//==============================================
	//  ListView in  Personal
	//==============================================
	

	//Define extension style of ListView
	(void)m_ListPersonal.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
	//Define header name and length of fields
	// Arlene modified to show the presence status.
#ifdef _SIMPLE
#ifndef _XCAPSERVER
	m_ListPersonal.SetHeadings( _T("Name,70;Presence,55;Call State,60;TEL,120;Company,70;Remark,70") );
#else
	m_ListPersonal.SetHeadings( _T("Name,65;Group,65;Presence,60;Call State,60;TEL,120;Company,70;Remark,70") );
#endif
#endif
#ifndef _SIMPLE
	m_ListPersonal.SetHeadings( _T("Name,70;TEL,120;Company,70;Remark,70") );	
#endif
	m_ListPersonal.LoadColumnInfo();


	//Set back color of ListView
	m_ListPersonal.SetBkColor(crBkColor);
	//Set back color of txt 
	m_ListPersonal.SetTextBkColor(crBkColor);


	// Query from database, class=2 indicate "Personal"


//add by alan
#ifdef _XCAPSERVER


	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");	
	SetOwnerName((LPSTR)(LPCTSTR)UserName);

	//check default group, if not exist ..create it
	int ret = isGroupExist(NULL);

	int ee = 0;
	if (ret == GRP_NOT_FOUND) {

		if (!CreateContactDoc())
			;//MessageBox(_T("LMS connected error!!"),_T("PhoneBook"),MB_OK|MB_ICONWARNING);

	}
	else if (ret == SERVER_ERROR) {

		//MessageBox(_T("LMS server error!!"),_T("PhoneBook"),MB_OK|MB_ICONWARNING);
	}
	else {

		ee = GetBuddyCount();
	}

#else
	CDBAccess dbPersonal;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
	//strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	dbPersonal.Initialize(strInitial,errStr);
	dbPersonal.OpenRecordSet("SELECT * FROM personal where class=2",errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	int ee=dbPersonal.GetRecordCount();
#endif

//	char charPersonalSn[20];
//	char charPersonalClass[10];
	
	//load image list for PhoneBook
	CBitmap bmOnline, bmUnknown;
#ifdef	_PCAUA_Res_
	LoadBitmap_(&bmOnline, IDB_PRE_ONLINE);
	LoadBitmap_(&bmUnknown, IDB_PRE_UNKNOWN);
	BOOL bReturn = CreateImageList_(&m_PresenceImg, IDB_PRE_OFFLINE, 16, 1, RGB(255, 255, 255));
#else
	bmOnline.LoadBitmap(IDB_PRE_ONLINE);
	bmUnknown.LoadBitmap(IDB_PRE_UNKNOWN);
	BOOL bReturn = m_PresenceImg.Create(IDB_PRE_OFFLINE, 16, 1, RGB(255, 255, 255));
#endif
	
	m_PresenceImg.Add(&bmOnline, RGB(255, 255, 255));
	m_PresenceImg.Add(&bmUnknown, RGB(255, 255, 255));
	m_ListPersonal.SetImageList(&m_PresenceImg, LVSIL_SMALL);
	
#ifdef _XCAPSERVER
	int intPersonalCount=GetBuddyCount();
#else
	int intPersonalCount=dbPersonal.GetRecordCount();
#endif
	// whether we get any record in database ?
	if(intPersonalCount)
	{
		// if we get records, then we evaluate the number of records
		CString getName,getDivision,getTel,getRemark,getPic,getGroup;
	
		
		if (intPersonalCount>0	)
		{

#ifdef _XCAPSERVER

			BuddyList *myBuddyList = GetAllBuddyList();
			BuddyList *tmpPtr;
			if(myBuddyList!=NULL)
			{
				tmpPtr = myBuddyList;

				while(tmpPtr!=NULL)
				{
					if(tmpPtr->buddy ==NULL)
					{
						tmpPtr = tmpPtr->next;
						continue;
					}
					strcpy((LPTSTR)getName.GetBuffer(64),tmpPtr->buddy->m_displayname);
					getName.ReleaseBuffer();
					if(tmpPtr->buddy->m_division[0] != 0)
					{
						strcpy((LPTSTR)getDivision.GetBuffer(64),tmpPtr->buddy->m_division);
						getDivision.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_remark[0] != 0)
					{
						strcpy((LPTSTR)getRemark.GetBuffer(64),tmpPtr->buddy->m_remark);
						getRemark.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_telephone[0] != 0)
					{
						strcpy((LPTSTR)getTel.GetBuffer(64),tmpPtr->buddy->m_telephone);
						getTel.ReleaseBuffer();
					
					}
					strcpy((LPTSTR)getGroup.GetBuffer(64),tmpPtr->group);
					getGroup.ReleaseBuffer();
					
					
					m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Offline, _T(getName), _T(getGroup),_T(PRE_OFFLINE), _T(PRE_OFFLINE), _T(getTel),_T(getDivision), _T(getRemark)),0);
//					UpdateBuddyPresence(getName, 0, PRE_OFFLINE, PRE_OFFLINE);
					tmpPtr = tmpPtr->next;
				}

				FreeBuddyList(myBuddyList);

			}

#else
			if(intPersonalCount==1)
			{	

				//transform m_sn from int to char[]
				itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
				//transform m_class form int to char
				itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
				//add items into ListView 

				dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);				
				getName.ReleaseBuffer();
				dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
				getDivision.ReleaseBuffer();
				dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
				getTel.ReleaseBuffer();
				dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
				getRemark.ReleaseBuffer();
				dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
				getPic.ReleaseBuffer();
				m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel),_T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));

			}
			else
			{
				//transform m_sn from int to char[]
				itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
				//transform m_class form int to char
				itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
				//add items into ListView 

				dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
				getName.ReleaseBuffer();
				dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
				getDivision.ReleaseBuffer();
				dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
				getTel.ReleaseBuffer();
				dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
				getRemark.ReleaseBuffer();
				dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
				getPic.ReleaseBuffer();
				m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
				AfxTrace("Add %s to Personal Book\n", getName);

				for (int i=0;i<(intPersonalCount-1);i++)
				{
					dbPersonal.MoveNext();
					//transform m_sn from int to char[]
					itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
					//transform m_class form int to char
					itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
					//add items into ListView 

					dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
					getName.ReleaseBuffer();
					dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
					getDivision.ReleaseBuffer();
					dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
					getTel.ReleaseBuffer();
					dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
					getRemark.ReleaseBuffer();
					dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
					getPic.ReleaseBuffer();
					m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel),_T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					AfxTrace("Add %s to Personal Book\n", getName);
					
				}
			}
			dbPersonal.CloseRecordSet();

#endif

		}
			
	}
	//add by alan: publish self status to online
	g_pMainDlg->UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	
	//add by alan: subscribe winfo
	g_pMainDlg->SubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	// Arlene added : subscribe the member in m_ListPersonal
	SubscribePhoneBook();

	//==============================================
	//  ListView in  Search
	//==============================================


	//Define extension style of ListView
	(void)m_ListSearch.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
	//Define header name and length of fields
#ifdef _XCAPSERVER
	m_ListSearch.SetHeadings( _T("sn,0;class,0;Name,70;ID(Group),70;Division(Company),70;Department ID(Remark),120;Ext(TEL),120;Pic,0;Remark,0") );
#else
	m_ListSearch.SetHeadings( _T("sn,0;class,0;Name,70;ID,70;Division(Company),70;Department ID,70;Ext(TEL),120;Pic,0;Remark,0") );
#endif
	m_ListSearch.LoadColumnInfo();


	//Set back color of ListView
	m_ListSearch.SetBkColor(crBkColor);
	//Set back color of txt 
	m_ListSearch.SetTextBkColor(crBkColor);	


	//close opened DB
	//dbItir.Close();
	//dbPersonal.Close();
	//==============================================
	//  ListView in  CallLog
	//==============================================

	CBitmap bm,bm1,bm2,bm3;	
#ifdef	_PCAUA_Res_
	LoadBitmap_(&bm, IDB_CALLLOG_OUT);
	LoadBitmap_(&bm1, IDB_CALLLOG_IN_MISS);
	LoadBitmap_(&bm2, IDB_CALLLOG_IN_REJECT);
	LoadBitmap_(&bm3, IDB_CALLLOG_OUT_FAIL);
	CreateImageList_(&m_SmallImg, IDB_CALLLOG_IN, 16, 1, RGB(255, 255, 255)); 
#else
	bm.LoadBitmap(IDB_CALLLOG_OUT);
	bm1.LoadBitmap(IDB_CALLLOG_IN_MISS);
	bm2.LoadBitmap(IDB_CALLLOG_IN_REJECT);
	bm3.LoadBitmap(IDB_CALLLOG_OUT_FAIL);	
	m_SmallImg.Create(IDB_CALLLOG_IN, 16, 1, RGB(255, 255, 255)); 
#endif
	m_SmallImg.Add(&bm,RGB(255,255,255));
	m_SmallImg.Add(&bm1,RGB(255,255,255));
	m_SmallImg.Add(&bm2,RGB(255,255,255));
	m_SmallImg.Add(&bm3,RGB(255,255,255));

	//Define extension style of ListView
	(void)m_ListCallLog.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
	//Define header name and length of fields
	m_ListCallLog.SetHeadings( _T("Tel,70;StartTime,140;EndTime,140;URI,70") );
	m_ListCallLog.SetImageList(&m_SmallImg,LVSIL_SMALL);
	m_ListCallLog.LoadColumnInfo();
	

	//Set back color of ListView
	m_ListCallLog.SetBkColor(crBkColor);
	//Set back color of txt 
	m_ListCallLog.SetTextBkColor(crBkColor);	
	
	CCallLog callLog;
	callLog.storeAllRecord();
	CSortListCtrl* plistctrl;
	plistctrl=&m_ListCallLog;

	callLog.pourRecord(plistctrl);


	//==============================================
	//  ListView in  VoiceMail
	//==============================================
	
#ifdef	_PCAUA_Res_
	CreateImageList_(&m_SmallImgVoiceMail, IDB_VOICEMAIL, 16, 1, RGB(255, 255, 255)); 
#else
	m_SmallImgVoiceMail.Create(IDB_VOICEMAIL, 16, 1, RGB(255, 255, 255)); 
#endif


	//Define extension style of ListView
	(void)m_ListVoiceMail.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
	//Define header name and length of fields
	//m_ListVoiceMail.SetHeadings( _T("Caller,70;Date,70;Time,70;URL,140") );
	m_ListVoiceMail.SetHeadings( _T("Caller,70;Date,70;Time,70") );

	m_ListVoiceMail.SetImageList(&m_SmallImgVoiceMail,LVSIL_SMALL);
	//Set back color of ListView
	m_ListVoiceMail.SetBkColor(crBkColor);
	//Set back color of txt 
	m_ListVoiceMail.SetTextBkColor(crBkColor);
	
	// init position of control items
	InitDlgControlPos();
	ShowPhoneBook();

	m_pUIDlg = AfxGetMainWnd();
	

	if ( bEnableVM)
	{
		// start Receive  VoiceMail
		OnBtnVoiceMailReceive();
	}
	//m_class_select=0;

// added by shen, 20050407
	BOOL b = CreateDirectory(projectpath+"\\pic", NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//add by alan for unsubscribe phone book
void CPanelDlg::unSubscribePhoneBook()
{
#ifdef _SIMPLE

	// can't subscribe the whole phone book if not initialized
	if ( m_ListPersonal.m_hWnd == NULL)
		return;	
	int nCount = m_ListPersonal.GetItemCount();
	for (int i=0 ; i<nCount ; i++)
	{
		CString strNum = m_ListPersonal.GetItemText(i,defPersonalTelephone);
		g_pMainDlg->unSubscribe(strNum);

	}
#endif
}

void CPanelDlg::SubscribePhoneBook()
{
#ifdef _SIMPLE

	// can't subscribe the whole phone book if not initialized
	if ( m_ListPersonal.m_hWnd == NULL)
		return;	
	char tmp_action[64];
	int tmpcount=0;
	int nCount = m_ListPersonal.GetItemCount();
	for (int i=0 ; i<nCount ; i++)
	{

		
		int nNum = m_ListPersonal.GetItemData(i);
		CString strNum = m_ListPersonal.GetItemText(i,defPersonalTelephone);
//		CString strNum = m_ListPersonal.GetItemText(i,defPersonalName);
		
		if(!GetRuleActionByURI((LPSTR)(LPCTSTR)strNum, tmp_action))//rule not exist....
		{	
			char buf[128];
			sprintf(buf,"Auth Rule not exist for tel: %s!",strNum);
			AfxMessageBox(buf);  
			continue;
		}
		if(!_stricmp(tmp_action,"block"))//if the user is block...do not need to subscribe this buddy
		{
			m_ListPersonal.SetItemText(i, defPersonalPresence, "Block");
			continue;
		}
		
		g_pMainDlg->Subscribe(strNum);

	}

#endif
}

void CPanelDlg::StateInitial()
{
	//In initial state, just show Itri.
	m_ListItri.ShowWindow(SW_SHOWNA);


	//Hide personal and search pages and "VoiceMail"	
	m_ListPersonal.ShowWindow(SW_HIDE);
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);



	// hide  Serach paragraph
	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_HIDE) ;



	//Hide buttons in Personal
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_ADD);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_DEL);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_Edit);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_ADD);
	pSEARCH->ShowWindow(SW_SHOWNA) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_DEL);
	pSEARCH->ShowWindow(SW_SHOWNA) ;

	//hide button Receive,Delete,Play in VoiceMail
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailReceive);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailDelete);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailPlay);
	pSEARCH->ShowWindow(SW_HIDE) ;	
	
	//hide itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_LAB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_HIDE);
}

void CPanelDlg::OnBtnpersonal() 
{
	m_bItriSearch=false;

	// TODO: Add your control notification handler code here
	StatePersonal();
}

void CPanelDlg::StatePersonal()
{
	//just show "Personal"
	m_ListPersonal.ShowWindow(SW_SHOWNA);

	//hide "Itri" and "Search" and "CallLog" and "VoiceMail"
	m_ListItri.ShowWindow(SW_HIDE);
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListCallLog.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);

	// hide  Serach paragraph

	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_HIDE) ;


	SetButtonPos("BTN_ADD", 10,66 );
	SetButtonShow("BTN_ADD", TRUE);
	SetButtonPos("BTN_EDIT", 80,66 );
	SetButtonShow("BTN_EDIT", TRUE);
	SetButtonPos("BTN_DELETE", 150,66 );
	SetButtonShow("BTN_DELETE", TRUE);
	SetButtonPos("BTN_DIAL", 220,66 );
	SetButtonShow("BTN_DIAL", TRUE);

	SetButtonShow("BTN_SEARCH", FALSE);
	SetButtonShow("BTN_CLEAR", FALSE);

	//hide itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_LAB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_HIDE);


}

void CPanelDlg::OnBtnItri() 
{
	m_bItriSearch=true;
	StateItri();

}

void CPanelDlg::StateSearch()
{

	//show ListView "Search"
	m_ListSearch.ShowWindow(SW_SHOWNA);

	//hide ListView "Itri" and "Personal" and "VoiceMail"
	m_ListItri.ShowWindow(SW_HIDE);
	m_ListPersonal.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);	

	// discover  Serach paragraph
	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_RESTORE) ;

	SetButtonShow("BTN_ADD", FALSE);
	SetButtonShow("BTN_EDIT", FALSE);
	SetButtonShow("BTN_DELETE", FALSE);
	SetButtonShow("BTN_DIAL", FALSE);

	//hide itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_LAB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_HIDE);

	SetButtonShow("BTN_SEARCH", FALSE);
	SetButtonShow("BTN_CLEAR", FALSE);
}

void CPanelDlg::OnBtnsearch() 
{
	m_bItriSearch=false;

	//change state into "Search"
	StateSearch();


	CSearchDlg searchDlg;
	CString strDbCond;

	int useSearch=0;    // whether see result, 0=NO,1=Yes
#ifdef _XCAPSERVER
	BuddyInfo *QueryKey;
#endif

	if (searchDlg.DoModal()==IDOK)
	{
		
		useSearch=1;

		//acquire data from dialog
		UpdateData(true);

#ifdef _XCAPSERVER
		QueryKey = NewBuddy();

		if (searchDlg.m_SearchName.GetLength()>0)
		{
			strcpy(QueryKey->m_displayname,(LPSTR)(LPCTSTR)searchDlg.m_SearchName);
		}
//		if (searchDlg.m_SearchID.GetLength()>0)//group
//		{
//			strcpy(QueryKey->m_id,(LPSTR)(LPCTSTR)searchDlg.m_SearchID);
//		}
		if (searchDlg.m_SearchDivision.GetLength()>0)
		{
			strcpy(QueryKey->m_division,(LPSTR)(LPCTSTR)searchDlg.m_SearchDivision);
		}
		if (searchDlg.m_SearchDepart.GetLength()>0)
		{
//			strcpy(QueryKey->m_department,(LPSTR)(LPCTSTR)searchDlg.m_SearchDepart);
			strcpy(QueryKey->m_remark,(LPSTR)(LPCTSTR)searchDlg.m_SearchDepart);
		}
		if (searchDlg.m_SearchTel.GetLength()>0)
		{
			strcpy(QueryKey->m_telephone,(LPSTR)(LPCTSTR)searchDlg.m_SearchTel);
		}
		

#else
		// strCond: prepare for search condition
		CString strCond="";

		
		if (searchDlg.m_SearchName.GetLength()>0)
		{
			strCond=strCond+" name LIKE '%"+searchDlg.m_SearchName+"%' AND";
		}

		if (searchDlg.m_SearchID.GetLength()>0)
		{
			strCond=strCond+" id LIKE '%"+searchDlg.m_SearchID+"%' AND";
		}
		
		if (searchDlg.m_SearchDivision.GetLength()>0)
		{
			strCond=strCond+" division LIKE '%"+searchDlg.m_SearchDivision+"%' AND";
		}

		if (searchDlg.m_SearchDepart.GetLength()>0)
		{
			strCond=strCond+" department LIKE '%"+searchDlg.m_SearchDepart+"%' AND";
		}

		if (searchDlg.m_SearchTel.GetLength()>0)
		{
			strCond=strCond+" telephone LIKE '%"+searchDlg.m_SearchTel+"%' AND";
		}

		strCond=strCond.Mid( 0,(strCond.GetLength()-3) );
		
		// cs : which is the condition of search action
		CString cs;

		if (strCond.GetLength()>0)
			cs="SELECT * FROM personal WHERE "+ strCond;
		else
			cs="SELECT * FROM personal ";
		strDbCond=cs;
#endif
	}


	if (useSearch==1)
	{

#ifdef _XCAPSERVER
		CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
		BuddyList *searchRes = SearchBuddy(QueryKey);
		
		if(searchRes == NULL)
		{
			MessageBox(_T("No data found"),_T("Search"),MB_OK|MB_ICONWARNING);
		}
		else
		{
//			char charSn[20],charClass[10];
			CString getName,getId,getDivision,getDepartment,getTel,getPic,getRemark,getGroup;
//			int getSn,getClass;

			int db_count=GetListCount(searchRes);
	
			if (db_count>0	)
			{
	
				m_ListSearch.DeleteAllItems();
				BuddyList *tmpPtr = searchRes;
				while(tmpPtr!=NULL)
				{
					strcpy((LPTSTR)getName.GetBuffer(64),tmpPtr->buddy->m_displayname);
					getName.ReleaseBuffer();

					if(tmpPtr->buddy->m_division[0] != 0)
					{
						strcpy((LPTSTR)getDivision.GetBuffer(64),tmpPtr->buddy->m_division);
						getDivision.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_remark[0] != 0)
					{
						strcpy((LPTSTR)getRemark.GetBuffer(64),tmpPtr->buddy->m_remark);
						getRemark.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_telephone[0] != 0)
					{
						strcpy((LPTSTR)getTel.GetBuffer(64),tmpPtr->buddy->m_telephone);
						getTel.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_remark[0] != 0)
					{
						strcpy((LPTSTR)getRemark.GetBuffer(255),tmpPtr->buddy->m_remark);
						getRemark.ReleaseBuffer();
					
					}
					strcpy((LPTSTR)getId.GetBuffer(64),tmpPtr->group);
					getId.ReleaseBuffer();
					
//					(void)m_ListSearch.AddItem( _T(charSn),_T(&charClass),_T(getName), _T(getId), _T(getDivision),_T(getDepartment),_T(getTel),_T(getPic),_T(getRemark) );
					(void)m_ListSearch.AddItem( _T(""),_T(""),_T(getName), _T(getId), _T(getDivision),_T(getRemark),_T(getTel),_T(""),_T(getRemark) );
					tmpPtr = tmpPtr->next;
				}


			}
			FreeBuddy(QueryKey);
			FreeBuddyList(searchRes);
		}
#else
		CDBAccess dbSearch;

		
		CString strInitial,errStr;
		strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
		strInitial=strInitial+projectpath+"\\PCAdb.mdb";
		dbSearch.Initialize(strInitial,errStr);
		dbSearch.OpenRecordSet(strDbCond,errStr);
		if( !errStr.IsEmpty() )
			AfxMessageBox(errStr);   
	
		//get the number of querying result

		if (dbSearch.GetRecordCount()==0)
		{
			MessageBox(_T("No data found"),_T("Search"),MB_OK|MB_ICONWARNING);
		}
		else
		{

		
			char charSn[20],charClass[10];
			CString getName,getId,getDivision,getDepartment,getTel,getPic,getRemark;
			int getSn,getClass;

			int db_count=dbSearch.GetRecordCount();
	
			if (db_count>0	)
			{
	
				m_ListSearch.DeleteAllItems();
				if(db_count==1)
				{
					getSn=dbSearch.GetIntegerFromField(0);
					getClass=dbSearch.GetIntegerFromField(1);
					itoa(getSn,charSn,10);
					itoa(getClass,charClass,10);

				 
					dbSearch.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
					getName.ReleaseBuffer();
					dbSearch.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
					getId.ReleaseBuffer();
					dbSearch.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
					getDivision.ReleaseBuffer();
					dbSearch.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
					getDepartment.ReleaseBuffer();
					dbSearch.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
					getTel.ReleaseBuffer();
					dbSearch.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
					getPic.ReleaseBuffer();
					dbSearch.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);				
					getRemark.ReleaseBuffer();
					
					(void)m_ListSearch.AddItem( _T(charSn),_T(&charClass),_T(getName), _T(getId), _T(getDivision),_T(getDepartment),_T(getTel),_T(getPic),_T(getRemark) );
				}
				else
				{
					getSn=dbSearch.GetIntegerFromField(0);
					getClass=dbSearch.GetIntegerFromField(1);
					itoa(getSn,charSn,10);
					itoa(getClass,charClass,10);
					dbSearch.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
					getName.ReleaseBuffer();
					dbSearch.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
					getId.ReleaseBuffer();
					dbSearch.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
					getDivision.ReleaseBuffer();
					dbSearch.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
					getDepartment.ReleaseBuffer();
					dbSearch.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
					getTel.ReleaseBuffer();
					dbSearch.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
					getPic.ReleaseBuffer();
					dbSearch.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);				
					getRemark.ReleaseBuffer();
					
					(void)m_ListSearch.AddItem( _T(charSn),_T(&charClass),_T(getName), _T(getId), _T(getDivision),_T(getDepartment),_T(getTel),_T(getPic),_T(getRemark) );

	
					for (int count=0;count<(db_count-1);count++)
					{
						dbSearch.MoveNext();
						getSn=dbSearch.GetIntegerFromField(0);
						getClass=dbSearch.GetIntegerFromField(1);
						itoa(getSn,charSn,10);
						itoa(getClass,charClass,10);
						dbSearch.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
						getName.ReleaseBuffer();
						dbSearch.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
						getId.ReleaseBuffer();
						dbSearch.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
						getDivision.ReleaseBuffer();
						dbSearch.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
						getDepartment.ReleaseBuffer();
						dbSearch.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
						getTel.ReleaseBuffer();
						dbSearch.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
						getPic.ReleaseBuffer();
						dbSearch.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);				
						getRemark.ReleaseBuffer();
					
						(void)m_ListSearch.AddItem( _T(charSn),_T(&charClass),_T(getName), _T(getId), _T(getDivision),_T(getDepartment),_T(getTel),_T(getPic),_T(getRemark) );
					}
				}

			}

		}
		dbSearch.CloseRecordSet();
#endif
	}
}
void CPanelDlg::OnDblclkListitir(NMHDR* pNMHDR, LRESULT* pResult) 
{

	// Get cursor client position
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);

	// Get Distance between top-Dialog and top-LISTCallLog
	CRect rct;
	GetDlgItem(IDC_LISTITRI)->GetWindowRect(&rct);
	ScreenToClient(&rct);
	int distop=rct.top;
	
	point.y=point.y-distop;

	UINT uFlags;
	int nItem = m_ListItri.HitTest(point, &uFlags);
	bool popdlg;
	popdlg=false;
	if(uFlags & LVHT_ONITEMLABEL)
		popdlg=true;


	CString cs;

	int nCount =m_ListItri.GetItemCount();
	if (popdlg==true )
	{
		int p=m_ListItri.GetSelectionMark() ;
		((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(m_ListItri.GetItemText(p,4));
 

	}
	//else
	//	CPanelDlg::OnBtnpersonalAdd();
	*pResult = 0;
}

void CPanelDlg::OnDblclkListpersonal(NMHDR* pNMHDR, LRESULT* pResult) 
{

	// Get cursor client position
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);

	// Get Distance between top-Dialog and top-LISTCallLog
	CRect rct;
	GetDlgItem(IDC_LISTPERSONAL)->GetWindowRect(&rct);
	ScreenToClient(&rct);
	int distop=rct.top;
	
	point.y=point.y-distop;

	UINT uFlags;
	int nItem = m_ListPersonal.HitTest(point, &uFlags);
	bool popdlg;
	popdlg=false;
	if(uFlags & LVHT_ONITEMLABEL)
		popdlg=true;


	CString cs;

	int nCount =m_ListPersonal.GetItemCount();
	if (popdlg==true )
	{
		int p=m_ListPersonal.GetSelectionMark() ;
		((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(m_ListPersonal.GetItemText(p,defPersonalTelephone));
 

	}
	else
		CPanelDlg::OnBtnpersonalAdd();
	*pResult = 0;
}



void CPanelDlg::ModifyDB(InfoDB* info)
{
	// In order to keep off the length of user's  fileds overflow,
	// we will limit the length of field

	int iLengthName=50;
	int iLengthId=50;
	int iLengthDivision=50;
	int iLengthDepartment=50;
	int iLengthTelephone=50;
	int iLengthRemark=255;

#ifdef _XCAPSERVER
	char tmpbuf[512];
	memset(tmpbuf, 0, sizeof(tmpbuf));

	BuddyInfo *tmpBuddy = NewBuddy();
	strcpy(tmpBuddy->m_displayname,info->m_name.Mid(0,iLengthName));

	// mandatory
	strcpy(tmpBuddy->m_telephone,info->m_telephone.Mid(0,iLengthTelephone));
	sprintf(tmpbuf, "sip:%s@itri.org.tw", tmpBuddy->m_telephone);
	strcpy(tmpBuddy->m_uri, tmpbuf);
/*
	tmpBuddy->isID= true;
	strcpy(tmpBuddy->m_id,info->m_id.Mid(0,iLengthId));
	tmpBuddy->isDepart= true;
	strcpy(tmpBuddy->m_department,info->m_department.Mid(0,iLengthDepartment));
*/	
	if(!info->m_division.IsEmpty())
	{
		strcpy(tmpBuddy->m_division,info->m_division.Mid(0,iLengthDivision));
	}
	if(!info->m_remark.IsEmpty())
	{
		strcpy(tmpBuddy->m_remark,info->m_remark.Mid(0,iLengthRemark));
	}
	if(!info->m_pic.IsEmpty())
	{
		strcpy(tmpBuddy->m_pic,info->m_pic.Mid(0,iLengthRemark));
	}
	ModifyBuddy(tmpBuddy, (LPSTR)(LPCTSTR)info->m_group);//using default Group
	FreeBuddy(tmpBuddy);

#else

	CDBAccess dbUpdatePersonal;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	dbUpdatePersonal.Initialize(strInitial,errStr);

	char txt[20];
	itoa(info->m_sn,txt,10);
	
	CString condi="sn=";
	condi+=txt;

	CString str;
	str="Update personal Set ";
	str=str+"name='"+CPanelDlg::tranStr(info->m_name.Mid(0,iLengthName))+"',";
	str=str+"id='"+CPanelDlg::tranStr(info->m_id.Mid(0,iLengthId))+"',";
	str=str+"division='"+CPanelDlg::tranStr(info->m_division.Mid(0,iLengthDivision))+"',";
	str=str+"department='"+CPanelDlg::tranStr(info->m_department.Mid(0,iLengthDepartment))+"',";
	str=str+"telephone='"+CPanelDlg::tranStr(info->m_telephone.Mid(0,iLengthTelephone))+"',";
	str=str+"remark='"+CPanelDlg::tranStr(info->m_remark.Mid(0,iLengthRemark))+"',";
	str=str+"pic='"+CPanelDlg::tranStr(info->m_pic)+"' ";
	str=str+" where "+condi;

	dbUpdatePersonal.ExecuteSql(str,errStr);
	
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);
#endif
}

void CPanelDlg::OnBtnpersonalAdd() 
{
	_OnBtnpersonalAdd(NULL);
}

void CPanelDlg::_OnBtnpersonalAdd(LPCTSTR szDefaultTelNum) 
{

	CAddPersonalDlg dlg;

	if ( szDefaultTelNum)
		dlg.m_TEL = szDefaultTelNum;

	int result;
	result=dlg.DoModal();
	

	if (result==IDOK)
	{
// added by shen, 20050407
		CString strLocalName = g_pdlgUAControl->m_strRegistarName;
		CString strStorePath = "";

		InfoDB db;

		db.m_class=2;
		db.m_department="";
		db.m_division=dlg.m_COMPANY;
		db.m_id="";
		db.m_name=dlg.m_NAME;
		db.m_pic=dlg.m_PIC;
		db.m_remark=dlg.m_REMARK;
		db.m_telephone=dlg.m_TEL;

#ifdef _XCAPSERVER
		int m_leng=dlg.m_PIC.GetLength();
		if (m_leng != 0) {
			int m_ext=dlg.m_PIC.Find('.');
			CString tt=dlg.m_PIC.Mid(m_ext,m_leng-m_ext);
			//restore pic by tel (tel is unique)

// modified by shen, 20050407
			strStorePath = "\\pic\\"+strLocalName+"-"+dlg.m_TEL+tt;
			CString newpath=projectpath + strStorePath;
			strStorePath=strStorePath+dlg.m_NAME+tt;

			CopyFile(dlg.m_PIC,newpath,FALSE);	
			db.m_pic = strStorePath;
		}
		db.m_group = dlg.m_GROUP;
		GroupList *myGroup = GetAllGroup();
		if(myGroup!=NULL)
		{
			if(!CheckExistence(myGroup, (LPSTR)(LPCTSTR)dlg.m_GROUP))
			{
				AfxMessageBox("Group not exist!Please new group first.");
				FreeGroupList(myGroup);
				return;
			}
			FreeGroupList(myGroup);
		}
		else
		{
			AfxMessageBox("Group not exist!Please new group first.");
			return;
		}
				
		if(!AddDB(&db))
		{
			AfxMessageBox("Add Buddy into PhoneBook Error!");
			return;
		}
		if(!isRuleExist((LPSTR)(LPCTSTR)dlg.m_TEL))//if rule not exist
		{
			if(!AddNewRule((LPSTR)(LPCTSTR)dlg.m_TEL,"allow"))
			{
				AfxMessageBox("Add New Authorization Rule Error!");
//				DelBuddy((LPSTR)(LPCTSTR)dlg.m_NAME, (LPSTR)(LPCTSTR)dlg.m_GROUP);
				char tmpbuf[512];
				memset(tmpbuf, 0, sizeof(tmpbuf));
				sprintf(tmpbuf, "sip:%s@itri.org.tw", dlg.m_TEL);
				DelBuddy(tmpbuf);
				return;
			}
		}
		else
			SetRuleAction2Allow((LPSTR)(LPCTSTR)dlg.m_TEL);
//		m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(db.m_name), _T(db.m_group),_T("Unknown"), _T("Unknown"), _T(db.m_telephone), _T(db.m_division), _T(db.m_remark)),0);
//modified by alan 2004,11,24
		m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Offline, _T(db.m_name), _T(db.m_group),_T(PRE_OFFLINE), _T(PRE_OFFLINE), _T(db.m_telephone), _T(db.m_division), _T(db.m_remark)),0);
//		UpdateBuddyPresence(db.m_name, 0, PRE_OFFLINE, PRE_OFFLINE);
		g_pMainDlg->Subscribe(dlg.m_TEL);
		
//		SubscribePhoneBook();

		
// added by shen, 20050407
	::WritePrivateProfileString(strLocalName, dlg.m_TEL, strStorePath, "pcaPic.ini");
	::WritePrivateProfileString(strLocalName, dlg.m_TEL+"_name", dlg.m_TEL, "pcaPic.ini");

#else
	
		
		AddDB(&db);
	
		
// Copy source pic to target dictionary : .\pic\B+...
// B is class=2  & A is class=1
// In Personal, I use personal.sn to be default file name
		int m_leng=dlg.m_PIC.GetLength();
		if (m_leng != 0) {
			int m_ext=dlg.m_PIC.Find('.');
			CString tt=dlg.m_PIC.Mid(m_ext,m_leng-m_ext);
			
			CDBAccess m_moddb;
			CString strInitial,errStr;
			strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
			strInitial=strInitial+projectpath+"\\PCAdb.mdb";
			m_moddb.Initialize(strInitial,errStr);
			m_moddb.OpenRecordSet("SELECT * FROM personal",errStr);
			if( !errStr.IsEmpty() )
				AfxMessageBox(errStr);   

			
		
			int intCount=m_moddb.GetRecordCount();
			if(intCount>0)
			{
				for(int i=0;i<(intCount-1);i++)
					m_moddb.MoveNext();
			}
			
			CString getName,getDivision,getTel,getRemark;
			int getSn,getClass;
			getSn=m_moddb.GetIntegerFromField(0);
			getClass=m_moddb.GetIntegerFromField(1);

			char c2[20];
			char class2[20];		
			itoa(getSn,c2,10);
			itoa(getClass,class2,10);
			
			
			m_moddb.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
			m_moddb.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
			m_moddb.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
			m_moddb.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);

			CString newpath=projectpath+"\\pic\\B"+c2+tt;
			CString strStorPath="\\pic\\B";
			strStorPath=strStorPath+c2+tt;
			//CPanelDlg::m_ListPersonal.AddItem( _T(c2),_T(class2),_T(getName), _T(getDivision), _T(getTel),_T(getRemark),_T(strStorPath) );
			m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),getSn);

			CopyFile(dlg.m_PIC,newpath,FALSE);
		}

		// Arlene added : subscribe the member in m_ListPersonal
		SubscribePhoneBook();
	
		
	

// Modify pic location in DB


		CDBAccess dbUp;
		
		strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
		strInitial=strInitial+projectpath+"\\PCAdb.mdb";
		dbUp.Initialize(strInitial,errStr);
		CString strUpdate;
		strUpdate="Update personal set pic='";
		strUpdate=strUpdate+strStorPath+"' where sn=";
		strUpdate=strUpdate+c2;
		dbUp.ExecuteSql(strUpdate,errStr);
		if( !errStr.IsEmpty() )
			AfxMessageBox(errStr);   
#endif		
	}
}

//modify by alan, need to response the operate result 
//void CPanelDlg::AddDB(InfoDB *info)
bool CPanelDlg::AddDB(InfoDB *info)
{
	// In order to keep off the length of user's  fileds overflow,
	// we will limit the length of field

	int iLengthName=50;
	int iLengthId=50;
	int iLengthDivision=50;
	int iLengthDepartment=50;
	int iLengthTelephone=50;
	int iLengthRemark=255;

#ifdef _XCAPSERVER
	if (GetBuddyCount() >= 200) {
		AfxMessageBox("Sorry! 好友名單已達上限 200!");
		return false;
	}
		
	char tmpbuf[512];

	memset(tmpbuf, 0, sizeof(tmpbuf));

	//CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
	BuddyInfo *tmpBuddy = NewBuddy();
	strcpy(tmpBuddy->m_displayname, info->m_name.Mid(0,iLengthName));
	
	// mandatory
	strcpy(tmpBuddy->m_telephone,info->m_telephone.Mid(0,iLengthTelephone));
	sprintf(tmpbuf, "sip:%s@itri.org.tw", tmpBuddy->m_telephone);
	strcpy(tmpBuddy->m_uri, tmpbuf);

	if(!info->m_division.IsEmpty())
	{
		strcpy(tmpBuddy->m_division,info->m_division.Mid(0,iLengthDivision));
	}
	if(!info->m_remark.IsEmpty())
	{
		strcpy(tmpBuddy->m_remark,info->m_remark.Mid(0,iLengthRemark));
	}
	if(!info->m_pic.IsEmpty())
	{
		strcpy(tmpBuddy->m_pic,info->m_pic.Mid(0,iLengthRemark));
	}

	bool ret = AddBuddyIntoList(tmpBuddy, (LPSTR)(LPCTSTR)info->m_group);
	FreeBuddy(tmpBuddy);
	return ret;

#else

	char strclass[20];
	
	itoa(info->m_class,strclass,10);
	CString str;
	str="insert into personal (class,name,id,division,department,telephone,remark,pic) values (";
	str=str+ strclass+ ",";
	str=str+"'"+CPanelDlg::tranStr(info->m_name.Mid(0,iLengthName))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_id.Mid(0,iLengthId))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_division.Mid(0,iLengthDivision))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_department.Mid(0,iLengthDepartment))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_telephone.Mid(0,iLengthTelephone))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_remark.Mid(0,iLengthRemark))+"',";
	str=str+"'"+CPanelDlg::tranStr(info->m_pic)+"')";


	CDBAccess db;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	db.Initialize(strInitial,errStr);	
	db.ExecuteSql(str,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	return true;

#endif
}



void CPanelDlg::OnBtnpersonalDel() 
{
	CString ms;
	CString ms2;
	CQueryDB qb;
	
	if (CPanelDlg::m_ListPersonal.GetItemCount()<=0)
		MessageBox(_T("Do not find any data in database"),_T("Attention"),MB_OK|MB_ICONWARNING);
	else
	{
		// append index of selected item into SelectItem. Think over multi-selected.
		// We left "-1," to be the end of SelectItem.
		CString SelectItem;
		char t[10];
		int i=-1;
		int j=-1;
		int count=0; // the number of selected items
		do
		{
			j=m_ListPersonal.GetNextItem(i,LVNI_SELECTED);
			i=j;		
			itoa(j,t,10);
			SelectItem=SelectItem+t+",";
			count++;
		}while (j>-1);
		count--;




		if (count <1)
			MessageBox(_T("Plese indicate which items be deleted"),_T("Attention"),MB_OK|MB_ICONWARNING);
		else
		{
			CString csCond,csCLSn;
			char csSn[10];

			char charNum[12];
			int intNum;
			csCond="sn=";
			int intPos=0;
			int intComma=0;
			//int intItemData;
			for (int s=0;s<count;s++)
			{
				intComma=SelectItem.Find(',',intComma);
				lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));
		
			
				intNum=atoi(charNum);				
				itoa(m_ListPersonal.GetItemData(intNum),csSn,10);
				
				csCond=csCond+csSn+" or sn=";
				ms=ms+m_ListPersonal.GetItemText(intNum,defPersonalName)+",";
				csCLSn=csCLSn+csSn+",";  // prepare for ListPersonal deleteItem


				intComma++;
				intPos=intComma;			
			}

			csCond=csCond.Left(csCond.GetLength()-6);
			ms=ms.Left(ms.GetLength()-1);



	
			ms2.Format( "Are you sure to remove this record '%s'?   ",ms);

			if (MessageBox(_T(ms2),_T("Delete"),MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
			{
				intPos=0;
				intComma=0;
				int subtract=0;
				// delet items from address book list
				CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
				for(s=0;s<count;s++)
				{
					intComma=SelectItem.Find(',',intComma);
					lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));			
					intNum=atoi(charNum);			
					
					// Arlene added : Unsubscribe the select one
					CString strNumber = m_ListPersonal.GetItemText(intNum-subtract, defPersonalTelephone);
#ifdef _XCAPSERVER
					char tmpbuf[512];
					memset(tmpbuf, 0, sizeof(tmpbuf));
					sprintf(tmpbuf, "sip:%s@itri.org.tw", m_ListPersonal.GetItemText(intNum-subtract, defPersonalTelephone));
					if (DelBuddy(tmpbuf)==false) {
//					if (DelBuddy((LPSTR)(LPCTSTR)m_ListPersonal.GetItemText(intNum-subtract, defPersonalName), NULL)==false)
						AfxMessageBox("Delete Failed!!");
						return;
					}
#endif

					g_pMainDlg->UnSubscribe(strNumber);

					
					m_ListPersonal.DeleteItem(intNum-subtract);
					subtract++;


					intComma++;
					intPos=intComma;

// added by shen, 20050407
	CString strLocalName = g_pdlgUAControl->m_strRegistarName;
	::WritePrivateProfileString(strLocalName, strNumber, "", "pcaPic.ini");
	::WritePrivateProfileString(strLocalName, strNumber+"_name", "Unknown", "pcaPic.ini");

				}

		
#ifndef _XCAPSERVER
				//delete items from database
				CDBAccess dbPersonal;
				CString strInitial,errStr;
				strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
				strInitial=strInitial+projectpath+"\\PCAdb.mdb";
				dbPersonal.Initialize(strInitial,errStr);
				CString str="Delete From personal where ";
				str=str+csCond;
				dbPersonal.ExecuteSql(str,errStr);	
				if( !errStr.IsEmpty() )
					AfxMessageBox(errStr);   
				dbPersonal.CloseConnection();
#endif
			} // usr is sure to delete itmes	
		}// end else if (count <1) -- user select itmes
	}// end else if (CPanelDlg::m_ListPersonal.GetItemCount()<=0)
}

void CPanelDlg::OnBtndoSearch() 
{
	// TODO: Add your control notification handler code here
	CSearchDlg m_search;
	CString cs_name,cs_id,cs_depid,cs_ext;
	CString cond;

//	GetDlgItemText(IDC_EDITNAME,cs_name);
//	GetDlgItemText(IDC_EDITID,cs_id);
//	GetDlgItemText(IDC_EDITDEPID,cs_depid);
//	GetDlgItemText(IDC_EDITEXT,cs_ext);
	//GetDlgItemText(IDC_EDITREMARK,cs_remark);

	if (cs_name.GetLength()>0)
	{
		cs_name.TrimRight();
		cs_name.TrimLeft();
		cond=" name='"+cs_name+"' ";
		
	}	

	if(cs_id.GetLength()>0)
	{
		if(cond.GetLength()>0)
			cond=cond+",";
		cs_id.TrimLeft();
		cs_id.TrimRight();
		cond=cond+" id='"+cs_id+"'";
	}

	if(cs_depid.GetLength()>0)
	{
		if(cond.GetLength()>0)
			cond=cond+",";
		cs_depid.TrimLeft();
		cs_depid.TrimRight();
		//cond=cond+" 
	}

	MessageBox(cond);

	m_search.DoModal();
}

void CPanelDlg::OnRadioa() 
{
	// TODO: Add your control notification handler code here

}

void CPanelDlg::OnRadiob() 
{
	// TODO: Add your control notification handler code here

}

void CPanelDlg::OnDblclkListsearch(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//get item infomation which double click by user
	int p=m_ListSearch.GetSelectionMark() ;
	
	// Do not process when use double click empty area
	if(p>-1)
	{


		//which class selected by user
		if (m_ListSearch.GetItemText(p,1)=='1')
		{
			//Keep for future to implement ITRI
		
		}
		else
		{
		
			CEditPersonalDlg personalDlg;

			//retrieve data from database into dialog
			personalDlg.m_PERSON2_NAME=m_ListSearch.GetItemText(p,2);
			personalDlg.m_PERSON2_COMPANY=m_ListSearch.GetItemText(p,4);
			personalDlg.m_PERSON2_TELEPHONE=m_ListSearch.GetItemText(p,6);
			personalDlg.m_PERSON2_REMARK=m_ListSearch.GetItemText(p,8);
			personalDlg.m_PERSON2_PIC=m_ListSearch.GetItemText(p,7);

			int rs=personalDlg.DoModal();
		
			if(rs==IDOK)
			{
			
			
				//retrieve data from dialog into InfoDB struct
				//Because "Personal" do not need fields: id and department
				//So, let them Empty.
				InfoDB info_personal;
				info_personal.m_sn=atoi(m_ListSearch.GetItemText(p,0));
				info_personal.m_class=atoi(m_ListSearch.GetItemText(p,1));
				info_personal.m_name=personalDlg.m_PERSON2_NAME;
				info_personal.m_id="";
				info_personal.m_division=personalDlg.m_PERSON2_COMPANY;
				info_personal.m_department="";
				info_personal.m_telephone=personalDlg.m_PERSON2_TELEPHONE;
				info_personal.m_remark=personalDlg.m_PERSON2_REMARK;
#ifdef _XCAPSERVER
				info_personal.m_group = personalDlg.m_PERSON2_GROUP;
#endif
		
				//We use 'B' + sn + file_type to be the target file name

				//retrieve 'sn'  
				CString strSn=m_ListSearch.GetItemText(p,0);					
			
				CString strStorePath;
				//evaluate length of pic field selected by users
				int intlength=personalDlg.m_PERSON2_PIC.GetLength();
				if (intlength != 0) {
					int intext=personalDlg.m_PERSON2_PIC.Find('.');

					//extract file type (extended file name)
					CString strExtend=personalDlg.m_PERSON2_PIC.Mid(intext,intlength-intext);
					CString newpath=projectpath+"\\pic\\B"+strSn+strExtend;   //newpath:full path
					strStorePath="\\pic\\B";	   
					strStorePath=strStorePath+strSn+strExtend;  //strStorePath: target file name
				
					//In database, we just store file name
					info_personal.m_pic=strStorePath;
				
					//copy picture selected by users into default path ".\pic\ "
					// parameter 'FALSE' : cover original data
					CopyFile(personalDlg.m_PERSON2_PIC,newpath,FALSE);	
				}
			

				//		Modify pic path , map into ./pic/B&...
				ModifyDB(&info_personal);			


				//Modify ListView Content, keep it is the same as database
				m_ListSearch.SetItemText(p,2,personalDlg.m_PERSON2_NAME);
				m_ListSearch.SetItemText(p,4,personalDlg.m_PERSON2_COMPANY);
				m_ListSearch.SetItemText(p,6,personalDlg.m_PERSON2_TELEPHONE);
				m_ListSearch.SetItemText(p,8,personalDlg.m_PERSON2_REMARK);
				//here, we just store file name, not full path
				m_ListSearch.SetItemText(p,7,strStorePath);
				}
			}
		}


	*pResult = 0;
}

void CPanelDlg::InitDlgControlPos()
{
	m_ListPersonal.SetWindowPos( NULL, 0,90,289,470, SWP_NOZORDER );
	m_ListSearch.SetWindowPos( NULL, 0,90,289,470, SWP_NOZORDER );
	m_ListItri.SetWindowPos( NULL, 0, 160, 289, 370, SWP_NOZORDER );
	m_ListCallLog.SetWindowPos( NULL, 0,90,289,470, SWP_NOZORDER );
	m_ListVoiceMail.SetWindowPos(NULL,0,90,289,470,SWP_NOZORDER);

	//Set position for CallLog : Profile,Detail
	GetDlgItem(IDC_BTN_CallLog_Profile)->SetWindowPos(NULL,146,66,60,18,SWP_NOZORDER);
	GetDlgItem(IDC_BTN_CallLog_Detail)->SetWindowPos(NULL,211,66,60,18,SWP_NOZORDER);

	//*** If want to hide button profile and detail , unmark following two lines
	GetDlgItem(IDC_BTN_CallLog_Profile)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTN_CallLog_Detail)->ShowWindow(SW_HIDE);


	//GetDlgItem(IDC_BTNPERSONAL_ADD)->SetWindowPos( NULL, 9,66,60,18, SWP_NOZORDER );
	//GetDlgItem(IDC_BTNPERSONAL_DEL)->SetWindowPos( NULL, 77,66,60,18, SWP_NOZORDER );
    //GetDlgItem(IDC_BTNPERSONAL_Edit)->SetWindowPos( NULL, 146,66,60,18, SWP_NOZORDER );
	GetDlgItem(IDC_BTNVoiceMailReceive)->SetWindowPos(NULL,9,66,60,18,SWP_NOZORDER);
	GetDlgItem(IDC_BTNVoiceMailDelete)->SetWindowPos(NULL,77,66,60,18,SWP_NOZORDER);
	GetDlgItem(IDC_BTNVoiceMailPlay)->SetWindowPos(NULL,146,66,60,18,SWP_NOZORDER);
	GetDlgItem(IDC_BTNVoiceMail)->SetWindowPos(NULL,211,66,60,18,SWP_NOZORDER);
	


	GetDlgItem(IDC_ITRI_EmployId)->SetWindowPos(NULL, 12, 103,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_CName)->SetWindowPos(NULL, 12, 136, 120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_LAB)->SetWindowPos(NULL,152, 103,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_DepartmentId)->SetWindowPos(NULL, 152, 136, 120,18,SWP_NOZORDER);
//	GetDlgItem(IDC_ITRI_Search)->SetWindowPos(NULL, 19, 70, 80, 20,SWP_NOZORDER);
//	GetDlgItem(IDC_ITRI_CLEAR)->SetWindowPos(NULL, 120, 70, 80, 20,SWP_NOZORDER);

/*	GetDlgItem(IDC_ITRI_EmployId)->SetWindowPos(NULL,6,102,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_CName)->SetWindowPos(NULL,6,133,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_DepartmentId)->SetWindowPos(NULL,160,102,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_Email)->SetWindowPos(NULL,160,133,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_Ext)->SetWindowPos(NULL,6,164,120,18,SWP_NOZORDER);
	GetDlgItem(IDC_ITRI_Search)->SetWindowPos(NULL,160,163,120,20,SWP_NOZORDER);*/

	GetDlgItem(IDC_BTNITIR)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNPERSONAL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNSEARCH)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_BTNPhone)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNCallLog)->ShowWindow(SW_HIDE);
	//hide IDC_BTNVoiceMail 
	GetDlgItem(IDC_BTNVoiceMail)->ShowWindow(SW_HIDE);

	//hide VoiceMail Buttons
	GetDlgItem(IDC_BTNVoiceMailReceive)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNVoiceMailDelete)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNVoiceMailPlay)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_BTNPERSONAL_ADD)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTNPERSONAL_DEL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BTNPERSONAL_Edit)->ShowWindow(SW_HIDE);

}

void CPanelDlg::ButtonPressed(CString m_ButtonName)
{

	if( m_ButtonName.Left(7) == "BTN_TAB" )
	{
		if( m_ButtonName == "BTN_TAB_PERSONAL" )
			OnBtnpersonal();
		else if( m_ButtonName == "BTN_TAB_SEARCH")
			OnBtnsearch();
		else if( m_ButtonName == "BTN_TAB_ITRI")
			OnBtnItri();

		SetButtonCheck( "BTN_TAB_PERSONAL", m_ButtonName == "BTN_TAB_PERSONAL");
		SetButtonCheck( "BTN_TAB_SEARCH", m_ButtonName == "BTN_TAB_SEARCH");
		SetButtonCheck( "BTN_TAB_ITRI", m_ButtonName == "BTN_TAB_ITRI");
	}

	if( m_ButtonName=="BTN_ADD" )
	{
		if( m_curPage == PAGEID_ADDRESSBOOK )
			OnBtnpersonalAdd();
		else if( m_curPage == PAGEID_CALLLOG )
			OnCallLogPro();

	}
	else if( m_ButtonName == "BTN_DELETE" )
	{
		if( m_curPage == PAGEID_ADDRESSBOOK )
			OnBtnpersonalDel();
		else if( m_curPage == PAGEID_CALLLOG )
			OnCallLogErase();
		// temporary for VoiceMail delete
		else if( m_curPage == PAGEID_VoiceMail)
			OnBtnVoiceMailDelete();
	}
	else if( m_ButtonName == "BTN_EDIT")
	{
		if( m_curPage == PAGEID_ADDRESSBOOK )
			OnAddressBkEdit();
		// temporary for VoiceMail play sound
		//else if( m_curPage == PAGEID_VoiceMail)
		//	OnBtnVoiceMailPlay();

		// temporary for VoiceMail EMail setting
		//else if ( m_curPage == PAGEID_VoiceMail)
			//OnBtnVoiceMailSetting();
	}
	else if( m_ButtonName == "BTN_DIAL" )
	{
		if( m_curPage == PAGEID_ADDRESSBOOK )
			OnAddressBkDial();
		else if( m_curPage == PAGEID_CALLLOG )
			OnCallLogDial();
	}
	else if( m_ButtonName == "BTN_PROFILE" )
	{
		OnBtnCallLogProfile();
	}
	else if(m_ButtonName == "BTN_DETAIL" )
	{
		OnBtnCallLogDetail();

	}
	else if(m_ButtonName=="BTN_LISTEN")
	{
		OnBtnVoiceMailPlay();
	}

	if( m_ButtonName=="BTN_Setting" )
	{
		if ( m_curPage == PAGEID_VoiceMail)
			OnBtnVoiceMailSetting();
	}
	if( m_ButtonName=="BTN_SEARCH" )
	{
//		if ( m_curPage == PAGEID_VoiceMail)
			OnBtnItriSearch();
	}
	if( m_ButtonName=="BTN_CLEAR" )
	{
//		if ( m_curPage == PAGEID_VoiceMail)
			OnBtnItriClear();
	}
	//SetButtonCheck( "BTN_TAB_ITRI", m_ButtonName == "BTN_TAB_ITRI");

}

void CPanelDlg::OnMoving(UINT fwSide, LPRECT pRect) 
{
	CSkinDialog::OnMoving(fwSide, pRect);
}

void CPanelDlg::ShowCallLog()
{
	// set page title
	SetText( "TEXT_TITLE", "Call Log");

	//show ListView "CallLog"
	m_ListCallLog.ShowWindow(SW_SHOWNA);
	

	//hide ListView "Itri" and "Personal" and "Search" and "VoiceMail"
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListItri.ShowWindow(SW_HIDE);
	m_ListPersonal.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);
	
	// discover  Serach paragraph
	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_RESTORE) ;

	//hide button Add,Del in "ITRI"
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_ADD);
	//pSEARCH->ShowWindow(SW_HIDE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_DEL);
	//pSEARCH->ShowWindow(SW_HIDE) ;

	//hide button Add,Del,Edit in "Personal"
	
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_ADD);
	//pSEARCH->ShowWindow(SW_HIDE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_DEL);
	//pSEARCH->ShowWindow(SW_HIDE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_Edit);
	//pSEARCH->ShowWindow(SW_HIDE) ;

	//hide button Receive,Delete,Play in "VoiceMail"
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailReceive);
	//pSEARCH->ShowWindow(SW_HIDE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailDelete);
	//pSEARCH->ShowWindow(SW_HIDE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailPlay);
	//pSEARCH->ShowWindow(SW_HIDE) ;

	//SetButtonPos("BTN_ADD", 10,66 );
	SetButtonShow("BTN_ADD", FALSE);
	SetButtonShow("BTN_EDIT", FALSE);
	SetButtonShow("BTN_LISTEN",FALSE);

	SetButtonPos("BTN_DELETE", 10,66 );
	SetButtonShow("BTN_DELETE", TRUE);

	SetButtonPos("BTN_DETAIL", 80,66 );
	SetButtonShow("BTN_DETAIL", TRUE);

	SetButtonPos("BTN_PROFILE", 150,66 );
	SetButtonShow("BTN_PROFILE", TRUE);

	SetButtonPos("BTN_DIAL", 220,66 );
	SetButtonShow("BTN_DIAL", TRUE);

	SetButtonShow("BTN_Setting",FALSE);

	SetButtonShow("BTN_SEARCH", FALSE);
	SetButtonShow("BTN_CLEAR", FALSE);

	SetButtonCheck( "BTN_TAB_PERSONAL", FALSE);
	SetButtonCheck( "BTN_TAB_SEARCH", FALSE);
	SetButtonCheck( "BTN_TAB_ITRI", FALSE);
	SetButtonEnable("BTN_TAB_PERSONAL", FALSE);
	SetButtonEnable("BTN_TAB_SEARCH", FALSE);
	SetButtonEnable("BTN_TAB_ITRI", FALSE);

	//hide itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_LAB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_HIDE);

	m_curPage = PAGEID_CALLLOG;

	m_bItriSearch=false;
}

void CPanelDlg::ShowPhoneBook()
{
	TRACE("ShowPhoneBook");
	// set page title
	SetText( "TEXT_TITLE", "Phone Book");

	//just show "Personal"
	m_ListPersonal.ShowWindow(SW_SHOWNA);

	//hide "Itri" and "Search" and "VoiceMail"
	m_ListItri.ShowWindow(SW_HIDE);
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListCallLog.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);


	// hide  Serach paragraph

	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_HIDE) ;


	//hide button Add,Del in ITRI
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_ADD);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_DEL);
	pSEARCH->ShowWindow(SW_HIDE) ;

	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_EmployId);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_DepartmentId);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_CName);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_Email);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_Ext);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_LAB);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_Search);
	pSEARCH->ShowWindow(SW_HIDE);
	pSEARCH=(CWnd*)GetDlgItem(IDC_ITRI_CLEAR);
	pSEARCH->ShowWindow(SW_HIDE);
	//hide button Receive,Delete,Play in VoiceMail
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailReceive);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailDelete);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailPlay);
	pSEARCH->ShowWindow(SW_HIDE) ;

	//show button Add,Del,Edit in Personal
	/*
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_ADD);
	pSEARCH->ShowWindow(SW_SHOWNA) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_DEL);
	pSEARCH->ShowWindow(SW_SHOWNA) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_Edit);
	pSEARCH->ShowWindow(SW_SHOWNA) ;
	*/

	SetButtonShow("BTN_PROFILE", FALSE);
	SetButtonShow("BTN_DETAIL", FALSE);
	SetButtonShow("BTN_LISTEN",FALSE);

	SetButtonPos("BTN_ADD", 10,66 );
	SetButtonShow("BTN_ADD", TRUE);
	SetButtonPos("BTN_EDIT", 80,66 );
	SetButtonShow("BTN_EDIT", TRUE);
	SetButtonPos("BTN_DELETE", 150,66 );
	SetButtonShow("BTN_DELETE", TRUE);
	SetButtonPos("BTN_DIAL", 220,66 );
	SetButtonShow("BTN_DIAL", TRUE);
	SetButtonShow("BTN_Setting",FALSE);


	SetButtonPos("BTN_SEARCH", 10,66 );
	SetButtonShow("BTN_SEARCH", FALSE);
	SetButtonPos("BTN_CLEAR", 80,66 );
	SetButtonShow("BTN_CLEAR", FALSE);
	
	// control light just in BTN_TAB_PERSONAL
	SetButtonCheck("BTN_TAB_PERSONAL", TRUE);
	SetButtonCheck("BTN_TAB_SEARCH",FALSE);
	SetButtonCheck("BTN_TAB_ITRI",FALSE);

	SetButtonEnable("BTN_TAB_PERSONAL", TRUE);
	SetButtonEnable("BTN_TAB_SEARCH", TRUE);
	SetButtonEnable("BTN_TAB_ITRI", g_pMainDlg->m_bEnablePublicAddressBook );


	m_curPage = PAGEID_ADDRESSBOOK;

}
void CPanelDlg::OnBTNCallLog() 
{

	CCallLog callLog;
	callLog.storeAllRecord();
	CSortListCtrl* plistctrl;
	plistctrl=&m_ListCallLog;

	callLog.pourRecord(plistctrl);
	ShowCallLog();
}
void CPanelDlg::OnBTNPhone() 
{
	ShowPhoneBook();
}

void CPanelDlg::OnDblclkLISTCallLog(NMHDR* pNMHDR, LRESULT* pResult) 
{

	// Get cursor client position
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);

	// Get Distance between top-Dialog and top-LISTCallLog
	CRect rct;
	GetDlgItem(IDC_LISTCallLog)->GetWindowRect(&rct);
	ScreenToClient(&rct);
	int distop=rct.top;

/*
	// Get height between each item
	CPoint p1,p2;
	m_ListCallLog.GetItemPosition(0,&p1);
	m_ListCallLog.GetItemPosition(1,&p2);
	int diseach=p2.y-p1.y;

	// Get height of header
	
	CHeaderCtrl *header;
	CRect rctheader;
	header=m_ListCallLog.GetHeaderCtrl();
	header->GetItemRect(0,&rctheader);
	int disheader=rctheader.bottom-rctheader.top;

*/
	
	point.y=point.y-distop;

	UINT uFlags;
	int nItem = m_ListCallLog.HitTest(point, &uFlags);
	bool popdlg;
	popdlg=false;
	if(uFlags & LVHT_ONITEMLABEL)
		popdlg=true;


	int nCount =m_ListCallLog.GetItemCount();


	if (popdlg==true )
	{

		//acquire informatin about which item double click by user
		int p=m_ListCallLog.GetSelectionMark() ;
		
		
		// Do not process when use double click empty area
		if(p>-1)
		{
			InfoCallLog infoCallLog;
			CCallLog callLog;
			callLog.getItemInfo(&infoCallLog,m_ListCallLog.GetItemData(p));
		
			//Implement CCallLogDlg
			CCallLogDlg dlgCallLog;
			dlgCallLog.m_CallLog_Tel=infoCallLog.m_Tel;
			dlgCallLog.m_CallLog_URI=infoCallLog.m_URI;
			dlgCallLog.m_CallLog_Start=callLog.tranHumenSenseTime(infoCallLog.m_StartTime);
			dlgCallLog.m_CallLog_End=callLog.tranHumenSenseTime(infoCallLog.m_EndTime);
			if (infoCallLog.m_Type==1)
				dlgCallLog.m_CallLog_Type="Call In";
			else
				dlgCallLog.m_CallLog_Type="Call Out";
			COleDateTime interval;
			interval=infoCallLog.m_EndTime-infoCallLog.m_StartTime;
			CString strInterval;
			strInterval=interval.Format(_T("%H:%M:%S"));
			dlgCallLog.m_CallLog_Interval=strInterval;
			dlgCallLog.DoModal();
			CSortListCtrl* plistctrl;
			plistctrl=&m_ListCallLog;
	
			if (dlgCallLog.isRefresh==true)
				callLog.pourRecord(plistctrl);			

			m_ListPersonal.DeleteAllItems();


			CDBAccess dbPersonal;
			CString strInitial,errStr;
			strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
	
	
			dbPersonal.Initialize(strInitial,errStr);
			dbPersonal.OpenRecordSet("SELECT * FROM personal where class=2",errStr);	
			if( !errStr.IsEmpty() )
				AfxMessageBox(errStr);   

			int ee=dbPersonal.GetRecordCount();


			char charPersonalSn[20];
			char charPersonalClass[10];
	



			// whether we get any record in database ?
			if(dbPersonal.GetRecordCount())
			{
				// if we get records, then we evaluate the number of records
		
				int intPersonalCount=dbPersonal.GetRecordCount();
				CString getName,getDivision,getTel,getRemark,getPic;
	
		
				if (intPersonalCount>0	)
				{

					if(intPersonalCount==1)
					{	
						//transform m_sn from int to char[]
						itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
						//transform m_class form int to char
						itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
						//add items into ListView 

						dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);				
						getName.ReleaseBuffer();
						dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
						getDivision.ReleaseBuffer();
						dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
						getTel.ReleaseBuffer();
						dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
						getRemark.ReleaseBuffer();
						dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
						getPic.ReleaseBuffer();
						m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					}
					else
					{
						//transform m_sn from int to char[]
						itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
						//transform m_class form int to char
						itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
						//add items into ListView 

						dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
						getName.ReleaseBuffer();
						dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
						getDivision.ReleaseBuffer();
						dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
						getTel.ReleaseBuffer();
						dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
						getRemark.ReleaseBuffer();
						dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
						getPic.ReleaseBuffer();
						m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
			

						for (int i=0;i<(intPersonalCount-1);i++)
						{
							dbPersonal.MoveNext();
							//transform m_sn from int to char[]
							itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
							//transform m_class form int to char
							itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
							//add items into ListView 

							dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
							getName.ReleaseBuffer();
							dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
							getDivision.ReleaseBuffer();
							dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
							getTel.ReleaseBuffer();
							dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
							getRemark.ReleaseBuffer();
							dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
							getPic.ReleaseBuffer();
							m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					
						}
					}

			}
		
			dbPersonal.CloseRecordSet();
			}
		}
	}
	*pResult = 0;
}

void CPanelDlg::SetPage(int idPage)
{
	switch( idPage )
	{
		case PAGEID_ADDRESSBOOK:
			OnBTNPhone();
			break;
		case PAGEID_CALLLOG:
			ShowCallLog();
			break;
		case PAGEID_VoiceMail:
			ShowVoiceMail();
			break;
	}
	m_curPage = idPage;

}

void CPanelDlg::UpdateCallLog()
{
	CCallLog callLog;
	callLog.storeAllRecord();

	callLog.pourRecord(&m_ListCallLog);
}

void CPanelDlg::OnRclickListCallLog(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Get cursor client position
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);
	// Get Distance between top-Dialog and top-LISTCallLog
	CRect rct;
	GetDlgItem(IDC_LISTCallLog)->GetWindowRect(&rct);
	ScreenToClient(&rct);
	int distop=rct.top;

	point.y=point.y-distop;

	UINT uFlags;
	int nItem = m_ListCallLog.HitTest(point, &uFlags);
	bool popdlg;
	popdlg=false;
	if(uFlags & LVHT_ONITEMLABEL)
		popdlg=true;

	if (popdlg==true)
	{
		CMenu popMenu;
		popMenu.LoadMenu(IDR_MAINFRAME);
	
		CPoint posMouse;
		GetCursorPos(&posMouse);

		popMenu.GetSubMenu(2)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);

		
	}
	*pResult = 0;
}

void CPanelDlg::OnCallLogDial() 
{
	int i=m_ListCallLog.GetSelectionMark();		
	CString tel;
	tel=m_ListCallLog.GetItemText(i,0);
	((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(tel);
}

void CPanelDlg::OnCallLogPro()
{
	int i=m_ListCallLog.GetSelectionMark();		
	CString tel;
	tel=m_ListCallLog.GetItemText(i,0);


	CQueryDB dbQuery;

	// check whether user with this tel in our database
	// if exist, store data in attributes of dbQuery
	if (dbQuery.SearchTel(tel))
	{

		//implement CEditPersonalDlg
		CEditPersonalDlg personal;

		//pass essential data into personal
		personal.m_PERSON2_NAME=dbQuery.m_name;
		personal.m_PERSON2_COMPANY=dbQuery.m_division;
		personal.m_PERSON2_TELEPHONE=dbQuery.m_telephone;
		personal.m_PERSON2_REMARK=dbQuery.m_remark;
		
		if (dbQuery.m_class==1)
		{
			personal.m_PERSON2_COMPANY="ITRI "+dbQuery.m_division;
//@Department			personal.m_PERSON2_DEPARTMENT=dbQuery.m_department;
		}
//@Department		else
//@Department			personal.m_PERSON2_DEPARTMENT="";

		// Because dbQuery.m_pic is full path,
		// we just need file name;

		CString pic;
		int nFirst,nCount;

		nFirst=dbQuery.projectpath.GetLength();
		nCount=dbQuery.m_pic.GetLength()-dbQuery.projectpath.GetLength();
		pic=dbQuery.m_pic.Mid(nFirst,nCount);
		
		personal.m_PERSON2_PIC=pic;



		
		int result;
		result=personal.DoModal();

		if (result==IDOK)
		{

			CString strTest;

			// collect information in dialog into info_personal
			InfoPersonal info_personal;

			// sn,class are static information
			// catch from dbQuery
			info_personal.m_sn=dbQuery.m_sn;
			info_personal.m_class=dbQuery.m_class;


			info_personal.m_name=personal.m_PERSON2_NAME;
			
			if (dbQuery.m_class==1)
			{
				//if class=1 ,We truncate "ITRI" 
				CString division;
				division=personal.m_PERSON2_COMPANY;
				division.Replace("ITRI","");
				division.TrimLeft();
				division.TrimRight();
			
				info_personal.m_division=division;
//@Department		info_personal.m_department=personal.m_PERSON2_DEPARTMENT;
			}
			else
			{
				info_personal.m_division=personal.m_PERSON2_COMPANY;
//@Department				info_personal.m_department="";
			}
			
			info_personal.m_telephone=personal.m_PERSON2_TELEPHONE;
			info_personal.m_remark=personal.m_PERSON2_REMARK;
			//info_personal.m_pic=personal.m_PERSON2_PIC;


			//		Copy original path into our target path
			//		Modify pic path , map into ./pic/B&...
			char c2[20];					
			itoa(dbQuery.m_sn,c2,10);

			CPersonal cpersonal;
			int m_leng=personal.m_PERSON2_PIC.GetLength();
			if (m_leng != 0) {
				int m_ext=personal.m_PERSON2_PIC.Find('.');
				CString tt=personal.m_PERSON2_PIC.Mid(m_ext,m_leng-m_ext);
				CString projectpath=cpersonal.projectpath;
				CString newpath=projectpath+"\\pic\\B"+c2+tt;
				CString strStorePath="\\pic\\B";	
				strStorePath=strStorePath+c2+tt;
				info_personal.m_pic=strStorePath;
				//copy picture selected by users into default path ".\pic\ "
				CopyFile(personal.m_PERSON2_PIC,newpath,FALSE);
			}

			cpersonal.ModifyDB(&info_personal);
		}
	}
	else
		MessageBox("This user is not in our database!","Not Found",MB_ICONWARNING|MB_OK);	
}

void CPanelDlg::OnCallLogErase()
{

	// check if user has selected some items ?

	CString SelectItem;
	char t[10];
	int i=-1;
	int j=-1;
	int count=0; // the number of selected items
	do
	{
		j=m_ListCallLog.GetNextItem(i,LVNI_SELECTED);
		i=j;		
		itoa(j,t,10);
		SelectItem=SelectItem+t+",";
		count++;
	}while (j>-1);
	count--;





		CCallLogDelDlg dlgCallLogDel;

		//which way we will delete in dlgCallLogDel
		int intSelect; 
		dlgCallLogDel.pintSelect=&intSelect;
		//dlgCallLogDel.SelectResult=0;


		//when ? to delete 
		COleDateTime dateTime;
		dlgCallLogDel.pdateTime=&dateTime;
		
		int result=dlgCallLogDel.DoModal();
		
		switch(*dlgCallLogDel.pintSelect)
		{
		case 0:	
			if (result!=IDCANCEL)
			{
				dateTime.Format(_T("%A, %m %d, %Y"));
				CCallLog cl;
				cl.deleteItem(dateTime);	
				cl.pourRecord(&m_ListCallLog);
			}
			break;
		case 1:	
			if (result!=IDCANCEL)
			{
				dateTime.Format(_T("%A, %m %d, %Y"));
				CCallLog cl;
				cl.deleteItem(dateTime);	
				cl.pourRecord(&m_ListCallLog);
			}
			break;
		case 2: 
			//following is Radio3	
			{
			CCallLog cl;
			cl.deleteAllItem();	
			cl.pourRecord(&m_ListCallLog);			
			}
			break;
		case 3:
			//following is Radio4
			{
				if(count==0)
				{
					AfxMessageBox("There are no items be selected.");
					return;
				}
				else
				{
					// SelectItem store the order of CallList
					// We need to transform it into sn mode

					//AfxMessageBox(SelectItem);
					CCallLog cl;
					char charNum[12];
					int intNum;
					int intPos=0;
					int intComma=0;
					int intSN;
					for (int i=0;i<count;i++)
					{
						intComma=SelectItem.Find(',',intComma);
						lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));
						intNum=atoi(charNum);	
						intSN=m_ListCallLog.GetItemData(intNum);
						cl.deleteItem(intSN);
						intComma++;
						intPos=intComma;	
					}					
					cl.pourRecord(&m_ListCallLog);	
				}
			
			}
			break;
		}


}

void CPanelDlg::OnBTNPERSONALEdit() 
{
	CString cs;
	CQueryDB qb;

	int nCount =m_ListPersonal.GetItemCount();
	if (nCount>-1 )
	{

		//acquire informatin about which item double click by user
		int p=m_ListPersonal.GetSelectionMark() ;
	
		// Do not process when use double click empty area
		if(p>-1)
		{

			m_LVpersonal_sn=m_ListPersonal.GetItemData(p);
			qb.SearchSn(m_LVpersonal_sn);
			m_LVpersonal_class=qb.m_class;
			m_LVpersonal_Name=m_ListPersonal.GetItemText(p,defPersonalName);
			m_LVpersonal_Company=m_ListPersonal.GetItemText(p,defPersonalCompany);
			m_LVpersonal_Telephone=m_ListPersonal.GetItemText(p,defPersonalTelephone);
			m_LVpersonal_Remark=m_ListPersonal.GetItemText(p,defPersonalRemark);
			qb.SearchSn(m_LVpersonal_sn);
			m_LVpersonal_PIC=qb.m_pic_short;

			CString oldTelNo = m_LVpersonal_Telephone;

			// added by shen, 20050407
			CString strLocalName = g_pdlgUAControl->m_strRegistarName;
			char buf[256];
			::GetPrivateProfileString(strLocalName, m_LVpersonal_Telephone, "", buf, sizeof(buf) - 1, "pcaPic.ini");
			m_LVpersonal_PIC = buf;

			CEditPersonalDlg personal;

			personal.m_PERSON2_NAME=m_LVpersonal_Name;
			personal.m_PERSON2_COMPANY=m_LVpersonal_Company;
			personal.m_PERSON2_TELEPHONE=m_LVpersonal_Telephone;
			personal.m_PERSON2_REMARK=m_LVpersonal_Remark;
			personal.m_PERSON2_PIC=m_LVpersonal_PIC;


			int rs=personal.DoModal();
	
			if(rs==IDOK)
			{
				//retrieve data from dialog into InfoDB struct
				InfoDB info_personal;
				info_personal.m_sn=m_LVpersonal_sn;
				info_personal.m_class=m_LVpersonal_class;
				info_personal.m_name=personal.m_PERSON2_NAME;
				info_personal.m_id="";
				info_personal.m_division=personal.m_PERSON2_COMPANY;
				info_personal.m_department="";
				info_personal.m_telephone=personal.m_PERSON2_TELEPHONE;
				info_personal.m_remark=personal.m_PERSON2_REMARK;
#ifdef _XCAPSERVER
				info_personal.m_group = personal.m_PERSON2_GROUP;
				GroupList *myGroup = GetAllGroup();
				if(myGroup!=NULL)
				{
					if(!CheckExistence(myGroup, (LPSTR)(LPCTSTR)personal.m_PERSON2_GROUP))
					{
						AfxMessageBox("Group not exist!Please new group first.");
						FreeGroupList(myGroup);
						return;
					}
					FreeGroupList(myGroup);
				}
				else
				{
					AfxMessageBox("Group not exist!Please new group first.");
					return;
				}
#endif

				//	Copy original path into our target path
				//	Modify pic path , map into ./pic/B&...
				char c2[20];					
				itoa(m_LVpersonal_sn,c2,10);
				CString strStorePath;

				int m_leng=personal.m_PERSON2_PIC.GetLength();
				if (m_leng != 0) {
					int m_ext=personal.m_PERSON2_PIC.Find('.');
					CString tt=personal.m_PERSON2_PIC.Mid(m_ext,m_leng-m_ext);
					strStorePath = "\\pic\\"+strLocalName+"-"+m_LVpersonal_Telephone+tt;
					CString newpath = projectpath + strStorePath;

					info_personal.m_pic=strStorePath;
				
					CopyFile(personal.m_PERSON2_PIC,newpath,FALSE);					
				}
				
				//copy picture selected by users into default path ".\pic\ "
				ModifyDB(&info_personal);
				

				// added by shen, 20050407
				::WritePrivateProfileString(strLocalName, m_LVpersonal_Telephone, strStorePath, "pcaPic.ini");
				::WritePrivateProfileString(strLocalName, m_LVpersonal_Telephone+"_name", m_LVpersonal_Name, "pcaPic.ini");


				m_LVpersonal_Name=personal.m_PERSON2_NAME;
				m_LVpersonal_Company=personal.m_PERSON2_COMPANY;
				m_LVpersonal_Telephone=personal.m_PERSON2_TELEPHONE;
				m_LVpersonal_Remark=personal.m_PERSON2_REMARK;
				m_LVpersonal_PIC=personal.m_PERSON2_PIC;

				//refresh data in ListVies
				m_ListPersonal.SetItemText(p,defPersonalName,m_LVpersonal_Name);
				m_ListPersonal.SetItemText(p,defPersonalCompany,m_LVpersonal_Company);
				m_ListPersonal.SetItemText(p,defPersonalTelephone,m_LVpersonal_Telephone);
				m_ListPersonal.SetItemText(p,defPersonalRemark,m_LVpersonal_Remark);
				//m_ListPersonal.SetItemText(p,6,strStorePath);
#ifdef _XCAPSERVER
				m_ListPersonal.SetItemText(p,defPersonalGroup,personal.m_PERSON2_GROUP);
#endif

				// re-subscribe if telno changed
				//marked by alan
/*
				if ( oldTelNo != m_LVpersonal_Telephone)
				{
					m_ListPersonal.SetItem(p, 0, LVIF_IMAGE, 0, Presence_Unknown, 0, 0, 0);
					m_ListPersonal.SetItemText(p, defPersonalPresence, "Unknown");
					m_ListPersonal.SetItemText(p, defPersonalCallStatus, "Unknown");

					g_pMainDlg->UnSubscribe( oldTelNo);
					g_pMainDlg->Subscribe( m_LVpersonal_Telephone);
				}
*/
			}
		}
	}
	
	
}

void CPanelDlg::DragWindow(int x, int y)
{

	//m_pUIDlg->DragWindow(x-300,y);
	m_pUIDlg->SetWindowPos(NULL,x-300, y,0,0,SWP_NOZORDER|SWP_NOSIZE );
	//m_pUIDlg->UpdateWindow();
	//UpdateWindow();

}

void CPanelDlg::OnRclickListPersonal(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//AfxMessageBox("ff");
	// Get cursor client position
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);
	// Get Distance between top-Dialog and top-LISTCallLog
	CRect rct;
	GetDlgItem(IDC_LISTPERSONAL)->GetWindowRect(&rct);
	ScreenToClient(&rct);
	int distop=rct.top;

	point.y=point.y-distop;

	UINT uFlags;
	int nItem = m_ListPersonal.HitTest(point, &uFlags);
	bool popdlg;
	popdlg=false;
	if(uFlags & LVHT_ONITEMLABEL)
		popdlg=true;
	else
		popdlg=false;

	if (popdlg==true)
	{
		CMenu popMenu;
		popMenu.LoadMenu(IDR_MAINFRAME);

#ifndef _SIMPLE
		popMenu.EnableMenuItem(IDM_PRESENCE_STATUS_OFFLINE,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_PRESENCE_STATUS_ONLINE,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkIM,MF_DISABLED|MF_GRAYED);
#endif
#ifndef _XCAPSERVER
		popMenu.EnableMenuItem(IDM_NewGroup,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_DeleteGroup,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_EditGroup,MF_DISABLED|MF_GRAYED);
//		popMenu.EnableMenuItem(IDM_NewGroup,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkBlock,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkUnBlock,MF_DISABLED|MF_GRAYED);

		//no provide import and export now

		popMenu.EnableMenuItem(IDM_EXPORT,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_IMPORT,MF_DISABLED|MF_GRAYED)
#endif
		if(m_ListPersonal.GetItemText(nItem, defPersonalPresence)=="Unknown")
		{
			popMenu.EnableMenuItem(IDM_AddressBkBlock,MF_DISABLED|MF_GRAYED);
			popMenu.EnableMenuItem(IDM_AddressBkUnBlock,MF_DISABLED|MF_GRAYED);
			popMenu.EnableMenuItem(IDM_AddressBkIM,MF_DISABLED|MF_GRAYED);
			popMenu.EnableMenuItem(IDM_AddressBkOfflineMsg,MF_DISABLED|MF_GRAYED);
		}
		else if(m_ListPersonal.GetItemText(nItem, defPersonalPresence)=="Block")
		{
			popMenu.EnableMenuItem(IDM_AddressBkBlock,MF_DISABLED|MF_GRAYED);
			popMenu.EnableMenuItem(IDM_AddressBkIM,MF_DISABLED|MF_GRAYED);
		}
		else 
			popMenu.EnableMenuItem(IDM_AddressBkUnBlock,MF_DISABLED|MF_GRAYED);
	
		// added by shen
		if (m_ListPersonal.GetItemText(nItem, defPersonalPresence)==PRE_ONLINE)
			popMenu.EnableMenuItem(IDM_AddressBkOfflineMsg,MF_DISABLED|MF_GRAYED);
		else if (m_ListPersonal.GetItemText(nItem, defPersonalPresence)==PRE_OFFLINE)
			popMenu.EnableMenuItem(IDM_AddressBkIM, MF_DISABLED|MF_GRAYED);

		// added by molisado, 20050406
		// if the user is offline, no IM function is available
		if( !g_pMainDlg->GetPresenceState() )
		{
			popMenu.EnableMenuItem( IDM_AddressBkOfflineMsg, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkIM, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkIMTo, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkGroupMsg, MF_GRAYED );
		}

		CPoint posMouse;
		GetCursorPos(&posMouse);

		popMenu.GetSubMenu(3)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);

		
	}
	else
	{
		CMenu popMenu;
		popMenu.LoadMenu(IDR_MAINFRAME);
		popMenu.EnableMenuItem(IDM_AddressBkIM,MF_DISABLED|MF_GRAYED);
		// added by molisado, 20050406
		popMenu.EnableMenuItem( IDM_AddressBkOfflineMsg, MF_GRAYED );
//		popMenu.EnableMenuItem(IDM_AddressBkAdd,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkDelete,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkEdit,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkDial,MF_DISABLED|MF_GRAYED);
#ifndef _SIMPLE
		popMenu.EnableMenuItem(IDM_PRESENCE_STATUS_OFFLINE,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_PRESENCE_STATUS_ONLINE,MF_DISABLED|MF_GRAYED);
#endif
#ifndef _XCAPSERVER		
//		popMenu.EnableMenuItem(IDM_NewGroup,MF_DISABLED|MF_GRAYED); 
		popMenu.EnableMenuItem(IDM_DeleteGroup,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_EditGroup,MF_DISABLED|MF_GRAYED);
#endif
		popMenu.EnableMenuItem(IDM_AddressBkBlock,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_AddressBkUnBlock,MF_DISABLED|MF_GRAYED);

#ifdef _XCAPSERVER//no provide import and export now

		popMenu.EnableMenuItem(IDM_EXPORT,MF_DISABLED|MF_GRAYED);
		popMenu.EnableMenuItem(IDM_IMPORT,MF_DISABLED|MF_GRAYED);
#endif
		// added by molisado, 20050406
		// if the user is offline, no IM function is available
		if( !g_pMainDlg->GetPresenceState() )
		{
			popMenu.EnableMenuItem( IDM_AddressBkOfflineMsg, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkIM, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkIMTo, MF_GRAYED );
			popMenu.EnableMenuItem( IDM_AddressBkGroupMsg, MF_GRAYED );
		}

		CPoint posMouse;
		GetCursorPos(&posMouse);

		popMenu.GetSubMenu(3)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);

	}


	*pResult = 0;
}


void CPanelDlg::OnAddressBkAdd()
{

	CPanelDlg::OnBtnpersonalAdd();
}

void CPanelDlg::OnAddressBkDelete()
{

	CPanelDlg::OnBtnpersonalDel();
}

void CPanelDlg::OnAddressBkEdit()
{
	
	CPanelDlg::OnBTNPERSONALEdit();
}

void CPanelDlg::OnAddressBkOfflineMsg() 
{
	int p=m_ListPersonal.GetSelectionMark();
	CString strURI = m_ListPersonal.GetItemText(p,defPersonalTelephone);
	CString strTel = m_ListPersonal.GetItemText(p,defPersonalTelephone);
	if ( strURI.Left(4) != "sip:")
		strURI = g_pMainDlg->ConvertNumberToURI(strTel);

	CDlgMessage* dlg = CDlgMessage::GetDlgByMessage( strURI, NULL, strTel );
	if (dlg)
		dlg->SetOffline(true);
}

void CPanelDlg::OnAddressBkIM()
{
	CString strURI;
	CString strTel;
	CArray<CString, CString&> vecTarget;

	int n = -1;
	POSITION pos=m_ListPersonal.GetFirstSelectedItemPosition();
	while (pos)
	{
		n = m_ListPersonal.GetNextSelectedItem(pos);
		strURI = m_ListPersonal.GetItemText(n, defPersonalTelephone);
		strTel = strURI;
		if ( strURI.Left(4) != "sip:")
			strURI = g_pMainDlg->ConvertNumberToURI(strTel);
		if (g_pMainDlg->GetBuddyPresence(strTel))	// need to be on-line
			vecTarget.Add(strURI);
	}

	if (vecTarget.GetSize() == 1)
	{
		CDlgMessage::GetDlgByMessage( strURI, NULL, strTel );
	}
	else if (vecTarget.GetSize() > 1)
	{
		// create conference
		CDlgMessage* dlgMsg = new CDlgMessage( g_pMainDlg, NULL, NULL);
		dlgMsg->CreateIMConference(&vecTarget, true);	// new created
	}
	else 
	{
		AfxMessageBox("No one on-line is selected.\nPlease choice one to send message.");
	}

/*	int p=m_ListPersonal.GetSelectionMark();
	CString strName = m_ListPersonal.GetItemText(p,defPersonalName); 
	CString strURI = m_ListPersonal.GetItemText(p,defPersonalTelephone);
	CString strTel = m_ListPersonal.GetItemText(p,defPersonalTelephone);

	if ( strURI.Left(4) != "sip:")
		strURI.Format("sip:%s@%s", strTel, CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"));		
//		strURI.Format("sip:%s@%s:%d", strName, CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
//		strURI = g_pMainDlg->ConvertNumberToURI(strURI);

	//modify by alan,2004,11,25
//	CDlgMessage::GetDlgByMessage( strURI, NULL, strName );
	CDlgMessage::GetDlgByMessage( strURI, NULL, strTel );*/
}

// added by shen for test
void CPanelDlg::OnAddressBkIMTo()
{
	CEnterURIDlg dlg;
	if ( dlg.DoModal() != IDOK || dlg.m_strURI.IsEmpty() )
		return;

	CString strName;
	CString strURI;
	if ( dlg.m_strURI.Left(4) != "sip:")
	{
		strURI = g_pMainDlg->ConvertNumberToURI(dlg.m_strURI);
		strName = dlg.m_strURI;
	}
	else
	{
		strName = CDlgMessage::GetNameFromURI(dlg.m_strURI);
		strURI = dlg.m_strURI;
	}

	CDlgMessage::GetDlgByMessage(strURI, NULL, strName );
}

void CPanelDlg::OnAddressBkDial()
{

	int p=m_ListPersonal.GetSelectionMark() ;
	((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(m_ListPersonal.GetItemText(p,defPersonalTelephone));

}

CString CPanelDlg::tranStr(CString str)
{
	str.Replace("'","''");
	return str;
}
void CPanelDlg::SetAccountId(CString strId)
{
	accountId=strId;
	AfxGetApp()->WriteProfileString("", "VoiceMailAccount", strId);
}

CString CPanelDlg::GetAccountId()
{
	return accountId;
}

void CPanelDlg::SetAccountPasswd(CString strPasswd)
{
	accountPasswd=strPasswd;
	AfxGetApp()->WriteProfileString("", "VoiceMailPassword", strPasswd);
}

CString CPanelDlg::GetAccountPasswd()
{
	return accountPasswd;
}

void CPanelDlg::SetInterval(int interval)
{
	AfxGetApp()->WriteProfileInt("", "VoiceMailInterval", interval);
}

int CPanelDlg::GetInterval()
{
	return 	AfxGetApp()->GetProfileInt("", "VoiceMailInterval", 1);

}

void CPanelDlg::ShowVoiceMail()
{
	// set page title
	SetText( "TEXT_TITLE", "Voice Mail");

	//show ListView "CallLog"
	m_ListVoiceMail.ShowWindow(SW_SHOWNA);

	//hide ListView "Itri" and "Personal" and "Search" and "CallLog"
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListItri.ShowWindow(SW_HIDE);
	m_ListPersonal.ShowWindow(SW_HIDE);
	m_ListCallLog.ShowWindow(SW_HIDE);

	// discover  VoiceMail  paragraph
	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_BTNVoiceMailReceive);
	//pSEARCH->ShowWindow(SW_RESTORE) ;
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailDelete);
	//pSEARCH->ShowWindow(SW_RESTORE);
	//pSEARCH=(CWnd*)GetDlgItem(IDC_BTNVoiceMailPlay);
	//pSEARCH->ShowWindow(SW_RESTORE);

	//hide button Add,Del in "ITRI"
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_ADD);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNITRI_DEL);
	pSEARCH->ShowWindow(SW_HIDE) ;

	//hide button Add,Del,Edit in "Personal"
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_ADD);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_DEL);
	pSEARCH->ShowWindow(SW_HIDE) ;
	pSEARCH=(CWnd*)GetDlgItem(IDC_BTNPERSONAL_Edit);
	pSEARCH->ShowWindow(SW_HIDE) ;

	//hide search paragraph
	pSEARCH=(CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_HIDE) ;
	
	SetButtonCheck( "BTN_TAB_PERSONAL", FALSE);
	SetButtonCheck( "BTN_TAB_SEARCH", FALSE);
	SetButtonCheck( "BTN_TAB_ITRI", FALSE);
	SetButtonEnable("BTN_TAB_PERSONAL", FALSE);
	SetButtonEnable("BTN_TAB_SEARCH", FALSE);
	SetButtonEnable("BTN_TAB_ITRI", FALSE);
//	SetButtonPos("BTN_SEARCH", 150,66 );
//	SetButtonPos("BTN_CLEAR", 80,66 );
	SetButtonShow("BTN_SEARCH", FALSE);
	SetButtonShow("BTN_CLEAR", FALSE);


	SetButtonPos("BTN_ADD", 150,66 );
	SetButtonShow("BTN_ADD", FALSE);
	SetButtonPos("BTN_LISTEN", 10,66 );
	SetButtonShow("BTN_LISTEN", TRUE);
	SetButtonPos("BTN_DELETE", 80,66 );
	SetButtonShow("BTN_DELETE", TRUE);
	SetButtonShow("BTN_DIAL", FALSE);
	SetButtonShow("BTN_PROFILE",FALSE);
	SetButtonShow("BTN_DIAL",FALSE);
	SetButtonShow("BTN_EDIT",FALSE);	
	SetButtonShow("BTN_DETAIL",FALSE);

	//hide itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_HIDE);
	m_curPage = PAGEID_VoiceMail;

	//Temporary identify "EMAIL Setting"
	// Use "BTN_EDIT"
	// You should change something at future.

	SetButtonShow("BTN_Setting",TRUE);
	SetButtonPos("BTN_Setting",150,66);
	
	m_bItriSearch=false;
}

void CPanelDlg::OnBtnVoiceMail()
{
	int		ret = 0;
	while ( (ret=OnAuthenticate())!=1 ) {
		int LoginResult;
		CVoiceMailLogin VoiceMailLogin;
		
		if ( ret==0 )
			return;
		VoiceMailLogin.m_VoiceMailAccount = GetAccountId();
		VoiceMailLogin.m_VoiceMailPWD = GetAccountPasswd();
		LoginResult=VoiceMailLogin.DoModal();
		UpdateData(TRUE);
		if(LoginResult==IDOK)
		{
			SetAccountId(VoiceMailLogin.m_VoiceMailAccount);
			SetAccountPasswd(VoiceMailLogin.m_VoiceMailPWD);
		} else {
			return;
		}
	}

	ShowVoiceMail();
	// Retrieve message immediately
	OnBtnVoiceMailReceive();
}

void CPanelDlg::OnBtnVoiceMailReceive()
{
	char	url[1024];
	int		ret = 0;
	presult=&m_ListVoiceMail;
	m_ListVoiceMail.DeleteAllItems();

	sprintf(url, "http://%s:%s/RPC2", GetConnectionURL(),GetConnectionPort());
	unsigned long milliseconds = 5000;
		g_hSem = CreateSemaphore(
		NULL, /* pointer to security attributes */
		0,	/*  initial count */
		5,	/*  maximum count */
		NULL	/*  pointer to semaphore-object name */);
    xmlrpc_env env;
	int minState = 40;
	xmlrpc_env_init(&env);
    /* Start up our XML-RPC client library using the curl transport. */
	xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
		VERSION, "curl");
	xmlrpc_client_call_asynch(url, "VoiceMailServer.GetMessageList",
				  print_state_name_callback, NULL,
				  "(ss)", (char*)(LPCTSTR)GetAccountId(),
				  (char*)(LPCTSTR)GetAccountPasswd());


	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
	switch (WaitForSingleObject (
		g_hSem /* handle to object to wait for */,
		milliseconds /* time-out interval in milliseconds*/) )
	{
		/* One may want to handle these cases  */

	case WAIT_OBJECT_0:	
		{
			int num;
			num=ArrayMsg.GetSize();
						
			for (int i=0;i<num;i++)				
				AddMsgItem(i,ArrayMsg[i].Caller,ArrayMsg[i].Date,ArrayMsg[i].Time,ArrayMsg[i].Read,ArrayMsg[i].URL);

			
		}
		xmlrpc_client_cleanup();
		iVoiceMailError=0;
		break;
		
	case WAIT_TIMEOUT:
		//AfxMessageBox("Can not connect into VoiceMail Server ");
		iVoiceMailError=iVoiceMailError+1;
		break;
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	switch(iVoiceMailError)
	{
	case 0:
		break;
	case 1:
		AfxMessageBox("Connect to VoiceMail server failed.\nPlease check preference setting.");
		break;
	case 2:
		iVoiceMailError=1;
		break;
	}
    xmlrpc_env_clean(&env);
    xmlrpc_client_cleanup();
	CloseHandle (g_hSem);


}

void CPanelDlg::SetConnectionURL(CString strURL)
{
	connectionURL=strURL;
}

void CPanelDlg::SetConnectionPort(CString port)
{
	connectionPort=port;
}

CString CPanelDlg::GetConnectionURL()
{
	return connectionURL;
}

CString CPanelDlg::GetConnectionPort()
{
	return connectionPort;
}

BOOL CPanelDlg::CheckXMLRPCError()
{
	char	msg[1024];
	CString mm;
	mm.Format(_T("%s"),m_xmlrpcEnv.fault_string);

    if (m_xmlrpcEnv.fault_occurred) {
		sprintf(msg, "XML-RPC Fault: %s (%d)",
			m_xmlrpcEnv.fault_string,
			m_xmlrpcEnv.fault_code);

		MessageBox(msg, "XML-RPC Error", MB_ICONSTOP|MB_OK);
		return FALSE;
    }
	return TRUE;
}


void CPanelDlg::OnOK()
{

}

void CPanelDlg::OnCancel()
{

}

void CPanelDlg::OnBtnVoiceMailPlay()
{
	CString	sServer;
	CString	sDir, sFile;
	DWORD	nService;
	unsigned short	nPort;
	int		index = -1;

	if (m_ListVoiceMail.GetItemCount()==0)
		return;
	
	if ((index=m_ListVoiceMail.GetSelectionMark())>=0) 
	{
		char ctx[1024];
		int	len = 0;

		// retrieve ftp URL
		strcpy(ctx,ArrayMsg[index].URL);

		if ( len=strlen(ctx)>0 ) 
		{
			char *url = strstr(ctx, "ftp://");
			if ( url ) 
			{
				if ( AfxParseURL( url, (DWORD&)nService, (CString&)sServer, 
								(CString&)sDir, (INTERNET_PORT&)nPort) ) 
				{
				
						
					if ( nService==AFX_INET_SERVICE_FTP ) 
					{
						CInternetSession	*cint = new CInternetSession("XMLRPC Client", 1, PRE_CONFIG_INTERNET_ACCESS);
						CFtpConnection		*cftp;
						int	i=sDir.Find('/');
						SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
						if (i>=0)
							sDir.Delete(i, 1);
						cftp = cint->GetFtpConnection( (LPCTSTR)sServer, 
							(LPCTSTR)GetAccountId(), (LPCTSTR)GetAccountPasswd(),
							nPort);

						CString sLocalFile;
						sLocalFile=projectpath+"\\"+sDir;

						// Has this file existed ?
						bool isExist=true;
						TRY
						{
							CFile::Rename(sLocalFile,sLocalFile);
						}
						CATCH(CFileException,e)
						{
							isExist=false;
						}
						END_CATCH

					
						if ( cftp && (!isExist) ) 
						{
								if (!cftp->GetFile((LPCTSTR)sDir, (LPCTSTR)sLocalFile,
									FALSE)) 
								{
										LPVOID lpMsgBuf;
										FormatMessage( 
										FORMAT_MESSAGE_ALLOCATE_BUFFER | 
										FORMAT_MESSAGE_FROM_SYSTEM | 
										FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL,
										GetLastError(),
										MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
										(LPTSTR) &lpMsgBuf,
										0,
										NULL 
									);
									// Process any inserts in lpMsgBuf.
									// ...
									// Display the string.
									MessageBox( (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
									// Free the buffer.
									LocalFree( lpMsgBuf );

									return;
								}
								//else 
								//{
								//	ShellExecute(NULL, "open", sLocalFile, NULL,
								//			NULL, SW_SHOWNORMAL);
								//	::PlaySound( sLocalFile, 
								//	GetModuleHandle(NULL), 
								//	SND_RESOURCE | SND_NODEFAULT | SND_ASYNC | SND_LOOP );
								//PlaySound(LPCTSTR(sLocalFile),NULL,SND_ASYNC);
								//}
			
						}
						cftp->Close();
						cint->Close();
						SetCursor(LoadCursor(NULL, IDC_ARROW));
						PlaySound(LPCTSTR(sLocalFile),NULL,SND_ASYNC);
						m_ListVoiceMail.SetItem(index,0,LVIF_IMAGE,NULL,1,0,0,0);
					}
				}
			}
		}
	}
}

void CPanelDlg::OnBtnVoiceMailDelete()
{

	char	url[1024];
	char	rpcurl[1024];
	int		index = -1;
	CString msg;

	xmlrpc_int32	dwTotal = 0;
	unsigned long milliseconds = 5000;
	bDeleteOK=false;
	if ( m_ListVoiceMail.GetSelectedCount()==0 )
	{
		AfxMessageBox("Plese select one message.");
		return;
	}
	index = m_ListVoiceMail.GetSelectionMark();
	msg="Do you want to delete this message : \n";	
	msg=msg+"Caller:\t"+m_ListVoiceMail.GetItemText(index,0)+"\n";
	msg=msg+"Date:\t"+m_ListVoiceMail.GetItemText(index,1)+"\n";
	msg=msg+"Time:\t"+m_ListVoiceMail.GetItemText(index,2);

	if(MessageBox(msg,"Delete",MB_ICONINFORMATION | MB_YESNO)==IDYES)
	{
		// do delete

		// retrieve URL
		strcpy(rpcurl,ArrayMsg[index].URL);
		//m_ListVoiceMail.GetItemText(index, 3, rpcurl, 1024);
		if ( strlen(rpcurl)<=0 )
			return;
		sprintf(url, "http://%s:%s/RPC2", GetConnectionURL(),GetConnectionPort());
	
		g_hSem = CreateSemaphore(
		NULL, /* pointer to security attributes */
		0,	/*  initial count */
		5,	/*  maximum count */
		NULL	/*  pointer to semaphore-object name */);
		xmlrpc_env env;
		int minState = 40;
		xmlrpc_env_init(&env);		
		
		xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
				VERSION, "curl");
		xmlrpc_client_call_asynch(url, "VoiceMailServer.DeleteMessage",
				  function_delete, NULL,
				  "(sss)", (char*)(LPCTSTR)GetAccountId(),
				  (char*)(LPCTSTR)GetAccountPasswd(),
				  (char*)(LPCTSTR)rpcurl);

		SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		bool bDeleteOK = false;
		switch (WaitForSingleObject (
			g_hSem /* handle to object to wait for */,
			milliseconds /* time-out interval in milliseconds*/) )
		{
			/* One may want to handle these cases  */

		case WAIT_OBJECT_0:	
			{
				AfxMessageBox("Delete Success", MB_ICONINFORMATION|MB_OK);
				bDeleteOK = true;
			}
			break;
			
		case WAIT_TIMEOUT:
				AfxMessageBox("Fail to delete message!", MB_ICONERROR|MB_OK);
			break;
		}
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		UpdateData(TRUE);

		xmlrpc_env_clean(&env);
		xmlrpc_client_cleanup();
		CloseHandle (g_hSem);	

		if ( bDeleteOK)
			OnBtnVoiceMailReceive();				

	}
}
/*
	char	url[1024];
	char	rpcurl[1024];
	int		index = -1;
	CString msg;
	xmlrpc_bool	bOK;
	xmlrpc_int32	dwTotal = 0;
	xmlrpc_server_info	*server;

	
	if ( m_ListVoiceMail.GetSelectedCount()==0 )
	{
		AfxMessageBox("Plese select one message.");
		return;
	}
	
	index = m_ListVoiceMail.GetSelectionMark();
	msg="Do you want to delete this message : \n";	
	msg=msg+"Caller:\t"+m_ListVoiceMail.GetItemText(index,0)+"\n";
	msg=msg+"Date:\t"+m_ListVoiceMail.GetItemText(index,1)+"\n";
	msg=msg+"Time:\t"+m_ListVoiceMail.GetItemText(index,2);
	if(MessageBox(msg,"Delete",MB_ICONINFORMATION | MB_YESNO)==IDYES)
	{
		// do delete
		m_ListVoiceMail.GetItemText(index, 3, rpcurl, 1024);
		if ( strlen(url)<=0 )
			return;
		sprintf(url, "http://%s:%s/RPC2", GetConnectionURL(),GetConnectionPort());
		server = xmlrpc_server_info_new( &m_xmlrpcEnv, url);
		m_xmlrpcResult = xmlrpc_client_call_server( &m_xmlrpcEnv, 
			server,
			"VoiceMailServer.DeleteMessage", 
			"(sss)", 
			(char*)(LPCTSTR)GetAccountId(),
			(char*)(LPCTSTR)GetAccountPasswd(),
			(char*)(LPCTSTR)rpcurl);
		if (!CheckXMLRPCError())
			return;

		xmlrpc_server_info_free(server);
		xmlrpc_parse_value(&m_xmlrpcEnv, m_xmlrpcResult, "b", &bOK);
		
		if ( !CheckXMLRPCError() )
		return;

		if ( bOK ) 
		{
			AfxMessageBox("Delete Success");
			OnBtnVoiceMailReceive();
		}
		else
		{
			AfxMessageBox("Fail to delete message!");
		}



	}
	else
	{
		return;
	}
	
	*/



void CPanelDlg::OnBtnCallLogProfile()
{
	CQueryDB dbQuery;

	if (m_ListCallLog.GetSelectedCount()==0)
	{
		AfxMessageBox("Plese select one message.");
		return ;
	}		

	int index = m_ListCallLog.GetSelectionMark();
	CString strPhoneNum;
	strPhoneNum=m_ListCallLog.GetItemText(index,0);


	// check whether user with this tel in our database
	// if exist, store data in attributes of dbQuery
	if (dbQuery.SearchTel(strPhoneNum))
	{

		//implement CEditPersonalDlg
		CEditPersonalDlg personal;

		//pass essential data into personal
		personal.m_PERSON2_NAME=dbQuery.m_name;
		personal.m_PERSON2_COMPANY=dbQuery.m_division;
		personal.m_PERSON2_TELEPHONE=dbQuery.m_telephone;
		personal.m_PERSON2_REMARK=dbQuery.m_remark;
		
		if (dbQuery.m_class==1)
		{
			personal.m_PERSON2_COMPANY="ITRI "+dbQuery.m_division;
//@Department			personal.m_PERSON2_DEPARTMENT=dbQuery.m_department;
		}
//@Department		else
//@Department			personal.m_PERSON2_DEPARTMENT="";

		// Because dbQuery.m_pic is full path,
		// we just need file name;

		CString pic;
		int nFirst,nCount;

		nFirst=dbQuery.projectpath.GetLength();
		nCount=dbQuery.m_pic.GetLength()-dbQuery.projectpath.GetLength();
		pic=dbQuery.m_pic.Mid(nFirst,nCount);
		
		personal.m_PERSON2_PIC=pic;



		
		int result;
		result=personal.DoModal();

		if (result==IDOK)
		{

			CString strTest;

			// collect information in dialog into info_personal
			InfoPersonal info_personal;

			// sn,class are static information
			// catch from dbQuery
			info_personal.m_sn=dbQuery.m_sn;
			info_personal.m_class=dbQuery.m_class;


			info_personal.m_name=personal.m_PERSON2_NAME;
			
			if (dbQuery.m_class==1)
			{
				//if class=1 ,We truncate "ITRI" 
				CString division;
				division=personal.m_PERSON2_COMPANY;
				division.Replace("ITRI","");
				division.TrimLeft();
				division.TrimRight();
			
				info_personal.m_division=division;
//@Department		info_personal.m_department=personal.m_PERSON2_DEPARTMENT;
			}
			else
			{
				info_personal.m_division=personal.m_PERSON2_COMPANY;
//@Department				info_personal.m_department="";
			}
			
			info_personal.m_telephone=personal.m_PERSON2_TELEPHONE;
			info_personal.m_remark=personal.m_PERSON2_REMARK;
			//info_personal.m_pic=personal.m_PERSON2_PIC;


			//		Copy original path into our target path
			//		Modify pic path , map into ./pic/B&...
			char c2[20];					
			itoa(dbQuery.m_sn,c2,10);
			int m_leng=personal.m_PERSON2_PIC.GetLength();
			int m_ext=personal.m_PERSON2_PIC.Find('.');
			CString tt=personal.m_PERSON2_PIC.Mid(m_ext,m_leng-m_ext);


			CPersonal cpersonal;
			CString projectpath=cpersonal.projectpath;
			

			CString newpath=projectpath+"\\pic\\B"+c2+tt;
			CString strStorePath="\\pic\\B";	
			strStorePath=strStorePath+c2+tt;
			info_personal.m_pic=strStorePath;
			//copy picture selected by users into default path ".\pic\ "
			CopyFile(personal.m_PERSON2_PIC,newpath,FALSE);


			cpersonal.ModifyDB(&info_personal);





		}

	}
	else
	{
		if(MessageBox("This user is not in our database!\n Add new one now?","Not Found",MB_ICONWARNING|MB_YESNO)==IDYES)
		{
			// Add new user with phone number into Address Book.
			_OnBtnpersonalAdd( strPhoneNum);
		}
		else
			return;
	}

}
void CPanelDlg::OnBtnCallLogDetail()
{




	int nCount =m_ListCallLog.GetItemCount();



		//acquire informatin about which item double click by user
		int p=m_ListCallLog.GetSelectionMark() ;
		
		
		// Do not process when use double click empty area
		if(p>-1)
		{
			InfoCallLog infoCallLog;
			CCallLog callLog;
			callLog.getItemInfo(&infoCallLog,m_ListCallLog.GetItemData(p));
		
			//Implement CCallLogDlg
			CCallLogDlg dlgCallLog;
			dlgCallLog.m_CallLog_Tel=infoCallLog.m_Tel;
			dlgCallLog.m_CallLog_URI=infoCallLog.m_URI;
			dlgCallLog.m_CallLog_Start=callLog.tranHumenSenseTime(infoCallLog.m_StartTime);
			dlgCallLog.m_CallLog_End=callLog.tranHumenSenseTime(infoCallLog.m_EndTime);
			if (infoCallLog.m_Type==1)
				dlgCallLog.m_CallLog_Type="Call In";
			else
				dlgCallLog.m_CallLog_Type="Call Out";
			COleDateTime interval;
			interval=infoCallLog.m_EndTime-infoCallLog.m_StartTime;
			CString strInterval;
			strInterval=interval.Format(_T("%H:%M:%S"));
			dlgCallLog.m_CallLog_Interval=strInterval;
			dlgCallLog.DoModal();
			CSortListCtrl* plistctrl;
			plistctrl=&m_ListCallLog;
	
			if (dlgCallLog.isRefresh==true)
				callLog.pourRecord(plistctrl);			

			// Sam marked these code: don't need to rebuild personal address book list
			/*
			m_ListPersonal.DeleteAllItems();

			CDBAccess dbPersonal;
			CString strInitial,errStr;
			strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
	
	
			dbPersonal.Initialize(strInitial,errStr);
			dbPersonal.OpenRecordSet("SELECT * FROM personal where class=2",errStr);	
			if( !errStr.IsEmpty() )
				AfxMessageBox(errStr);   

			int ee=dbPersonal.GetRecordCount();


			char charPersonalSn[20];
			char charPersonalClass[10];
	



			// whether we get any record in database ?
			if(dbPersonal.GetRecordCount())
			{
				// if we get records, then we evaluate the number of records
		
				int intPersonalCount=dbPersonal.GetRecordCount();
				CString getName,getDivision,getTel,getRemark,getPic;
	
		
				if (intPersonalCount>0	)
				{

					if(intPersonalCount==1)
					{	
						//transform m_sn from int to char[]
						itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
						//transform m_class form int to char
						itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
						//add items into ListView 

						dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);				
						getName.ReleaseBuffer();
						dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
						getDivision.ReleaseBuffer();
						dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
						getTel.ReleaseBuffer();
						dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
						getRemark.ReleaseBuffer();
						dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
						getPic.ReleaseBuffer();
						m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel),_T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					}
					else
					{
						//transform m_sn from int to char[]
						itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
						//transform m_class form int to char
						itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
						//add items into ListView 

						dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
						getName.ReleaseBuffer();
						dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
						getDivision.ReleaseBuffer();
						dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
						getTel.ReleaseBuffer();
						dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
						getRemark.ReleaseBuffer();
						dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
						getPic.ReleaseBuffer();
						m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
			

						for (int i=0;i<(intPersonalCount-1);i++)
						{
							dbPersonal.MoveNext();
							//transform m_sn from int to char[]
							itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
							//transform m_class form int to char
							itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
							//add items into ListView 

							dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
							getName.ReleaseBuffer();
							dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
							getDivision.ReleaseBuffer();
							dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
							getTel.ReleaseBuffer();
							dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
							getRemark.ReleaseBuffer();
							dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
							getPic.ReleaseBuffer();
							m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					
						}
					}

			}
		
			dbPersonal.CloseRecordSet();
			}
			*/
		}

}

void CPanelDlg::AddressExport()
{

	// select place to store exporting file
	char BASED_CODE szFilter[] = "Ext Files (*.ext)|*.ext|CSV Files (*.csv)|*.csv||";
	CFileDialog SaveFileDialog(FALSE, "ext", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
	SaveFileDialog.m_ofn.lpstrTitle = "Save an extension file";
	CString PathFile;

	if(SaveFileDialog.DoModal()==IDOK)
	{		
		PathFile.Format("%s", SaveFileDialog.m_ofn.lpstrFile);
	}

	if(PathFile.GetLength()==0)
		return;

	// distinguish   *.ext or *.csv
	if(PathFile.Right(3)=="csv")
	{
		//process CSV
		CPack pk;
		pk.SetTitle("PCAEXPERT");
		pk.CSVExport(PathFile);
		

	} //end (ImFileFormat(PathFile)=="CSV")
	else
	{

		// pack PCAdb.mdb
		CString PathMDB;
		PathMDB=projectpath+"\\PCAdb.mdb";
		CPack pack;
		pack.SetTitle("PCAEXPERT");
		pack.CreatePack(PathMDB,PathFile);

		// pack ./pic/*.*
		CString PathPic,FileName;
		PathPic=projectpath+"\\pic\\*.*";
		CFileFind finder;
		BOOL bWorking;
		bWorking=finder.FindFile(PathPic);
		while(bWorking)
		{
			bWorking=finder.FindNextFile();
			if(finder.IsDots())
				continue;			
			pack.AddPack(finder.GetFilePath());
		}

	}//end else (ImFileFormat(PathFile)=="CSV")

	AfxMessageBox("Finish Exporting PhoneBook", MB_ICONINFORMATION|MB_OK);
}


//alan: maybe need to be modified
void CPanelDlg::AddressImport()
{
	CString strConnectionOri;
	CString strConnectionNew;
	CPersonal PersonalModify;
	CDBAccess dbOri,dbNew;
	CString errStr;
	CPack pack;
	int i;
	pack.SetTitle("PCAEXPERT");

	
	// OpenFile Dialog
	CString SourceFilePath;
	char BASED_CODE szFilter[] = "Ext Files (*.ext)|*.ext|CSV Files (*.csv)|*.csv|All Files(*.*)|*.*||";
	CFileDialog OpenFileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
	OpenFileDialog.m_ofn.lpstrTitle = "Open an extension file";

	
	if(OpenFileDialog.DoModal()==IDOK)
	{		
		SourceFilePath.Format("%s", OpenFileDialog.m_ofn.lpstrFile);
	}

	if (SourceFilePath.GetLength()==0)
		return ;


	
	// distinguish   *.ext or *.csv
	if(ImFileFormat(SourceFilePath)=="CSV")
	{
		//process CSV
		

		// store all import data into cp.CSVArray[]		
		CPack cp;
		cp.CSVImport(SourceFilePath);
		CQueryDB qDB;
		InfoPersonal infoDB;


		//setup connection string for originate db
		strConnectionOri.Format("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s\\PCAdb.mdb",projectpath);
		dbOri.Initialize(strConnectionOri,errStr);
		dbOri.OpenRecordSet("SELECT * FROM personal",errStr);

		CArray<InfoPersonal,InfoPersonal>ArrayOriDB;

		//sote oriDB into ArrayOriDB
		SqlRecordToArray(strConnectionOri,"SELECT * FROM personal",&ArrayOriDB);

		bool bProcessOne=true;  // true: process only one; false: process all
		bool bProcessCover=true; // true: cover  ;  false:skip
		bool bIfRepeat=false;

		// check whether repeat
		CString ShowMsg;
		int SnNumber;
		int j;
		int Records=cp.CSVArray.GetSize();
		int RecordsDBOri=dbOri.GetRecordCount();

		for( i=0;i<Records;i++)
		{
			for(j=0;j<RecordsDBOri;j++)
			{

				// option of repeat
				if(cp.CSVArray[i].m_telephone==ArrayOriDB[j].m_telephone)
				{
					bIfRepeat=true;			
				}
			}


			if(bIfRepeat)
			{
						CRepeatOptDlg RepeatDlg;
						ShowMsg.Format("Phone number %s is duplicate!",cp.CSVArray[i].m_telephone);
						RepeatDlg.m_msg=ShowMsg;
						RepeatDlg.m_opt=0;
						
						if(bProcessOne)
						{
							if(RepeatDlg.DoModal()!=IDOK)
								return;
							else
							{
								UpdateData(TRUE);
								switch(RepeatDlg.m_opt)
								{
								case 0: //cover one
									bProcessOne=true;
									bProcessCover=true;
									break;
								case 1: //cover all
									bProcessOne=false;
									bProcessCover=true;
									break;
								case 2: //skip one
									bProcessOne=true;
									bProcessCover=false;
									break;
								case 3: //skip all
									bProcessOne=false;
									bProcessCover=false;
									break;

								} //end switch
							} // end if(RepeatDlg.DoModal()!=IDOK)
						} //end if(bProcessOne)

						
						// if user select cover....
						if(bProcessCover)
						{
							//get oriDB's sn
							qDB.SearchTel(cp.CSVArray[i].m_telephone); // we get qDB.m_sn
							infoDB.m_sn=qDB.m_sn;
							infoDB.m_class=cp.CSVArray[i].m_class;
							infoDB.m_department=cp.CSVArray[i].m_department;
							infoDB.m_division=cp.CSVArray[i].m_division;
							infoDB.m_name=cp.CSVArray[i].m_name;					
							infoDB.m_remark=cp.CSVArray[i].m_remark;
							infoDB.m_telephone=cp.CSVArray[i].m_telephone;	
							infoDB.m_pic=""; // in CSV, set m_pic empty
						}// end bProcessCover
					
						PersonalModify.ModifyDB(&infoDB);
						
			} // end if(bIfRepeat)
			else
			{
				// action of "No Repeat"
				infoDB.m_class=2;
				infoDB.m_department=cp.CSVArray[i].m_department;
				infoDB.m_division=cp.CSVArray[i].m_division;
				infoDB.m_name=cp.CSVArray[i].m_name;
				infoDB.m_pic="";
				infoDB.m_remark=cp.CSVArray[i].m_remark;
				infoDB.m_telephone=cp.CSVArray[i].m_telephone;

				//Add new record into personal and get sn number
				SnNumber=PersonalModify.AddDB(&infoDB);

			} //end else if(bIfRepeat)			
			


			// reset initial setting
			bIfRepeat=false;

		} // end for( i=0;i<RecordsDBNew;i++)

		// refresh Personal list
		RefreshPersonalList();

	} // end "CSV" process
	else
	{
		if(!pack.UnPack(SourceFilePath))
			return;

		strConnectionNew.Format("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s\\PCAdb.mdb",pack.UnpackPath);

		//setup connection string for originate db
		strConnectionOri.Format("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s\\PCAdb.mdb",projectpath);
		dbOri.Initialize(strConnectionOri,errStr);
		dbNew.Initialize(strConnectionNew,errStr);
		dbOri.OpenRecordSet("SELECT * FROM personal",errStr);
		dbNew.OpenRecordSet("SELECT * FROM personal",errStr);

		int RecordsDBOri,RecordsDBNew;
		RecordsDBOri=dbOri.GetRecordCount();
		RecordsDBNew=dbNew.GetRecordCount();

		CArray<InfoPersonal,InfoPersonal>ArrayOriDB;
		CArray<InfoPersonal,InfoPersonal>ArrayNewDB;

		CQueryDB qDB;
		InfoPersonal infoDB;


		//sote oriDB into ArrayOriDB
		SqlRecordToArray(strConnectionOri,"SELECT * FROM personal",&ArrayOriDB);


		//store newDB into ArrayNewDB
		SqlRecordToArray(strConnectionNew,"SELECT * FROM personal",&ArrayNewDB);



		bool bProcessOne=true;  // true: process only one; false: process all
		bool bProcessCover=true; // true: cover  ;  false:skip
		bool bIfRepeat=false;
		// check whether repeat

		CString ShowMsg;
		int SnNumber;
		int j;
		for( i=0;i<RecordsDBNew;i++)
		{
			for(j=0;j<RecordsDBOri;j++)
			{

				// option of repeat
				if(ArrayNewDB[i].m_telephone==ArrayOriDB[j].m_telephone)
				{
					bIfRepeat=true;			
				}
			}

			if(bIfRepeat)
			{
						CRepeatOptDlg RepeatDlg;
						ShowMsg.Format("Phone number %s is duplicate!",ArrayNewDB[i].m_telephone);
						RepeatDlg.m_msg=ShowMsg;
						RepeatDlg.m_opt=0;
						
						if(bProcessOne)
						{
							if(RepeatDlg.DoModal()!=IDOK)
								return;
							else
							{
								UpdateData(TRUE);
								switch(RepeatDlg.m_opt)
								{
								case 0: //cover one
									bProcessOne=true;
									bProcessCover=true;
									break;
								case 1: //cover all
									bProcessOne=false;
									bProcessCover=true;
									break;
								case 2: //skip one
									bProcessOne=true;
									bProcessCover=false;
									break;
								case 3: //skip all
									bProcessOne=false;
									bProcessCover=false;
									break;

								} //end switch
							} // end if(RepeatDlg.DoModal()!=IDOK)
						} //end if(bProcessOne)

						
						// if user select cover....
						if(bProcessCover)
						{
							//get oriDB's sn
							qDB.SearchTel(ArrayNewDB[i].m_telephone); // we get qDB.m_sn
							infoDB.m_sn=qDB.m_sn;
							infoDB.m_class=ArrayNewDB[i].m_class;
							infoDB.m_department=ArrayNewDB[i].m_department;
							infoDB.m_division=ArrayNewDB[i].m_division;
							infoDB.m_name=ArrayNewDB[i].m_name;					
							infoDB.m_remark=ArrayNewDB[i].m_remark;
							infoDB.m_telephone=ArrayNewDB[i].m_telephone;

							SnNumber=infoDB.m_sn;

							//process picture
							// According sn number, modify pic name
							// if class = 1 : A###.##
							// else         : B###.##

							CString c1,picFileName,picFileName2,sCopyFrom,sCopyTo;
							switch(ArrayNewDB[i].m_class)
							{
							case 1:
								infoDB.m_pic.Format("\\pic\\A%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));							

								c1=ArrayNewDB[i].m_pic;

								//extract file name
								picFileName=c1;
								picFileName.Delete(0,5);
								picFileName2.Format("A%d",SnNumber);
								sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
								sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
								CopyFile(sCopyFrom,sCopyTo,FALSE);

								break;
							case 2:
								infoDB.m_pic.Format("\\pic\\B%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));													

								c1=ArrayNewDB[i].m_pic;

								//extract file name
								picFileName=c1;
								picFileName.Delete(0,5);
								picFileName2.Format("B%d",SnNumber);
								sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
								sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
								CopyFile(sCopyFrom,sCopyTo,FALSE);

								
								break;
							default:
								infoDB.m_pic.Format("//pic//B%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));				
								c1=ArrayNewDB[i].m_pic;

								//extract file name
								picFileName=c1;
								picFileName.Delete(0,5);
								picFileName2.Format("B%d",SnNumber);
								sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
								sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
								CopyFile(sCopyFrom,sCopyTo,FALSE);

							}
							PersonalModify.ModifyDB(&infoDB);
						}

						
			} // end if(bIfRepeat)
			else
			{
				// action of "No Repeat"
				infoDB.m_class=ArrayNewDB[i].m_class;
				infoDB.m_department=ArrayNewDB[i].m_department;
				infoDB.m_division=ArrayNewDB[i].m_division;
				infoDB.m_name=ArrayNewDB[i].m_name;
				infoDB.m_pic=ArrayNewDB[i].m_pic;
				infoDB.m_remark=ArrayNewDB[i].m_remark;
				infoDB.m_telephone=ArrayNewDB[i].m_telephone;

				//Add new record into personal and get sn number
				SnNumber=PersonalModify.AddDB(&infoDB);

				// According sn number, modify pic name
				// if class = 1 : A###.##
				// else         : B###.##
		


				CString c1,picFileName,picFileName2,sCopyFrom,sCopyTo;
				switch(ArrayNewDB[i].m_class)
				{
				case 1:
					infoDB.m_pic.Format("\\pic\\A%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));				
					c1=ArrayNewDB[i].m_pic;
					//extract file name
					picFileName=c1;
					picFileName.Delete(0,5);
					picFileName2.Format("A%d",SnNumber);
					sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
					sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
					CopyFile(sCopyFrom,sCopyTo,FALSE);

					break;
				case 2:
					infoDB.m_pic.Format("\\pic\\B%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));				
					c1=ArrayNewDB[i].m_pic;
					//extract file name
					picFileName=c1;
					picFileName.Delete(0,5);
					picFileName2.Format("B%d",SnNumber);
					sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
					sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
					CopyFile(sCopyFrom,sCopyTo,FALSE);

					
					break;
				default:
					infoDB.m_pic.Format("\\pic\\B%d%s",SnNumber,ArrayNewDB[i].m_pic.Right(ArrayNewDB[i].m_pic.GetLength()-ArrayNewDB[i].m_pic.ReverseFind('.')));				
					c1=ArrayNewDB[i].m_pic;
					//extract file name
					picFileName=c1;
					picFileName.Delete(0,5);
					picFileName2.Format("B%d",SnNumber);
					sCopyFrom.Format("%s\\tmppack\\%s",projectpath,picFileName);
					sCopyTo.Format("%s\\pic\\%s%s",projectpath,picFileName2,sCopyFrom.Right(sCopyFrom.GetLength()-sCopyFrom.ReverseFind('.')));
					CopyFile(sCopyFrom,sCopyTo,FALSE);


				}
				infoDB.m_sn=SnNumber;
				PersonalModify.ModifyDB(&infoDB);

			} //end else if(bIfRepeat)

			// reset initial setting
			bIfRepeat=false;

		}


		// clean tmp file
		dbOri.CloseRecordSet();
		dbOri.CloseConnection();
		dbNew.CloseRecordSet();
		dbNew.CloseConnection();
		pack.CleanTemp();
		// delete ./tmppack 
		_rmdir(projectpath+"\\tmppack");

		// refresh Personal list
		RefreshPersonalList();
	}



	AfxMessageBox("Finish Import PhoneBook", MB_ICONINFORMATION|MB_OK);
}

void CPanelDlg::RefreshPersonalList()
{

#ifdef _XCAPSERVER

	m_ListPersonal.DeleteAllItems();
	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");	
	int ee = GetBuddyCount();
	if(ee>0)
	{
			BuddyList *myBuddyList = GetAllBuddyList();
			BuddyList *tmpPtr;
			CString getName,getDivision,getTel,getRemark,getPic;
			if(myBuddyList!=NULL)
			{
				tmpPtr = myBuddyList;

				while(tmpPtr!=NULL)
				{
					if(tmpPtr->buddy==NULL)
					{
						tmpPtr = tmpPtr->next;
						continue;
					}
					strcpy((LPTSTR)getName.GetBuffer(64),tmpPtr->buddy->m_displayname);
					getName.ReleaseBuffer();
					if(tmpPtr->buddy->m_division[0] != 0)
					{
						strcpy((LPTSTR)getDivision.GetBuffer(64),tmpPtr->buddy->m_division);
						getDivision.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_remark[0] != 0)
					{
						strcpy((LPTSTR)getRemark.GetBuffer(64),tmpPtr->buddy->m_remark);
						getRemark.ReleaseBuffer();
					
					}
					if(tmpPtr->buddy->m_telephone[0] != 0)
					{
						strcpy((LPTSTR)getTel.GetBuffer(64),tmpPtr->buddy->m_telephone);
						getTel.ReleaseBuffer();
					
					}
					
					//add group info....
//					m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel),_T(getDivision), _T(getRemark)),0);
					m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Offline, _T(getName), _T(PRE_OFFLINE), _T(PRE_OFFLINE), _T(getTel),_T(getDivision), _T(getRemark)),0);
					tmpPtr = tmpPtr->next;
				}

				FreeBuddyList(myBuddyList);

			}
	}
	
#else
	CDBAccess dbPersonal;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
	
	m_ListPersonal.DeleteAllItems();
	dbPersonal.Initialize(strInitial,errStr);
	dbPersonal.OpenRecordSet("SELECT * FROM personal where class=2",errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	int ee=dbPersonal.GetRecordCount();


	char charPersonalSn[20];
	char charPersonalClass[10];
	



	// whether we get any record in database ?
	if(dbPersonal.GetRecordCount())
	{
		// if we get records, then we evaluate the number of records
		
		int intPersonalCount=dbPersonal.GetRecordCount();
		CString getName,getDivision,getTel,getRemark,getPic;
	
		
		if (intPersonalCount>0	)
		{

			if(intPersonalCount==1)
			{	
				//transform m_sn from int to char[]
				itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
				//transform m_class form int to char
				itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
				//add items into ListView 

				dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);				
				getName.ReleaseBuffer();
				dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
				getDivision.ReleaseBuffer();
				dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
				getTel.ReleaseBuffer();
				dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
				getRemark.ReleaseBuffer();
				dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
				getPic.ReleaseBuffer();
				m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
			}
			else
			{
				//transform m_sn from int to char[]
				itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
				//transform m_class form int to char
				itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
				//add items into ListView 

				dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
				getName.ReleaseBuffer();
				dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
				getDivision.ReleaseBuffer();
				dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
				getTel.ReleaseBuffer();
				dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
				getRemark.ReleaseBuffer();
				dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
				getPic.ReleaseBuffer();
				m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
	

				for (int i=0;i<(intPersonalCount-1);i++)
				{
					dbPersonal.MoveNext();
					//transform m_sn from int to char[]
					itoa(dbPersonal.GetIntegerFromField(0),charPersonalSn,10);
					//transform m_class form int to char
					itoa(dbPersonal.GetIntegerFromField(1),charPersonalClass,10);
					//add items into ListView 

					dbPersonal.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
					getName.ReleaseBuffer();
					dbPersonal.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);				
					getDivision.ReleaseBuffer();
					dbPersonal.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
					getTel.ReleaseBuffer();
					dbPersonal.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
					getRemark.ReleaseBuffer();
					dbPersonal.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
					getPic.ReleaseBuffer();
					m_ListPersonal.SetItemData(m_ListPersonal.AddItem(Presence_Unknown, _T(getName), _T("Unknown"), _T("Unknown"), _T(getTel), _T(getDivision), _T(getRemark)),dbPersonal.GetIntegerFromField(0));
					
				}
			}

		}
		
		dbPersonal.CloseRecordSet();
	}
#endif
	// update subscription
	SubscribePhoneBook();
}

void CPanelDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	switch(nIDEvent)
	{
	case ID_TIMER_VM:
		OnBtnVoiceMailReceive();
		break;

	}
	TRACE("Finish OnBtnVoiceMailReceive() one times");
	CDialog::OnTimer(nIDEvent);
}

CString CPanelDlg::ImFileFormat(CString sSource)
{
	// Identify format of this file  ( *.ext or *.csv)

	CString result;
	CFile file(sSource,CFile::modeRead|CFile::typeBinary);
	unsigned long lActualfile;
	lActualfile=file.Seek(0,CFile::begin);
	char Buffer[256];
	file.Read(Buffer,9);
	Buffer[9]='\0';


	if(strcmp(Buffer,"PCAEXPERT")==0)
		result="EXT";
	else
		result="CSV";

	file.Close();
	
	return result;

}

//alan: need to be modified

void CPanelDlg::SqlRecordToArray(CString connection,CString operation,CArray<InfoPersonal,InfoPersonal> *cArray)
{
//  Store sql results into cArray
//  ex:
//	connection="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=\\PCAdb.mdb";
//  operation="SELECT * FROM personal"

	CDBAccess db;
	CString errStr;	
	db.Initialize(connection,errStr);
	db.OpenRecordSet(operation,errStr);
	int RecordsDB=db.GetRecordCount();

	InfoPersonal infoDB;
	//sote into infoDB 
	for(int i=0;i<RecordsDB;i++)
	{
		
		// temporary setting
		infoDB.m_sn=db.GetIntegerFromField(0);
		infoDB.m_class=db.GetIntFromField(1);
			
		db.GetStringFromField(2,(LPTSTR)infoDB.m_name.GetBuffer(128),128);
		infoDB.m_name.ReleaseBuffer();
		db.GetStringFromField(3,(LPTSTR)infoDB.m_id.GetBuffer(128),128);
		infoDB.m_id.ReleaseBuffer();
		db.GetStringFromField(4,(LPTSTR)infoDB.m_division.GetBuffer(128),128);
		infoDB.m_division.ReleaseBuffer();
		db.GetStringFromField(5,(LPTSTR)infoDB.m_department.GetBuffer(128),128);
		infoDB.m_department.ReleaseBuffer();
		db.GetStringFromField(6,(LPTSTR)infoDB.m_telephone.GetBuffer(128),128);
		infoDB.m_telephone.ReleaseBuffer();
		db.GetStringFromField(7,(LPTSTR)infoDB.m_remark.GetBuffer(128),128);
		infoDB.m_remark.ReleaseBuffer();
		db.GetStringFromField(8,(LPTSTR)infoDB.m_pic.GetBuffer(128),128);
		infoDB.m_pic.ReleaseBuffer();
		cArray->Add(infoDB);
		
		if(i!=RecordsDB-1)
			db.MoveNext();

	}
	db.CloseRecordSet();
	db.CloseConnection();
}

void CPanelDlg::StateItri()
{
	// show ITRI list
	m_ListItri.ShowWindow(SW_SHOWNA);
	
	// hide CallLog,Personal,Search,VoiceMail
	m_ListCallLog.ShowWindow(SW_HIDE);
	m_ListPersonal.ShowWindow(SW_HIDE);
	m_ListSearch.ShowWindow(SW_HIDE);
	m_ListVoiceMail.ShowWindow(SW_HIDE);


	SetButtonShow("BTN_SEARCH", TRUE);
	SetButtonShow("BTN_CLEAR", TRUE);

	SetButtonShow("BTN_ADD", FALSE);
	SetButtonShow("BTN_EDIT", FALSE);
	SetButtonShow("BTN_DELETE", FALSE);
	SetButtonShow("BTN_DIAL", FALSE);

	// hide Serach paragraph
	CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_STATIC_SPN);
	pSEARCH->ShowWindow(SW_HIDE) ;

	//show itri
	GetDlgItem(IDC_ITRI_EmployId)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_DepartmentId)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_CName)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_Email)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_Ext)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_LAB)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_Search)->ShowWindow(SW_SHOWNA);
	GetDlgItem(IDC_ITRI_CLEAR)->ShowWindow(SW_SHOWNA);

	SetButtonCheck("BTN_TAB_PERSONAL", FALSE);
	SetButtonCheck("BTN_TAB_SEARCH", FALSE);
	SetButtonCheck("BTN_TAB_ITRI", g_pMainDlg->m_bEnablePublicAddressBook );

}

void CPanelDlg::OnBtnItriSearch()
{
	m_ListItri.DeleteAllItems();
	UpdateData(TRUE);
	
	if (m_ITRI_CName == "" && m_ITRI_EmployId == "") {
		if (m_strITRILab == "" && m_ITRI_DepartmentId == "") {
			MessageBox("\nCannot search. \n\nPlease give some keyword. \n", "ITRI Search", MB_OK);
			return;
		}
		else if  (m_strITRILab != "" && m_ITRI_DepartmentId == "") {
			MessageBox("\nThe search range is too large.\n\nPlease give more keyword. \n", "ITRI Search", MB_OK);
			return;
		}
		else if  (m_strITRILab == "" && m_ITRI_DepartmentId != "") {
			MessageBox("\nThe search terms are ambiguous.\n\n Please give Laboratories to search only by Department.\n", "ITRI Search", MB_OK);
			return;
		}
	}

	m_bItriSearch=true;

	CWinApp *pApp=AfxGetApp();
	CString PCAServerURL,PCAServerPort,strPCAServer;
	char charPCAServer[128];

	PCAServerURL=pApp->GetProfileString("","PCAServerAddress");
	if(PCAServerURL.IsEmpty())
	{
		pApp->WriteProfileString("","PCAServerURL","127.0.0.1");
		PCAServerURL="127.0.0.1";
	}

	PCAServerPort=pApp->GetProfileString("","PCAServerPort");
	if(PCAServerPort.IsEmpty())
	{
		pApp->WriteProfileString("","PCAServerPort","8080");
		PCAServerPort="8080";
	}

	strPCAServer.Format(_T("http://%s:%s/RPC2"),PCAServerURL,PCAServerPort);
	strcpy(charPCAServer,strPCAServer);

	CString strLabCode;
	if (m_strITRILab == "電通")	// 電腦與通訊研究所
		strLabCode = "CCL";
	else if (m_strITRILab == "電子")	// 電子工業研究所
		strLabCode = "ERSO";
	else if (m_strITRILab == "光電")	// 光電工業研究所
		strLabCode = "OES";
	else if (m_strITRILab == "化工")	// 化學工業研究所
		strLabCode = "UCL";
	else if (m_strITRILab == "材料")	// 工業材料研究所
		strLabCode = "MRL";
	else if (m_strITRILab == "機械")	// 機械工業研究所
		strLabCode = "MIRL";
	else if (m_strITRILab == "環安")	// 環境與安全衛生技術發展中心
		strLabCode = "CESH";
	else if (m_strITRILab == "生醫")	// 生物醫學工程中心
		strLabCode = "BMEC";
	else if (m_strITRILab == "能資")	// 能源與資源研究所
		strLabCode = "ERL";
	else if (m_strITRILab == "量測")	// 量測技術發展中心
		strLabCode = "CMS";
	else if (m_strITRILab == "晶片")	// 系統晶片技術發展中心
		strLabCode = "STC";
	else if (m_strITRILab == "經資")	// 產業經濟與資訊服務中心
		strLabCode = "IEK";
	else if (m_strITRILab == "系統")	// 系統與航太技術發展中心
		strLabCode = "CAST";
	else if (m_strITRILab == "院部")
		strLabCode = "HQ";
	else if (m_strITRILab == "行政")
		strLabCode = "ASC";
	else if (m_strITRILab == "資訊")
		strLabCode = "ISC";
	else if (m_strITRILab == "技轉")
		strLabCode = "TTSC";
	else if (m_strITRILab == "會計")
		strLabCode = "ARC";
	else if (m_strITRILab == "奈米")
		strLabCode = "NTRC";
	else if (m_strITRILab == "學院")
		strLabCode = "IA";
	else
		strLabCode = "";

	int size=30;

	unsigned long milliseconds = 5000;
	g_hSem = CreateSemaphore(
	NULL, /* pointer to security attributes */
	0,	/*  initial count */
	5,	/*  maximum count */
	NULL	/*  pointer to semaphore-object name */);
    xmlrpc_env env;
	int minState = 40;
	xmlrpc_env_init(&env);
    /* Start up our XML-RPC client library using the curl transport. */
	xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
		VERSION, "curl");
	xmlrpc_client_call_asynch(charPCAServer, "examples.search_itri",
				  search_itri, NULL,
				  "(s6ssss)", 
				  (char*)(LPCTSTR)m_ITRI_EmployId,
				  (char*)(LPCTSTR)m_ITRI_CName,30,
				  (char*)(LPCTSTR)m_ITRI_DepartmentId,
				  (char*)(LPCTSTR)m_ITRI_Email,
				  (char*)(LPCTSTR)m_ITRI_Ext,
				  (char*)(LPCTSTR) strLabCode);


	switch (WaitForSingleObject (
		g_hSem /* handle to object to wait for */,
		milliseconds /* time-out interval in milliseconds*/) )
	{
		/* One may want to handle these cases  */
		case WAIT_OBJECT_0:	
		{
			int num = ArrayMsgItri.GetSize();
							
			if (num <= 0)
			{
				MessageBox("\nThere is no item matched.\t\n", "Not Found", MB_OK);
			}
			else
			{
				for (int i=0;i<num;i++)				
					m_ListItri.AddItem(_T(ArrayMsgItri[i].m_EmployId),_T(ArrayMsgItri[i].m_DepartmentId),_T(ArrayMsgItri[i].m_CName),_T(ArrayMsgItri[i].m_Ext),_T(ArrayMsgItri[i].m_Email));

				if (num>=ResultLimit)
				{
					MessageBox("\nOnly first 100 results are listed.\nPlease give more filters to narrow down search results.","Too many results",MB_OK);
				}
			}
		}
			break;
		
	case WAIT_TIMEOUT:

		MessageBox("\nTime-out.\nPlease Check the PCA Server or Server Configuration.\n", "Not Found", MB_OK);
		break;
	}
    xmlrpc_env_clean(&env);
    xmlrpc_client_cleanup();
	CloseHandle (g_hSem);	
}

void CPanelDlg::OnBtnVoiceMailSetting()
{
	char	rpcurl[1024];
	CString	sEmail;
	int		dwNotify = 0;
	int		index = -1;
	int		ret = 0;
	CEMailDialog	*dlg;

	UpdateData();
	while ( (ret=OnAuthenticate())!=1 ) {
		int LoginResult;
		CVoiceMailLogin VoiceMailLogin;

		if ( ret==0 )
			return;
		VoiceMailLogin.m_VoiceMailAccount = GetAccountId();
		VoiceMailLogin.m_VoiceMailPWD = GetAccountPasswd();
		LoginResult=VoiceMailLogin.DoModal();
		UpdateData(TRUE);
		if(LoginResult==IDOK)
		{
			Sleep(10);
			SetAccountId(VoiceMailLogin.m_VoiceMailAccount);
			SetAccountPasswd(VoiceMailLogin.m_VoiceMailPWD);
		} else {
			return;
		}
	}

	sprintf(rpcurl, "http://%s:%s/RPC2", GetConnectionURL(),GetConnectionPort());
	unsigned long milliseconds = 5000;
		g_hSem = CreateSemaphore(
		NULL, /* pointer to security attributes */
		0,	/*  initial count */
		5,	/*  maximum count */
		NULL	/*  pointer to semaphore-object name */);

    xmlrpc_env env;
	xmlrpc_env_init(&env);
	xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
		VERSION, "curl");

	xmlrpc_client_call_asynch(rpcurl, "VoiceMailServer.GetEmail",
				  getEmail, NULL,
				  "(ss)", (char*)(LPCTSTR)GetAccountId(),
				  (char*)(LPCTSTR)GetAccountPasswd());

	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		switch (WaitForSingleObject (
			g_hSem /* handle to object to wait for */,
			milliseconds /* time-out interval in milliseconds*/) )
		{
			/* One may want to handle these cases  */

		case WAIT_OBJECT_0:	
			{
				
			}
			break;
			
		case WAIT_TIMEOUT:
			AfxMessageBox("Can not connect into VoiceMail Server ");
			return ;

			break;
		}
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	dlg = new CEMailDialog;
	dlg->m_notifyType = iSaveNotify;
	CString strEmail(cSaveEmail);
	CString	strInterval;
	dlg->m_emailAddr = strEmail;
	dlg->m_newPassword = GetAccountPasswd();
	strInterval.Format("%d", GetInterval());
	dlg->m_interval = strInterval;
	
	ret = dlg->DoModal();
	if ( ret!=IDOK )
		return;
	int interval = atoi(dlg->m_interval.GetBuffer(10));
	if ( GetInterval()!=interval ) {
		KillTimer(ID_TIMER_VM);
		SetTimer(ID_TIMER_VM, interval*60000,NULL);
	}
	SetInterval(atoi(dlg->m_interval.GetBuffer(10)));

	// Check if the email setting modified
	if ( ((dlg->m_notifyType!=iSaveNotify) ||
		 (dlg->m_emailAddr!=strEmail)) &&
		 (dlg->m_newPassword!=GetAccountPasswd()) )
	{
		// Set email setting first with the old password
		xmlrpc_env_init(&env);
		xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
			VERSION, "curl");
		xmlrpc_client_call_asynch(rpcurl, "VoiceMailServer.SetEmailPassword",
					  setEmail, NULL,
					  "(ssssi)", (char*)(LPCTSTR)GetAccountId(),
					  (char*)(LPCTSTR)GetAccountPasswd(),
					  (char*)(LPCTSTR)dlg->m_newPassword,
					  (char*)(LPCTSTR)dlg->m_emailAddr,
					  (char*)(LPCTSTR)dlg->m_notifyType);

		SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		switch (WaitForSingleObject (
			g_hSem /* handle to object to wait for */,
			5000 /* time-out interval in milliseconds*/) )
		{
			/* One may want to handle these cases  */

		case WAIT_OBJECT_0:	
			{
				//AfxMessageBox("Update email settings complete!");
			}
			break;
			
		case WAIT_TIMEOUT:
			//AfxMessageBox("Error at SetEmail ");
			break;
		}

		SetCursor(LoadCursor(NULL, IDC_ARROW));
		xmlrpc_env_clean(&env);
		xmlrpc_client_cleanup();
		CloseHandle (g_hSem);
		
		if ( bSaveEmailResult ) {
			SetAccountPasswd(dlg->m_newPassword);
			MessageBox("Update email and password settings complete!", "Update Settings", MB_ICONINFORMATION|MB_OK);
		} else {
			MessageBox("Fail to change password!", "Update Settings",
				MB_ICONERROR | MB_OK);
		}
		
		delete dlg;
		return;
	}

	if ( (dlg->m_notifyType!=iSaveNotify) ||
		 (dlg->m_emailAddr!=strEmail) )
	{
		// Set email setting first with the old password
		xmlrpc_env_init(&env);
		xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
			VERSION, "curl");
		xmlrpc_client_call_asynch(rpcurl, "VoiceMailServer.SetEmail",
					  setEmail, NULL,
					  "(sssi)", (char*)(LPCTSTR)GetAccountId(),
					  (char*)(LPCTSTR)GetAccountPasswd(),
					  (char*)(LPCTSTR)dlg->m_emailAddr,
					  (char*)(LPCTSTR)dlg->m_notifyType);

		SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		switch (WaitForSingleObject (
			g_hSem /* handle to object to wait for */,
			5000 /* time-out interval in milliseconds*/) )
		{
			/* One may want to handle these cases  */

		case WAIT_OBJECT_0:	
			{
				//AfxMessageBox("Update email settings complete!");
			}
			break;
			
		case WAIT_TIMEOUT:
			//AfxMessageBox("Error at SetEmail ");
			break;
		}

		SetCursor(LoadCursor(NULL, IDC_ARROW));
		xmlrpc_env_clean(&env);
		xmlrpc_client_cleanup();
		CloseHandle (g_hSem);

		if ( bSaveEmailResult ) {
			MessageBox("Update email settings complete!", "Update Settings", MB_ICONINFORMATION|MB_OK);
		} else {
			MessageBox("Fail to change email settings!", "Update Settings",
				MB_ICONERROR | MB_OK);
		}

		delete dlg;
		return;
	}

	if ( dlg->m_newPassword!=GetAccountPasswd() ) 
	{
		// Set email setting first with the old password
		xmlrpc_env_init(&env);
		xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
			VERSION, "curl");
		xmlrpc_client_call_asynch(rpcurl, "VoiceMailServer.SetPassword",
					  setPassword, NULL,
					  "(sss)", (char*)(LPCTSTR)GetAccountId(),
					  (char*)(LPCTSTR)GetAccountPasswd(),
					  (char*)(LPCTSTR)dlg->m_newPassword);

		SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
		switch (WaitForSingleObject (
			g_hSem /* handle to object to wait for */,
			10000 /* time-out interval in milliseconds*/) )
		{
			/* One may want to handle these cases  */

		case WAIT_OBJECT_0:	
			{
				//AfxMessageBox("Update email settings complete!");
			}
			break;
			
		case WAIT_TIMEOUT:
			//AfxMessageBox("Error at SetEmail ");
			break;
		}

		SetCursor(LoadCursor(NULL, IDC_ARROW));
		xmlrpc_env_clean(&env);
		xmlrpc_client_cleanup();
		CloseHandle (g_hSem);	 
		
		if ( bSavePasswordResult ) {
			SetAccountPasswd(dlg->m_newPassword);
			MessageBox("Update password complete!", "Update Settings", MB_ICONINFORMATION|MB_OK);
		} else {
			MessageBox("Fail to change password!", "Update Settings",
				MB_ICONERROR | MB_OK);
		}

		delete dlg;
		return;
	}

	delete dlg;
}

BOOL CPanelDlg::PreTranslateMessage(MSG* pMsg) 
{
	if( !m_bItriSearch )
	{
		if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
		{

			::TranslateAccelerator(
				AfxGetMainWnd()->GetSafeHwnd(), 
				((CPCAUADlg*)AfxGetMainWnd())->m_hAccel, pMsg);
			//return TRUE;
		}
	}
	return CSkinDialog::PreTranslateMessage(pMsg);
}

static void doAuth(char * /*server_url*/,
				       char * /*method_name*/,
				       xmlrpc_value *param_array,
				       void * /*user_data*/,
				       xmlrpc_env *env,
				       xmlrpc_value *result)
{
	long prevCount;

	if (!die_if_fault_occurred(env))
		return;
	xmlrpc_parse_value(env, result, "b", &bPassAuth);
	if (!die_if_fault_occurred(env))
		return;

	//bPassAuth = bResult;
	ReleaseSemaphore(
	  g_hSem,   /* handle to the semaphore object */
	  1,  /* amount to add to current count */
	  &prevCount   /* address of previous count */ );

}

int CPanelDlg::OnAuthenticate()
{
	char	rpcurl[1024];
	int		ret = 1;
    xmlrpc_env env;
	int minState = 40;

	sprintf(rpcurl, "http://%s:%s/RPC2", GetConnectionURL(),GetConnectionPort());
	unsigned long milliseconds = 5000;
		g_hSem = CreateSemaphore(
		NULL, /* pointer to security attributes */
		0,	/*  initial count */
		5,	/*  maximum count */
		NULL	/*  pointer to semaphore-object name */);
	xmlrpc_env_init(&env);
    /* Start up our XML-RPC client library using the curl transport. */
	xmlrpc_client_init_with_transport (&env, XMLRPC_CLIENT_NO_FLAGS, NAME, 
		VERSION, "curl");

	// Use current password to login first
	xmlrpc_client_call_asynch(rpcurl, "VoiceMailServer.Authenticate", 
				  doAuth, NULL,
				  "(ss)", (char*)(LPCTSTR)GetAccountId(),
				  (char*)(LPCTSTR)GetAccountPasswd());

	SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
	switch (WaitForSingleObject (
		g_hSem /* handle to object to wait for */,
		milliseconds /* time-out interval in milliseconds*/) )
	{
		/* One may want to handle these cases  */
	case WAIT_ABANDONED:
		// VMServer is stopped
		MessageBox("Voice Mail is stopped!", "Voice Mail", 
				MB_ICONINFORMATION|MB_OK);
		break;
	case WAIT_OBJECT_0:	
		//xmlrpc_client_cleanup();
		break;		
	case WAIT_TIMEOUT:
		break;
	case WAIT_FAILED:
		// VMServer is stopped
		MessageBox("Voice Mail is stopped!", "Voice Mail", 
				MB_ICONINFORMATION|MB_OK);
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if ( !bPassAuth ) {
		if (MessageBox("Cannot connect to voice mail service. Try again ?",
					"Voice Mail Service", MB_ICONWARNING | MB_YESNO)==IDYES)
		{
			ret = -1;  // Fail but try again
		} else {
			ret = 0; // Fail and give up
		}
	} else {
		ret = 1; // Pass auth 
	}

    xmlrpc_env_clean(&env);
    xmlrpc_client_cleanup();
	CloseHandle (g_hSem);
	return ret;
}

void CPanelDlg::UpdateBuddyPresence(CString strNumber, int nPresence, CString strDes, CString strCallStatus)
{
#ifdef _SIMPLE

	int nCount = m_ListPersonal.GetItemCount();
	for (int i=0 ; i<nCount ; i++)
	{
		int nNum = m_ListPersonal.GetItemData(i);
		CString strThisNum = m_ListPersonal.GetItemText(i,defPersonalTelephone);
//		CString strThisNum = m_ListPersonal.GetItemText(i,defPersonalName);
		if (strThisNum == strNumber)
		{
			if (nPresence == 1)
				m_ListPersonal.SetItem(i, defPersonalName, LVIF_IMAGE, 0, Presence_Online, 0, 0, 0);
			else
			{
				// If the presence status is "Offline", check the call status.
				// If the call status is "Idle", show the green light, else show the red light.
				if (strCallStatus == "Idle")
					m_ListPersonal.SetItem(i, defPersonalName, LVIF_IMAGE, 0, Presence_Online, 0, 0, 0);
				else
					m_ListPersonal.SetItem(i, defPersonalName, LVIF_IMAGE, 0, Presence_Offline, 0, 0, 0);
			}
			if(!strDes.IsEmpty())
				m_ListPersonal.SetItemText(i, defPersonalPresence, strDes);
			if(!strCallStatus.IsEmpty())
				m_ListPersonal.SetItemText(i, defPersonalCallStatus, strCallStatus);
			AfxTrace("Update %s's Presence\n!!", strNumber);
		}
	}

#endif
}

void CPanelDlg::OnBtnItriClear() 
{
	m_ITRI_CName = "";
	m_ITRI_EmployId = "";
	m_ITRI_DepartmentId = "";
	m_strITRILab = "";

	m_ListItri.DeleteAllItems();
	UpdateData(FALSE);
	
}


void CPanelDlg::OnSetPresenceStatusOffline() 
{
	// TODO: Add your command handler code here
	
	// added by shen, 20041225
	if (CDlgMessage::GetCount() > 0)
		if ( MessageBox( "All windows of conversation will be closed.\n"
			"Are you sure to be off-line?", 
			"Message", MB_ICONQUESTION | MB_YESNO) != IDYES)
			return;
	// leave conference for all windows 
	CDlgMessage::OfflineFromAll(true);

	g_pMainDlg->UpdatePresence(0, CUACDlg::GetUAComRegString("User_Info","Username"));
	
}

void CPanelDlg::OnSetPresenceStatusOnline() 
{
	// TODO: Add your command handler code here
	g_pMainDlg->UpdatePresence(1, CUACDlg::GetUAComRegString("User_Info","Username"));
	
}

void CPanelDlg::OnNewGroup() 
{
	// TODO: Add your command handler code here

	CGroupManageDlg dlg;

	int result;
	result=dlg.DoModal();
	

	if (result==IDOK)
	{
#ifdef _XCAPSERVER	
		if(!CreateGroup((LPSTR)(LPCTSTR)dlg.m_newgroupname))//create failed...
			AfxMessageBox("Create Group Failed");

#endif
	}
}

void CPanelDlg::OnAddressBkBlock() 
{
	// TODO: Add your command handler code here
	CString ms;
	CString ms2;
	CString SelectItem;
	char t[10];
	int i=-1;
	int j=-1;
	int count=0; // the number of selected items
	do
	{
		j=m_ListPersonal.GetNextItem(i,LVNI_SELECTED);
		i=j;		
		itoa(j,t,10);
		SelectItem=SelectItem+t+",";
		count++;
	}while (j>-1);
	count--;




	if (count <1)
		MessageBox(_T("Plese indicate which items be blocked"),_T("Attention"),MB_OK|MB_ICONWARNING);
	else
	{
		CString csCond,csCLSn;
		char csSn[10];
		char charNum[12];
		int intNum;
		csCond="sn=";
		int intPos=0;
		int intComma=0;
		//int intItemData;
		for (int s=0;s<count;s++)
		{
			intComma=SelectItem.Find(',',intComma);
			lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));
	
			
			intNum=atoi(charNum);				
			itoa(m_ListPersonal.GetItemData(intNum),csSn,10);
				
			csCond=csCond+csSn+" or sn=";
			ms=ms+m_ListPersonal.GetItemText(intNum,defPersonalName)+",";
			csCLSn=csCLSn+csSn+",";  // prepare for ListPersonal deleteItem


			intComma++;
			intPos=intComma;			
		}

		csCond=csCond.Left(csCond.GetLength()-6);
		ms=ms.Left(ms.GetLength()-1);



	
		ms2.Format( "Are you sure to block this record '%s'?   ",ms);

		if (MessageBox(_T(ms2),_T("Block"),MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
		{
			intPos=0;
			intComma=0;
			int subtract=0;
			// delet items from address book list
			CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
			for(s=0;s<count;s++)
			{
				intComma=SelectItem.Find(',',intComma);
				lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));			
				intNum=atoi(charNum);			
					
				// Arlene added : Unsubscribe the select one
				CString strNumber = m_ListPersonal.GetItemText(intNum-subtract, defPersonalTelephone);
#ifdef _XCAPSERVER
				//add by alan for testing..
//				AddNewRule((LPSTR)(LPCTSTR)m_ListPersonal.GetItemText(intNum-subtract, defPersonalName));
				if(SetRuleAction2Block((LPSTR)(LPCTSTR)strNumber)==false)
				{
					AfxMessageBox("Set Authorization Rule to Block Failed");
					return;
				}
#endif
				g_pMainDlg->BlockBuddy((LPSTR)(LPCTSTR)strNumber);
				g_pMainDlg->UnSubscribe(strNumber);
				m_ListPersonal.SetItem(intNum-subtract,defPersonalName,LVIF_IMAGE,0,Presence_Offline,0,0,0);
				m_ListPersonal.SetItemText(intNum-subtract, defPersonalPresence, "Block");
				m_ListPersonal.SetItemText(intNum-subtract, defPersonalCallStatus, PRE_OFFLINE);
				subtract++;

				intComma++;
				intPos=intComma;

			}

		}		

	}
	
}

void CPanelDlg::OnAddressBkUnBlock() 
{
	// TODO: Add your command handler code here
	CString ms;
	CString ms2;
	CString SelectItem;
	char t[10];
	int i=-1;
	int j=-1;
	int count=0; // the number of selected items
	do
	{
		j=m_ListPersonal.GetNextItem(i,LVNI_SELECTED);
		i=j;		
		itoa(j,t,10);
		SelectItem=SelectItem+t+",";
		count++;
	}while (j>-1);
	count--;




	if (count <1)
		MessageBox(_T("Plese indicate which items be deleted"),_T("Attention"),MB_OK|MB_ICONWARNING);
	else
	{
		CString csCond,csCLSn;
		char csSn[10];
		char charNum[12];
		int intNum;
		csCond="sn=";
		int intPos=0;
		int intComma=0;
		//int intItemData;
		for (int s=0;s<count;s++)
		{
			intComma=SelectItem.Find(',',intComma);
			lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));
	
			
			intNum=atoi(charNum);				
			itoa(m_ListPersonal.GetItemData(intNum),csSn,10);
				
			csCond=csCond+csSn+" or sn=";
			ms=ms+m_ListPersonal.GetItemText(intNum,defPersonalName)+",";
			csCLSn=csCLSn+csSn+",";  // prepare for ListPersonal deleteItem


			intComma++;
			intPos=intComma;			
		}

		csCond=csCond.Left(csCond.GetLength()-6);
		ms=ms.Left(ms.GetLength()-1);



	
		ms2.Format( "Are you sure to Unblock this record '%s'?   ",ms);

		if (MessageBox(_T(ms2),_T("Block"),MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
		{
			intPos=0;
			intComma=0;
			int subtract=0;
			// delet items from address book list
			CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
			for(s=0;s<count;s++)
			{
				intComma=SelectItem.Find(',',intComma);
				lstrcpy(charNum,SelectItem.Mid(intPos,intComma-intPos));			
				intNum=atoi(charNum);			
					
				// Arlene added : Unsubscribe the select one
				CString strNumber = m_ListPersonal.GetItemText(intNum-subtract, defPersonalTelephone);
#ifdef _XCAPSERVER
				SetRuleAction2Allow((LPSTR)(LPCTSTR)strNumber);
#endif
				g_pMainDlg->UnBlockBuddy(strNumber);
				g_pMainDlg->Subscribe(strNumber);
				subtract++;
//				m_ListPersonal.SetItemText(intNum-subtract, defPersonalPresence, "Block");


				intComma++;
				intPos=intComma;

			}

		}		

	}	
}

void CPanelDlg::OnDeleteGroup() 
{
	// TODO: Add your command handler code here

	GroupDeleteDlg dlg;

	int result;
	result=dlg.DoModal();
	

	if (result==IDOK)
	{
#ifdef _XCAPSERVER	
		BuddyList *delGroupMem = GetGroupMembers((LPSTR)(LPCTSTR)dlg.m_DeleteName);
		BuddyList *currentPtr;
		
		if(!DelGroup((LPSTR)(LPCTSTR)dlg.m_DeleteName))//create failed...
		{
			AfxMessageBox("Delete Group Failed");
			FreeBuddyList(delGroupMem);
			return;
		}
		if(delGroupMem!=NULL)
		{
			int nCount =m_ListPersonal.GetItemCount();
			for(int i=0;i<nCount;i++)
			{
				currentPtr = delGroupMem;
				while(currentPtr!=NULL)
				{
					if(m_ListPersonal.GetItemText(i,defPersonalName) == currentPtr->buddy->m_displayname)
						m_ListPersonal.SetItemText(i,defPersonalGroup,DEFAULT_GROUPNAME);
					currentPtr = currentPtr->next;
				}
			}
			FreeBuddyList(delGroupMem);
		}
		
#endif
	}
		
}

void CPanelDlg::OnEditGroup() 
{
	// TODO: Add your command handler code here

	GroupEditDlg dlg;

	int result;
	result=dlg.DoModal();
	

	if (result==IDOK)
	{
#ifdef _XCAPSERVER	
		BuddyList *selGroupMem = GetGroupMembers((LPSTR)(LPCTSTR)dlg.m_selGroupName);
		BuddyList *currentPtr;
		
		if(!ModifyGroupName((LPSTR)(LPCTSTR)dlg.m_selGroupName,(LPSTR)(LPCTSTR)dlg.m_newGroupName))//create failed...
		{
			AfxMessageBox("Change Group Name Failed");
			FreeBuddyList(selGroupMem);
			return;
		}
		if(selGroupMem!=NULL)
		{
			int nCount =m_ListPersonal.GetItemCount();
			for(int i=0;i<nCount;i++)
			{
				currentPtr = selGroupMem;
				while(currentPtr!=NULL)
				{
					if(m_ListPersonal.GetItemText(i,defPersonalName) == currentPtr->buddy->m_displayname)
						m_ListPersonal.SetItemText(i,defPersonalGroup,dlg.m_newGroupName);
					currentPtr = currentPtr->next;
				}
			}
			FreeBuddyList(selGroupMem);
		}
		
#endif
	}
	
}


// added by shen, 20041225
// modified by molisado, 20050406
// change return type from bool to int
// change return value form ture/false to Presence_Online/Presence_Offline/Presence_Unknown
int CPanelDlg::GetBuddyPresence(CString strNumber)
{
#ifdef _SIMPLE

	int nCount = m_ListPersonal.GetItemCount();
	for (int i=0 ; i<nCount ; i++)
	{
		int nNum = m_ListPersonal.GetItemData(i);
		CString strThisNum = m_ListPersonal.GetItemText(i,defPersonalTelephone);
		if (strThisNum == strNumber)
		{
			// found, retrieve the presence state
			CString strPres = m_ListPersonal.GetItemText(i, defPersonalPresence);
			if (strPres == PRE_ONLINE)
				return Presence_Online;
			return Presence_Offline;
		}
	}
#endif

	return Presence_Unknown;

}


void CPanelDlg::OnAddressBkGroupMsg() 
{
	// TODO: Add your command handler code here

	CGroupMsgDlg dlg;
	CArray<CString, CString&> vecTarget;
	BuddyList *myGroupmember,*tmpPtr;
	CString strURI;
	CString strTel;

	int result;
	result=dlg.DoModal();
	

	if (result==IDOK)
	{
#ifdef _XCAPSERVER	


		myGroupmember = GetGroupMembers((LPSTR)(LPCTSTR)dlg.m_SelectedGroupName);
		tmpPtr = myGroupmember;
		while (tmpPtr!=NULL)
		{
			strURI = tmpPtr->buddy->m_telephone;
			strTel = strURI;
			if ( strURI.Left(4) != "sip:")
				strURI = g_pMainDlg->ConvertNumberToURI(strTel);
			if (g_pMainDlg->GetBuddyPresence(strTel))	// need to be on-line
				vecTarget.Add(strURI);
			tmpPtr = tmpPtr->next;
		}
		if(myGroupmember!=NULL)
			FreeBuddyList(myGroupmember);

		if (vecTarget.GetSize() == 1)
		{
			CDlgMessage::GetDlgByMessage( strURI, NULL, strTel );
		}
		else if(vecTarget.GetSize() == 0)
		{
			AfxMessageBox("No online group members!");
		}
		else if (vecTarget.GetSize() > 1)
		{
			// create conference
			CDlgMessage* dlgMsg = new CDlgMessage( g_pMainDlg, NULL, NULL);
			dlgMsg->CreateIMConference(&vecTarget, true);	// new created
		}
		

#endif
	}
	
}
