// SDPManager.cpp: implementation of the CSDPManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "SDPManager.h"
#include "CallManager.h"
#include "MediaManager.h"

#ifdef _FOR_VIDEO
#include "..\VIDEO\VideoManager.h"
#endif

#ifdef USESTUN
#include "stunapi.h"
#endif

#include "UAProfile.h"
#include "WavInOut.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CSDPManager *CSDPManager::s_pSDPManager = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDPManager::CSDPManager(int SDPcount)
{
	m_SDPhash = dxHashNew(SDPcount, FALSE, DX_HASH_POINTER);
	s_pSDPManager = this;
}

CSDPManager::~CSDPManager()
{
	if (m_SDPhash)
		dxHashFree(m_SDPhash, (void (*)(void *))uaSDPFree);
}

CODEC_NUMBER CSDPManager::GetAudioCodecChoiceNum(SdpSess sdp)
{
	int CodecType;
	CODEC_NUMBER retval = CODEC_NONE;

	if (!sdp)
		return CODEC_NONE;
	
	CUAProfile* pProfile = CUAProfile::GetProfile();

	CodecLst tmpCodecLst = uaSDPGetCodecList(sdp, UA_MEDIA_AUDIO);
	UaCodec tmpCodec;
	
	for (int i=uaCodecLstGetCount(tmpCodecLst);i > 0 ;i--) {
		tmpCodec = uaCodecLstGetAt(tmpCodecLst, i-1);
		CodecType = uaCodecGetType(tmpCodec);
		for (int j=0;j < (int)pProfile->m_AudioCodecCount;j++) {
			switch ( CodecType ) {
			case 0:
				if ( pProfile->m_AudioCodec[j] == CODEC_PCMU ) {
					retval = CODEC_PCMU;
					continue;
				}
				break;

			case 3:
				if ( pProfile->m_AudioCodec[j] == CODEC_GSM ) {
					retval = CODEC_GSM;
					continue;
				}
				break;

			case 4:
				if ( pProfile->m_AudioCodec[j] == CODEC_G723 ) {
					retval = CODEC_G723;
					continue;
				}
				break;

			case 8:
				if ( pProfile->m_AudioCodec[j] == CODEC_PCMA ) {
					retval = CODEC_PCMA;
					continue;
				}
				break;

			case 18:
				if ( pProfile->m_AudioCodec[j] == CODEC_G729 ) {
					retval = CODEC_G729;
					continue;
				}
				break;
			}
		}
	}
	uaCodecLstFree(tmpCodecLst);
	return retval;
}

CODEC_NUMBER CSDPManager::GetVideoCodecChoiceNum(SdpSess sdp)
{
	int CodecType;
	CODEC_NUMBER retval = CODEC_NONE;

	if (!sdp)
		return CODEC_NONE;
	
	CUAProfile* pProfile = CUAProfile::GetProfile();

	CodecLst tmpCodecLst = uaSDPGetCodecList(sdp, UA_MEDIA_VIDEO);
	UaCodec tmpCodec;
	
	for (int i=uaCodecLstGetCount(tmpCodecLst);i > 0 ;i--) {
		tmpCodec = uaCodecLstGetAt(tmpCodecLst, i-1);
		CodecType = uaCodecGetType(tmpCodec);
		for (int j=0;j < (int)pProfile->m_VideoCodecCount;j++) {
			switch ( CodecType ) {
			case 34: //UA_VIDEO_H263
				if ( pProfile->m_VideoCodec[j] == CODEC_H263 ) {
					retval = CODEC_H263;
					continue;
				}
				break;

			default:
				if (tmpCodec) {
					if ( stricmp( uaCodecGetCodecname(tmpCodec), "MP4V-ES" ) == 0 ) {
						if ( pProfile->m_VideoCodec[j] == CODEC_MPEG4 ) {
							retval = CODEC_MPEG4;
							continue;
						}
					}
				}
			}
		}
	}
	uaCodecLstFree(tmpCodecLst);
	return retval;
}

UaCodec CSDPManager::GetAudioCodecChoice(SdpSess sdp)
{
	UaCodec retCodec = NULL;

	if (!sdp)
		return NULL;

	CUAProfile* pProfile = CUAProfile::GetProfile();

	switch ( GetAudioCodecChoiceNum(sdp) ) {
	case CODEC_PCMU:
		retCodec = uaCodecNew(UA_WAV_PCMU, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_PCMA:
		retCodec = uaCodecNew(UA_WAV_PCMA, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_G723:
		retCodec = uaCodecNew(UA_WAV_723, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_G729:
		retCodec = uaCodecNew(UA_WAV_729, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_GSM:
		retCodec = uaCodecNew(UA_WAV_GSM, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	default:
		break;
	}
	return retCodec;
}

UaCodec CSDPManager::GetVideoCodecChoice(SdpSess sdp)
{
	int i;
	UaCodec retCodec = NULL, tmpCodec = NULL;
	CodecLst tmpCodecLst = 	uaSDPGetCodecList(sdp, UA_MEDIA_VIDEO);
	CUAProfile* pProfile = CUAProfile::GetProfile();
	
	if (!sdp)
		return NULL;

	switch ( GetVideoCodecChoiceNum(sdp) ) {
	case CODEC_H263:
		tmpCodec = uaSDPGetMediaCodec(sdp, UA_MEDIA_VIDEO, MediaCodectoPayload(UA_VIDEO_H263) );

		// sam use the channel field to tell is QCIF or CIF
/*		
		if (uaCodecGetChannel(tmpCodec)==2 || uaCodecGetChannel(tmpCodec)==3 )
			retCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 30, 90000, uaCodecGetChannel(tmpCodec), NULL );
		else
			retCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 30, 90000, 0, NULL );
*/		
		retCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 0, 90000, 0, NULL );

		uaCodecFree(tmpCodec);
		tmpCodec = NULL;
		break;

	case CODEC_MPEG4:
		for (i=0; i < uaCodecLstGetCount(tmpCodecLst); i++) {
			tmpCodec = uaCodecLstGetAt(tmpCodecLst, i);
			if ( stricmp( uaCodecGetCodecname(tmpCodec), "MP4V-ES" ) == 0 ) {
				if ( !retCodec ) {
/*					
					if (uaCodecGetChannel(tmpCodec)==2 || uaCodecGetChannel(tmpCodec)==3 )
						retCodec = uaCodecNew( uaCodecGetType(tmpCodec), UAMEDIA_SENDRECV, 30, 90000, uaCodecGetChannel(tmpCodec), "MP4V-ES");
					else
*/						retCodec = uaCodecNew( uaCodecGetType(tmpCodec), UAMEDIA_SENDRECV, 30, 90000, 0, "MP4V-ES");
					 
					retCodec = uaCodecNew( uaCodecGetType(tmpCodec), UAMEDIA_SENDRECV, 0, 90000, 0, "MP4V-ES");
				}
			}
		}
		break;
	default:
		break;
	}
	uaCodecLstFree(tmpCodecLst);
	return retCodec;
}

unsigned int CSDPManager::GetVideoCodecChoiceSizeFormat(SdpSess sdp)
{
	int retval = 2;
	UaCodec retCodec = GetVideoCodecChoice(sdp);

	if ( retCodec )
	{
		retval = uaCodecGetChannel(retCodec);
		uaCodecFree(retCodec);
		retCodec = NULL;
	}
	return retval;
}

SdpSess CSDPManager::GetSDPbyDlg(UaDlg dlg)
{
	if (!dlg)
		return NULL;
	
	char* callid = uaDlgGetCallID(dlg);
	if (callid)
		return static_cast<SdpSess>( dxHashItem(m_SDPhash, callid));
	else
		return NULL;
}

RCODE CSDPManager::SetSDPbyDlg(UaDlg dlg, SdpSess sdp)
{
	if (!dlg || !sdp)
		return RC_ERROR;

	char* callid = uaDlgGetCallID(dlg);
	if (dxHashItem(m_SDPhash, callid))
		uaSDPFree(static_cast<SdpSess>( dxHashDel(m_SDPhash, callid) ));
	dxHashAdd(m_SDPhash, callid, sdp);
	return RC_OK;
}

RCODE CSDPManager::DelSDPbyDlg(UaDlg dlg)
{
	if (!dlg)
		return RC_ERROR;

	char* callid = uaDlgGetCallID(dlg);
	if (dxHashItem(m_SDPhash, callid))
		uaSDPFree(static_cast<SdpSess>( dxHashDel(m_SDPhash, callid) ));
	return RC_OK;
}

/* Get local SDP */
SdpSess CSDPManager::GetLocalSDPbyDlg(UaDlg dlg)
{
	CodecLst AudioCodecLst = NULL;
	CodecLst VideoCodecLst = NULL;
	UaCodec audioCodec = NULL;
	UaCodec videoCodec = NULL;
	UaCodec tmpCodec = NULL;
	unsigned int LocalAudioPort = 0, LocalVideoPort = 0;
	SdpSess sdp;

#ifdef USESTUN
	unsigned short tmpport;
#endif
	
	MyCString tmpStr;

	CCallManager* pCallManager = CCallManager::GetCallMgr();
	CMediaManager* pMediaManager = CMediaManager::GetMediaMgr();
#ifdef _FOR_VIDEO
	CVideoManager* pVideoManager = CVideoManager::GetVideoMgr();
#endif
	CUAProfile* pProfile = CUAProfile::GetProfile();
	SdpSess PeerSDP = GetSDPbyDlg(dlg);

#ifdef USESTUN
	if ( pProfile->m_bUseSTUN && !pProfile->m_STUNserver.IsEmpty() )
	{
		if ( pMediaManager->RTPOpenPort(WAV_PCMU) < 0 ) {
			DebugMsg("Local RTP audio port open fail!");
			return NULL;
		}
		tmpport = pMediaManager->RTPGetPort();
		pMediaManager->RTPClosePort();
		LocalAudioPort = stunGetExtPort( pProfile->m_STUNserver, tmpport );
		pMediaManager->RTPOpenPort(WAV_PCMU);
	} else {
		if ( pMediaManager->RTPOpenPort(WAV_PCMU) < 0 ) {
			DebugMsg("Local RTP audio port open fail!");
			return NULL;
		}
		LocalAudioPort = pMediaManager->RTPGetPort();
	}
#else
	if ( pMediaManager->RTPOpenPort(WAV_PCMU) < 0 ) {
		DebugMsg("Local RTP audio port open fail!");
		return NULL;
	}
	LocalAudioPort = pMediaManager->RTPGetPort();
#endif

	AudioCodecLst = uaCodecLstNew();

#ifdef _FOR_VIDEO

	#ifdef USESTUN
		if ( pProfile->m_bUseSTUN && !pProfile->m_STUNserver.IsEmpty() )
		{
			if ( pVideoManager->RTPOpenPort() < 0 ) {
				DebugMsg("Local RTP video port open fail!");
				return NULL;
			}
			tmpport = pVideoManager->RTPGetPort();
			pVideoManager->RTPClosePort();
			LocalVideoPort = stunGetExtPort( pProfile->m_STUNserver, tmpport );
			pVideoManager->RTPOpenPort();
		} else {
			if ( pVideoManager->RTPOpenPort() < 0 ) {
				DebugMsg("Local RTP video port open fail!");
				return NULL;
			}
			LocalVideoPort = pVideoManager->RTPGetPort();
		}
	#else
		if ( pVideoManager->RTPOpenPort() < 0 ) {
			DebugMsg("Local RTP video port open fail!");
			return NULL;
		}
		LocalVideoPort = pVideoManager->RTPGetPort();
	#endif

	VideoCodecLst = uaCodecLstNew();
#endif

	if (PeerSDP != NULL) {
			audioCodec = GetAudioCodecChoice(PeerSDP);
			if (audioCodec) {
				uaCodecLstAddTail(AudioCodecLst, audioCodec);
				uaCodecFree(audioCodec);
				audioCodec = NULL;
			} else {
				uaCodecLstFree(AudioCodecLst);
				AudioCodecLst = NULL;
			}

#ifdef _FOR_VIDEO
			videoCodec = GetVideoCodecChoice(PeerSDP);
			if (pProfile->m_bUseVideo && videoCodec) {
				uaCodecLstAddTail(VideoCodecLst, videoCodec);	
				uaCodecFree(videoCodec);
				videoCodec = NULL;
			} else {
				uaCodecLstFree(VideoCodecLst);
				VideoCodecLst = NULL;
			}
#endif
			if ( pProfile->m_ExtAddr[0] == '[' )
				tmpStr = pProfile->m_ExtAddr.Mid(1, pProfile->m_ExtAddr.GetLength()-2);
			else
				tmpStr = pProfile->m_ExtAddr;

			sdp = uaSDPNew( pCallManager->m_UaMgr, 
							tmpStr,
							AudioCodecLst,
							LocalAudioPort,
							VideoCodecLst,
							LocalVideoPort,
							NULL); // "CT:300"
	} else {	
		/* Codec list order should follow User setting */
		for (int i=0;i < (int)pProfile->m_AudioCodecCount;i++)
		{
			switch (pProfile->m_AudioCodec[i]) {
			case CODEC_PCMU:
				audioCodec = uaCodecNew(UA_WAV_PCMU, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_PCMA:
				audioCodec = uaCodecNew(UA_WAV_PCMA, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_G723:
				audioCodec = uaCodecNew(UA_WAV_723, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_G729:
				audioCodec = uaCodecNew(UA_WAV_729, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_GSM:
				audioCodec = uaCodecNew(UA_WAV_GSM, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			default:
				break;
			}
			if (audioCodec) {
				uaCodecLstAddTail(AudioCodecLst, audioCodec);
				uaCodecFree(audioCodec);
				audioCodec = NULL;
			}
		}
		
		/* Use default codec if no codec preference set */
		if ( pProfile->m_AudioCodecCount == 0 ) {
			/*
			audioCodec = uaCodecNew(UA_WAV_PCMU, UAMEDIA_SENDRECV, pProfile->m_RTPPacketizePeriod, 8000, 1, NULL );
			uaCodecLstAddTail(AudioCodecLst, audioCodec);
			uaCodecFree(audioCodec);
			audioCodec = NULL;
			*/
			if (AudioCodecLst != NULL)
			{
				uaCodecLstFree(AudioCodecLst);
				AudioCodecLst = NULL;
			}
		}

#ifdef _FOR_VIDEO
		if (pProfile->m_bUseVideo) {
			/* Codec list order should follow User setting */
			for (int i=0;i < (int)pProfile->m_VideoCodecCount;i++)
			{
				switch (pProfile->m_VideoCodec[i]) {
				case CODEC_H263:
					// sam: use to old way or CIF video transport is not available
					//videoCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 30, 90000, pProfile->m_VideoSizeFormat, NULL ); 
					videoCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 0, 90000, 0, NULL );
					break;
				case CODEC_MPEG4:
					// sam: use to old way or CIF video transport is not available
					//videoCodec = uaCodecNew(CODEC_MPEG4, UAMEDIA_SENDRECV, 30, 90000, pProfile->m_VideoSizeFormat, "MP4V-ES");
					videoCodec = uaCodecNew(CODEC_MPEG4, UAMEDIA_SENDRECV, 0, 90000, 0, "MP4V-ES");
					//uaCodecSetCodecFmtp( videoCodec, "QCIF=1" );
					break;
				default:
					break;
				}
				if (videoCodec) {
					uaCodecLstAddTail(VideoCodecLst, videoCodec);
					uaCodecFree(videoCodec);
					videoCodec = NULL;
				}
			}
			/* Use default codec if no codec preference set */
			if ( pProfile->m_VideoCodecCount == 0 ) {
				//videoCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 30, 90000, pProfile->m_VideoSizeFormat, NULL );
				videoCodec = uaCodecNew(UA_VIDEO_H263, UAMEDIA_SENDRECV, 0, 90000, 0, NULL );
				uaCodecLstAddTail(VideoCodecLst, videoCodec);
				uaCodecFree(videoCodec);
				videoCodec = NULL;
			}
		} else {
			uaCodecLstFree(VideoCodecLst);
			VideoCodecLst = NULL;
		}
#endif

		if ( pProfile->m_ExtAddr[0] == '[' )
			tmpStr = pProfile->m_ExtAddr.Mid(1, pProfile->m_ExtAddr.GetLength()-2);
		else
			tmpStr = pProfile->m_ExtAddr;

		sdp = uaSDPNew( pCallManager->m_UaMgr, 
				       tmpStr,
				       AudioCodecLst,
				       LocalAudioPort,
				       VideoCodecLst,
				       LocalVideoPort,
					   NULL); // "CT:300"
	}

	if (audioCodec)
		uaCodecFree(audioCodec);

	if (videoCodec)
		uaCodecFree(videoCodec);

	if (AudioCodecLst != NULL)
		uaCodecLstFree(AudioCodecLst);

	if (VideoCodecLst != NULL)
		uaCodecLstFree(VideoCodecLst);

	/*** ljchuang added for SIP/ENUM trial 2005/09/13 ***/
	/*** <------------------ BEGIN -----------------> ***/
/*	
	SdpMsess tmpMsess = NULL;
	tmpMsess = sdpSessGetMsessAt(sdp, 0);

	if (tmpMsess) {
		for (int i = sdpMsessGetTaSize(tmpMsess)-1; i >= 0; i--) {
			sdpMsessDelTaAt(tmpMsess, i);
		}
	}
*/	
	/*
	tmpMsess = sdpSessGetMsessAt(sdp, 1);

	if (tmpMsess) {
		for (int i = sdpMsessGetTaSize(tmpMsess)-1; i >= 0; i--) {
			sdpMsessDelTaAt(tmpMsess, i);
		}
	}
	*/
	/*** <------------------- END ------------------> ***/
	
	return sdp;
}