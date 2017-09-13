/*----------------------------------------------------------------------
Copyright (C)2001 MJSoft. All Rights Reserved.
          This source may be used freely as long as it is not sold for
					profit and this copyright information is not altered or removed.
					Visit the web-site at www.mjsoft.co.uk
					e-mail comments to info@mjsoft.co.uk
File:     SortListCtrl.h
Purpose:  Provides a sortable list control, it will sort text, numbers
          and dates, ascending or descending, and will even draw the
					arrows just like windows explorer!
----------------------------------------------------------------------*/

#ifndef SORTLISTCTRL_H
#define SORTLISTCTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef SORTHEADERCTRL_H
	#include "SortHeaderCtrl.h"
#endif	// SORTHEADERCTRL_H

#ifdef _DEBUG
	#define ASSERT_VALID_STRING( str ) ASSERT( !IsBadStringPtr( str, 0xfffff ) )
#else	//	_DEBUG
	#define ASSERT_VALID_STRING( str ) ( (void)0 )
#endif	//	_DEBUG

#include <afxtempl.h>				// Let manage CArray
// --------------------------
// -- Definitions of STYLE --		// LIS = List Item Style ;o)
// --------------------------
#define LIS_BOLD          1			// Set the item to BOLD
#define LIS_ITALIC        2			// Set the item to ITALIC
#define LIS_UNDERLINE     4			// Set the item to UNDERLINE
#define LIS_STROKE		  8			// Set the item to STROKE
#define LIS_TXTCOLOR	  16		// Text color is valid and must be set
#define LIS_BGCOLOR	      32		// BackGround color is valid and must be set
#define LIS_NO_COL_STYLE  64        // The Column Style has no effect on this item / subitem
#define LIS_NO_ROW_STYLE  128		// The Row Stylehas no effect on this item / subitem
#define LIS_FIXED_STYLE	  LIS_NO_COL_STYLE | LIS_NO_ROW_STYLE

/*
// -------------------------------------
// -- Definition of LS_item structure --
// -------------------------------------
// This structure allow this derived CListCtrl to store for any items/subitems his own style (bold,color,etc..)
// All styles (item & subitems) for an item are accessible with the helpfull of lParam member.
// But for make the usage transparancy, the "lParam" access method is always available and return the pogrammer value ;o)
// because LS_item structure hold the lParam member before override it, and restore it when needed.
//
typedef struct iLS_item
{	LPARAM lParam;											// The user-32 bits data lParam member
    bool   mParam;											// let you know if the original item have a lParam significant member

   	DWORD StyleFlag;										// The style of this item
  	bool in_use;											// true if a font style is in use (except for colors)

	COLORREF txtColor;										// Text color if LIS_TXTCOLOR style 		(default otherwise)
  	COLORREF bgColor;										// BackGround color if LIS_BGCOLOR style	(default otherwise)

	CArray<struct iLS_item *,struct iLS_item *> subitems;   // Allow to have an individual style for subitems  (this array is empty if it's a subitem structure style instance)
	struct iLS_item * row_style;							// Access to the row style (valid only for the ITEM, subitems structure have NULL on this member)
	struct iLS_item * selected_style;						// The selected style for a component

	CFont * cfont;											// The CFont object pointer used for draw this item or subitem
	bool ifont;												// Allow to know if the CFont is a internal or user Cfont object and allow to know if we must memory manage it !
	CFont * merged_font;									// If a combination of differents font is needed (Style from Columns,Line and Item), this is the last CFont object

} LS_item;
*/
class CSortListCtrl : public CListCtrl
{
// Construction
public:
	CSortListCtrl();

// Attributes
public:

// Operations
public:
	BOOL SetHeadings( UINT uiStringID );
	BOOL SetHeadings( const CString& strHeadings );

	int AddItem( LPCTSTR pszText, ... );
	int AddItem( int bmp,LPCTSTR pszText, ... );
	BOOL DeleteItem( int iItem );
	BOOL DeleteAllItems();
	void LoadColumnInfo();
	void SaveColumnInfo();
	BOOL SetItemText( int nItem, int nSubItem, LPCTSTR lpszText );
	void Sort( int iColumn, BOOL bAscending );
	BOOL SetItemData(int nItem, DWORD dwData);
	DWORD GetItemData(int nItem) const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSortListCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	
//	void SetRowStyle(int nRow,DWORD Style,bool redraw);
//	void InitLVITEM(int nItem,int nSubItem,LVITEM * pItem);
//	void SetRowBgColor(int nRow,COLORREF txtBgColor,bool redraw);
//	void Init_LS_item(LS_item * lpLS_item,bool allow_subitems);
	virtual ~CSortListCtrl();

	// Generated message map functions
protected:
	
	static int CALLBACK CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData );
	void FreeItemMemory( const int iItem );
	BOOL CSortListCtrl::SetTextArray( int iItem, LPTSTR* arrpsz );
	LPTSTR* CSortListCtrl::GetTextArray( int iItem ) const;

	CSortHeaderCtrl m_ctlHeader;

	int m_iNumColumns;
	int m_iSortColumn;
	BOOL m_bSortAscending;

	//{{AFX_MSG(CSortListCtrl)
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // SORTLISTCTRL_H
