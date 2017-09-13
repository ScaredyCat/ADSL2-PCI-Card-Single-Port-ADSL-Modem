#if !defined(AFX_VOICEMAILLOGIN_H__EEA1DB02_9006_4338_BF27_9C155EDF5D5C__INCLUDED_)
#define AFX_VOICEMAILLOGIN_H__EEA1DB02_9006_4338_BF27_9C155EDF5D5C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VoiceMailLogin.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVoiceMailLogin dialog

class CVoiceMailLogin : public CDialog
{
// Construction
public:
	CVoiceMailLogin(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVoiceMailLogin)
	enum { IDD = IDD_VOICEMAILLOGIN };
	CString	m_VoiceMailAccount;
	CString	m_VoiceMailIP;
	CString	m_VoiceMailPort;
	CString	m_VoiceMailPWD;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVoiceMailLogin)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVoiceMailLogin)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOICEMAILLOGIN_H__EEA1DB02_9006_4338_BF27_9C155EDF5D5C__INCLUDED_)
