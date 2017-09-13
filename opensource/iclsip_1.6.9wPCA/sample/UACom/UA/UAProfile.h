/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* $Id: UAProfile.h,v 1.25 2006/08/04 02:50:54 shinjan Exp $ */

// UAProfile.h: Configuration page 
//
#ifndef __UA_PROFILE_H__
#define __UA_PROFILE_H__

#include "UACommon.h"

#define		SESSION_HTTPTUNNEL	_T("Http_Tunnel")
#define		KEY_USEHTTPTUNNEL	_T("Use_HttpTunnel")
#define		KEY_HTTPTUNNELADDR	_T("HttpTunnel_Addr")
#define		KEY_HTTPTUNNELPORT	_T("HttpTunnel_Port")

#define		SESSION_OUTBOUND	_T("Outbound")
#define		KEY_USEOUTBOUND		_T("Use_Outbound")
#define		KEY_OUTBOUNDADDR	_T("Outbound_Addr")
#define		KEY_OUTBOUNDPORT	_T("Outbound_Port")

#define		SESSION_CALLSERVER	_T("CALL_Server")
#define		KEY_USECALLSERVER	_T("Use_Call_Server")
#define		KEY_CALLSERVERADDR	_T("CALL_Server_Addr")
#define		KEY_CALLSERVERPORT	_T("CALL_Server_Port")

#define		SESSION_REGISTRAR	_T("Registrar")
#define		KEY_USEREGISTRAR	_T("Use_Registrar")
#define		KEY_REGISTRARADDR	_T("Registrar_Addr")
#define		KEY_REGISTRARPORT	_T("Registrar_Port")
#define		KEY_EXPIRETIME		_T("Expire_Time")
#define		KEY_EXPLICIT_PORT	_T("Explicit_Port")

#define		KEY_USEREGISTRAR2	_T("Use_Registrar2")
#define		KEY_REGISTRAR2ADDR	_T("Registrar2_Addr")
#define		KEY_REGISTRAR2PORT	_T("Registrar2_Port")
#define		KEY_EXPIRETIME2		_T("Expire_Time2")
#define		KEY_EXPLICIT_PORT2	_T("Explicit_Port2")


#define		SESSION_SIMPLESERVER	_T("SIMPLE_Server")
#define		KEY_USESIMPLESERVER		_T("Use_SIMPLE_Server")
#define		KEY_SIMPLESERVERADDR	_T("SIMPLE_Server_Addr")
#define		KEY_SIMPLESERVERPORT	_T("SIMPLE_Server_Port")

#define		SESSION_USERINFO	_T("User_Info")
#define		KEY_DISPLAY_NAME	_T("Display_Name")
#define		KEY_USERNAME		_T("Username")
#define		KEY_CONTACT_ADDR	_T("Contact_Addr")
#define		KEY_PUBLIC_ADDR		_T("Public_Addr")
#define		KEY_Q_VALUE			_T("Qvalue")
#define		KEY_STUNSERVER		_T("STUN_server")
#define		KEY_USESTUN			_T("Use_STUN")

#define		SESSION_AUTHENTICATION	_T("Authentication")
#define		KEY_REALM			_T("Realm")
#define		KEY_ACCOUNTID1		_T("AccountID1")
#define		KEY_PASSWD			_T("Password")

#define		SESSION_LOCAL		_T("Local_Settings")
#define		KEY_TRANSPORT		_T("Transport")
#define		KEY_USESTATICIP		_T("Use_Static_IP")
#define		KEY_LOCALADDR		_T("Local_IP")
#define		KEY_LOCALPORT		_T("Local_Port")
#define		KEY_SIP_TOS			_T("SIP_ToS")
#define		KEY_RTP_AUDIO_TOS	_T("RTP_Audio_ToS")
#define		KEY_RTP_VIDEO_TOS	_T("RTP_Video_ToS")

#define		SESSION_HISTORY		_T("History")
#define		SESSION_CREDENTIAL	_T("Credential")

#define		SESSION_AUDIOCODEC	_T("AudioCodec")
#define		SESSION_VIDEOCODEC	_T("VideoCodec")
#define		KEY_VIDEONAME		_T("VideoName")
#define		KEY_USEVIDEO		_T("UseVideo")
#define		KEY_VIDEOSIZEFORMAT	_T("VideoSizeFormat")
#define		KEY_G711PACKETIZEPERIOD	_T("G.711 PacketizePeriod")
#define		KEY_RTPPACKETIZEPERIOD	_T("RTP PacketizePeriod")
#define		KEY_VIDEOBITRATE	_T("VideoBitrate")
#define		KEY_VIDEOFRAMERATE	_T("VideoFramerate")
#define		KEY_VIDEOKEYINTERVAL	_T("VideoKeyInterval")
#define		KEY_USEQUANT		_T("UseQuant")
#define		KEY_QUANTVALUE		_T("QuantValue")
#define		KEY_ONLYIFRAME		_T("Only I frame")
#define		KEY_RTPFRAMES		_T("Frames per RTP packet")
#define		KEY_ENABLEAEC		_T("Enable AEC")
#define		KEY_TAILLENGTH		_T("Tail Length")
#define		KEY_FARENDECHOSIGNALLAG	_T("FarEnd Echo Signal Lag")

#define		SESSION_SIPEXT		_T("SIP Extension")
#define		KEY_ANONFROM		_T("Anonymous FROM URI")
#define		KEY_PREFERREDID		_T("P-Preferred Identity")
#define		KEY_USEPREFERREDID	_T("Use P-Preferred Identity")
#define		KEY_USESESSIONTIMER	_T("Use Session Timer")
#define		KEY_SESSIONTIMER	_T("Session Timer")

class CUAProfile  
{
public:
	void	init(CString localIPAddr, 
				 CString username,
				 CString realm = "",
				 CString password = "");
	void	Write();
	void	SetHistoryURL(CString strURL);
	void	Dump();
	

	CUAProfile()
	{
			s_pUAProfile = this;
			m_bInitialized = FALSE;
	}

	~CUAProfile()
	{
		if ( this == s_pUAProfile )
			s_pUAProfile = NULL;
	}

	static CUAProfile* GetProfile()	
	{ 
		return s_pUAProfile; 
	}

public:
	BOOL		m_bInitialized;

	//Call Server Settings
	BOOL		m_bUseCallServer;
	MyCString	m_szCallServerAddr;
	unsigned short	m_usCallServerPort;
	
	//Outbound Settings
	BOOL		m_bUseOutbound;
	MyCString	m_szOutboundAddr;
	unsigned short	m_usOutboundPort;
	
	//Http Tunnel Settings
	BOOL		m_bUseHttpTunnel;
	MyCString	m_szHttpTunnelAddr;
	unsigned short	m_usHttpTunnelPort;

	//Registrar Settings
	BOOL		m_bUseRegistrar;
	MyCString	m_szRegistrarAddr;
	unsigned short	m_usRegistrarPort;
	unsigned long	m_ulExpireTime;
	BOOL		m_bExplicitPort;

	//Registrar2 Settings
	BOOL		m_bUseRegistrar2;
	MyCString	m_szRegistrar2Addr;
	unsigned short	m_usRegistrar2Port;
	unsigned long	m_ulExpireTime2;
	BOOL		m_bExplicitPort2;

	//SIMPLE Settings
	BOOL		m_bUseSIMPLEServer;
	MyCString	m_szSIMPLEServerAddr;
	unsigned short	m_usSIMPLEServerPort;
	
	//User Info
	MyCString	m_DisplayName;
	MyCString	m_Username;
	MyCString	m_ContactAddr;
	MyCString	m_TLSContactAddr;
	MyCString	m_PublicAddr;
	MyCString	m_Realm;
	MyCString	m_Password;
	MyCString	m_STUNserver;
	BOOL		m_bUseSTUN;
	unsigned short	m_Qvalue;

	//Options
	//MyCString	m_LocalHostName;
	MyCString	m_LocalAddr;
	MyCString	m_ExtAddr;
	unsigned short	m_LocalPort;
	unsigned short	m_ExtPort;
	MyCString	m_Transport;
	BOOL		m_bTransport; // 1:TCP  0:UDP
	BOOL		m_bUseStaticIP;
	
	//History
	MyCString	m_HistoryURL[10];
	unsigned short	m_HistoryCount;

	//Credential
	MyCString	m_Credential[10];
	unsigned short	m_CredentialCount;

	//AudioCodec
	CODEC_NUMBER	m_AudioCodec[10];
	unsigned short	m_AudioCodecCount;
	unsigned short	m_G711PacketizePeriod;
	unsigned short	m_RTPPacketizePeriod;
	unsigned short  m_RTPFrames;
	BOOL		m_bEnableAEC;
	unsigned short	m_TailLength;
	unsigned short	m_FarEndEchoSignalLag;

	//VideoCodec
	CODEC_NUMBER	m_VideoCodec[10];
	unsigned short	m_VideoCodecCount;
	unsigned short	m_VideoSizeFormat;
	BOOL		m_bUseVideo;
	unsigned short	m_VideoBitrate;
	unsigned short	m_VideoFramerate;
	BOOL		m_bUseQuant;
	unsigned short	m_QuantValue;
	BOOL		m_bOnlyIframe;
	unsigned short  m_VideoKeyInterval;
	MyCString		m_VideoName;

	//SIP Extension
	BOOL		m_bAnonFrom;
	BOOL		m_bUsePreferredID;
	MyCString	m_strPreferredID;
	BOOL		m_bUseSessionTimer;
	unsigned short	m_uSessionTimer;

	static CUAProfile* s_pUAProfile;
	
protected:
	CString		m_czFileName;

	// return true if szIP if one of the local ip address
	bool _CheckLocalIPExist( LPCSTR szIP);
};

#endif // __UA_PROFILE_H__
