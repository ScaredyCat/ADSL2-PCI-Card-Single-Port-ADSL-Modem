// SDPManager.h: interface for the CSDPManager class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(SDPMANAGER_H)
#define SDPMANAGER_H

#include <UACore/ua_core.h>
#include <UACommon.h>

#include <adt/dx_hash.h>
#include <sdp/sdp_sess.h>
typedef enum {
	WAV_PCMU = 0,
	WAV_GSM = 3,
	WAV_G723 = 4,
	WAV_PCMA = 8,
	WAV_G729 = 18,
	WAV_iLBC = 97
} WAV_FORMAT;

//CSDPManager(int SDPcount);
//static CSDPManager s_pSDPManager;
RCODE SetSDPbyDlg(UaDlg, SdpSess);
RCODE DelSDPbyDlg(UaDlg);
SdpSess GetSDPbyDlg(UaDlg);
SdpSess GetLocalSDPbyDlg(UaDlg);
CODEC_NUMBER GetAudioCodecChoiceNum(SdpSess);
UaCodec GetAudioCodecChoice(SdpSess);
DxHash m_SDPhash;

#endif
