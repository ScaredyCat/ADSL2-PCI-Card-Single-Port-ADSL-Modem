/*----------------------------------------------------------------------
Copyright (C)2001 MJSoft. All Rights Reserved.
          This source may be used freely as long as it is not sold for
					profit and this copyright information is not altered or removed.
					Visit the web-site at www.mjsoft.co.uk
					e-mail comments to info@mjsoft.co.uk
File:     SortListCtrl.cpp
Purpose:  Provides a sortable list control, it will sort text, numbers
          and dates, ascending or descending, and will even draw the
					arrows just like windows explorer!
----------------------------------------------------------------------*/

#include "stdafx.h"
#include "SortListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


LPCTSTR g_pszSection = _T("ListCtrls");


struct ItemData
{
public:
	ItemData() : arrpsz( NULL ), dwData( NULL ) {}

	LPTSTR* arrpsz;
	DWORD dwData;

private:
	// ban copying.
	ItemData( const ItemData& );
	ItemData& operator=( const ItemData& );
};


CSortListCtrl::CSortListCtrl()
	: m_iNumColumns( 0 )
	, m_iSortColumn( -1 )
	, m_bSortAscending( TRUE )
{
}


CSortListCtrl::~CSortListCtrl()
{
}


BEGIN_MESSAGE_MAP(CSortListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CSortListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSortListCtrl message handlers

void CSortListCtrl::PreSubclassWindow()
{
	// the list control must have the report style.
	ASSERT( GetStyle() & LVS_REPORT );

	CListCtrl::PreSubclassWindow();
	VERIFY( m_ctlHeader.SubclassWindow( GetHeaderCtrl()->GetSafeHwnd() ) );
}


BOOL CSortListCtrl::SetHeadings( UINT uiStringID )
{
	CString strHeadings;
	VERIFY( strHeadings.LoadString( uiStringID ) );
	return SetHeadings( strHeadings );
}


// the heading text is in the format column 1 text,column 1 width;column 2 text,column 3 width;etc.
BOOL CSortListCtrl::SetHeadings( const CString& strHeadings )
{
	int iStart = 0;

	for( ;; )
	{
		const int iComma = strHeadings.Find( _T(','), iStart );

		if( iComma == -1 )
			break;

		const CString strHeading = strHeadings.Mid( iStart, iComma - iStart );

		iStart = iComma + 1;

		int iSemiColon = strHeadings.Find( _T(';'), iStart );

		if( iSemiColon == -1 )
			iSemiColon = strHeadings.GetLength();

		const int iWidth = atoi( strHeadings.Mid( iStart, iSemiColon - iStart ) );
		
		iStart = iSemiColon + 1;

		if( InsertColumn( m_iNumColumns++, strHeading, LVCFMT_LEFT, iWidth ) == -1 )
			return FALSE;
	}

	return TRUE;
}

int CSortListCtrl::AddItem( int bmp,LPCTSTR pszText, ... )
{
	const int iIndex = InsertItem( GetItemCount(), pszText,bmp );

	LPTSTR* arrpsz = new LPTSTR[ m_iNumColumns ];
	arrpsz[ 0 ] = new TCHAR[ lstrlen( pszText ) + 1 ];
	(void)lstrcpy( arrpsz[ 0 ], pszText );

 	va_list list;
	va_start( list, pszText );

	for( int iColumn = 1; iColumn < m_iNumColumns; iColumn++ )
	{
		pszText = va_arg( list, LPCTSTR );
		ASSERT_VALID_STRING( pszText );
		VERIFY( CListCtrl::SetItem( iIndex, iColumn, LVIF_TEXT, pszText, 0, 0, 0, 0 ) );

		arrpsz[ iColumn ] = new TCHAR[ lstrlen( pszText ) + 1 ];
		(void)lstrcpy( arrpsz[ iColumn ], pszText );
	}

	va_end( list );

	VERIFY( SetTextArray( iIndex, arrpsz ) );

	return iIndex;
}

int CSortListCtrl::AddItem( LPCTSTR pszText, ... )
{	
	const int iIndex = InsertItem( GetItemCount(), pszText );

	LPTSTR* arrpsz = new LPTSTR[ m_iNumColumns ];
	arrpsz[ 0 ] = new TCHAR[ lstrlen( pszText ) + 1 ];
	(void)lstrcpy( arrpsz[ 0 ], pszText );

 	va_list list;
	va_start( list, pszText );

	for( int iColumn = 1; iColumn < m_iNumColumns; iColumn++ )
	{
		pszText = va_arg( list, LPCTSTR );
		ASSERT_VALID_STRING( pszText );
		VERIFY( CListCtrl::SetItem( iIndex, iColumn, LVIF_TEXT, pszText, 0, 0, 0, 0 ) );

		arrpsz[ iColumn ] = new TCHAR[ lstrlen( pszText ) + 1 ];
		(void)lstrcpy( arrpsz[ iColumn ], pszText );
	}

	va_end( list );

	VERIFY( SetTextArray( iIndex, arrpsz ) );

	return iIndex;
}


void CSortListCtrl::FreeItemMemory( const int iItem )
{
	ItemData* pid = reinterpret_cast<ItemData*>( CListCtrl::GetItemData( iItem ) );
	if ( !pid)
		return;

	__try {
		LPTSTR* arrpsz = pid->arrpsz;

		for( int i = 0; i < m_iNumColumns; i++ )
			delete[] arrpsz[ i ];

		delete[] arrpsz;
		delete pid;
	} 
	__except(1) 
	{
	}

	VERIFY( CListCtrl::SetItemData( iItem, NULL ) );
}


BOOL CSortListCtrl::DeleteItem( int iItem )
{
	FreeItemMemory( iItem );
	return CListCtrl::DeleteItem( iItem );
}


BOOL CSortListCtrl::DeleteAllItems()
{
	for( int iItem = 0; iItem < GetItemCount(); iItem ++ )
		FreeItemMemory( iItem );

	return CListCtrl::DeleteAllItems();
}


bool IsNumber( LPCTSTR pszText )
{
	ASSERT_VALID_STRING( pszText );

	for( int i = 0; i < lstrlen( pszText ); i++ )
		if( !_istdigit( pszText[ i ] ) )
			return false;

	return true;
}


int NumberCompare( LPCTSTR pszNumber1, LPCTSTR pszNumber2 )
{
	ASSERT_VALID_STRING( pszNumber1 );
	ASSERT_VALID_STRING( pszNumber2 );

	const int iNumber1 = atoi( pszNumber1 );
	const int iNumber2 = atoi( pszNumber2 );

	if( iNumber1 < iNumber2 )
		return -1;
	
	if( iNumber1 > iNumber2 )
		return 1;

	return 0;
}


bool IsDate( LPCTSTR pszText )
{
	ASSERT_VALID_STRING( pszText );

	// format should be 99/99/9999.

/*
	if( lstrlen( pszText ) != 10 )
		return false;

	return _istdigit( pszText[ 0 ] )
		&& _istdigit( pszText[ 1 ] )
		&& pszText[ 2 ] == _T('/')
		&& _istdigit( pszText[ 3 ] )
		&& _istdigit( pszText[ 4 ] )
		&& pszText[ 5 ] == _T('/')
		&& _istdigit( pszText[ 6 ] )
		&& _istdigit( pszText[ 7 ] )
		&& _istdigit( pszText[ 8 ] )
		&& _istdigit( pszText[ 9 ] );
		*/
	
	
	// format should be 9999/99/99.
	if ( _istdigit(pszText[ 0 ] )&&_istdigit( pszText[ 1 ] )&&_istdigit( pszText[ 2 ] )&&_istdigit( pszText[ 3 ] )&&pszText[ 4] == _T('/') )
		return true;
	else
		return false;
}


int DateCompare( const CString& strDate1, const CString& strDate2 )
{
	// compare year
	const int iYear1 = atoi( strDate1.Mid( 0, 4 ) );
	const int iYear2 = atoi( strDate2.Mid( 0, 4 ) );

	if( iYear1 < iYear2 )
		return -1;

	if( iYear1 > iYear2 )
		return 1;

	//compare month
	const int iMonth1 = atoi( strDate1.Mid( 5, 2 ) );
	const int iMonth2 = atoi( strDate2.Mid( 5, 2 ) );

	if( iMonth1 < iMonth2 )
		return -1;

	if( iMonth1 > iMonth2 )
		return 1;

	//compare day
	const int iDay1 = atoi( strDate1.Mid( 8, 2 ) );
	const int iDay2 = atoi( strDate2.Mid( 8, 2 ) );

	if( iDay1 < iDay2 )
		return -1;

	if( iDay1 > iDay2 )
		return 1;

	
	if(strDate1.GetLength()>14)
	{

		// compare AM or PM
		int p1,p2;
		p1=atoi(strDate1.Mid(20,2));
		p2=atoi(strDate2.Mid(20,2));

		if(p1<p2)
			return -1;
		if(p1>p2)
			return 1;

		//compare hour
		int hour1,hour2;
		hour1=atoi(strDate1.Mid(11,2));
		hour2=atoi(strDate2.Mid(11,2));

		if(hour1<hour2)
			return -1;
		if(hour1>hour2)
			return 1;

		//compare minute
		int minute1,minute2;
		minute1=atoi(strDate1.Mid(14,2));
		minute2=atoi(strDate2.Mid(14,2));

		if (minute1<minute2)
			return -1;
		if (minute1>minute2)
			return 1;

		//compare second
		int second1,second2;
		second1=atoi(strDate1.Mid(17,2));
		second2=atoi(strDate2.Mid(17,2));
		
		if (second1<second2)
			return -1;
		if (second1>second2)
			return 1;
	}

	return 0;
}


int CALLBACK CSortListCtrl::CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData )
{
	CSortListCtrl* pListCtrl = reinterpret_cast<CSortListCtrl*>( lParamData );
	ASSERT( pListCtrl->IsKindOf( RUNTIME_CLASS( CListCtrl ) ) );

	ItemData* pid1 = reinterpret_cast<ItemData*>( lParam1 );
	ItemData* pid2 = reinterpret_cast<ItemData*>( lParam2 );
 
	ASSERT( pid1 );
	ASSERT( pid2 );

	LPCTSTR pszText1 = pid1->arrpsz[ pListCtrl->m_iSortColumn ];
	LPCTSTR pszText2 = pid2->arrpsz[ pListCtrl->m_iSortColumn ];

	ASSERT_VALID_STRING( pszText1 );
	ASSERT_VALID_STRING( pszText2 );

	/*
	if( IsNumber( pszText1 ) )
		return pListCtrl->m_bSortAscending ? NumberCompare( pszText1, pszText2 ) : NumberCompare( pszText2, pszText1 );
	else if( IsDate( pszText1 ) )
		return pListCtrl->m_bSortAscending ? DateCompare( pszText1, pszText2 ) : DateCompare( pszText2, pszText1 );
	else
		// text.
		return pListCtrl->m_bSortAscending ? lstrcmp( pszText1, pszText2 ) : lstrcmp( pszText2, pszText1 );
	*/

	if( IsDate( pszText1 ) )
		return pListCtrl->m_bSortAscending ? DateCompare( pszText1, pszText2 ) : DateCompare( pszText2, pszText1 );
	else
		// text.
		return pListCtrl->m_bSortAscending ? lstrcmp( pszText1, pszText2 ) : lstrcmp( pszText2, pszText1 );

}


void CSortListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	const int iColumn = pNMListView->iSubItem;

	// if it's a second click on the same column then reverse the sort order,
	// otherwise sort the new column in ascending order.
	Sort( iColumn, iColumn == m_iSortColumn ? !m_bSortAscending : TRUE );

	*pResult = 0;
}


void CSortListCtrl::Sort( int iColumn, BOOL bAscending )
{
	m_iSortColumn = iColumn;
	m_bSortAscending = bAscending;

	// show the appropriate arrow in the header control.
	m_ctlHeader.SetSortArrow( m_iSortColumn, m_bSortAscending );

	VERIFY( SortItems( CompareFunction, reinterpret_cast<DWORD>( this ) ) );
}


void CSortListCtrl::LoadColumnInfo()
{
	// you must call this after setting the column headings.
	ASSERT( m_iNumColumns > 0 );

	CString strKey;
	strKey.Format( _T("%d"), GetDlgCtrlID() );

	UINT nBytes = 0;
	BYTE* buf = NULL;
	if( AfxGetApp()->GetProfileBinary( g_pszSection, strKey, &buf, &nBytes ) )
	{
		if( nBytes > 0 )
		{
			CMemFile memFile( buf, nBytes );
			CArchive ar( &memFile, CArchive::load );
			m_ctlHeader.Serialize( ar );
			ar.Close();

			m_ctlHeader.Invalidate();
		}

		delete[] buf;
	}
}


void CSortListCtrl::SaveColumnInfo()
{
	ASSERT( m_iNumColumns > 0 );

	CString strKey;
	strKey.Format( _T("%d"), GetDlgCtrlID() );

	CMemFile memFile;

	CArchive ar( &memFile, CArchive::store );
	m_ctlHeader.Serialize( ar );
	ar.Close();

	DWORD dwLen = memFile.GetLength();
	BYTE* buf = memFile.Detach();	

	VERIFY( AfxGetApp()->WriteProfileBinary( g_pszSection, strKey, buf, dwLen ) );

	free( buf );
}


void CSortListCtrl::OnDestroy() 
{
	for( int iItem = 0; iItem < GetItemCount(); iItem ++ )
		FreeItemMemory( iItem );

	CListCtrl::OnDestroy();
}


BOOL CSortListCtrl::SetItemText( int nItem, int nSubItem, LPCTSTR lpszText )
{
	if( !CListCtrl::SetItemText( nItem, nSubItem, lpszText ) )
		return FALSE;

	LPTSTR* arrpsz = GetTextArray( nItem );
	LPTSTR pszText = arrpsz[ nSubItem ];
	delete[] pszText;
	pszText = new TCHAR[ lstrlen( lpszText ) + 1 ];
	(void)lstrcpy( pszText, lpszText );
	arrpsz[ nSubItem ] = pszText;

	return TRUE;
}


BOOL CSortListCtrl::SetItemData( int nItem, DWORD dwData )
{
	if( nItem >= GetItemCount() )
		return FALSE;

	ItemData* pid = reinterpret_cast<ItemData*>( CListCtrl::GetItemData( nItem ) );
	ASSERT( pid );
	if ( !pid)
		return FALSE;

	pid->dwData = dwData;

	return TRUE;
}


DWORD CSortListCtrl::GetItemData( int nItem ) const
{
	ASSERT( nItem < GetItemCount() );

	ItemData* pid = reinterpret_cast<ItemData*>( CListCtrl::GetItemData( nItem ) );
	ASSERT( pid );
	if ( !pid)
		return FALSE;

	return pid->dwData;
}


BOOL CSortListCtrl::SetTextArray( int iItem, LPTSTR* arrpsz )
{
	ASSERT( CListCtrl::GetItemData( iItem ) == NULL );
	ItemData* pid = new ItemData;
	pid->arrpsz = arrpsz;
	return CListCtrl::SetItemData( iItem, reinterpret_cast<DWORD>( pid ) );
}


LPTSTR* CSortListCtrl::GetTextArray( int iItem ) const
{
	ASSERT( iItem < GetItemCount() );

	ItemData* pid = reinterpret_cast<ItemData*>( CListCtrl::GetItemData( iItem ) );
	return pid->arrpsz;
}

/*
void CSortListCtrl::SetRowBgColor(int nRow, COLORREF txtBgColor, bool redraw)
{	// We must retrieve the Style info structure of this item
	//
	LVITEM pItem;
	InitLVITEM(nRow,0,&pItem);

	LS_item * lpLS_item = NULL;
	lpLS_item = (LS_item*) pItem.lParam;





	// Verify if a style for this Row already exist or not
	//
	LS_item * lpLS_row = NULL;
	lpLS_row = lpLS_item->row_style;

	//if(lpLS_row == NULL)
	//{	// We must create one
		//
		lpLS_row = new LS_item;
		this->Init_LS_item(lpLS_row,false);

		// attach to the item
		//
		lpLS_item->row_style = lpLS_row;
	//}

	// no we can update the style
	//
		lpLS_row->bgColor = txtBgColor;

	// Redraw it
	//if(redraw)	CListCtrl::Update(nRow);
	CListCtrl::Update(nRow);
	CListCtrl::SetItem(&pItem);

}

void CSortListCtrl::InitLVITEM(int nItem, int nSubItem, LVITEM *pItem)
{/*
  typedef struct _LV_ITEM {
    UINT   mask;         // see below
    int    iItem;        // see below
    int    iSubItem;     // see below
    UINT   state;        // see below
    UINT   stateMask;    // see below
    LPSTR  pszText;      // see below
    int    cchTextMax;   // see below
    int    iImage;       // see below
    LPARAM lParam;       // 32-bit value to associate with item
   } LV_ITEM;
 
	pItem->mask = LVIF_PARAM;
	pItem->iItem = nItem;
	pItem->iSubItem = nSubItem;
	pItem->state = NULL;
	pItem->stateMask = NULL;
	pItem->pszText = NULL;
	pItem->cchTextMax = NULL;
	pItem->iImage = NULL;
	pItem->lParam = NULL;
	CListCtrl::GetItem(pItem);

}

void CSortListCtrl::Init_LS_item(LS_item *lpLS_item, bool allow_subitems)
{	// lParam Init
	//
	lpLS_item->lParam = 0;
	lpLS_item->mParam = false;

	// Set Default Style
	//
	lpLS_item->StyleFlag = 0;
	lpLS_item->in_use = false;

	lpLS_item->txtColor = 0;
	lpLS_item->bgColor = 0;
	lpLS_item->cfont = NULL;
	lpLS_item->ifont = false;
	lpLS_item->merged_font = NULL;
	lpLS_item->row_style = NULL;
	lpLS_item->selected_style = NULL;

	// Init the Array for hold SubItems Style
	//
	if(allow_subitems)
	{	int nSubItems = this->GetHeaderCtrl()->GetItemCount();
		if(nSubItems > 0) nSubItems--;
		lpLS_item->subitems.SetSize( nSubItems );
	}
	else
		lpLS_item->subitems.SetSize(0);

	lpLS_item->subitems.RemoveAll();

}

/*
void CSortListCtrl::SetRowStyle(int nRow, DWORD Style, bool redraw)
{	// We must retrieve the Style info structure of this item
	//
	LVITEM pItem;
	InitLVITEM(nRow,0,&pItem);

	LS_item * lpLS_item = NULL;
	lpLS_item = (LS_item*) pItem.lParam;

	// Verify if a style for this Row already exist or not
	//
	LS_item * lpLS_row = NULL;
	lpLS_row = lpLS_item->row_style;

	if(lpLS_row == NULL)
	{	// We must create one
		//
		lpLS_row = new LS_item;
		this->Init_LS_item(lpLS_row,false);

		// attach to the item
		//
		lpLS_item->row_style = lpLS_row;
	}

	// no we can update the style
	//
	lpLS_row->StyleFlag = Style;

	DWORD mask = LIS_BOLD | LIS_ITALIC | LIS_UNDERLINE| LIS_STROKE ;
	lpLS_row->in_use = (Style & mask) > 0;

	// if any font exist for this item then delete it
	//
	//this->Free_LS_font(lpLS_row);

	// Redraw it
	if(redraw)	CListCtrl::Update(nRow);

}
*/

