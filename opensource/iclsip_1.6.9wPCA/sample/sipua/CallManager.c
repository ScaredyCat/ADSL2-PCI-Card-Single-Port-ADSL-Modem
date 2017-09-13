/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * coretest.c
 *
 * $Id: coretest.c,v 1.42 2007/01/12 02:34:15 tyhuang Exp $
 */

#include <sys_drv_ifxos.h>
#include <stdio.h>
#include <time.h>

#include <UACore/ua_core.h>
#include <CallManager.h>
#include <MediaManager.h>
#include <SDPManager.h>
#include <UACommon.h>
#include <common.h>
#include <tapidemo.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <low/cx_event.h>
#include <low/cx_mutex.h>
#include <low/cx_thrd.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "ifx_common.h"
#include "../../../ifx/ifx_httpd/ifx_amazon_cfg.h"
#define _snprintf snprintf
#endif


#define MAX_FORWARD 70
#define MAXCALLER	1	// if MAXCALLER=1, then caller=callee=1
#define TOTALCALL	1
#define EVENTDISP	100
#define DEFAULTIP	"127.0.0.1"
char* gCalleeGName[MAX_DATA_LEN];
char* gCallerGName[MAX_DATA_LEN];

char gDialURL[MAXCALLER][30];
char gCallerID[MAXCALLER][10];
char gCalleeID[MAXCALLER][10];
BOOL gDoing=TRUE;
	
SdpSess gCallerSDP[MAXCALLER],gCalleeSDP[MAXCALLER];
UaMgr gCallerMgr[MAXCALLER],gCalleeMgr[MAXCALLER];
UaUser gCallerUser[MAXCALLER],gCalleeUser[MAXCALLER];
UaCfg gCallerCfg[MAXCALLER],gCalleeCfg[MAXCALLER];
UaAuthz gCallerAuthz[MAXCALLER],gCalleeAuthz[MAXCALLER];
UaDlg CallerDlg[MAXCALLER];
UaDlg lastConnectedDlg = NULL;
time_t ticks_s,ticks_e;
RCODE rCode=RC_OK;

DxLst m_EventMsgLst=NULL;
DxLst m_UICmdMsgLst=NULL;
CxThread m_hCMThread;
CxThread m_hUAThread;

const char *getMyAddr()
{
	static char buf[64];
		int i=1;
    char  sTAG_NAME[MAX_DATA_LEN];
    char  sValue[MAX_DATA_LEN];

	buf[0] = '\0';
	
	buf[sizeof(buf) - 1] = '\0';
		for (i=1;i<=MAX_VCCs;i++)
		{
				sprintf(sTAG_NAME,"Wan%d_IF_Info",i);
    		memset(sValue, 0x00, sizeof(sValue));
    		if (ifx_GetCfgData(FILE_SYSTEM_STATUS, sTAG_NAME, "STATUS", sValue) == 1)
    		{
    			if(!strcmp(sValue, "CONNECTED"))
    			{
    				memset(sValue, 0x00, sizeof(sValue));	
						if (ifx_GetCfgData(FILE_SYSTEM_STATUS, sTAG_NAME, "IP", sValue) == 1)		
						{
							_snprintf(buf, sizeof(buf), "%s", sValue);
    					break;
    				}	
    			}
    		}	
		}
	buf[sizeof(buf) - 1] = '\0';
	
	return buf;
}
/* Hold active dialog - revised - */
RCODE Hold()
{
	RCODE retcode;
	
	int dlgHandle = GetHandleFromDlg(m_ActiveDlg);

	if ( dlgHandle == -1 ) {
		printf("Error: Can't hold call! Dialog handle doesn't exist\n");
		return RC_ERROR;
	}

	printf("[Dialog #%d] hold call\n", dlgHandle );
	retcode = uaDlgHold(m_ActiveDlg);
	if (retcode != RC_OK) {
		printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
	}
	return retcode;
}

/* UnHold dialog of dlgHandle - revised - */
RCODE UnHold(int dlgHandle)
{
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	if (m_ActiveDlg) {
		printf("Error: Can't unhold call! Please hold active call first\n");
		return RC_ERROR;
	}
	if (dlg) {
		printf("[Dialog #%d] unhold call\n", dlgHandle );
		SdpSess LocalSDP = GetLocalSDPbyDlg(dlg);
		retcode = uaDlgUnHold(dlg, LocalSDP);
		uaSDPFree(LocalSDP);
	
		if (retcode == RC_OK) {
			m_ActiveDlg = dlg;
		} else {
			printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
		}
	} else {
		printf("Error: Can't unhold call! Dialog not found\n");
		retcode = RC_ERROR;
	}
	return retcode;
}

/* Accept dialog of dlgHandle - revised - */
RCODE AcceptCall(int dlgHandle)
{
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);
	UACallStateType dlgState = uaDlgGetState(dlg);

	if ( m_ActiveDlg && ( m_ActiveDlg != dlg ) ) {
		printf("Error: can't answer call! Please hold active call first\n") ;
		return RC_ERROR;
	}
	if (dlg) {
		printf("[Dialog #%d] user answer call\n", dlgHandle );

		SdpSess LocalSDP = GetLocalSDPbyDlg(dlg);
		retcode = uaDlgAnswer(dlg, UA_ANSWER_CALL, sipOK, LocalSDP);
		uaSDPFree(LocalSDP);

		if (retcode == RC_OK) {
			m_ActiveDlg = dlg;
		} else {
			printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
		}
	} else {
		printf("Error: Can't answer call! Dialog not found\n");
		retcode = RC_ERROR;
	}
	return retcode;
}
/* Post call established procedure - revised - */
RCODE CallEstablished(UaDlg dlg, SdpSess sdp, short status)
{
	int AudioPort, dlgHandle, VideoPort = 0;
	CODEC_NUMBER AudioCodecChoice;
	char tmpBuf[128];
	RCODE retcode;

	if( m_bEarlyMedia )
	{
		m_bEarlyMedia = FALSE;
		m_bRTPConnected = FALSE;
	}
	
	if (m_bRTPConnected)
		return RC_ERROR;
	
	memset(tmpBuf, 0, 128);

	//StopPlaySound();
	//Media_RTP_Stop();

	dlgHandle = GetHandleFromDlg(dlg);

	if ( dlgHandle == -1 ) {
		printf("Fatal Error: Can't establish media! Dialog handle doesn't exist\n" );
		return RC_ERROR;
	}

	if ( sdp == NULL ) {
		printf("[Dialog #%d] Error: No remote SDP, fail to start media\n", dlgHandle );
		DisconnectCall( dlgHandle );
		return RC_ERROR;
	}

	/* Get remote audio channel parameter */
	if ((retcode = uaSDPGetMediaAddr(sdp, UA_MEDIA_AUDIO, tmpBuf, 128)) != RC_OK) {
		/* Get remote SDP IP address */
		if ((retcode = uaSDPGetDestAddress(sdp, tmpBuf, 128)) != RC_OK) {
			printf("[Dialog #%d] Error: Can't get remote SDP address, fail to start media\n", dlgHandle );
			DisconnectCall( dlgHandle );
			return retcode;
		}
	}
	AudioPort = uaSDPGetMediaPort(sdp, UA_MEDIA_AUDIO);
	AudioCodecChoice = GetAudioCodecChoiceNum(sdp);

	if ( AudioCodecChoice == CODEC_NONE ) {
		printf("[Dialog #%d] Error: No match media, fail to start media\n", dlgHandle );
		DisconnectCall( dlgHandle );
		return RC_ERROR;
	}

	printf("[Dialog #%d] start media\n", dlgHandle );
	
	/* Http Tunnel Server channel, by sjhuang 20060120 */
	// ToDo:
	/*
	if( pProfile->m_bUseHttpTunnel ) {
		SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
		int LocalAudioPort = uaSDPGetMediaPort(LocalSDP, UA_MEDIA_AUDIO);
		CString strLocalPort,strRemotePort;
		strLocalPort.Format("%d",LocalAudioPort);
		strRemotePort.Format("%d",AudioPort);
		reqHTTPTunnel(TRUE,strLocalPort,CString(tmpBuf),strRemotePort);
	}
	*/
	/* Connect remote audio channel */
	//SetMediaParameter(AudioCodecChoice);
	/* Added by sjhuang 20060120 */
	/*
	if( pProfile->m_bUseHttpTunnel ) { 
		CString strTunnelIP;
		int iTunnelPort;
		strTunnelIP = AfxGetApp()->GetProfileString( "Http_Tunnel", "HttpTunnel_Addr");//GetUAComRegString( "Http_Tunnel","HttpTunnel_Addr");
		iTunnelPort = AfxGetApp()->GetProfileInt("Http_Tunnel", "HttpTunnel_Port",8080);;//GetUAComRegDW( "Http_Tunnel","HttpTunnel_Port",8080);
		pMediaManager->RTPPeerConnect(strTunnelIP,iTunnelPort+1);
	} else
	*/
	 {
		//RTPPeerConnect(tmpBuf,AudioPort);
	}

	
	/* Used to calculate call elapsed time */
	m_ElapsedHours = 0;
	m_ElapsedMins = 0;
	m_ElapsedSecs = 0;


	m_bRTPConnected = TRUE;


	return RC_OK;
}
RCODE ValidateURL(char *strURL)
{
	SipURL url;
	char temp[80];

	if(strURL == NULL){
		return RC_ERROR;
	}
	strcpy(temp, strURL);

	url =  sipURLNewFromTxt( temp );

	if ( url == NULL ){
		return RC_ERROR;
	}
	sipURLFree(url);
	return RC_OK;
}
/* Make a new call to DialURL - revised - */
RCODE MakeCall(char *DialURL, int dlgHandle)
{
	if (m_DlgCount == MAX_DLG_COUNT) {
		dlgHandle = -1;
		return RC_ERROR;
	}

	if (m_bSingleCall && (m_DlgCount == 1) ) {
		dlgHandle = -1;
		return RC_ERROR;
	}
		
	if (RC_ERROR == ValidateURL(DialURL)) {
		dlgHandle = -1;
		return RC_ERROR;
	}

	if (m_ActiveDlg) {
		printf("Error: Can't make another call! Please hold active call first\n");
		dlgHandle = -1;
		return RC_ERROR;
	}

	UaDlg dlg = uaDlgNew(m_UaMgr);
	dlgHandle = InsertDlgMAP(dlg);
	m_ActiveDlg = dlg;
	m_DialURL = DialURL;

	SdpSess LocalSDP = GetLocalSDPbyDlg(dlg);
#if 0	
	if (m_bAnonFrom)
		uaDlgSetUserPrivacy(dlg, m_bAnonFrom);

	if (m_bUsePreferredID)
		uaDlgSetPPreferredID(dlg, m_strPreferredID);

	if (m_bUseSessionTimer)
		uaDlgSetSETimer(dlg, m_uSessionTimer);
	else
#endif	
		uaDlgSetSETimer(dlg, 0);

	RCODE retcode = uaDlgDial(dlg,DialURL,LocalSDP);
	uaSDPFree(LocalSDP);

	if (retcode != RC_OK) {
		printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
	}
	return retcode;
}
/* Post call disconnected procedure - revised - */
RCODE CallDisconnected(UaDlg dlg, UACallStateType dlgState)
{
	RCODE retcode;
	char tmpBuf[128];

	int dlgHandle = GetHandleFromDlg(dlg);

	if (!dlg || (dlgHandle == -1))
		return RC_ERROR;

	if ( lastConnectedDlg == dlg )
		lastConnectedDlg = NULL;

	/* If is Active dialog, deactive it and stop media */
	if (m_ActiveDlg == dlg) {
	
		/* Deactive dialog */
		m_ActiveDlg = NULL;

		//Media_RTP_Stop();
		//StopPlaySound();

		m_bRTPConnected = FALSE;

		/* Http Tunnel Server channel, by sjhuang 20060120 */
		// ToDo:
		/* Get remote audio channel parameter */
		SdpSess peersdp = GetSDPbyDlg(dlg);// LocalSDPbyDlg(dlg);
		if ((retcode = uaSDPGetMediaAddr(peersdp, UA_MEDIA_AUDIO, tmpBuf, 128)) != RC_OK) {
			/* Get remote SDP IP address */
			if ((retcode = uaSDPGetDestAddress(peersdp, tmpBuf, 128)) != RC_OK) {
				printf("[Dialog] Error: Can't get remote SDP address, fail to start media\n");
			}
		}
		int AudioPort = uaSDPGetMediaPort(peersdp, UA_MEDIA_AUDIO);

	}


	/* Delete SDP associated with the dlg */
	DelSDPbyDlg(dlg);

	
	/* Release Dialog object */
	if (dlg != NULL) {
		uaDlgRelease(dlg);
		dlg = NULL;
	}

	// clear early media flag
	m_bEarlyMedia = FALSE;

	return RC_OK;
}
int GetHandleFromDlg(UaDlg dlg)
{
	int i;
	if (!dlg)
		return -1;

	for (i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == dlg) {
			return i;
		}
	}
	return -1;
}
UaDlg GetDlgFromHandle(int dlgHandle)
{
	if ( (dlgHandle >= 0) && (dlgHandle < MAX_DLG_COUNT) ) {
		return dlgMAP[dlgHandle];
	} else {
		return NULL;
	}
}
BOOL DeleteDlgMAP(UaDlg dlg)
{
	int i;
	if (!dlg)
		return FALSE;

	for (i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == dlg) {
			dlgMAP[i] = NULL;
			printf("[DlgMAP] delete dialog handle %d\n", i);
			printf("[DlgMAP] Dialog count = %d\n", --m_DlgCount);
			return TRUE;
		}
	}
	printf("[DlgMAP] dialog handle not found!");
	return FALSE;
}
int InsertDlgMAP(UaDlg dlg)
{
	int i;
	if (!dlg)
		return -1;

	for (i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == NULL) {
			dlgMAP[i] = dlg;
			printf("[DlgMAP] Insert dialog handle %d\n", i);
			printf("[DlgMAP] Dialog count = %d\n", ++m_DlgCount);
			return i;
		}
	}
	printf("[DlgMAP] Can't insert dialog! Dialog CAP reached!\n");

	return -1;
}
RCODE MakeCallbyRefer(UaDlg ReferDlg, UaMgr uamgr)
{
	char dest[128] = {'\0'};
	int destlen = 128;
	
	uaDlgGetReferTo(ReferDlg,dest,&destlen);
	printf("[Dialog ] transferred to %s", dest);
	
	UaDlg dlg = uaDlgNew(uamgr);
	InsertDlgMAP(dlg);

	/* If is Referdlg is Active dialog, deactive it and stop media */
	if (m_ActiveDlg == ReferDlg) {
		
		//pMediaManager->Media_RTP_Stop();
		//pMediaManager->StopPlaySound();

		m_bRTPConnected = FALSE;
	}

	SdpSess LocalSDP = GetLocalSDPbyDlg(dlg);
	RCODE retcode = uaDlgDialRefer(dlg, dest, ReferDlg, LocalSDP);
	uaSDPFree(LocalSDP);

	m_ActiveDlg = dlg;

	/* modified by ljchuang 08/13/2003 */
	/* CallDisconnected(ReferDlg, uaDlgGetState(ReferDlg) ); */
	/* DisconnectCall(dlgHandle); */


	if (retcode == RC_OK) {
		m_ActiveDlg = dlg;
	} else {
		printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
	}

	return retcode;
}
RCODE Query()
{
	RCODE retcode;
	
	printf("[CallManager] Performing registration query\n");

	if (m_bUseRegistrar) {
		if ( (retcode = uaMgrRegister(m_UaMgr, UA_REGISTER_QUERY, m_ContactAddr)) != RC_OK )
			printf("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
	}
	return retcode;
}
RCODE UnRegister()
{
	RCODE retcode;
	
	printf("[CallManager] Performing un-registration\n");

	if (m_bUseRegistrar) {
		if ( (retcode = uaMgrRegister(m_UaMgr, UA_UNREGISTERALL, NULL)) != RC_OK )
		  printf("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
	}
	return retcode;
}
RCODE Register(void)
{
	
	RCODE retcode;

	printf("[CallManager] Performing registration\n");
	if ( (retcode = uaMgrRegister(m_UaMgr, UA_REGISTER, m_ContactAddr)) != RC_OK )
		printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
	return retcode;
}
/* Cancel active dialing dialog - revised - */
RCODE CancelCall()
{
	int dlgHandle = GetHandleFromDlg(m_ActiveDlg);

	if ( dlgHandle == -1 ) {
		printf("Error: Can't cancel call! Dialog handle doesn't exist\n");
		return RC_ERROR;
	}

	return DisconnectCall( dlgHandle );

}

/* Disconnect dialog of dlgHandle - revised - */
RCODE DisconnectAll()
{
	DxLst dlgLst = uaMgrGetDlgLst(m_UaMgr);
	UaDlg dlg;
	int dlgpos,dlgnum;


	if (dlgLst != NULL) {
		dlgnum = dxLstGetSize(dlgLst);
		for (dlgpos = 0; dlgpos < dlgnum; dlgpos++) {
			dlg = (UaDlg)dxLstPeek(dlgLst,dlgpos);
			if (dlg && !IsDlgReleased(dlg))
				DisconnectCall( GetHandleFromDlg(dlg) );
		}
	}
	return RC_OK;
}
RCODE DisconnectCall(int dlgHandle)
{
	
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	//if( pMediaManager->m_WaveType == WAV_G723 )
	//{
	//	AfxMessageBox("disconnectCall");
	//}

	if (dlg) {
		UACallStateType dlgState = uaDlgGetState(dlg);
		switch (dlgState) {
		case UACSTATE_DIALING:
			retcode = CallDisconnected(dlg, UACSTATE_DIALING);
			break;

		case UACSTATE_PROCEEDING:			
		case UACSTATE_RINGBACK:
			printf("[Dialog #%d] user cancel call\n", dlgHandle );
			retcode = uaDlgCANCEL(dlg);
			break;

		case UACSTATE_OFFERING:
		case UACSTATE_OFFERING_REPLACE:
			printf("[Dialog #%d] user reject call\n", dlgHandle );
			retcode =  uaDlgAnswer(dlg, UA_REJECT_CALL, Busy_Here, NULL);
			break;

		default:
			printf("[Dialog #%d] user disconnect call\n", dlgHandle );
			retcode = uaDlgDisconnect(dlg);

			printf("[Dialog #%d] after user disconnect call\n", dlgHandle );
		}
		if (retcode != RC_OK) {
			printf("Error: %s\n", uaCMRet2Phase((UARetCode)retcode) );
		}
	} else {
		printf("Error: Can't drop call! Dialog not found\n");
		retcode = RC_ERROR;
	}

	printf("[Dialog #%d] leave disconnect call\n", dlgHandle );
	return retcode;
}
static void EvtCB(UAEvtType event, void* msg)
{

		if (!msg)
		return;
	
	dxLstPutTail(m_EventMsgLst, uaMsgDup((UaMsg)msg) );

}

void ProcessEvt(UaMsg msg)
{
	UaMsg uamsg = msg;
	UaDlg uadlg = uaMsgGetDlg(uamsg);
	SdpSess peersdp;
	SdpSess sdp = uaMsgContainSDP(uamsg)? uaMsgGetSdp(uamsg):NULL;
	UaContent content = uaMsgGetContent(uamsg);
	UACallStateType eDlgState = uaMsgGetCallState(uamsg);
	UAEvtType event = uaMsgGetEvtType(msg);
	UaMgr uamgr = uaDlgGetMgr(uadlg);
	UaUser uauser = uaMgrGetUser(uamgr);
	int i,j,nCallNum = 0;
	
	int dlgHandle = GetHandleFromDlg(uadlg);
	
	if ( dlgHandle == -1 && IsDlgReleased(uadlg) )
	{
		printf("[UAEvtCB] Dialog already terminated\n");
	}
	/*printf("uaMsgGetUser: %s\n",uaMsgGetUser(uamsgA));*/

	if(uaMsgGetSupported(msg))
		printf("Get Supported header:%s\n",uaMsgGetSupported(msg));
	
	switch(event){
	case UAMSG_TX_TIMEOUT:
		switch (eDlgState) {
		case UACSTATE_REGISTERTIMEOUT:
			printf("[UACSTATE_REGISTERTIMEOUT]:%s\n",uaMsgGetUser(uamsg));
			j = uaUserGetNumOfContactAddr(uauser);
			if (j)
			{	
				for(i=0; i < j; i++)
				{
					if (rCode|=uaMgrRegister(uamgr,UA_REGISTER,uaUserGetContactAddr(uauser,i)) != RC_OK)
						printf("Register %s Fail %d\n", uaUserGetContactAddr(uauser,i),rCode);
				}		
			}		
			break;

		case UACSTATE_TIMEOUT:
		default:
			printf("[Dialog] Transaction timeout\n");
		
			if (uadlg)
			  CallDisconnected(uadlg, UACSTATE_TIMEOUT);
		}
		
		break;	
	case UAMSG_TX_TRANSPORTERR:
		switch (eDlgState) {
		case UACSTATE_REGISTERERR:
			printf("[Dialog] Register transport error\n");
			break;

		case UACSTATE_TRANSPORTERR:
		default:
			printf("[Dialog] Transport error\n");		
			if (uadlg)
			  CallDisconnected(uadlg, UACSTATE_TRANSPORTERR);
		}
		break;	
	
	case UAMSG_REFER:
		if ( m_ActiveDlg || m_ActiveDlg == uadlg){
			printf("[Dialog ] has been transferred\n");
			MakeCallbyRefer(uadlg,uamgr);
		} else {
			printf("Error: can't be transferred! There is still an active call in progress\n");
		}
		break;	
		
	case UAMSG_INFO:

		printf("[EvtCB] UAMSG_INFO\n");
		break;
	case UAMSG_SIP_INFO_RSP:
		printf("[UAEvtCB] Receive 200 OK INFO response\n");

		break;	
	case UAMSG_SIP_INFO_FAIL:
		printf("[UAEvtCB] Receive Error INFO response\n");
		break;	
	case UAMSG_SIP_INFO:
		printf("[UAEvtCB] Receive INFO request\n");
		break;		
	case UAMSG_CALLSTATE:
		switch(eDlgState) {
			case 	UACSTATE_IDLE:
			printf("[UAEvtCB] idle state\n");
			break;
			case UACSTATE_OFFERING:
				printf("[UACSTATE_OFFERING]:%s\n",uaMsgGetUser(uamsg));
				dlgHandle = InsertDlgMAP(uadlg);

				//rCode |= uaDlgAnswer(uadlg, UA_RINGING, Ringing, NULL);
				rCode |= uaDlgAnswer(uadlg, UA_100rel, Ringing, NULL);
				if ( !m_ActiveDlg )
				{
					m_ActiveDlg = uadlg;

					printf("[Dialog #%d] Alerting...\n", dlgHandle);

				}
				else
				{
					if (m_bSingleCall ) {
						DisconnectCall( dlgHandle );
						break;
					}
				printf("[Dialog #%d] Waiting...\n", dlgHandle);				
				}
				if (sdp) {
					SetSDPbyDlg(uadlg, sdpSessDup(sdp));
				}
				/*if off-hook*/
				rCode |= uaDlgAnswer(uadlg, UA_ANSWER_CALL, sipOK, sdp);
					
				break;
			case	UACSTATE_OFFERING_REPLACE:
			printf("[UAEvtCB] New incoming call\n");
			dlgHandle = InsertDlgMAP(uadlg);

			uaDlgAnswer(uadlg, UA_RINGING, Ringing, NULL);
						
			if ( !m_ActiveDlg || (m_ActiveDlg == uadlg) ) {
				printf("[Dialog #%d] Replace existing dialog\n", dlgHandle);
				AcceptCall( dlgHandle );

			} else {
				if ( m_bSingleCall ) {
					DisconnectCall( dlgHandle );
					break;
				}
			}
				printf("[Dialog #%d] Waiting...\n", dlgHandle);
			if (sdp) {
				SetSDPbyDlg(uadlg, sdpSessDup(sdp));
			}	
			  break;
			case UACSTATE_DIALING:
				printf("[UACSTATE_DIALING]:%s\n",uaMsgGetUser(uamsg));
				break;
			case UACSTATE_PROCEEDING:
				printf("[UACSTATE_PROCEEDING]:%s\n",uaMsgGetUser(uamsg));
				break;
			case UACSTATE_RINGBACK:
				printf("[UACSTATE_RINGBACK]:%s\n",uaMsgGetUser(uamsg));
				if (sdp) {
					SetSDPbyDlg(uadlg, sdpSessDup(sdp));
				}
				peersdp = GetSDPbyDlg(uadlg);
				
				if( peersdp )
				{
					printf("[Dialog #%d] 183 RingBack\n", dlgHandle);
					m_bClient = FALSE;
					CallEstablished(uadlg, peersdp, 183);
				}
				break;
			case UACSTATE_REGISTER:
				printf("[UACSTATE_REGISTER]:%s\n",uaMsgGetUser(uamsg));
				break;
			case UACSTATE_REGISTERED:
				printf("[UACSTATE_REGISTERED]:%s\n",uaMsgGetUser(uamsg));
				nRegCount=1;
				break;
			case UACSTATE_ACCEPT:
				printf("[UACSTATE_ACCEPT]:%s\n",uaMsgGetUser(uamsg));
				break;
			case UACSTATE_CONNECTED:
				printf("[UACSTATE_CONNECTED]:%s\n",uaMsgGetUser(uamsg));		
				break;
			case UACSTATE_DISCONNECT:
				printf("[UACSTATE_DISCONNECT]:%s\n",uaMsgGetUser(uamsg));
				if (uadlg)
				CallDisconnected(uadlg, UACSTATE_DISCONNECT);
							
				break;
			default:
				break;
		}
		break;
	case UAMSG_REPLY:
		printf("[EvtCB] UAMSG_REPLY:%s\n",uaMsgGetUser(uamsg));
		break;
	case UAMSG_TX_TERMINATED:
		printf("[EvtCB] UAMSG_TX_TERMINATED:%s\n",uaMsgGetUser(uamsg));
		break;
	case UAMSG_REGISTER_EXPIRED:
		break;	
	default:
		printf("[EvtCB] event: %d\n",event);
		break;
	}	
}
void AddCommand(int CommandID, 
							  int dlgHandle1, 
							  int dlgHandle2, 
							  const char* dialURL,
							  UaContent content)
{
	if ( ( dialURL != NULL ) && ( strlen(dialURL) > 255 ) )
	{
		printf("Error: dialURL buffer overrun!\n");
		return;
	}

	UICmdMsg msg = (UICmdMsg)malloc(sizeof(struct UICmdMsgObj));
	
	msg->CmdID = CommandID;
	msg->dlgHandle1 = dlgHandle1;
	msg->dlgHandle2 = dlgHandle2;

	if (content)
		msg->content = uaContentDup(content);
	else
		msg->content = NULL;

	if (dialURL)
		strcpy(msg->dialURL, dialURL);
	dxLstPutTail( m_UICmdMsgLst, msg );

	switch (msg->CmdID) {
	default:
		break;
	}
}

void ProcessCmd(UICmdMsg uimsg)
{
	int retval;
	int cnt=0;
	switch (uimsg->CmdID) {
	case UICMD_DIAL:
		printf("Make Call %s\n", uimsg->dialURL);
		MakeCall( uimsg->dialURL, retval );
		break;
	
	case UICMD_CANCEL:
		CancelCall();
		break;

	case UICMD_DROP:
		DisconnectCall( uimsg->dlgHandle1 );
		break;
		
	case UICMD_HOLD:
	  Hold();
		break;
	
	case UICMD_UNHOLD:
	  UnHold( uimsg->dlgHandle1 );
		break;
			
	case UICMD_ANSWER:
	case UICMD_REJECT:
	case UICMD_UXFER:
	case UICMD_AXFER:
		break;

	case UICMD_REG:
		Register();
		break;

	case UICMD_UNREG:
		UnRegister();
		break;

	case UICMD_REGQUERY:
		Query();
		break;
/*
	case UICMD_MESSAGE:
		pCallManager->SendMessage( uimsg->dialURL, uimsg->content );
		break;
*/
	default:
		break;
	}

	if (uimsg) {
		if (uimsg->content)
		{
			uaContentFree(uimsg->content);
			uimsg->content = NULL;
		}
		free(uimsg);
		uimsg = NULL;
	}
}

void CallManagerThreadFunc(void)
{
	while (1)
	{
		if (dxLstGetSize(m_EventMsgLst) > 0)
			ProcessEvt((UaMsg)dxLstGetHead(m_EventMsgLst));

		if (dxLstGetSize(m_UICmdMsgLst) > 0)
			ProcessCmd((UICmdMsg)dxLstGetHead(m_UICmdMsgLst));
	}
	return 0;
}	

void UAThreadFunc(void)
{
	while (1) 
	{
		uaEvtDispatch(500);
		sleep(1);
	}
	return 0;
}	

void PhoneThread(void)
{
	
	while (1) 
	{
		if(abc_pbx_start() == IFX_ERROR)
			break;
	}
	return 0;
}	


/* Stop the CallManager */
void stop()
{
	
	m_bDoing = FALSE;	// flag to stop event process thread
    
	m_bInitialized = FALSE;


	if ( m_hCMThread ) {
		// wait thread terminated, max. 5 seconds
		// WaitForSingleObject( m_hCMThread, 5000);
		cxThreadCancel( m_hCMThread);
		m_hCMThread = NULL;
	}

	if ( m_hUAThread ) {
		cxThreadCancel( m_hUAThread);
		m_hUAThread = NULL;
	}
	
	if (m_EventMsgLst) {
		dxLstFree(m_EventMsgLst, (void (*)(void *))uaMsgFree);
		m_EventMsgLst = NULL;
	}
	
	if (m_UICmdMsgLst) {
		dxLstFree(m_UICmdMsgLst, (void (*)(void *))free);
		m_UICmdMsgLst = NULL;
	}
	
	if ( m_UaMgr ) {
		DisconnectAll();
		UnRegister();
		sleep(1);
		uaMgrFree(m_UaMgr);
		if (m_UaCfg)
		{
			uaCfgFree(m_UaCfg);
			m_UaCfg = NULL;
		}
		if (m_UaUser)
		{
			uaUserFree(m_UaUser);
			m_UaUser = NULL;
		}
		m_UaMgr = NULL;
		m_ActiveDlg = NULL;
	}

	

	/* clear all falg, by sjhuang 2006/03/30 */
	m_ActiveDlg = NULL;
	m_bDoing = FALSE;
	m_bInitialized = FALSE;
	m_bRTPConnected = FALSE;
	m_bClient = FALSE;
	m_DlgCount = 0;
	m_bEarlyMedia = FALSE;
	uaLibClean();
	//TCRSetMsgCB(NULL);
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

/* Initialize the CallManager */
void CallManager_init()
{
	int i,rCode,cseq;
	const char* LOCALIP;
	uint LOCALPORT,PROXYPORT;
	
	char  sValue[MAX_DATA_LEN],PROXYIP[MAX_DATA_LEN];
	
	
	m_LocalAddr = LOCALIP = getMyAddr();
	
	m_EventMsgLst = dxLstNew(DX_LST_POINTER);
	m_UICmdMsgLst = dxLstNew(DX_LST_POINTER);
	
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "LOCAL_PORT",sValue)==1)
		LOCALPORT=atoi(sValue);
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_PROXY_PORT",sValue)==1)
		PROXYPORT=atoi(sValue);
		
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_EXPIRETIME",sValue)==1)
		m_ulExpireTime=atol(sValue);	
		
	ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_PROXY_ADDR",PROXYIP);
	
	ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_DOMAIN",sValue);
	m_Realm = strdup(sValue);
	ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_USERNAME",sValue);
	m_Username = strdup(sValue);
	ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_PASSWORD",sValue);
	m_Password = strdup(sValue);
	ifx_GetCfgData(FILE_RC_CONF, TAG_HOSTNAME, "hostname",sValue); 
	m_Hostname = strdup(sValue);
	m_ExtAddr = LOCALIP;
	m_ExtPort= LOCALPORT;
	m_ConfigStr.tcp_port = LOCALPORT;
	m_ConfigStr.log_flag = LOG_FLAG_CONSOLE;
	m_ConfigStr.trace_level = TRACE_LEVEL_ALL;
	strcpy(m_ConfigStr.laddr,LOCALIP);
	m_LocalPort = LOCALPORT;

	while ( !IsPortAvailable(m_LocalPort) ) {
		m_LocalPort += 2;
		printf("Port occupied, change to %d\n", m_LocalPort );
	}
	
	
#ifdef USESTUN
	if ( pProfile->m_bUseSTUN && !pProfile->m_STUNserver.IsEmpty() )
		rCode=uaLibInitWithStun(UAEvtCB,m_ConfigStr,pProfile->m_STUNserver);
	else
		rCode=uaLibInitWithStun(UAEvtCB,m_ConfigStr,NULL);
#else
	rCode=uaLibInit(EvtCB,m_ConfigStr);
#endif

	
	if( rCode==RC_ERROR )
	{
		
		stop();
		return;
	}

	
	
	/* Setting UaUser */
	m_UaUser=uaUserNew();
	rCode=uaUserSetName(m_UaUser,m_Username);  
	rCode=uaUserSetLocalAddr(m_UaUser,m_ExtAddr);
	rCode=uaUserSetLocalPort(m_UaUser,m_ExtPort); 
	rCode=uaUserSetLocalHost(m_UaUser,m_Hostname);
	sprintf(sValue,"sip:%s@%s:%d",m_Username,m_ExtAddr,m_ExtPort);
	m_ContactAddr = strdup(sValue);
	printf("m_ContactAddr=%s\n",m_ContactAddr);
	rCode=uaUserSetContactAddr(m_UaUser,m_ContactAddr);
	//rCode=uaUserSetContactAddr(m_UaUser,pProfile->m_TLSContactAddr);
	rCode=uaUserSetMaxForwards(m_UaUser,MAX_FORWARD);
	rCode=uaUserSetExpires(m_UaUser,m_ulExpireTime);

	/* Setting Credentials */
	UaAuthz	tmpUaAuthz = NULL;
	tmpUaAuthz=uaAuthzNew(m_Realm,m_Username,m_Password);
		rCode|=uaUserAddAuthz(m_UaUser,tmpUaAuthz);
	/* Setting UaCfg */
	m_UaCfg=uaCfgNew();
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_OUTBOUND",sValue)==1)
	{
		if(!strcmp(sValue,"ENABLE"))
		{
			ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_OUTBOUND_ADDR",sValue);
			m_szOutboundAddr = strdup(sValue);
			if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_OUTBOUND_PORT",sValue)==1)	
				m_usOutboundPort = atoi(sValue);
			m_bUseOutbound = 1;	
			rCode=uaCfgSetProxy(m_UaCfg,m_bUseOutbound);
			rCode=uaCfgSetProxyAddr(m_UaCfg,m_szOutboundAddr);
			rCode=uaCfgSetProxyPort(m_UaCfg,m_usOutboundPort);
		}		
	}
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_PROXY_TXP",sValue)==1)
	{
		if(!strcmp(sValue,"TCP"))	
			rCode|=uaCfgSetProxyTXP(m_UaCfg,TCP);
		else
			rCode|=uaCfgSetProxyTXP(m_UaCfg,UDP);
	}
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_REGISTRAR",sValue)==1)
	{
		if(!strcmp(sValue,"ENABLE"))
		{
			ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_REGISTRAR_ADDR",sValue);
			m_szRegistrarAddr = strdup(sValue);
			if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_REGISTRAR_PORT",sValue)==1)	
				m_usRegistrarPort = atoi(sValue);
			m_bUseRegistrar = 1;	
			rCode=uaCfgSetRegistrar(m_UaCfg,m_bUseRegistrar);
			rCode=uaCfgSetRegistrarAddr(m_UaCfg,m_szRegistrarAddr);
			rCode=uaCfgSetRegistrarPort(m_UaCfg,m_usRegistrarPort);
		}		
	}
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_REGISTRAR_TXP",sValue)==1)
	{
		if(!strcmp(sValue,"TCP"))	
			rCode=uaCfgSetRegistrarTXP(m_UaCfg,TCP);
		else
			rCode=uaCfgSetRegistrarTXP(m_UaCfg,UDP);
	}
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_SIP, "SIP_LISTEN_PORT",sValue)==1)
		m_LocalPort=atoi(sValue);
	rCode=uaCfgSetListenPort(m_UaCfg,m_LocalPort);

	/* Setting UaMgr */
	m_UaMgr=uaMgrNew();
	uaMgrSetUser(m_UaMgr,m_UaUser);
	uaMgrSetCfg(m_UaMgr,m_UaCfg);
	cseq=uaMgrGetCmdSeq(m_UaMgr);
	for (i = 0; i < MAX_DLG_COUNT; i++)
		dlgMAP[i] = NULL;

	m_DlgCount = 0; 
	
	/* Start Call Manager thread */
	m_bDoing = TRUE;
	ticks_s=time(NULL);
	m_hCMThread = cxThreadCreate(CallManagerThreadFunc, NULL);
	m_hUAThread = cxThreadCreate(UAThreadFunc, NULL);
	
	m_bSingleCall = FALSE;
	m_bInitialized = TRUE;
	
}

RCODE main(int argc, char **argv)
{
	
	char* pLogFileName = "/tmp/calllog.txt";
	int i=0,c;	
	FILE *fp;
	char  sValue[MAX_DATA_LEN];
	
	pthread_t PT; 
	
	nRegCount=0;
	m_OwnerDlg = NULL;
	m_RtpChan = 0;
	m_WaveType = 0;
	m_ToneType = -1;
	m_idx = 0;
	m_IsPlayingRTP = FALSE;
	m_bInitialized = FALSE;
	m_BufRTP2IO = NULL;
	m_BufIO2RTP = NULL;
	m_HashRTP2IO = NULL;
	m_RTPPacketsize = 0;
	m_RTPPacketizePeriod = 0;
	m_CodecPacketizePeriod = 0;
	m_BitsPerSample = 0;
	
  if (pthread_create(&PT,NULL,(void *)PhoneThread,NULL)!=0)
   return RC_ERROR;  
	while(1){}
	fp = fopen(pLogFileName,"w");	
	CMediaManager_init();	
	CallManager_init();	
	m_bInitialized = TRUE;
	AddCommand(UICMD_REG, -1, -1, NULL, NULL);
	if (argc>1)
		while ((c = getopt(argc, argv,"d:u:c")) !=EOF)
			switch (c) {
				case 'd':
					sprintf(gCalleeGName,"%s",argv[2]);
					sprintf(sValue,"sip:%s",gCalleeGName);
					m_DialURL = strdup(sValue); 
					while(nRegCount <=0)
					{
						sleep(2);
					}					
					AddCommand(UICMD_DIAL, -1, -1, m_DialURL, NULL);
					break;
				case 'u':
					while(nRegCount <=0)
					{
						sleep(2);
					}				
					AddCommand(UICMD_UNREG, -1, -1, NULL, NULL);
					break;	
				case 'c':
					while(nRegCount <=0)
					{
						sleep(2);
					}
					AddCommand(UICMD_CANCEL, -1, -1, NULL, NULL);
					break;	
			}
	
	
	rCode =RC_OK;
	while(1);
	stop();
	fclose(fp);
	return rCode;
	
				
}