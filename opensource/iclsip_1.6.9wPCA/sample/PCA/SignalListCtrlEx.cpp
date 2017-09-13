// ListCtrlEx.cpp : implementation file
//

#include "stdafx.h"
#include "SignalListCtrlEx.h"


// CListCtrlEx

IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)
CListCtrlEx::CListCtrlEx() : m_ProgressColumn(0), m_ProgressAccording(0)
{
	m_bFirst = FALSE;
}

CListCtrlEx::~CListCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CListCtrlEx message handlers


void CListCtrlEx::OnPaint()
{
	// TODO: Add your message handler code here
	// Do not call CListCtrl::OnPaint() for painting messages

	int Top=GetTopIndex();
	int Total=GetItemCount();
	int PerPage=GetCountPerPage();
	int LastItem=((Top+PerPage)>Total)?Total:Top+PerPage;

	if( 1 )
	{
		m_bFirst = TRUE;
		
			// if the count in the list os nut zero delete all the progress controls and them procede
		{
			int Count=(int)m_ProgressList.GetSize();// GetCount();
			for(int i=0;i<Count;i++)
			{
			CProgressCtrl* pControl=m_ProgressList.GetAt(0);
			pControl->DestroyWindow();
			m_ProgressList.RemoveAt(0);
			
			}
		}

		CHeaderCtrl* pHeader=GetHeaderCtrl();
		for(int i=Top;i<LastItem;i++)
		{
		CRect ColRt;
		pHeader->GetItemRect(m_ProgressColumn,&ColRt);
		// get the rect
		CRect rt;
		GetItemRect(i,&rt,LVIR_LABEL);
		rt.top+=1;
		rt.bottom-=1;
		rt.left+=ColRt.left;
		int Width=ColRt.Width();
		rt.right=rt.left+Width-4;
		/*
		rt.left=ColRt.left+1;
		rt.right=ColRt.right-1;
		*/

		// create the progress control and set their position
		CProgressCtrl* pControl=new CProgressCtrl();
		pControl->Create(NULL,rt,this,IDC_PROGRESS_LIST+i);

		CString Data=GetItemText(i,m_ProgressAccording);
		int Percent=atoi(Data);

		// set the position on the control
		pControl->SetPos(Percent);
		//pControl->ShowWindow(SW_SHOWNORMAL);
		

		// add them to the list
		m_ProgressList.Add(pControl);
		}
	}
	else
	{
	
	// if the count in the list os nut zero delete all the progress controls and them procede
	{
		int Count=(int)m_ProgressList.GetSize();// GetCount();
		for(int i=0;i<Count;i++)
		{
			//CProgressCtrl* pControl=m_ProgressList.GetAt(0);
			//pControl->DestroyWindow();
			//m_ProgressList.RemoveAt(0);
			//CProgressCtrl* pControl=m_ProgressList.GetAt(i);
			//pControl->ShowWindow(SW_HIDE);
		}
	}

	CHeaderCtrl* pHeader=GetHeaderCtrl();
	for(int i=Top;i<LastItem;i++)
	{
		CRect ColRt;
		pHeader->GetItemRect(m_ProgressColumn,&ColRt);
		// get the rect
		CRect rt;
		GetItemRect(i,&rt,LVIR_LABEL);
		rt.top+=1;
		rt.bottom-=1;
		rt.left+=ColRt.left;
		int Width=ColRt.Width();
		rt.right=rt.left+Width-4;
		/*
		rt.left=ColRt.left+1;
		rt.right=ColRt.right-1;
		*/

		// create the progress control and set their position
		//CProgressCtrl* pControl=new CProgressCtrl();
		//pControl->Create(NULL,rt,this,IDC_PROGRESS_LIST+i);

		CString Data=GetItemText(i,m_ProgressAccording);
		int Percent=atoi(Data);

		// set the position on the control
		//pControl->SetPos(Percent);
		//pControl->ShowWindow(SW_SHOWNORMAL);
		CProgressCtrl* pControl=m_ProgressList.GetAt(i);
		pControl->ShowWindow(SW_SHOWNORMAL);

		// add them to the list
		//m_ProgressList.Add(pControl);
	}

	}
	
	CListCtrl::OnPaint();
}

void CListCtrlEx::InitProgressColumn(int ColNum/*=0*/,int According/*=0*/)
{
	m_ProgressColumn=ColNum;
	m_ProgressAccording=According;
}