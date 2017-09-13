// MediaManager.h: interface for the CMediaManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEDIAMANAGER_H__3FAEB45E_86C5_4132_A4FB_6DDAB6207CF1__INCLUDED_)
#define AFX_MEDIAMANAGER_H__3FAEB45E_86C5_4132_A4FB_6DDAB6207CF1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxmt.h>
#include "WavInOut.h"
#include <adt/dx_buf.h>
#include <adt/dx_hash.h>
#include "UACommon.h"
#include "cclRtp.h"

#include "g711.h"
#include "g723.h"
#include "g729.h"
#include "iLBC.h"

#include "DSPFramework.h"



typedef struct {
    int     timeStamp;
    t_CMB   cmb;
} t_event;

class CMediaManager  
{
public:
	CString		m_strAppPath;
	int			m_maxJB;
	int			m_dropJB;
	HANDLE		m_hEventKill;
	unsigned char *m_pRecord;
	unsigned char *m_pPlay;
	BOOL		StopWaveThread(); // sjhuang 2006/02/21
	static int RTPSequence(int,rtp_hdr_t*, const char*,int,int);
	
	RCODE EndDTMFTone();
	BOOL m_bInitialized;
#ifndef _WIN32_WCE
	void ReadTonePCMRes();
	void ReadToneFiles();
#endif
	BOOL m_IsPlayingRTP;
	void init();
	RCODE PlayRTPWav(WAV_FORMAT wavformat);
	CMediaManager();
	virtual ~CMediaManager();

	static CMediaManager* GetMediaMgr()
	{
		return s_pMediaManager;
	}

	static int Wav2RTP(char* buff, int len);
	static int RTP2Wav(char* buff, int len);
	static int RTPEventHandler(int,const char*,int);
	static int RTPEventHandlerEx(int,rtp_hdr_t*, const char*,int);
	static int RTPEventHandlerPkt(int channel,rtp_pkt_t* pkt);

	RCODE	Media_RTP_Stop();
	RCODE   SetMediaParameter(int mediatype);
	void	SetOwnerDlg(UaDlg dlg) { m_OwnerDlg = dlg; }
	UaDlg	GetOwnerDlg() { return m_OwnerDlg; }
	RCODE	RTPPeerConnect(MyCString ip, int port);
	RCODE	RTPOpenPort(WAV_FORMAT wavetype);
	RCODE	RTPClosePort();
	unsigned int RTPGetPort();
	RCODE	SendDTMFTone(int type);
	RCODE	StopPlaySound();
	int		GetMediaType() { return m_WaveType; }

	G711Codec* m_G711Codec;
	unsigned int	m_RtpChan;
	unsigned int	m_RTPPacketsize;
	unsigned int	m_RTPPacketizePeriod;
	unsigned int	m_CodecPacketizePeriod;
	unsigned int	m_BitsPerSample;

	/* 0 : no event (neutral state)
	   1 : prepare sending event
	   2 : sending event
	   3 : first time transmit last packet
	   4 : re-transmit last packet #1
	   5 : re-transmit last packet #2
	 */
	typedef enum {
		DTMF_IDLE,
		DTMF_PREPARE,
		DTMF_SENDING,
		DTMF_LAST1,
		DTMF_LAST2,
		DTMF_LAST3,
	} DTMFState;

	DTMFState		m_DTMFState; 
	unsigned int	m_DTMFEvent;
	unsigned int	m_DTMFDuration;
	rtp_pkt_param_t	m_DTMFparam;

	int				m_WaveType;
	int				m_ToneType;
	int				m_idx;

	CString		m_CurDirectory;
	WavInOut*	m_WaveIOObj;
	DxBuffer	m_BufRTP2IO;
	DxBuffer	m_BufIO2RTP;

	DxHash		m_HashRTP2IO;

	CCriticalSection	m_CriticalSection;
	UaDlg m_OwnerDlg;

	static char RTPBuffer[4096];
	static unsigned int RTPLen;
	static unsigned int TotalPacket;
	static CMediaManager *s_pMediaManager;
	static BOOL bPacketReceived;
	static BOOL bRetry;
	static BOOL bMyFlag;

	static DWORD __stdcall DSPThreadFunc( LPVOID pParam);
	HANDLE m_hDSPThread;
	t_event * m_cmdevent;

	// sam add: calc data receive rate in bytes per seconds
	// avg rate in 3 seconds
	time_t m_StatBufTimestamp;	// the time of latest buffer (i.e. m_StatBuf[0])
	int m_StatBuf[4];			// statistic buffer for bytes in a seconds

	time_t m_SilenceTimestamp;	// the time of last RTP silence suppresion packet comes (payload type=19)
	time_t m_DataTimestamp;		// the time of last RTP data packet comes (payload type != 19)

	void _ClearStatBuf();		// reset statistic buffer with expected data rate
	void _ShiftStatBuf();		// shift m_StatBuf if current time > m_StatBufTimestamp
	void _IncStatBuf( int n);	// log received bytes

	double GetReceiveQuality();	// % of real / expected received bytes
	void PrintOutRTPStat();
};

#endif // !defined(AFX_MEDIAMANAGER_H__3FAEB45E_86C5_4132_A4FB_6DDAB6207CF1__INCLUDED_)
