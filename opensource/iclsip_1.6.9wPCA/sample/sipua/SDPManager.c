// SDPManager.cpp: implementation of the CSDPManager class.
//
//////////////////////////////////////////////////////////////////////


#include "SDPManager.h"
#include "CallManager.h"
#include "MediaManager.h"
//#include "WavInOut.h"
#include <string.h>
#include <stdlib.h>
#include "ifx_common.h"
#include "../../../ifx/ifx_httpd/ifx_amazon_cfg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSDPManager(int SDPcount)
{
	m_SDPhash = dxHashNew(SDPcount, FALSE, DX_HASH_POINTER);
}

CSDPManagerFree()
{
	if (m_SDPhash)
		dxHashFree(m_SDPhash, (void (*)(void *))uaSDPFree);
}

CODEC_NUMBER GetAudioCodecChoiceNum(SdpSess sdp)
{
	int i,j,CodecType,m_AudioCodecCount,m_AudioCodec;
	char  sValue[MAX_DATA_LEN],sTAG_NAME[MAX_DATA_LEN];
	CODEC_NUMBER retval = CODEC_NONE;

	if (!sdp)
		return CODEC_NONE;
	
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, "RTP_CODEC_NUM",sValue)==1)
		m_AudioCodecCount=atoi(sValue);
	
	CodecLst tmpCodecLst = uaSDPGetCodecList(sdp, UA_MEDIA_AUDIO);
	UaCodec tmpCodec;
	i=uaCodecLstGetCount(tmpCodecLst);
	for (i;i > 0 ;i--) {
		tmpCodec = uaCodecLstGetAt(tmpCodecLst, i-1);
		CodecType = uaCodecGetType(tmpCodec);
		for (j=0;j < m_AudioCodecCount;j++) {
			sprintf(sTAG_NAME,"RTP_CODEC%d",j);
			if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, sTAG_NAME,sValue)==1)
				m_AudioCodec=atoi(sValue);
			switch ( CodecType ) {
			case 0:
				if ( m_AudioCodec == CODEC_PCMU ) {
					retval = CODEC_PCMU;
					continue;
				}
				break;

			case 3:
				if ( m_AudioCodec == CODEC_GSM ) {
					retval = CODEC_GSM;
					continue;
				}
				break;

			case 4:
				if ( m_AudioCodec == CODEC_G723 ) {
					retval = CODEC_G723;
					continue;
				}
				break;

			case 8:
				if ( m_AudioCodec == CODEC_PCMA ) {
					retval = CODEC_PCMA;
					continue;
				}
				break;

			case 18:
				if ( m_AudioCodec == CODEC_G729 ) {
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

UaCodec GetAudioCodecChoice(SdpSess sdp)
{
	UaCodec retCodec = NULL;
	char  sValue[MAX_DATA_LEN];
	int m_RTPPacketizePeriod;

	if (!sdp)
		return NULL;
	if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, "RTP_PacketPeriod",sValue)==1)
		m_RTPPacketizePeriod=atoi(sValue);
		
	switch ( GetAudioCodecChoiceNum(sdp) ) {
	case CODEC_PCMU:
		retCodec = uaCodecNew(UA_WAV_PCMU, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_PCMA:
		retCodec = uaCodecNew(UA_WAV_PCMA, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_G723:
		retCodec = uaCodecNew(UA_WAV_723, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_G729:
		retCodec = uaCodecNew(UA_WAV_729, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	case CODEC_GSM:
		retCodec = uaCodecNew(UA_WAV_GSM, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
		break;
	default:
		break;
	}
	return retCodec;
}

SdpSess GetSDPbyDlg(UaDlg dlg)
{
	if (!dlg)
		return NULL;
	
	char* callid = uaDlgGetCallID(dlg);
	
	if (callid)
		return (SdpSess)dxHashItem(m_SDPhash, callid);
	else
		return NULL;
}

RCODE SetSDPbyDlg(UaDlg dlg, SdpSess sdp)
{
	if (!dlg || !sdp)
		return RC_ERROR;

	char* callid = uaDlgGetCallID(dlg);
	
	if (dxHashItem(m_SDPhash, callid))
		uaSDPFree((SdpSess)dxHashDel(m_SDPhash, callid));
	dxHashAdd(m_SDPhash, callid, sdp);
	return RC_OK;
}

RCODE DelSDPbyDlg(UaDlg dlg)
{
	if (!dlg)
		return RC_ERROR;

	char* callid = uaDlgGetCallID(dlg);
	if (dxHashItem(m_SDPhash, callid))
		uaSDPFree((SdpSess)dxHashDel(m_SDPhash, callid));
	return RC_OK;
}

/* Get local SDP */
SdpSess GetLocalSDPbyDlg(UaDlg dlg)
{
	CodecLst AudioCodecLst = NULL;
	CodecLst VideoCodecLst = NULL;
	UaCodec audioCodec = NULL;
	UaCodec videoCodec = NULL;
	unsigned int LocalAudioPort = 0, LocalVideoPort = 0;
	SdpSess sdp;
	int i,m_AudioCodecCount,m_AudioCodec;
	char  sValue[MAX_DATA_LEN],sTAG_NAME[MAX_DATA_LEN];
	int m_RTPPacketizePeriod;
	
#ifdef USESTUN
	unsigned short tmpport;
#endif
	
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
  if (RTPOpenPort(WAV_PCMU) < 0 ) {
		printf("Local RTP audio port open fail!\n");
		return NULL;
	}
	LocalAudioPort = RTPGetPort();
#endif

	AudioCodecLst = uaCodecLstNew();



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


			
			sdp = uaSDPNew( m_UaMgr, 
							getMyAddr(),
							AudioCodecLst,
							LocalAudioPort,
							VideoCodecLst,
							LocalVideoPort,
							NULL); // "CT:300"
	} else {	
		if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, "RTP_CODEC_NUM",sValue)==1)
		m_AudioCodecCount=atoi(sValue);
		if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, "RTP_PacketPeriod",sValue)==1)
		m_RTPPacketizePeriod=atoi(sValue);	
		/* Codec list order should follow User setting */
		for (i=0;i < m_AudioCodecCount;i++)
		{
			sprintf(sTAG_NAME,"RTP_CODEC%d",i);
			if(ifx_GetCfgData(FILE_RC_CONF, TAG_RTP_CODEC, sTAG_NAME,sValue)==1)
				m_AudioCodec=atoi(sValue);
			switch (m_AudioCodec) {
			case CODEC_PCMU:
				audioCodec = uaCodecNew(UA_WAV_PCMU, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_PCMA:
				audioCodec = uaCodecNew(UA_WAV_PCMA, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_G723:
				audioCodec = uaCodecNew(UA_WAV_723, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_G729:
				audioCodec = uaCodecNew(UA_WAV_729, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
				break;
			case CODEC_GSM:
				audioCodec = uaCodecNew(UA_WAV_GSM, UAMEDIA_SENDRECV, m_RTPPacketizePeriod, 8000, 1, NULL );
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
		if ( m_AudioCodecCount == 0 ) {
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

		sdp = uaSDPNew( m_UaMgr, 
				       getMyAddr(),
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