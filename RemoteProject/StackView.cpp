
#include "StdAfx.h"
#include "resource.h"

#include "StackView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CStackView::CStackView() :
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CStackView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   m_dwCurThread = 1;
}

bool CStackView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CStackView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("cwd")) == 0 ) 
   {
      m_ctrlStack.ResetContent();
      m_ctrlThreads.ResetContent();
   }
   else if( _tcscmp(pstrType, _T("thread-ids")) == 0 ) 
   {
      m_ctrlThreads.ResetContent();
      CString sValue = info.GetItem(_T("thread-id"));
      while( !sValue.IsEmpty() ) {
         CString sText;
         sText.Format(IDS_THREAD, sValue, sValue);
         int iItem = m_ctrlThreads.AddString(sText);
         DWORD dwThreadId = _ttol(sValue);
         m_ctrlThreads.SetItemData(iItem, dwThreadId);
         if( m_dwCurThread == dwThreadId ) m_ctrlThreads.SetCurSel(iItem);
         sValue = info.FindNext(_T("thread-id"));
      }
      if( m_ctrlThreads.GetCurSel() == -1 ) m_ctrlThreads.SetCurSel(0);
   }
   else if( _tcscmp(pstrType, _T("stack")) == 0 
            || _tcscmp(pstrType, _T("new-thread-id")) == 0 )
   {
      CString sValue = info.GetItem(_T("new-thread-id"));
      if( !sValue.IsEmpty() ) _SelectThread(_ttol(sValue));
      m_ctrlStack.ResetContent();
      CString sLevel = info.GetItem(_T("level"));
      while( !sLevel.IsEmpty() ) {
         CString sFunction = info.GetSubItem(_T("func"));
         CString sAddr = info.GetSubItem(_T("addr"));
         CString sFilename = info.GetSubItem(_T("file"));
         if( sFilename.IsEmpty() ) sFilename = info.GetSubItem(_T("from"));
         long iLineNum = _ttoi(info.GetSubItem(_T("line")));
         CString sText;
         sText.Format(IDS_STACKLINE, sFunction, sFilename, iLineNum);
         if( iLineNum == 0 ) sText.Format(IDS_STACKLINE3, sFunction, sFilename, sAddr);
         if( sFunction == _T("??") ) sText.Format(IDS_STACKLINE2, sFunction, sAddr);
         m_ctrlStack.AddString(sText);
         sLevel = info.FindNext(_T("level"));
      }
   }
   else if( _tcscmp(pstrType, _T("stopped")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("thread-id"));
      if( !sValue.IsEmpty() ) _SelectThread(_ttol(sValue));
   }
}


/////////////////////////////////////////////////////////////////////////
// Implementation

void CStackView::_SelectThread(long lThreadId)
{
   int nCount = m_ctrlThreads.GetCount();
   for( int i = 0; i < nCount; i++ ) {
      if( (long) m_ctrlThreads.GetItemData(i) == lThreadId ) {
         if( m_ctrlThreads.GetCurSel() != i ) m_ctrlThreads.SetCurSel(i);
         m_dwCurThread = (DWORD) lThreadId;
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CStackView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   SetFont(AtlGetDefaultGuiFont());
   m_ctrlThreads.Create(m_hWnd, &rcDefault, NULL, WS_CHILD | WS_VISIBLE | CBS_SORT | CBS_DROPDOWNLIST, 0, IDC_TYPE);
   ATLASSERT(m_ctrlThreads.IsWindow());
   m_ctrlStack.Create(m_hWnd, &rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, WS_EX_CLIENTEDGE, IDC_LIST);
   ATLASSERT(m_ctrlStack.IsWindow());
   m_ctrlThreads.SetFont(AtlGetDefaultGuiFont());
   m_ctrlStack.SetFont(AtlGetDefaultGuiFont());
   return 0;
}

LRESULT CStackView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rc = m_hWnd;
   RECT rcTop = { rc.left, rc.top, rc.right, rc.top + 200 };
   m_ctrlThreads.MoveWindow(&rcTop);
   CWindowRect rcCombo = m_ctrlThreads;
   RECT rcBottom = { rc.left, rcCombo.bottom - rcCombo.top, rc.right, rc.bottom };
   m_ctrlStack.MoveWindow(&rcBottom);
   return 0;
}

LRESULT CStackView::OnListDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pProject);
   int iIndex = m_ctrlStack.GetCurSel();
   if( iIndex < 0 ) return 0;
   // Select the stack frame
   CString sCommand;
   sCommand.Format(_T("-stack-select-frame %ld"), iIndex);
   m_pProject->DelayedDebugCommand(sCommand);
   m_pProject->DelayedDebugEvent();
   // Attempt to open the source file
   CString sLine;
   m_ctrlStack.GetText(iIndex, sLine);
   int iFilePos = sLine.Find(_T("file='"));
   int iLinePos = sLine.Find(_T("line="));
   if( iFilePos < 0 || iLinePos < 0 ) return 0;
   CString sFile = sLine.Mid(iFilePos + 6).SpanExcluding(_T("'"));
   int iLineNum = _ttoi(sLine.Mid(iLinePos + 5));
   m_pProject->OpenView(sFile, iLineNum);
   return 0;
}

LRESULT CStackView::OnThreadSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pProject);
   int iIndex = m_ctrlThreads.GetCurSel();
   if( iIndex < 0 ) return 0;
   CString sCommand;
   sCommand.Format(_T("-thread-select %ld"), (long) m_ctrlThreads.GetItemData(iIndex));
   m_pProject->DelayedDebugCommand(sCommand);
   m_pProject->DelayedDebugEvent();
   return 0;
}

void CStackView::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
   int iIndex = lpDIS->itemID;
   if( iIndex == -1 ) return;

   CDCHandle dc = lpDIS->hDC;
   RECT rc = lpDIS->rcItem;
   bool bSelected = (lpDIS->itemState & ODS_SELECTED) != 0;

   CString sText;
   m_ctrlStack.GetText(iIndex, sText);

   COLORREF clrSecond = COLOR_GRAYTEXT;
   int iPosLine = sText.Find(_T("line="));
   if( iPosLine >= 0 ) {
      clrSecond = ::GetSysColor(COLOR_WINDOWTEXT);
      clrSecond = BlendRGB(clrSecond, RGB(0,0,60), 80);
   }

   dc.FillSolidRect(&rc, ::GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));

   CString sSecond;
   int iPosComma = sText.Find(_T("file="));
   if( iPosComma > 0 ) {
      sSecond = sText.Mid(iPosComma);
      sText = sText.Left(iPosComma);
   }

   RECT rcFirst = rc;
   RECT rcSecond = rc;
   dc.SetBkMode(TRANSPARENT);
   dc.SetTextColor(::GetSysColor(bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
   dc.DrawText(sText, -1, &rcFirst, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_CALCRECT);
   dc.DrawText(sText, -1, &rcFirst, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
   if( !sSecond.IsEmpty() ) {
      rcSecond.left = rcFirst.right;
      dc.SetTextColor(bSelected ? (::GetSysColor(COLOR_HIGHLIGHTTEXT)) : clrSecond);
      dc.DrawText(sSecond, -1, &rcSecond, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
   }
}

