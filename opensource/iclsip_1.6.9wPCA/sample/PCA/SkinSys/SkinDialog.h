///////////////////////////////////////////////////////////////
//
// Dosya Adý: SkinDialog
// Yazan    : Cüneyt ELÝBOL
// Açýklama : Asýl Modül
// 
// Detaylý Bilgi için 
//       
//    www.celibol.freeservers.com  adresini ziyaret edin
//            veya
//    celibol@hotmail.com adresine mesaj atýn.
//
// Dikkat:
//    Bu program kodlarýný kullanýrken Aciklama.txt dosyasýndaki
//  gerekleri yerine getirmeniz gerekir.
//
///////////////////////////////////////////////////////////////


#if !defined(AFX_SKINDIALOG_H__6206972E_1F54_11D4_8166_D172E91C6E8C__INCLUDED_)
#define AFX_SKINDIALOG_H__6206972E_1F54_11D4_8166_D172E91C6E8C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXTEMPL_H__
#include <afxtempl.h>
#endif

#include "IniFile.h"
#include "SkinButton.h"
#include "SkinSlider.h"
#include "SkinLabel.h"
#include "SkinProgress.h"
// SkinDialog.h : header file
//

void CopyBitmap(CDC* dc, CBitmap& mRes, const CBitmap& hbmp, RECT r);
void LoadPictureFile(HDC hdc, LPCTSTR szFile, CBitmap* pBitmap, CSize& mSize);
void DrawBitmap(CDC* dc, HBITMAP hbmp, RECT r, BOOL Stretch = FALSE);

CString getPath(CString m_Path);
CString GetFirstParam(CString& mName);
CRect StringToRect(CString& m_Data);
char* GetFileName(HWND hwndOwner, const char* m_Filter);

/////////////////////////////////////////////////////////////////////////////
// CSkinDialog dialog

class CSkinDialog : public CDialog
{
protected:
	CTypedPtrArray<CPtrArray, CSkinButton*> m_Buttons;
	CTypedPtrArray<CPtrArray, CSkinSlider*> m_Sliders;
	CTypedPtrArray<CPtrArray, CSkinLabel*> m_Labels;
	CTypedPtrArray<CPtrArray, CSkinProgress*> m_Progress;
	CRgn *m_Rgn;
	void AssignValues();
// Construction
public:
	BOOL AddCustSkinButton(LPCTSTR idName, CRect r, UINT idBmp, LPCTSTR toolTip);
	void SetButtonPos(LPCTSTR btnName, int x, int y );
	BOOL AddCustSkinButton( LPCTSTR idName, CRect r, UINT idBmpNormal, UINT idBmpOver, UINT idBmpDown, UINT idBmpDisabled, LPCTSTR toolTip );
	void SetTextShow( CString m_IDName, BOOL bVal );
	virtual void DragWindow( int x, int y );
	void SetButtonShow( CString btnName, BOOL bShow);
	BOOL GetButtonEnabled( CString btnName );
	void SetButtonToolTip(CString m_IDName, CString m_NewToolTip);
	void SetTextToolTip(CString m_IDName, CString m_NewToolTip);
	void SetProgressToolTip(CString m_IDName, CString m_NewToolTip);

	virtual void ProgresChanged(CString m_Name);
	void SetProgressPos(CString m_ID, int m_newPos);
	int GetProgressPos(CString m_ID);
	void ChangeProgress(int m_ID);
	void SetButtonCheck(CString m_IDName, BOOL m_NewValue);
	BOOL GetButtonCheck(CString m_IDName);
	void SetButtonEnable(CString m_IDName, BOOL m_Enable);
	int GetTrackBarPos(CString m_IDName);
	CString GetDisplayText(CString m_IDName);
	void SetText(CString m_ID, CString m_newText);
	void SetTrackBarPos(CString m_ID, int m_newPos);
	void ReadTrackBarInfo(CIniFile m_File);
	void ReadProgressInfo(CIniFile m_File);
	CRgn* GetRGN();
	void SetMenuID(UINT mMenuID);
	void SetSkinFile(LPCTSTR mSkinName);
	CSkinDialog();   // standard constructor
	CSkinDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	CSkinDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	~CSkinDialog();

	virtual void ButtonPressed(CString m_ButtonName);
	virtual void PressButton(int m_ID);

	virtual void TrackChange(CString m_ButtonName, UINT nSBCode, UINT nPos);
	virtual void ChangeTrack(int m_ID, UINT nSBCode, UINT nPos);

	virtual void TextClicked(CString m_TextName);
	virtual void ClickText(int m_ID);
	virtual void MouseMoved(CString m_ButtonName, int x, int y);
// Dialog Data
	//{{AFX_DATA(CSkinDialog)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinDialog)
	//}}AFX_VIRTUAL

	void SetMode2( bool b, LPCRECT rt = NULL);
	bool GetMode2()							{ return m_UseMode2; }
	void ToggleMode2( LPCRECT rt = NULL)	{ SetMode2( !m_UseMode2, rt); }
	
// Implementation
protected:
	void ReadTextInfo(CIniFile m_File);
	void ReadButtonInfo(CIniFile m_File);
	void LoadBitmap(const char* szFilename, CBitmap* pBitmap);
	CString m_SkinName;
	virtual void Setup(CBitmap& mBitmap);
	UINT m_MenuID;
	CBitmap m_Normal, m_Over, m_Down, m_Disabled;

	bool m_UseMode2;
	CBitmap m_Normal2, m_Over2, m_Down2, m_Disabled2;
	CBitmap& GetNormalBmp()		{ return m_UseMode2? m_Normal2: m_Normal; }
 
	virtual void LoadSkins(); //LPCTSTR mNormal, LPCTSTR mOver, LPCTSTR mDown, LPCTSTR mDisabled);
	void Free();
	// Generated message map functions
	//{{AFX_MSG(CSkinDialog)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL m_bDragging;
	CPoint m_ptDrag;


};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINDIALOG_H__6206972E_1F54_11D4_8166_D172E91C6E8C__INCLUDED_)
