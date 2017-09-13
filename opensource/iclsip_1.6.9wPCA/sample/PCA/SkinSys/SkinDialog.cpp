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

#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxwin.h>
#include <afxcmn.h>
#include <io.h>

#ifdef	_PCAUA_Res_
#include "PCAUARes.h"
#endif

#include "SkinDialog.h"
#include "SkinSlider.h"

#include "..\PCAUA.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BackGroundColor RGB(255, 255, 255)
#define HIMETRIC_INCH   2540    // HIMETRIC units per inch

//
//	BitmapToRegion :	Create a region from the "non-transparent" pixels of a bitmap
//	Author :			Jean-Edouard Lachand-Robert (http://www.geocities.com/Paris/LeftBank/1160/resume.htm), June 1998.
//
//	hBmp :				Source bitmap
//	cTransparentColor :	Color base for the "transparent" pixels (default is black)
//	cTolerance :		Color tolerance for the "transparent" pixels.
//
//	A pixel is assumed to be transparent if the value of each of its 3 components (blue, green and red) is 
//	greater or equal to the corresponding value in cTransparentColor and is lower or equal to the 
//	corresponding value in cTransparentColor + cTolerance.
//
HRGN BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor = 0, COLORREF cTolerance = 0x101010)
{
	HRGN hRgn = NULL;

	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = CreateCompatibleDC(NULL);
		if (hMemDC)
		{
			// Get bitmap size
			BITMAP bm;
			GetObject(hBmp, sizeof(bm), &bm);

			// Create a 32 bits depth bitmap and select it into the memory DC 
			BITMAPINFOHEADER RGB32BITSBITMAPINFO = {	
					sizeof(BITMAPINFOHEADER),	// biSize 
					bm.bmWidth,					// biWidth; 
					bm.bmHeight,				// biHeight; 
					1,							// biPlanes; 
					32,							// biBitCount 
					BI_RGB,						// biCompression; 
					0,							// biSizeImage; 
					0,							// biXPelsPerMeter; 
					0,							// biYPelsPerMeter; 
					0,							// biClrUsed; 
					0							// biClrImportant; 
			};
			VOID * pbits32; 
			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);

				// Create a DC just to copy the bitmap into the memory DC
				HDC hDC = CreateCompatibleDC(hMemDC);
				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
					BITMAP bm32;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory DC
					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);

					// For better performances, we will use the ExtCreateRegion() function to create the
					// region. This function take a RGNDATA structure on entry. We will add rectangles by
					// amount of ALLOC_UNIT number in this structure.
					#define ALLOC_UNIT	100
					DWORD maxRects = ALLOC_UNIT;
					HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

					// Keep on hand highest and lowest values for the "transparent" pixels
					BYTE lr = GetRValue(cTransparentColor);
					BYTE lg = GetGValue(cTransparentColor);
					BYTE lb = GetBValue(cTransparentColor);
					BYTE hr = min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = min(0xff, lb + GetBValue(cTolerance));

					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
					for (int y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to right
						for (int x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								BYTE b = GetRValue(*p);
								if (b >= lr && b <= hr)
								{
									b = GetGValue(*p);
									if (b >= lg && b <= hg)
									{
										b = GetBValue(*p);
										if (b >= lb && b <= hb)
											// This pixel is "transparent"
											break;
									}
								}
								p++;
								x++;
							}

							if (x > x0)
							{
								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
								if (pData->rdh.nCount >= maxRects)
								{
									GlobalUnlock(hData);
									maxRects += ALLOC_UNIT;
									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
									pData = (RGNDATA *)GlobalLock(hData);
								}
								RECT *pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
								// large (ie: > 4000). Therefore, we have to create the region by multiple steps.
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically)
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangles
					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean up
					GlobalUnlock(hData);
					GlobalFree(hData);
					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			DeleteDC(hMemDC);
		}	
	}

	return hRgn;
}

CString getPath(CString m_Path)
{
	TCHAR szPath[_MAX_PATH];
	LPTSTR lpszName;

	GetFullPathName(m_Path, _MAX_PATH, szPath, &lpszName);

	CString mPath = szPath;
	int p = mPath.Find(lpszName, 0);
	if (p > -1)
	{
		mPath.Delete(p, strlen(lpszName));
	}
	p = mPath.GetLength() - 1;
	if(mPath.GetAt(p) == '\\')
		mPath.Delete(p, 1);
	return mPath + '\\';
}

CString GetFirstParam(CString& mName)
{
	int P = mName.Find(",");
	if ( P == -1)
	{
		CString ret = mName;
		mName.Empty();
		return ret;
	}
	
	char m_Res[256];
	sprintf(m_Res, "%s", mName);
	m_Res[P] = '\0';

	mName.Delete(0, P + 1);
	mName.TrimLeft();
	mName.TrimRight();

	return m_Res;
}

CRect StringToRect(CString& m_Data)
{
	CString l = GetFirstParam(m_Data);
	CString t = GetFirstParam(m_Data);
	CString w = GetFirstParam(m_Data);
	CString h = GetFirstParam(m_Data);

	CRect r;
	r.left = atoi(l);
	r.top  = atoi(t);
	r.right = r.left + atoi(w);
	r.bottom = r.top + atoi(h);
	return r;
}

BOOL CreateRegion (RECT r, CRgn *pRgn, CBitmap *pBitmap, COLORREF keycol)
{
	if(!pBitmap) return FALSE;
	if(!pRgn) return FALSE;

	CDC					memDC;
	CBitmap				*pOldMemBmp = NULL;
	CRect				cRect(r);
	int					x, y;
	int					n=0;		//number of transparent pixels
	CRgn				rgnTemp;

	memDC.CreateCompatibleDC(NULL);
	pOldMemBmp = memDC.SelectObject(pBitmap);
	pRgn->CreateRectRgn(0, 0, cRect.Width(), cRect.Height());

	COLORREF m_Color = memDC.GetPixel(0, 0);
	for (x = 0; x <= r.right - r.left; x++)
	{
		for (y = 0; y <= r.bottom - r.top; y++)
		{
			if (memDC.GetPixel(x,y) == m_Color)
			{
				rgnTemp.CreateRectRgn(x, y, x + 1, y + 1);
				pRgn->CombineRgn(pRgn, &rgnTemp, RGN_XOR);
				rgnTemp.DeleteObject();	
				++n;
			}
		}
	}
	if (pOldMemBmp) 
		memDC.SelectObject(pOldMemBmp);
	memDC.DeleteDC();
	return n > 0;
}

// Microsoft Article den alýnma. 

void LoadPictureFile(HDC hdc, LPCTSTR szFile, CBitmap* pBitmap, CSize& mSize)
{
	// open file
	HANDLE hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	_ASSERTE(INVALID_HANDLE_VALUE != hFile);

	// get file size
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	_ASSERTE(-1 != dwFileSize);

	LPVOID pvData = NULL;
	// alloc memory based on file size
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);
	_ASSERTE(NULL != hGlobal);

	pvData = GlobalLock(hGlobal);
	_ASSERTE(NULL != pvData);

	DWORD dwBytesRead = 0;
	// read file and store in global memory
	BOOL bRead = ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL);
	_ASSERTE(FALSE != bRead);
	GlobalUnlock(hGlobal);
	CloseHandle(hFile);

	LPSTREAM pstm = NULL;
	// create IStream* from global memory
	HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pstm);
	_ASSERTE(SUCCEEDED(hr) && pstm);

	// Create IPicture from image file
	LPPICTURE gpPicture;

	hr = ::OleLoadPicture(pstm, dwFileSize, FALSE, IID_IPicture, (LPVOID *)&gpPicture);
	_ASSERTE(SUCCEEDED(hr) && gpPicture);	
	pstm->Release();

	OLE_HANDLE m_picHandle;
	/*
	long hmWidth, hmHeight;
	gpPicture->get_Width(&hmWidth);
	gpPicture->get_Height(&hmHeight);
	int nWidth	= MulDiv(hmWidth, GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);
	int nHeight	= MulDiv(hmHeight, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);
	*/
	gpPicture->get_Handle(&m_picHandle);
	pBitmap->DeleteObject();
	pBitmap->Attach((HGDIOBJ) m_picHandle);

	BITMAP bm;
	GetObject(pBitmap->m_hObject, sizeof(bm), &bm);
	mSize.cx = bm.bmWidth; //nWidth;
	mSize.cy = bm.bmHeight; //nHeight;
}

// load skin bitmap from resource -jason
void LoadPictureRes( LPCTSTR szResName, CBitmap* pBitmap, CSize& mSize)
{
#ifdef	_PCAUA_Res_
	LoadPictureRes_(szResName, pBitmap, mSize);
#else
	pBitmap->DeleteObject();
	pBitmap->LoadBitmap(szResName);

	BITMAP bm;
	GetObject(pBitmap->m_hObject, sizeof(bm), &bm);
	mSize.cx = bm.bmWidth; //nWidth;
	mSize.cy = bm.bmHeight; //nHeight;
#endif
}

char* GetFileName(HWND hwndOwner, const char* m_Filter)
{
	OPENFILENAME	ofn;				// common dialog box structure
	CString			str;

	char ndir[260]={"\0"},nname[1024]={"\0"};
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndOwner;
	ofn.hInstance = AfxGetInstanceHandle ();
	ofn.lpstrFile = nname;
	ofn.nMaxFile = sizeof(nname);
	ofn.lpstrFilter = m_Filter; //"bitmap (*.bmp)\0*.BMP\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = ndir;
	ofn.Flags = OFN_PATHMUSTEXIST | 
//				OFN_FILEMUSTEXIST | 
				OFN_EXPLORER | 
				OFN_ALLOWMULTISELECT |
				OFN_HIDEREADONLY;
	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn) != TRUE) 
		return _T("");
	return ofn.lpstrFile;
}

//extern void DrawBitmap(CDC* dc, HBITMAP hbmp, RECT r, BOOL Stretch);
void DrawBitmap(CDC* dc, HBITMAP hbmp, RECT r, BOOL Stretch)
{
	if(!hbmp) return;

	CDC memdc;
	memdc.CreateCompatibleDC(dc);
	memdc.SelectObject(hbmp);
	int w = r.right - r.left,
		h = r.bottom - r.top;

	if(!Stretch)
	{
		dc->BitBlt(r.left, r.top, w, h, &memdc, 0, 0, SRCCOPY);
	}
	else
	{
		dc->SetStretchBltMode( HALFTONE ); // shinjan	
		//dc->StretchBlt(0, 0, w , h, &memdc, 0, 0, cx, cy, SRCCOPY); 

#if 1  // shinjan
		dc->StretchBlt(r.left, r.top, w, h, &memdc, 0, 0, w, h, SRCCOPY); 
#endif
		
	}
	memdc.DeleteDC();
}

void CopyBitmap(CDC* dc, CBitmap& mRes, const CBitmap& hbmp, RECT r)
{
	if(!hbmp.m_hObject) return;
	int w = r.right - r.left,
		h = r.bottom - r.top;

	CDC memdc, hDC;
	
	mRes.CreateCompatibleBitmap(dc, w, h);

	hDC.CreateCompatibleDC(dc);
	hDC.SelectObject((HBITMAP) mRes);

	memdc.CreateCompatibleDC(dc);
	memdc.SelectObject(hbmp);

	hDC.StretchBlt(0, 0, w, h, &memdc, r.left, r.top, w, h, SRCCOPY); 
	hDC.DeleteDC();
	memdc.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
// CSkinDialog dialog

BEGIN_MESSAGE_MAP(CSkinDialog, CDialog)
	//{{AFX_MSG_MAP(CSkinDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	
	// add by shinjan 20050928
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinDialog message handlers

CSkinDialog::CSkinDialog()
{
	//{{AFX_DATA_INIT(CSkinDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
}

CSkinDialog::CSkinDialog(UINT nIDTemplate, CWnd* pParentWnd)
		: CDialog(nIDTemplate, pParentWnd)
{
  AssignValues();
}

CSkinDialog::CSkinDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
		: CDialog(lpszTemplateName, pParentWnd)
{
  AssignValues();
}

CSkinDialog::~CSkinDialog()
{
	Free();
}

void CSkinDialog::SetMenuID(UINT mMenuID)
{
	m_MenuID = mMenuID;
}

void CSkinDialog::LoadBitmap(const char* szFilename, CBitmap* pBitmap)
{
	ASSERT(szFilename);
	ASSERT(pBitmap);

	pBitmap->DeleteObject();

	HBITMAP hBitmap = NULL; 
	hBitmap = (HBITMAP)LoadImage(NULL, szFilename, IMAGE_BITMAP, 0, 0, 
		LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE); 
	pBitmap->Attach(hBitmap); 
}

void CSkinDialog::AssignValues()
{
	m_Rgn      = NULL;
	m_MenuID   = 0;
	m_bDragging = FALSE;
}

void CSkinDialog::Free()
{
	m_UseMode2 = false;

	m_Normal.DeleteObject();
	m_Over.DeleteObject();
	m_Down.DeleteObject();
	m_Disabled.DeleteObject();

	m_Normal2.DeleteObject();
	m_Over2.DeleteObject();
	m_Down2.DeleteObject();
	m_Disabled2.DeleteObject();

	for(int i = 0; i < m_Buttons.GetSize(); i++)
		delete m_Buttons.GetAt(i);
	m_Buttons.RemoveAll();

	for(i = 0; i < m_Sliders.GetSize(); i++)
		delete m_Sliders.GetAt(i);
	m_Sliders.RemoveAll();

	for(i = 0; i < m_Labels.GetSize(); i++)
		delete m_Labels.GetAt(i);
	m_Labels.RemoveAll();

	for(i = 0; i < m_Progress.GetSize(); i++)
		delete m_Progress.GetAt(i);
	m_Progress.RemoveAll();
}

CRgn* CSkinDialog::GetRGN()
{
	return m_Rgn;
}

void CSkinDialog::Setup(CBitmap& mBitmap)
{
	const char* MsgCaption ="Error";

	if (GetSafeHwnd()) 
	{
		RECT r;
		GetWindowRect(&r);

//		m_Rgn = new CRgn;
		HRGN hr = BitmapToRegion((HBITMAP) mBitmap, RGB(255, 255, 255));
		//BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor = 0, COLORREF cTolerance = 0x101010)
//		m_Rgn->Attach(hr);
		//if (CreateRegion(r, m_Rgn, &m_Normal, BackGroundColor))
		if(hr != NULL)
		{
			if ( !::SetWindowRgn (GetSafeHwnd(), hr, TRUE)) //(HRGN) *m_Rgn, TRUE))
			{
				MessageBox ("SetWindowRgn fail.", MsgCaption, MB_OK);
				DeleteObject(hr);	// if SetWindowRgn() ok, don't release region
			}
		}
		else
			MessageBox ("BitmapToRegion fail.", MsgCaption, MB_OK);
		Invalidate ();
	}
}

void CSkinDialog::LoadSkins()// LPCTSTR mNormal, LPCTSTR mOver, LPCTSTR mDown, LPCTSTR mDisabled)
{
	Free();

	CIniFile m_SkinFile(m_SkinName);

	CString mPath(m_SkinName); //"C:/Belgelerim/Test/Cola/Main.bmp", //
	CString mNormal = m_SkinFile.ReadString("Screen", "Main", ""),
		    mMask = m_SkinFile.ReadString("Screen", "Mask", mNormal),
		    mOver  = m_SkinFile.ReadString("Screen", "Over", mNormal),
		    mDown = m_SkinFile.ReadString("Screen", "Down", mNormal),
		    mDisabled = m_SkinFile.ReadString("Screen", "Disabled", mNormal);

	CString mNormal2 = m_SkinFile.ReadString("Screen", "Main2", mNormal);
	CString mOver2 = m_SkinFile.ReadString("Screen", "Over2", mOver);
	CString mDown2 = m_SkinFile.ReadString("Screen", "Down2", mDown);
	CString mDisabled2 = m_SkinFile.ReadString("Screen", "Disabled2", mDisabled);

	CDC* hdc = GetDC();
	CSize mSize;

	CBitmap m_Mask;

	//LoadPictureFile(hdc->m_hDC, getPath(mPath) + mNormal, &m_Normal, mSize);
	//LoadPictureFile(hdc->m_hDC, getPath(mPath) + mMask, &m_Mask, mSize);

	LoadPictureRes(mNormal, &m_Normal, mSize);
	LoadPictureRes(mMask, &m_Mask, mSize);
//	extend_x = mSize.cx;
//	extend_y = mSize.cy;

	RECT r;
	GetWindowRect(&r);
	SetWindowPos (&wndTop, r.left, r.top, mSize.cx, mSize.cy, SWP_NOREPOSITION);

	
	//LoadPictureFile(hdc->m_hDC, getPath(mPath) + mOver, &m_Over, mSize);
	//LoadPictureFile(hdc->m_hDC, getPath(mPath) + mDown, &m_Down, mSize);
	//LoadPictureFile(hdc->m_hDC, getPath(mPath) + mDisabled, &m_Disabled, mSize);
	LoadPictureRes(mOver, &m_Over, mSize);
	LoadPictureRes(mDown, &m_Down, mSize);
	LoadPictureRes(mDisabled, &m_Disabled, mSize);

	LoadPictureRes(mNormal2, &m_Normal2, mSize);
	LoadPictureRes(mOver2, &m_Over2, mSize);
	LoadPictureRes(mDown2, &m_Down2, mSize);
	LoadPictureRes(mDisabled2, &m_Disabled2, mSize);

	Setup(m_Mask);
	ReadButtonInfo(m_SkinFile);
	ReadTextInfo(m_SkinFile);
	ReadTrackBarInfo(m_SkinFile);
	ReadProgressInfo(m_SkinFile);


	ReleaseDC(hdc);
	Invalidate();
}

void CSkinDialog::SetSkinFile(LPCTSTR mSkinName)
{
	m_SkinName = mSkinName;
}

void CSkinDialog::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
#if 0
	// shinjan test, 2006/06/28
	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
#else
	m_ptDrag = point;
	m_bDragging = TRUE;
	SetCapture();
#endif

	::SetCursor( ::LoadCursor( NULL, IDC_SIZEALL ));

	CDialog::OnLButtonDown(nFlags, point);
}

void CSkinDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	//SetRedraw(FALSE);
	//RECT r;
	//GetClientRect(&r);
	//DrawBitmap(GetDC(), (HBITMAP) m_Normal, r, TRUE, extend_x, extend_y);	

	int i;

	for( i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Btn = m_Buttons.GetAt(i);
		m_Btn->Invalidate();
	}

	for(i = 0; i < m_Sliders.GetSize(); i++)
	{
		CSkinSlider* m_Slider = m_Sliders.GetAt(i);
		m_Slider->Invalidate();
	}

	for(i = 0; i < m_Labels.GetSize(); i++)
	{
		CSkinLabel* m_Label = m_Labels.GetAt(i);
		m_Label->Invalidate();
	}

	
	//
	//TRACE0("OnPaint()\r\n");
	//Invalidate();
	//UpdateWindow();
}

BOOL CSkinDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (_access(m_SkinName, 0) == 0)
		LoadSkins();		
	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSkinDialog::SetMode2( bool b, LPCRECT rt)	
{ 
	m_UseMode2 = b; 

	int i;
	for ( i=0; i<m_Buttons.GetSize(); i++)
		m_Buttons.GetAt(i)->SetMode2(b);
	for ( i=0; i<m_Labels.GetSize(); i++)
		m_Labels.GetAt(i)->SetMode2(b);
	for ( i=0; i<m_Progress.GetSize(); i++)
		m_Progress.GetAt(i)->SetMode2(b);

	if ( rt)
		InvalidateRect(rt); 
	else
		Invalidate();
}

void CSkinDialog::ReadButtonInfo(CIniFile m_File)
{
	CStringArray m_BtnInfo;
	
	m_File.ReadSection("BUTTONINFO", m_BtnInfo);
	for(int i = 0; i < m_BtnInfo.GetSize(); i++)
	{
		CString ident;
		ident.Format("%d", i + 1);
		CString mBtnName = m_File.ReadString("BUTTONINFO", ident, "");
		
		if (!mBtnName.IsEmpty())
		{
			CString mName = GetFirstParam(mBtnName);
			CRect r = StringToRect(mBtnName);
			CString m_TTip = GetFirstParam(mBtnName);
			CString m_Check = GetFirstParam(mBtnName);
			CString m_bBorder = GetFirstParam(mBtnName);
			CString m_bVisible = GetFirstParam(mBtnName);

			CSkinButton* m_Button;
			m_Button = new CSkinButton;
			if( m_bBorder=="TRUE" )
			{
				
				m_Button->Create("", WS_VISIBLE | WS_BORDER , r, this, 0);
			}
			else
			{
				if( m_bVisible=="FALSE")
					m_Button->Create("", WS_DISABLED  , r, this, 0);	
				else
					m_Button->Create("", WS_VISIBLE  , r, this, 0);		
				m_Button->CopyFrom(r, m_Normal, m_Over, m_Down, m_Disabled );
				m_Button->CopyFrom2(r, m_Normal2, m_Over2, m_Down2, m_Disabled2 );
			}
			
				m_Button->m_ID = i;
				m_Button->m_IDName = mName;
				m_Button->SetTooltipText(&m_TTip);
				m_Button->m_CheckedButton = (!m_Check.IsEmpty() && (m_Check.GetAt(0) == 'T'));
				
		
				m_Buttons.Add(m_Button);
			
		}
	}
}

void CSkinDialog::ReadProgressInfo(CIniFile m_File)
{
	CStringArray m_BtnInfo;
	
	m_File.ReadSection("PROGRESSINFO", m_BtnInfo);
	for(int i = 0; i < m_BtnInfo.GetSize(); i++)
	{
		CString ident;
		ident.Format("%d", i + 1);
		CString mBtnName = m_File.ReadString("PROGRESSINFO", ident, "");
		if (!mBtnName.IsEmpty())
		{
			CString mName = GetFirstParam(mBtnName);
			CString m_TTip = GetFirstParam(mBtnName);
			CRect r = StringToRect(mBtnName);
			CString m_Dir = GetFirstParam(mBtnName);

			CSkinProgress* m_Button = new CSkinProgress;

			m_Button->Create("", WS_VISIBLE|WS_BORDER, r, this, 0);
			m_Button->CopyFrom(r, m_Normal, m_Down );
			m_Button->CopyFrom2(r, m_Normal2, m_Down2 );
			m_Button->m_ID = i;
			m_Button->m_IDName = mName;
			m_Button->SetTooltipText(&m_TTip);
			m_Button->m_Horizantal = m_Dir[0] == 'H';
			m_Progress.Add(m_Button);
		}
	}
}

void CSkinDialog::ReadTrackBarInfo(CIniFile m_File)
{
	CStringArray m_SlidersInfo;
	
	m_File.ReadSection("TRACKBARINFO", m_SlidersInfo);
	for(int i = 0; i < m_SlidersInfo.GetSize(); i++)
	{
		CString ident;
		ident.Format("%d", i + 1);
		CString mBtnName = m_File.ReadString("TRACKBARINFO", ident, "");
		if (!mBtnName.IsEmpty())
		{
			CString mPath(m_SkinName);

			CString mName = GetFirstParam(mBtnName);
			CString mThumbN = GetFirstParam(mBtnName);
			CString mThumbD = GetFirstParam(mBtnName);

			CRect r = StringToRect(mBtnName);

			CString mThumbDir = GetFirstParam(mBtnName);
			CString mThumbDef = GetFirstParam(mBtnName);
			CString mThumbTT = GetFirstParam(mBtnName);

			CSkinSlider* m_Slider = new CSkinSlider;

			m_Slider->Create(WS_VISIBLE|WS_BORDER, r, this, 0);
			m_Slider->CopyFrom(r, m_Normal, getPath(mPath) + mThumbN, getPath(mPath) + mThumbD);
			m_Slider->m_ID = i;
			m_Slider->m_IDName = mName;

			m_Slider->SetPos(atoi(mThumbDef));
			if(mThumbDir[0] != 'H')
				m_Slider->ModifyStyle(0, TBS_VERT);
			m_Sliders.Add(m_Slider);

		}
	}
}

void CSkinDialog::ReadTextInfo(CIniFile m_File)
{
	CStringArray m_TextInfo;
	
	m_File.ReadSection("TEXTINFO", m_TextInfo);
	for(int i = 0; i < m_TextInfo.GetSize(); i++)
	{
		CString ident;
		ident.Format("%d", i + 1);
		CString mBtnName = m_File.ReadString("TEXTINFO", ident, "");
		if (!mBtnName.IsEmpty())
		{
			CString mName = GetFirstParam(mBtnName);
			CString mFName = GetFirstParam(mBtnName);
			CString mBold = GetFirstParam(mBtnName);
			CString mItalic = GetFirstParam(mBtnName);
			CString mSize = GetFirstParam(mBtnName);
			CString mColor = GetFirstParam(mBtnName);

			CRect r = StringToRect(mBtnName);
			CString mDef = GetFirstParam(mBtnName);
			CString mToolT = GetFirstParam(mBtnName);

			CString mAlign = GetFirstParam(mBtnName); // jason
			CString bBorder = GetFirstParam(mBtnName);

			CSkinLabel* m_Label = new CSkinLabel;

			if( bBorder=="TRUE" )
				m_Label->Create(mDef, WS_VISIBLE | WS_BORDER , r, this, 0);
			else
				m_Label->Create(mDef, WS_VISIBLE , r, this, 0);

			m_Label->CopyFrom(r, m_Normal );
			m_Label->CopyFrom2(r, m_Normal2 );
			m_Label->m_ID = i;
			m_Label->m_IDName = mName;
			m_Label->SetText(mDef);
			m_Label->m_TextColor = atol(mColor);

			m_Label->m_textAlign = (mAlign=="C")? DT_CENTER : (mAlign=="L")? DT_LEFT: DT_RIGHT; // jason


			CFont mFont;
			mFont.CreatePointFont(atoi(mSize), mFName);
			LOGFONT lf;
			strcpy(lf.lfFaceName, mFName);
			ZeroMemory(&lf, sizeof(lf));
			mFont.GetLogFont(&lf);
			lf.lfItalic = mItalic[0] == 'F';
			if(mBold[0] == 'T')
				lf.lfWeight = FW_BOLD;
			else
				lf.lfWeight = FW_NORMAL;
			lf.lfHeight = atoi(mSize);//mFntSize;
			mFont.DeleteObject();
			mFont.CreateFontIndirect(&lf);
			m_Label->SetFont(mFont);
			m_Labels.Add(m_Label);
		}
	}
}

void CSkinDialog::ButtonPressed(CString m_ButtonName)
{
}

void CSkinDialog::PressButton(int m_ID)
{
	CSkinButton* mBtn = m_Buttons.GetAt(m_ID);
	ButtonPressed(mBtn->m_IDName);
}

void CSkinDialog::TrackChange(CString m_ButtonName, UINT nSBCode, UINT nPos)
{
}

void CSkinDialog::ChangeTrack(int m_ID, UINT nSBCode, UINT nPos)
{
	CSkinSlider* mSlider = m_Sliders.GetAt(m_ID);
	TrackChange(mSlider->m_IDName, nSBCode, nPos);
}

void CSkinDialog::TextClicked(CString m_TextName)
{
}

void CSkinDialog::ClickText(int m_ID)
{
	CSkinLabel* mLabel = m_Labels.GetAt(m_ID);
	TextClicked(mLabel->m_IDName);
}



void CSkinDialog::ProgresChanged(CString m_Name)
{

}

void CSkinDialog::ChangeProgress(int m_ID)
{
	CSkinProgress* m_Prgr = m_Progress.GetAt(m_ID);
	ProgresChanged(m_Prgr->m_IDName);
}

void CSkinDialog::SetTrackBarPos(CString m_ID, int m_newPos)
{
	for(int i = 0; i < m_Sliders.GetSize(); i++)
	{
		CSkinSlider* m_Slider = (CSkinSlider*) m_Sliders.GetAt(i);
		if(m_Slider->m_IDName == m_ID)
		{
			m_Slider->SetPos(m_newPos);
			m_Slider->Invalidate();
			break;
		}

	}
}

void CSkinDialog::SetProgressPos(CString m_ID, int m_newPos)
{
	for(int i = 0; i < m_Progress.GetSize(); i++)
	{
		CSkinProgress* m_Prg = (CSkinProgress*) m_Progress.GetAt(i);
		if(m_Prg->m_IDName == m_ID)
		{
			m_Prg->SetPos(m_newPos);
			break;
		}
	}
}

int CSkinDialog::GetProgressPos(CString m_ID)
{
	for(int i = 0; i < m_Progress.GetSize(); i++)
	{
		CSkinProgress* m_Prg = (CSkinProgress*) m_Progress.GetAt(i);
		if(m_Prg->m_IDName == m_ID)
		{
			return m_Prg->GetPos();
		}
	}
	return 0;
}

void CSkinDialog::SetText(CString m_ID, CString m_newText)
{

	for(int i = 0; i < m_Labels.GetSize(); i++)
	{
		CSkinLabel* m_Label = (CSkinLabel*) m_Labels.GetAt(i);
		if(m_Label->m_IDName == m_ID)
		{
			m_Label->SetText(m_newText);
			break;
		}

	}
}

void CSkinDialog::MouseMoved(CString m_ButtonName, int x, int y)
{

}

void CSkinDialog::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnMouseMove(nFlags, point);
	if( m_bDragging)
	{
		CRect r;
		int x,y;
		
		GetWindowRect( &r);

		x = r.left-(m_ptDrag.x-point.x);
		y = r.top-(m_ptDrag.y-point.y);

		
#if 1 // shinjan
		SetWindowPos(NULL,x, y, 0,0,SWP_NOSIZE|SWP_NOZORDER );
#endif
		DragWindow(x,y);
		//Invalidate();
	}	
	//MouseMoved("", point.x, point.y);
	//TRACE0("CSkinDialog::OnMouseMove()\n");
}

void CSkinDialog::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_MenuID > 0)
	{
		CMenu popMenu;

		popMenu.LoadMenu(m_MenuID);
	
		CPoint posMouse;
		GetCursorPos(&posMouse);

		popMenu.GetSubMenu(0)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);
	} 
	else CDialog::OnRButtonDown(nFlags, point);
}

CString CSkinDialog::GetDisplayText(CString m_IDName)
{
	for(int i = 0; i < m_Labels.GetSize(); i++)
	{
		CSkinLabel* m_Lbl = m_Labels.GetAt(i);
		if(m_Lbl->m_IDName == m_IDName)
		{
			CString m_RStr;
			m_Lbl->GetWindowText(m_RStr);
			return m_RStr;
		}
	}
	return "";
}

int CSkinDialog::GetTrackBarPos(CString m_IDName)
{
	for(int i = 0; i < m_Sliders.GetSize(); i++) 
	{
		CSkinSlider* m_Slider = m_Sliders.GetAt(i);
		if(m_Slider->m_IDName == m_IDName)
		{
			return m_Slider->GetPos();
		}
	}
	return 0;
}

void CSkinDialog::SetButtonEnable(CString m_IDName, BOOL m_Enable)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == m_IDName)
		{
			m_Button->EnableWindow(m_Enable);
			if(!m_Enable)
				m_Button->ActivateTooltip(TRUE);
			//m_Button->Invalidate();
			break;
		}
	}
}

BOOL CSkinDialog::GetButtonCheck(CString m_IDName)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == m_IDName)
		{
			return m_Button->m_Check;
		}
	}
	return FALSE;
}

void CSkinDialog::SetButtonCheck(CString m_IDName, BOOL m_NewValue)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == m_IDName)
		{
			m_Button->SetCheck(m_NewValue);
			break;
		}
	}
}

void CSkinDialog::SetButtonToolTip(CString m_IDName, CString m_NewToolTip)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == m_IDName)
		{
			m_Button->SetTooltipText(&m_NewToolTip);
			break;
		}
	}
}

#if 0
void CSkinDialog::SetButtonText(CString m_IDName, CString m_NewToolTip)
{
	// bitmap button no use button_text.
	/*
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == m_IDName)
		{
			m_Button->SetWindowText(m_NewToolTip) ;
			break;
		}

	}*/
}
#endif
void CSkinDialog::SetTextToolTip(CString m_IDName, CString m_NewToolTip)
{
	for(int i = 0; i < m_Labels.GetSize(); i++)
	{
		CSkinLabel* m_Lbl = m_Labels.GetAt(i);
		if(m_Lbl->m_IDName == m_IDName)
		{
			m_Lbl->SetTooltipText(&m_NewToolTip);
			break;
		}
	}
}

void CSkinDialog::SetProgressToolTip(CString m_IDName, CString m_NewToolTip)
{
	for(int i = 0; i < m_Progress.GetSize(); i++)
	{
		CSkinProgress* m_Prg = (CSkinProgress*) m_Progress.GetAt(i);
		if(m_Prg->m_IDName == m_IDName)
		{
			m_Prg->SetTooltipText(&m_NewToolTip);
			break;
		}
	}
}


BOOL CSkinDialog::GetButtonEnabled(CString btnName)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == btnName)
		{
			return m_Button->IsWindowEnabled();
		}
	}
	return FALSE;

}

void CSkinDialog::SetButtonShow(CString btnName, BOOL bShow)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == btnName)
		{
			m_Button->ShowWindow( bShow?SW_SHOW:SW_HIDE );
		}
	}

}

void CSkinDialog::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if( m_bDragging )
	{
		ReleaseCapture();
		m_bDragging=FALSE;
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CSkinDialog::DragWindow(int x, int y)
{

}
#if 0
void CSkinDialog::OnSize(UINT nType, int cx, int cy) 
{
	// shinjan test 20050929
	//Invalidate();
}
#endif
BOOL CSkinDialog::OnEraseBkgnd(CDC* pDC) 
{
#if 0
	CRect   rect;
    GetClientRect(rect);
	
	CDC dc;
    dc.CreateCompatibleDC(pDC);

    HBITMAP    pbmpOldBmp = NULL;

    pbmpOldBmp = (HBITMAP)::SelectObject(dc.m_hDC, (HBITMAP) GetNormalBmp());

	pDC->SetStretchBltMode( HALFTONE );
    pDC->StretchBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, extend_x, extend_y, SRCCOPY);

	::SelectObject(dc.m_hDC, (HBITMAP) GetNormalBmp());
#endif
#if 1
	RECT r;
	GetClientRect(&r);

	DrawBitmap(pDC, (HBITMAP) GetNormalBmp(), r, TRUE);	
#if 0 // shinjan
	DrawBitmap(pDC, (HBITMAP) GetNormalBmp(), r, FALSE);	
#endif

#endif
	
	return TRUE;
}

void CSkinDialog::SetTextShow( CString m_IDName, BOOL bVal)
{
	for(int i = 0; i < m_Labels.GetSize(); i++)
	{
		CSkinLabel* m_Label = (CSkinLabel*) m_Labels.GetAt(i);
		if(m_Label->m_IDName == m_IDName)
		{
			m_Label->ShowWindow( bVal? SW_SHOW:SW_HIDE );
			break;
		}
	}
}

BOOL CSkinDialog::AddCustSkinButton(LPCTSTR idName, CRect r, 
									UINT idBmpNormal, UINT idBmpOver, 
									UINT idBmpDown, UINT idBmpDisabled, 
									LPCTSTR toolTip)
{
	CSkinButton* m_Button = new CSkinButton;

	m_Button->Create("", WS_VISIBLE | WS_BORDER, r, this, 0);

	CBitmap bmpN, bmpO, bmpDn, bmpDb;
	CString tltip = toolTip;

#ifdef	_PCAUA_Res_
	LoadBitmap_(&bmpN, idBmpNormal);
	LoadBitmap_(&bmpO, idBmpOver);
	LoadBitmap_(&bmpDn, idBmpDown);
	LoadBitmap_(&bmpDb, idBmpDisabled);
#else
	bmpN.LoadBitmap(idBmpNormal);
	bmpO.LoadBitmap(idBmpOver);
	bmpDn.LoadBitmap(idBmpDown);
	bmpDb.LoadBitmap(idBmpDisabled);
#endif
	m_Button->CopyFrom(r, bmpN, bmpO, bmpDn, bmpDb);
	m_Button->CopyFrom2(r, bmpN, bmpO, bmpDn, bmpDb);
	m_Button->m_ID = m_Buttons.GetSize();
	m_Button->m_IDName = idName;
	m_Button->SetTooltipText(&tltip);
	m_Button->m_CheckedButton = FALSE;
	m_Button->ShowWindow(SW_HIDE);

	//SetWindowTheme(m_Button->m_hWnd," ", " " );
	m_Buttons.Add(m_Button);

	return TRUE;
}

void CSkinDialog::SetButtonPos(LPCTSTR btnName, int x, int y)
{
	for(int i = 0; i < m_Buttons.GetSize(); i++)
	{
		CSkinButton* m_Button = m_Buttons.GetAt(i);
		if(m_Button->m_IDName == btnName)
		{
			m_Button->SetWindowPos(NULL, x, y,0,0,SWP_NOSIZE|SWP_NOZORDER);
		}
	}

}

BOOL CSkinDialog::AddCustSkinButton(LPCTSTR idName, CRect r, UINT idBmp, LPCTSTR toolTip)
{
	CSkinButton* m_Button = new CSkinButton;
	CBitmap bmpAll, bmpN, bmpO, bmpDn, bmpDb;
	CString tltip = toolTip;
	int btnWidth = r.Width();

	m_Button->Create("", WS_VISIBLE | WS_BORDER, r, this, 0);

#ifdef	_PCAUA_Res_
	LoadBitmap_(&bmpAll, idBmp);
#else
	bmpAll.LoadBitmap(idBmp);
#endif

	CDC* dc = GetDC();
	CopyBitmap( dc, bmpN, bmpAll, r );
	r.OffsetRect(btnWidth,0);
	CopyBitmap( dc, bmpO, bmpAll, r );
	r.OffsetRect(btnWidth,0);
	CopyBitmap( dc, bmpDn, bmpAll, r );
	r.OffsetRect(btnWidth,0);
	CopyBitmap( dc, bmpDb, bmpAll, r );
	ReleaseDC(dc);

	r.left=0;
	r.right=btnWidth;
	m_Button->CopyFrom(r, bmpN, bmpO, bmpDn, bmpDb);
	m_Button->CopyFrom2(r, bmpN, bmpO, bmpDn, bmpDb);
	m_Button->m_ID = m_Buttons.GetSize();
	m_Button->m_IDName = idName;
	m_Button->SetTooltipText(&tltip);
	m_Button->m_CheckedButton = FALSE;
	m_Button->ShowWindow(SW_HIDE);

	//SetWindowTheme(m_Button->m_hWnd," ", " " );
	m_Buttons.Add(m_Button);

	return TRUE;
}


