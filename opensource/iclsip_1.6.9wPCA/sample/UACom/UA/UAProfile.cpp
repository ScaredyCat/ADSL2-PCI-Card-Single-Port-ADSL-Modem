/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* $Id: UAProfile.cpp,v 1.31 2006/11/01 03:07:08 shinjan Exp $ */

// UAProfile.cpp: Configuration page 
//
#include "stdafx.h"
#include "UAProfile.h"
#include "UACommon.h"

#ifdef USESTUN
#include "stunapi.h"
#endif

#define DEFAULT_SERVERADDR _T("140.96.102.162")
#define DEFAULT_SERVERPORT 5060
#define DEFAULT_EXPIRETIME 3600
#define DEFAULT_DISPLAYNAME _T("K200 USER")
#define DEFAULT_TRANSPORT _T("UDP")
#define DEFAULT_UAPORT 5060
#define DEFAULT_711_PACKETIZEPERIOD 20
#define DEFAULT_RTP_PACKETIZEPERIOD 20
#define DEFAULT_RTP_RTPFRAMES 2
#define DEFAULT_Q_VALUE 0
#define DEFAULT_STUNSERVER _T("140.96.102.162")
#define DEFAULT_VIDEO_BITRATE 200
#define DEFAULT_VIDEO_FRAMERATE 15
#define DEFAULT_VIDEO_KEYINTERVAL 5
#define DEFAULT_AEC_TAILLENGTH 32
#define DEFAULT_AEC_FARENDECHOSIGNALLAG 200

const int DefaultAudioCodec[] =
{
	CODEC_PCMU,
	CODEC_PCMA,
	CODEC_GSM,
	CODEC_G723,
	CODEC_G729,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE
};

const int DefaultVideoCodec[] = 
{
	CODEC_H263,
	CODEC_MPEG4,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE,
	CODEC_NONE
};

CUAProfile *CUAProfile::s_pUAProfile = NULL;

void CUAProfile::init(CString localIPAddr, 
		      CString username,
		      CString realm,
		      CString password)
{
	char tmpStr[80];

	//if (m_bInitialized)
	//	return;

	CWinApp* pApp = AfxGetApp();

	//Call Server Settings
	m_bUseCallServer = (BOOL)pApp->GetProfileInt(SESSION_CALLSERVER,KEY_USECALLSERVER,0);
	m_szCallServerAddr = pApp->GetProfileString(SESSION_CALLSERVER,KEY_CALLSERVERADDR,DEFAULT_SERVERADDR);
	m_usCallServerPort = pApp->GetProfileInt(SESSION_CALLSERVER,KEY_CALLSERVERPORT,DEFAULT_SERVERPORT);
	
	//Outbound Settings
	m_bUseOutbound = (BOOL)pApp->GetProfileInt(SESSION_OUTBOUND,KEY_USEOUTBOUND,0);
	m_szOutboundAddr = pApp->GetProfileString(SESSION_OUTBOUND,KEY_OUTBOUNDADDR,DEFAULT_SERVERADDR);
	m_usOutboundPort = pApp->GetProfileInt(SESSION_OUTBOUND,KEY_OUTBOUNDPORT,DEFAULT_SERVERPORT);

	//Http Tunnel Settings
	m_bUseHttpTunnel = (BOOL)pApp->GetProfileInt(SESSION_HTTPTUNNEL,KEY_USEHTTPTUNNEL,0);
	m_szHttpTunnelAddr = pApp->GetProfileString(SESSION_HTTPTUNNEL,KEY_HTTPTUNNELADDR,DEFAULT_SERVERADDR);
	m_usHttpTunnelPort = pApp->GetProfileInt(SESSION_HTTPTUNNEL,KEY_HTTPTUNNELPORT,DEFAULT_SERVERPORT);


	//SIMPLE Settings
	m_bUseSIMPLEServer = (BOOL)pApp->GetProfileInt(SESSION_SIMPLESERVER,KEY_USESIMPLESERVER,0);
	m_szSIMPLEServerAddr = pApp->GetProfileString(SESSION_SIMPLESERVER,KEY_SIMPLESERVERADDR,DEFAULT_SERVERADDR);
	m_usSIMPLEServerPort = pApp->GetProfileInt(SESSION_SIMPLESERVER,KEY_SIMPLESERVERPORT,DEFAULT_SERVERPORT);

	//Registrar Settings
	m_bUseRegistrar = (BOOL)pApp->GetProfileInt(SESSION_REGISTRAR,KEY_USEREGISTRAR,0);
	m_szRegistrarAddr = pApp->GetProfileString(SESSION_REGISTRAR,KEY_REGISTRARADDR,DEFAULT_SERVERADDR);
	m_usRegistrarPort = pApp->GetProfileInt(SESSION_REGISTRAR,KEY_REGISTRARPORT,DEFAULT_SERVERPORT);
	m_bExplicitPort = (BOOL)pApp->GetProfileInt(SESSION_REGISTRAR,KEY_EXPLICIT_PORT,0);
	m_ulExpireTime = pApp->GetProfileInt(SESSION_REGISTRAR,KEY_EXPIRETIME,DEFAULT_EXPIRETIME);

	//Registrar2 Settings
	m_bUseRegistrar2 = (BOOL)pApp->GetProfileInt(SESSION_REGISTRAR,KEY_USEREGISTRAR2,0);
	m_szRegistrar2Addr = pApp->GetProfileString(SESSION_REGISTRAR,KEY_REGISTRAR2ADDR,DEFAULT_SERVERADDR);
	m_usRegistrar2Port = pApp->GetProfileInt(SESSION_REGISTRAR,KEY_REGISTRAR2PORT,DEFAULT_SERVERPORT);
	m_bExplicitPort2 = (BOOL)pApp->GetProfileInt(SESSION_REGISTRAR,KEY_EXPLICIT_PORT2,0);
	m_ulExpireTime2 = pApp->GetProfileInt(SESSION_REGISTRAR,KEY_EXPIRETIME2,DEFAULT_EXPIRETIME);
	
	//User Info
	m_DisplayName = pApp->GetProfileString(SESSION_USERINFO,KEY_DISPLAY_NAME,DEFAULT_DISPLAYNAME);
	m_Username = pApp->GetProfileString(SESSION_USERINFO,KEY_USERNAME,username);
	m_Qvalue = pApp->GetProfileInt(SESSION_USERINFO,KEY_Q_VALUE,DEFAULT_Q_VALUE);
	m_bUseSTUN = pApp->GetProfileInt(SESSION_USERINFO,KEY_USESTUN,0);
	m_STUNserver= pApp->GetProfileString(SESSION_USERINFO,KEY_STUNSERVER,DEFAULT_STUNSERVER);
	m_Realm = realm;
	m_Password = password;
	
	//Local Settings
	m_bUseStaticIP = (BOOL)pApp->GetProfileInt(SESSION_LOCAL,KEY_USESTATICIP,0);

	// sam: should use user specified local addr if it exist
	CString strLocalIPAddr = pApp->GetProfileString( SESSION_LOCAL,KEY_LOCALADDR, localIPAddr);
	m_LocalAddr = strLocalIPAddr;
	/*
	if ( _CheckLocalIPExist( strLocalIPAddr))
		m_LocalAddr = strLocalIPAddr;
	else
		m_LocalAddr = localIPAddr; // else use the first local IP
	 */

	m_LocalPort = pApp->GetProfileInt(SESSION_LOCAL,KEY_LOCALPORT,DEFAULT_UAPORT);
	m_Transport = pApp->GetProfileString(SESSION_LOCAL,KEY_TRANSPORT,DEFAULT_TRANSPORT);
	if (!CString(m_Transport).CompareNoCase( _T("tcp") ))
		m_bTransport = TRUE;
	else
		m_bTransport = FALSE;

	sprintf(tmpStr, "sip:%s@", m_Username);

#ifdef USESTUN
	char addrbuf[24];
	if (m_bUseSTUN && !m_STUNserver.IsEmpty() )
	{
		stunGetExtAddr( m_STUNserver, addrbuf );
		m_ExtPort = stunGetExtPort(m_STUNserver, m_LocalPort);
		strncat(tmpStr, addrbuf, strlen(addrbuf) );
		m_ExtAddr.Format("%s", addrbuf);
	} else {
		strncat(tmpStr, m_LocalAddr, m_LocalAddr.GetLength() );
		m_ExtAddr = m_LocalAddr;
		m_ExtPort = m_LocalPort;
	}
#else
	strncat(tmpStr, m_LocalAddr, m_LocalAddr.GetLength() );
	m_ExtAddr = m_LocalAddr;
	m_ExtPort = m_LocalPort;
#endif

	//Set Contact address
#ifdef USESTUN
	if (m_bTransport) {
		m_ContactAddr.Format(_T("<%s:%d;transport=tcp>"), tmpStr, m_ExtPort);
		m_TLSContactAddr.Format(_T("<%s:%d;transport=tcp>"), tmpStr, m_ExtPort+1);
	} else {
		m_ContactAddr.Format(_T("<%s:%d>"), tmpStr, m_ExtPort);
		m_TLSContactAddr.Format(_T("<%s:%d>"), tmpStr, m_ExtPort+1);
	}
#else
	if (m_bTransport) {
		m_ContactAddr.Format(_T("<%s:%d;transport=tcp>"), tmpStr, m_LocalPort );
		m_TLSContactAddr.Format(_T("<%s:%d;transport=tcp>"), tmpStr, m_LocalPort+1 );
	} else {
		m_ContactAddr.Format(_T("<%s:%d>"), tmpStr, m_LocalPort );
		m_TLSContactAddr.Format(_T("<%s:%d>"), tmpStr, m_LocalPort+1 );
	}
#endif

	m_TLSContactAddr.Replace( _T("sip"), _T("sips") );

	//Set Public address
	if (m_bUseRegistrar)
	{
		memset((void*)tmpStr, 0, 80);
		sprintf(tmpStr, "sip:%s@", m_Username);
		strncat(tmpStr, m_szRegistrarAddr, m_szRegistrarAddr.GetLength() );
		m_PublicAddr.Format(_T("%s"), tmpStr);
	} else {
		m_PublicAddr = m_ContactAddr;
	}

	//History
	memset((void*)tmpStr, 0, 80);
	m_HistoryCount = 0;
	for (int i = 0; i < 10; i++) {
		sprintf(tmpStr,"URL#%d", i);
		m_HistoryURL[i] = pApp->GetProfileString(SESSION_HISTORY,CString(tmpStr), _T("") );
		if (strlen( MyCString(m_HistoryURL[i]) ) != 0)
			m_HistoryCount++;
	}

	//Credential
	m_CredentialCount = 0;
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"Credential#%d", i);
		m_Credential[i] = pApp->GetProfileString(SESSION_CREDENTIAL,CString(tmpStr),_T(""));
		if (strlen( MyCString(m_Credential[i]) ) != 0)
			m_CredentialCount++;
	}

	//AudioCodec
	m_AudioCodecCount = 0;
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"AudioCodec#%d", i);
		m_AudioCodec[i] = (CODEC_NUMBER)pApp->GetProfileInt(SESSION_AUDIOCODEC,CString(tmpStr),DefaultAudioCodec[i]);
		if (m_AudioCodec[i] != -1)
			m_AudioCodecCount++;
	}
	m_G711PacketizePeriod = pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_G711PACKETIZEPERIOD,DEFAULT_711_PACKETIZEPERIOD);
	m_RTPPacketizePeriod = pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_RTPPACKETIZEPERIOD,DEFAULT_RTP_PACKETIZEPERIOD);
	m_RTPFrames = pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_RTPFRAMES,DEFAULT_RTP_RTPFRAMES);
	m_bEnableAEC = (BOOL)pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_ENABLEAEC,1);
	m_TailLength = pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_TAILLENGTH,DEFAULT_AEC_TAILLENGTH);
	m_FarEndEchoSignalLag = pApp->GetProfileInt(SESSION_AUDIOCODEC,KEY_FARENDECHOSIGNALLAG,DEFAULT_AEC_FARENDECHOSIGNALLAG);

	//VideoCodec
	m_VideoCodecCount = 0;
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"VideoCodec#%d", i);
		m_VideoCodec[i] = (CODEC_NUMBER)pApp->GetProfileInt(SESSION_VIDEOCODEC,CString(tmpStr),DefaultVideoCodec[i]);
		if (m_VideoCodec[i] != -1)
			m_VideoCodecCount++;
	}
	m_bUseVideo = (BOOL)pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_USEVIDEO,0);
	m_VideoSizeFormat = pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOSIZEFORMAT,2);
	m_VideoBitrate = pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOBITRATE,DEFAULT_VIDEO_BITRATE);
	m_VideoFramerate = pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOFRAMERATE,DEFAULT_VIDEO_FRAMERATE);
	m_VideoKeyInterval = pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOKEYINTERVAL,DEFAULT_VIDEO_KEYINTERVAL);
	
	m_bUseQuant = (BOOL)pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_USEQUANT,0);
	m_QuantValue = pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_QUANTVALUE,1);
	m_bOnlyIframe = (BOOL)pApp->GetProfileInt(SESSION_VIDEOCODEC,KEY_ONLYIFRAME,0);
	m_VideoName = pApp->GetProfileString(SESSION_VIDEOCODEC,KEY_VIDEONAME,_T(""));

	// SIP Extension
	m_bAnonFrom =  pApp->GetProfileInt(SESSION_SIPEXT,KEY_ANONFROM,0);
	m_bUsePreferredID = pApp->GetProfileInt(SESSION_SIPEXT, KEY_USEPREFERREDID, 0);
	m_strPreferredID = pApp->GetProfileString(SESSION_SIPEXT,KEY_PREFERREDID,"k200_user");
	m_bUseSessionTimer = pApp->GetProfileInt(SESSION_SIPEXT, KEY_USESESSIONTIMER, 0);
	m_uSessionTimer = pApp->GetProfileInt(SESSION_SIPEXT, KEY_SESSIONTIMER, 30);

	m_bInitialized = TRUE;
}

void CUAProfile::Write()
{
	char	czValue[256];
	CWinApp* pApp = AfxGetApp();

	if (!pApp)
		return;

	//Call Server
	pApp->WriteProfileInt(SESSION_CALLSERVER,KEY_USECALLSERVER,m_bUseCallServer);
	pApp->WriteProfileString(SESSION_CALLSERVER,KEY_CALLSERVERADDR,m_szCallServerAddr);
	pApp->WriteProfileInt(SESSION_CALLSERVER,KEY_CALLSERVERPORT,m_usCallServerPort);

	//Outbound
	pApp->WriteProfileInt(SESSION_OUTBOUND,KEY_USEOUTBOUND,m_bUseOutbound);
	pApp->WriteProfileString(SESSION_OUTBOUND,KEY_OUTBOUNDADDR,m_szOutboundAddr);
	pApp->WriteProfileInt(SESSION_OUTBOUND,KEY_OUTBOUNDPORT,m_usOutboundPort);

	//Http Tunnel
	pApp->WriteProfileInt(SESSION_HTTPTUNNEL,KEY_USEHTTPTUNNEL,m_bUseHttpTunnel);
	pApp->WriteProfileString(SESSION_HTTPTUNNEL,KEY_HTTPTUNNELADDR,m_szHttpTunnelAddr);
	pApp->WriteProfileInt(SESSION_HTTPTUNNEL,KEY_HTTPTUNNELPORT,m_usHttpTunnelPort);


	//SIMPLE Server
	pApp->WriteProfileInt(SESSION_SIMPLESERVER,KEY_USESIMPLESERVER,m_bUseSIMPLEServer);
	pApp->WriteProfileString(SESSION_SIMPLESERVER,KEY_SIMPLESERVERADDR,m_szSIMPLEServerAddr);
	pApp->WriteProfileInt(SESSION_SIMPLESERVER,KEY_SIMPLESERVERPORT,m_usSIMPLEServerPort);

	//Registar
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_USEREGISTRAR,m_bUseRegistrar);
	pApp->WriteProfileString(SESSION_REGISTRAR,KEY_REGISTRARADDR,m_szRegistrarAddr);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_REGISTRARPORT,m_usRegistrarPort);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_EXPLICIT_PORT,m_bExplicitPort);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_EXPIRETIME,m_ulExpireTime);

	//Registar2
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_USEREGISTRAR2,m_bUseRegistrar2);
	pApp->WriteProfileString(SESSION_REGISTRAR,KEY_REGISTRAR2ADDR,m_szRegistrar2Addr);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_REGISTRAR2PORT,m_usRegistrar2Port);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_EXPLICIT_PORT2,m_bExplicitPort2);
	pApp->WriteProfileInt(SESSION_REGISTRAR,KEY_EXPIRETIME2,m_ulExpireTime2);

	//User Info
	pApp->WriteProfileString(SESSION_USERINFO,KEY_DISPLAY_NAME,m_DisplayName);
	pApp->WriteProfileString(SESSION_USERINFO,KEY_USERNAME,m_Username);
	pApp->WriteProfileString(SESSION_USERINFO,KEY_STUNSERVER,m_STUNserver);
	pApp->WriteProfileInt(SESSION_USERINFO,KEY_USESTUN,m_bUseSTUN);
	//pApp->WriteProfileString(SESSION_USERINFO,KEY_CONTACT_ADDR,m_ContactAddr);
	//pApp->WriteProfileString(SESSION_USERINFO,KEY_PUBLIC_ADDR,m_PublicAddr);
	
	//Local Settings
	pApp->WriteProfileString(SESSION_LOCAL,KEY_LOCALADDR,m_LocalAddr);
	pApp->WriteProfileInt(SESSION_LOCAL,KEY_LOCALPORT,m_LocalPort);
	if ( m_bTransport )
		sprintf(czValue,"%s","TCP");
	else
		sprintf(czValue,"%s","UDP");
	pApp->WriteProfileString(SESSION_LOCAL,KEY_TRANSPORT,CString(czValue));
	pApp->WriteProfileInt(SESSION_LOCAL,KEY_USESTATICIP,m_bUseStaticIP);
	
	//History
	char tmpStr[16];
	for (int i = 0; i < 10; i++) {
		sprintf(tmpStr,"URL#%d", i);
		pApp->WriteProfileString(SESSION_HISTORY,CString(tmpStr),m_HistoryURL[i]);
	}

	//Credential
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"Credential#%d", i);
		pApp->WriteProfileString(SESSION_CREDENTIAL,CString(tmpStr),m_Credential[i]);
	}

	//AudioCodec
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"AudioCodec#%d", i);
		pApp->WriteProfileInt(SESSION_AUDIOCODEC,CString(tmpStr),m_AudioCodec[i]);
	}
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_G711PACKETIZEPERIOD,m_G711PacketizePeriod);
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_RTPPACKETIZEPERIOD,m_RTPPacketizePeriod);
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_RTPFRAMES,m_RTPFrames);
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_TAILLENGTH,m_TailLength);
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_FARENDECHOSIGNALLAG,m_FarEndEchoSignalLag);
	pApp->WriteProfileInt(SESSION_AUDIOCODEC,KEY_ENABLEAEC,m_bEnableAEC);

	//VideoCodec
	for (i = 0; i < 10; i++) {
		sprintf(tmpStr,"VideoCodec#%d", i);
		pApp->WriteProfileInt(SESSION_VIDEOCODEC,CString(tmpStr),m_VideoCodec[i]);
	}
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_USEVIDEO,m_bUseVideo);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOSIZEFORMAT,m_VideoSizeFormat);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOBITRATE,m_VideoBitrate);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOFRAMERATE,m_VideoFramerate);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_VIDEOKEYINTERVAL,m_VideoKeyInterval);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_USEQUANT,m_bUseQuant);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_QUANTVALUE,m_QuantValue);
	pApp->WriteProfileInt(SESSION_VIDEOCODEC,KEY_ONLYIFRAME,m_bOnlyIframe);
	pApp->WriteProfileString(SESSION_VIDEOCODEC,KEY_VIDEONAME,m_VideoName);

	//SIP Extension
	pApp->WriteProfileInt(SESSION_SIPEXT,KEY_ANONFROM,m_bAnonFrom);
	pApp->WriteProfileInt(SESSION_SIPEXT,KEY_USEPREFERREDID,m_bUsePreferredID);
	pApp->WriteProfileString(SESSION_SIPEXT,KEY_PREFERREDID,m_strPreferredID);
	pApp->WriteProfileInt(SESSION_SIPEXT,KEY_USESESSIONTIMER,m_bUseSessionTimer);
	pApp->WriteProfileInt(SESSION_SIPEXT,KEY_SESSIONTIMER,m_uSessionTimer);

	m_bInitialized = TRUE;
}

void CUAProfile::SetHistoryURL(CString strURL)
{
	char tmpStr[8];
	int i;

	for (i = 0; i < 10; i++) {
		if (!strcmp( MyCString(strURL), MyCString(m_HistoryURL[i]) ) )
			return;
	}
	if (m_HistoryCount < 10) {
		for (i = 0; i < 10; i++) {
			if (!strlen( MyCString(m_HistoryURL[i])) ) {
				m_HistoryURL[i] = strURL;
				sprintf(tmpStr,"URL#%d", i);
				m_HistoryCount++;
				break;
			}
		} 
	} else {
		m_HistoryCount++;
		m_HistoryURL[m_HistoryCount % 10] = strURL;
		sprintf( tmpStr,"URL#%d", m_HistoryCount % 10);
	}
	AfxGetApp()->WriteProfileString(SESSION_HISTORY,CString(tmpStr),strURL);
}

// return true if szIP if one of the local ip address
bool CUAProfile::_CheckLocalIPExist( LPCSTR szIP)
{
	const char** szIPList = getAllMyAddr();
	for (int i=0; i< 16; i++) 
	{
		if (strlen(szIPList[i]) > 0)
		{
			if ( strcmp( szIPList[i], szIP) == 0)
				return true;
		}
	}
	return false;
}
