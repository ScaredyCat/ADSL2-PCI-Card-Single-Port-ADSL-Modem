#if !defined(CALLMANAGER_H)
#define CALLMANAGER_H

#define MAX_DLG_COUNT 10

int nRegCount;
struct UICmdMsgObj {
	unsigned int CmdID;
	int dlgHandle1;
	int dlgHandle2;
	char dialURL[256];
	UaContent content;
};

UaUser			m_UaUser;
UaCfg			m_UaCfg;
UaMgr			m_UaMgr;
UaMgr			m_UaMgr2;

UaDlg GetDlgFromHandle(int dlgHandle);
int GetHandleFromDlg(UaDlg dlg);
int InsertDlgMAP(UaDlg dlg);
BOOL  DeleteDlgMAP(UaDlg dlg);
const char *getMyAddr(void);
void ProcessEvt(UaMsg uamsg);
UaDlg			dlgMAP[MAX_DLG_COUNT];
typedef struct UICmdMsgObj* UICmdMsg;
UaDlg			m_ActiveDlg;
BOOL 			m_bRTPConnected;
BOOL   m_bEarlyMedia;
BOOL   m_bSingleCall;
unsigned short	m_DlgCount;

BOOL		m_bInitialized;

	//Call Server Settings
BOOL			m_bUseCallServer;
char *m_szCallServerAddr;
unsigned short	m_usCallServerPort;
	
	//Outbound Settings
BOOL			m_bUseOutbound;
char *m_szOutboundAddr;
unsigned short	m_usOutboundPort;
	
	//Http Tunnel Settings
BOOL			m_bUseHttpTunnel;
char *m_szHttpTunnelAddr;
unsigned short	m_usHttpTunnelPort;

	//Registrar Settings
BOOL			m_bUseRegistrar;
char *m_szRegistrarAddr;
unsigned short	m_usRegistrarPort;
unsigned long	m_ulExpireTime;
BOOL			m_bExplicitPort;

	//Registrar2 Settings
BOOL		m_bUseRegistrar2;
char *m_szRegistrar2Addr;
unsigned short	m_usRegistrar2Port;
unsigned long	m_ulExpireTime2;
BOOL			m_bExplicitPort2;

	//SIMPLE Settings
BOOL		m_bUseSIMPLEServer;
char *m_szSIMPLEServerAddr;
unsigned short	m_usSIMPLEServerPort;
	
	//User Info
char *m_DisplayName;
char *m_Username;
char *m_ContactAddr;
char *m_TLSContactAddr;
char *m_PublicAddr;
char *m_Realm;
char *m_Password;
char *m_STUNserver;
BOOL		m_bUseSTUN;
unsigned short	m_Qvalue;

	//Options
	//char *	m_LocalHostName;
char *m_LocalAddr;
char *m_ExtAddr;
char *m_Hostname;
unsigned short	m_LocalPort;
unsigned short	m_ExtPort;
char *m_Transport;
BOOL			m_bTransport; // 1:TCP  0:UDP
SipConfigData m_ConfigStr;
BOOL			m_bDoing;
BOOL			m_bClient;
unsigned short	m_ElapsedSecs;
unsigned short	m_ElapsedMins;
unsigned short  m_ElapsedHours;

char *m_DialURL;
char *m_LastDialURL;
	
RCODE MakeCall(char *DialURL, int dlgHandle);
RCODE CancelCall();
RCODE AcceptCall(int dlgHandle);
RCODE RejectCall(int dlgHandle);
RCODE DisconnectCall(int dlgHandle);
RCODE Hold(); 
RCODE UnHold(int dlgHandle);
RCODE AttendTransfer(int dlgHandle1, int dlgHandle2);
	//RCODE UnAttendTransfer(int dlgHandle, MyCString XferURL);
	
RCODE Register();
RCODE UnRegister();
	//RCODE Query();
RCODE DisconnectAll();
void CallManager_init(void);
void ProcessCmd(UICmdMsg uimsg);
void AddCommand(int CommandID, int dlgHandle1, int dlgHandle2, const char* dialURL, UaContent content);	
#endif