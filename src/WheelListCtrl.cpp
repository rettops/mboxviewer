// WheelListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "mboxview.h"
#include "WheelListCtrl.h"
#include "NListView.h"

#ifdef _DEBUG
#undef THIS_FILE
#define THIS_FILE __FILE__
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CWheelListCtrl

CWheelListCtrl::CWheelListCtrl(const NListView *cListCtrl)
{
	m_list = cListCtrl;
	int deb = 1;
}

CWheelListCtrl::~CWheelListCtrl()
{
	int deb = 1;
}


BEGIN_MESSAGE_MAP(CWheelListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CWheelListCtrl)
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWheelListCtrl message handlers

BOOL CWheelListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	CWnd *wnd = WindowFromPoint(pt);
	if( wnd == NULL )
		return FALSE;
	if( wnd == this ) {
		return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
	}
	if ((GetKeyState(VK_CONTROL) & 0x80) == 0) { // if CTRL key not Down; Do we need to post msg further anyway
		// Commented out, it freezes mbox viewer and and even IE for few seconds when CTRL/SHIFT/etc key are kept down
		; // return wnd->PostMessage(WM_MOUSEWHEEL, MAKELPARAM(nFlags, zDelta), MAKELPARAM(pt.x, pt.y));
	}
	return TRUE;
}


COLORREF CWheelListCtrl::OnGetCellBkColor(int /*nRow*/, int /*nColum*/) 
{ 
	return GetBkColor(); 
}

void CWheelListCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: Add your message handler code here and/or call default

	CListCtrl::OnDrawItem(nIDCtl, lpDrawItemStruct);

	CDC dc;
	BOOL ret = dc.Attach(lpDrawItemStruct->hDC);

	CRect rect = lpDrawItemStruct->rcItem;
	dc.FillRect(&rect, &CBrush(RGB(255, 255, 255)));
	dc.SetBkMode(TRANSPARENT);

	dc.Detach();

	if ((lpDrawItemStruct->itemID >= 0) && (lpDrawItemStruct->itemID <= 5))
	{
		CStringW txtW;
		if (lpDrawItemStruct->itemID == 0)
			txtW = L"! ";
		else if (lpDrawItemStruct->itemID == 1) {
			txtW = L"date";
			if (m_list->m_gmtTime == 1)
				txtW += L" (GMT) ";
			else
				txtW += L" (Local) ";
		}
		if (lpDrawItemStruct->itemID == 2)
			txtW = L"from ";
		if (lpDrawItemStruct->itemID == 3)
			txtW = L"to ";
		if (lpDrawItemStruct->itemID == 4)
			txtW = L"subject ";
		if (lpDrawItemStruct->itemID == 5)
			txtW = L"size(KB) ";

		int x_offset = 6;
		int xpos = rect.left + x_offset;
		int ypos = rect.top + 6;

		HDC hDC = lpDrawItemStruct->hDC;

		int which_sorted = m_list->MailsWhichColumnSorted();
		if ((lpDrawItemStruct->itemID == abs(which_sorted)) || ((lpDrawItemStruct->itemID == 0) && (abs(which_sorted) == 99)))
		{
			if (which_sorted > 0)
				txtW += L'\x2191';
			else
				txtW += L'\x2193';
		}

		::ExtTextOutW(hDC, xpos, ypos, ETO_CLIPPED, &rect, (LPCWSTR)txtW, txtW.GetLength(), NULL);

		BOOL ret = dc.Attach(lpDrawItemStruct->hDC);
		int nsave = dc.SaveDC();

		CRect rect = lpDrawItemStruct->rcItem;

		xpos = rect.left + rect.Width();
		ypos = rect.top;

		CPen penRed(PS_SOLID, 1, RGB(212, 208, 200));  // gray
		dc.SelectObject(&penRed);

		dc.MoveTo(xpos - 1, ypos);
		dc.LineTo(xpos - 1, rect.bottom);

		xpos = rect.left;
		ypos = rect.top + rect.Height() - 1;

		dc.MoveTo(xpos, ypos);
		//dc.LineTo(rect.right, ypos);  // comment out to draw horizontal line


		dc.RestoreDC(nsave);
		dc.Detach();
	}

	int deb = 1;
}
