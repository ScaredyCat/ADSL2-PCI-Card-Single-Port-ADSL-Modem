#include "UACommon.h"
#include <low/cx_sock.h>
#include <tchar.h>

#include "UAControl.h"

CRITICAL_SECTION debug_cs;
HWND debugWin;
FILE *debugFile = NULL;

void InitDebug(HWND hWin)
{
	/*
	InitializeCriticalSection(&debug_cs);
	debugWin = hWin;
	*/
//#ifdef _DEBUG
	//debugFile = fopen("UACom_evt.log","wt");
//#endif
}

void ExitDebug()
{
	/*
	DeleteCriticalSection(&debug_cs);
	*/
//#ifdef _DEBUG
	//fclose(debugFile);
	//debugFile = NULL;
//#endif
}

void DebugMsg2UI(const char* buf)
{

#ifdef DEBUG_DEBUG
	CUAControl* pUAControl = CUAControl::GetControl();

	CString Msg(buf);
	Msg += "\r\n";
	//Msg = "[LEVEL_UI]" + Msg;

	if (pUAControl)
		pUAControl->Fire_ShowText( CComBSTR(Msg) );
#endif

}

void DebugMsg(const char* buf)
{
	
	CUAControl* pUAControl = CUAControl::GetControl();

	CString Msg(buf);
	Msg += "\r\n";
	//Msg = "[LEVEL_COM]" + Msg;

	if (pUAControl)
		pUAControl->Fire_ShowText( CComBSTR(Msg) );

}

DWORD AddrCStrToDW(CString CStrAddr)
{
	DWORD _this = 0;
	int i = 0;
	char tmpStr[32];
	MyCString temp(CStrAddr);

	strcpy(tmpStr, temp);

	char* token = strtok( tmpStr, "." );
	while( token != NULL )
	{
		_this |= LOBYTE(atoi(token)) << (8 * (3 - i++));
		token = strtok( NULL, "." );
	}
	return _this;
}

CString AddrDWToCStr(DWORD DWAddr)
{
	CString _this;

	_this.Format(_T("%d.%d.%d.%d"), 
		(DWAddr & 0xFF000000) >> 24, 
		(DWAddr & 0x00FF0000) >> 16,
		(DWAddr & 0x0000FF00) >> 8,
		DWAddr & 0x000000FF );

	return _this;
}

MyCString GetIP()
{
	MyCString strIp;

	//Init winsock
	WSADATA wsaData;
	int nErrorCode = WSAStartup(MAKEWORD(2,1), &wsaData);
	if (nErrorCode != 0) {
		//Cannot initialize winsock
		return _T("");
	}

	char strHostName[81];
	if (gethostname(strHostName, 80)==0)
	{
		hostent *pHost = gethostbyname(strHostName);
		if (pHost->h_addrtype == AF_INET)
		{
			in_addr **ppip=(in_addr**)pHost->h_addr_list; 
			
			//Enumarate all addresses
			while (*ppip) 
			{
				in_addr ip=**ppip;
				strIp = CString(inet_ntoa(ip));
				ppip++; 
				if (strIp!=_T("")) {
					break;
				}
			}
		}
	}

	return strIp;
}

const char *getMyAddr()
{
 static char buf[64];

 struct hostent *hp;
 char hostname[128];
 struct in_addr addr;

 buf[0] = '\0';

 if (::gethostname(hostname, sizeof(hostname)) < 0)
  return buf;

 if ((hp = ::gethostbyname(hostname)) == NULL)
  return buf;

 memcpy(&addr.s_addr, hp->h_addr, sizeof(addr.s_addr));

 _snprintf(buf, sizeof(buf), "%s", ::inet_ntoa(addr));
 buf[sizeof(buf) - 1] = '\0';
 return buf;
}


const char **getAllMyAddr()
{
 int i;

 static char  bufs[16][32];
 static const char *pbufs[16];
 struct hostent *hp;
 char hostname[256];
 struct in_addr addr;

 for (i = 0; i < 16; i++) {
  bufs[i][0] = '\0';
  pbufs[i] = bufs[i];
 }

 if (::gethostname(hostname, sizeof(hostname)) < 0)
  return pbufs;

 if ((hp = ::gethostbyname(hostname)) == NULL)
  return pbufs;

 for (i = 0; i < 16 && hp->h_addr_list[i]; i++) {
  memcpy(&addr.s_addr, hp->h_addr_list[i], sizeof(addr.s_addr));
  _snprintf(bufs[i], 32, "%s", ::inet_ntoa(addr));
  bufs[i][31] = '\0';
 }

 return pbufs;
}


MyCString::~MyCString() {
	if (buf != NULL) free(buf);
	buf = NULL;
}

MyCString::operator LPCTSTR() {
	return (CString)*this;
}

#ifdef _WIN32_WCE
MyCString::operator LPCSTR() {
	if (buf == NULL)
		buf = (char*)malloc(_tcslen((CString)*this) * sizeof(TCHAR));
	WideCharToMultiByte(CP_ACP,0,(CString)*this,-1,buf,_tcslen((CString)*this) * sizeof(TCHAR),NULL,NULL);
	return buf;
}

char* strDupFromTCHAR(const TCHAR* wtemp) {
	char* temp = NULL;
	temp = (char*)malloc(_tcslen(wtemp) * sizeof(TCHAR));
	WideCharToMultiByte(CP_ACP,0,wtemp,-1,temp,_tcslen(wtemp) * sizeof(TCHAR),NULL,NULL);
	return temp;
}

#endif /* WinCE */

CListItem::CListItem()
{
	m_callid.Empty();
	m_uri.Empty();
	m_status.Empty();
}

CListItem::CListItem(CString callid, CString uri, CString status)
{
	m_callid = callid;
	m_uri = uri;
	m_status = status;
}

CListItem::~CListItem()
{

}

MyCString UACallStateToCStr(UACallStateType stateType)
{	
	switch (stateType) {
	case 	UACSTATE_IDLE:
		return "idle";
	
	case	UACSTATE_DIALING:
		return "dialing";
		
	case	UACSTATE_PROCEEDING:
		return "proceeding";
	
	case	UACSTATE_OFFERING:
		return "incoming call";
	
	case	UACSTATE_RINGBACK:
		return "alerting";
	
	case	UACSTATE_BUSY:
		return "busy";
	
	case	UACSTATE_REJECT:
		return "rejceted";
				
	case	UACSTATE_CANCEL:
		return "canceled";
				
	case	UACSTATE_ACCEPT:
		return "accept";
				
	case	UACSTATE_CONNECTED:
		return "connected";
	
	case	UACSTATE_DISCONNECT:
		return "disconnected";

	case	UACSTATE_REGISTER:
		return "registering";

	case	UACSTATE_REGISTER_AUTHN:
		return "Need authentication";

	case	UACSTATE_REGISTERED:
		return "Registration done";

	case	UACSTATE_REGISTERFAIL:
		return "Registration fail";

	case	UACSTATE_ONHOLDING:
		return "holding";
	
	case	UACSTATE_ONHELD:
		return "held";
	
	case	UACSTATE_ONHOLDPENDCONF:
		return "Onholdpendconf";
	
	case	UACSTATE_ONHOLDPENDTRANSFER:
		return "Onholdpendtransfer";

	case	UACSTATE_CONFERENCE:
		return "Conference";

	case	UACSTATE_SPECIALINFO:
		return "special info";

	case	UACSTATE_TIMEOUT:
		return "Timeout";

	case	UACSTATE_TRANSPORTERR:
		return "transport error";

	default:
		return "Unknown";
	}
}

BOOL IsPortAvailable(unsigned short port)
{
	CxSock		sock;
	CxSockAddr	lsockaddr;

	cxSockInit();

	lsockaddr = cxSockAddrNew(NULL, port);
	if (!lsockaddr) {
		cxSockClean();
		return FALSE;
	}
	sock = cxSockNew(CX_SOCK_DGRAM, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		cxSockClean();
		return FALSE;
	}
	cxSockFree(sock);
	cxSockAddrFree(lsockaddr);
	cxSockClean();

	return TRUE;
}

