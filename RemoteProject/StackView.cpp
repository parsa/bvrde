
#include "StdAfx.h"
#include "resource.h"

#include "StackView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// CStackView

CStackView::CStackView() :
   m_pProject(NULL)
{
}

CStackView::~CStackView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

#pragma code_seg( "VIEW" )

void CStackView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   m_dwCurThread = 1L;
}

bool CStackView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CStackView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("bvrde_init")) == 0 ) 
   {
      if( m_ctrlStack.IsWindow() ) m_ctrlStack.ResetContent();
      if( m_ctrlThreads.IsWindow() ) m_ctrlThreads.ResetContent();
   }
   else if( _tcscmp(pstrType, _T("thread-ids")) == 0 ) 
   {
      m_ctrlThreads.SetRedraw(FALSE);
      m_ctrlThreads.ResetContent();
      // New current thread?
      CString sCurrentId = info.GetItem(_T("current-thread-id"));
      if( !sCurrentId.IsEmpty() ) m_dwCurThread = (DWORD) _ttol(sCurrentId);
      // Populate thread list
      CString sText;
      CString sThreadId = info.GetItem(_T("thread-id"));
      while( !sThreadId.IsEmpty() ) {
         DWORD dwThreadId = (DWORD) _ttol(sThreadId);
         sText.Format(IDS_THREAD, sThreadId, sThreadId);
         int iItem = m_ctrlThreads.AddString(sText);
         m_ctrlThreads.SetItemData(iItem, dwThreadId);
         if( m_dwCurThread == dwThreadId ) m_ctrlThreads.SetCurSel(iItem);
         sThreadId = info.FindNext(_T("thread-id"));
      }
      if( m_ctrlThreads.GetCurSel() == -1 ) m_ctrlThreads.SetCurSel(0);
      m_ctrlThreads.SetRedraw(TRUE);
      m_ctrlThreads.Invalidate();
   }
   else if( _tcscmp(pstrType, _T("threads")) == 0 ) 
   {
      m_ctrlThreads.SetRedraw(FALSE);
      m_ctrlThreads.ResetContent();
      // New current thread?
      CString sCurrentId = info.GetItem(_T("current-thread-id"));
      if( !sCurrentId.IsEmpty() ) m_dwCurThread = (DWORD) _ttol(sCurrentId);
      // Populate thread list
      CString sText;
      CString sThreadId = info.GetItem(_T("id"), _T("threads"));
      while( !sThreadId.IsEmpty() ) {
         DWORD dwThreadId = (DWORD) _ttol(sThreadId);
         CString sTarget = info.GetSubItem(_T("target-id"));
         sText.Format(_T("%ld - %s"), dwThreadId, sTarget.IsEmpty() ? sThreadId : sTarget);
         int iItem = m_ctrlThreads.AddString(sText);
         m_ctrlThreads.SetItemData(iItem, (LPARAM) dwThreadId);
         if( m_dwCurThread == dwThreadId ) m_ctrlThreads.SetCurSel(iItem);
         sThreadId = info.FindNext(_T("id"), _T("threads"));
      }
      if( m_ctrlThreads.GetCurSel() == -1 ) m_ctrlThreads.SetCurSel(0);
      m_ctrlThreads.SetRedraw(TRUE);
      m_ctrlThreads.Invalidate();
   }
   else if( _tcscmp(pstrType, _T("stack")) == 0 
            || _tcscmp(pstrType, _T("new-thread-id")) == 0 )
   {
      // New current thread?
      CString sValue = info.GetItem(_T("new-thread-id"));
      if( !sValue.IsEmpty() ) _SelectThreadId(_ttol(sValue));
      // Populate stack frame list
      CString sLevel = info.GetItem(_T("level"), _T("frame"));
      if( sLevel.IsEmpty() ) return;
      m_ctrlStack.SetRedraw(FALSE);
      m_ctrlStack.ResetContent();
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
         sLevel = info.FindNext(_T("level"), _T("frame"));
      }
      m_ctrlStack.SetRedraw(TRUE);
      m_ctrlStack.Invalidate();
   }
   else if( _tcscmp(pstrType, _T("stopped")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("thread-id"));
      if( !sValue.IsEmpty() ) _SelectThreadId(_ttol(sValue));
   }
}

void CStackView::EvaluateView(CSimpleArray<CString>& aDbgCmd)
{
   // NOTE: We also refresh the thread-list, though it is handled through the CThreadView class too.
   //       @see CRemoteProject::OnProcess

   aDbgCmd.Add(CString(_T("-stack-list-frames")));
}

// Implementation

void CStackView::_SelectThreadId(long lThreadId)
{
   int nCount = m_ctrlThreads.GetCount();
   for( int i = 0; i < nCount; i++ ) {
      if( (long) m_ctrlThreads.GetItemData(i) == lThreadId ) {
         if( m_ctrlThreads.GetCurSel() != i ) m_ctrlThreads.SetCurSel(i);
         break;
      }
   }
   // Make a note of the current thread ID in any case. We may not
   // have refreshed the thread list yet, but when we do we should
   // select the correct thread as current.
   m_dwCurThread = (DWORD) lThreadId;
}

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
   CWaitCursor cursor;
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
   CString sFileStr(MAKEINTRESOURCE(IDS_STACK_FILE));
   CString sLineStr(MAKEINTRESOURCE(IDS_STACK_LINE));
   int iFilePos = sLine.Find(sFileStr);
   int iLinePos = sLine.Find(sLineStr);
   if( iFilePos < 0 || iLinePos < 0 ) return 0;
   CString sFile = sLine.Mid(iFilePos + sFileStr.GetLength()).SpanExcluding(_T("'"));
   int iLineNum = _ttoi(sLine.Mid(iLinePos + sLineStr.GetLength()));
   m_pProject->OpenView(sFile, iLineNum, FINDVIEW_ALL, true);
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

   static CString sFileStr(MAKEINTRESOURCE(IDS_STACK_FILE));
   static CString sLineStr(MAKEINTRESOURCE(IDS_STACK_LINE));

   CString sText;
   m_ctrlStack.GetText(iIndex, sText);

   COLORREF clrSecond = COLOR_GRAYTEXT;
   int iPosLine = sText.Find(sLineStr);
   if( iPosLine >= 0 ) {
      clrSecond = ::GetSysColor(COLOR_WINDOWTEXT);
      clrSecond = BlendRGB(clrSecond, RGB(0,0,60), 80);
   }

   dc.FillSolidRect(&rc, ::GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));

   // Find stuff after function-name. This is where the filename appears.
   //   main(),  file='foo.c', line=1
   CString sSecond;
   int iPosComma = sText.Find(sFileStr);
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

