// SDPManager.h: interface for the CSDPManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SDPMANAGER_H__7E0E0BE8_AF81_4FA2_9678_0BBC81C4283B__INCLUDED_)
#define AFX_SDPMANAGER_H__7E0E0BE8_AF81_4FA2_9678_0BBC81C4283B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <uacore/ua_core.h>
#include <adt/dx_hash.h>
#include "UAProfile.h"

class CSDPManager  
{
public:

	CSDPManager(int SDPcount);
	virtual ~CSDPManager();
	static CSDPManager* GetSDPMgr()
	{
		return s_pSDPManager;
	}

	RCODE SetSDPbyDlg(UaDlg dlg, SdpSess SDP);
	RCODE DelSDPbyDlg(UaDlg dlg);
	SdpSess GetSDPbyDlg(UaDlg dlg);
	SdpSess GetLocalSDPbyDlg(UaDlg dlg);
	CODEC_NUMBER GetAudioCodecChoiceNum(SdpSess sdp);
	CODEC_NUMBER GetVideoCodecChoiceNum(SdpSess sdp);
	unsigned int GetVideoCodecChoiceSizeFormat(SdpSess sdp);
	UaCodec GetAudioCodecChoice(SdpSess sdp);
	UaCodec GetVideoCodecChoice(SdpSess sdp);

	static CSDPManager* s_pSDPManager;
	DxHash m_SDPhash;

};

#endif // !defined(AFX_SDPMANAGER_H__7E0E0BE8_AF81_4FA2_9678_0BBC81C4283B__INCLUDED_)
