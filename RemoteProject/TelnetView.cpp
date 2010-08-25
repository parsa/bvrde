
#include "StdAfx.h"
#include "resource.h"

#include "TelnetView.h"

#include "atldataobj.h"


////////////////////////////////////////////////////////
//

static CComAutoCriticalSection g_csLineData;

struct CLockLineDataInit
{
   CLockLineDataInit() { g_csLineData.Lock(); };
   ~CLockLineDataInit() { g_csLineData.Unlock(); };
};


/////////////////////////////////////////////////////////////////////////
// CTelnetView

CTelnetView::CTelnetView() :
  m_pShell(NULL),
  m_dwFlags(0UL),
  m_nLineCount(MAX_DISPLAY_LINES),
  m_clrDefBack(RGB(120,120,200)),
  m_clrBack(RGB(120,120,200)),
  m_clrText(RGB(255,255,255))
{
}

CTelnetView::~CTelnetView()
{
   ATLASSERT(m_pShell==NULL);
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

#pragma code_seg( "VIEW" )

void CTelnetView::Init(CShellManager* pShell)
{
   ATLASSERT(pShell!=NULL);
   Close();
   Clear();
   m_pShell = pShell;
   m_pShell->AddLineListener(this);
}

void CTelnetView::Close()
{
   if( m_pShell != NULL ) m_pShell->RemoveLineListener(this);
   m_pShell = NULL;
}

DWORD CTelnetView::GetFlags() const
{
   return m_dwFlags;
}

void CTelnetView::SetFlags(DWORD dwFlags)
{
   m_dwFlags = dwFlags;
}

void CTelnetView::ModifyFlags(DWORD dwRemove, DWORD dwAdd)
{
   m_dwFlags = (m_dwFlags & ~dwRemove) | dwAdd;
}

void CTelnetView::SetColors(COLORREF clrText, COLORREF clrBack)
{
   m_clrText = clrText;
   m_clrBack = clrBack;
}

void CTelnetView::SetLineCount(int iMaxLines)
{
   m_nLineCount = max(MAX_DISPLAY_LINES, iMaxLines);
   _SetScrollSize();
   Clear();
}

void CTelnetView::Clear()
{
   CLockLineDataInit lock;
   m_aLines.RemoveAll();
   LINE line = { VT100_DEFAULT, 0 };
   for( int i = 0; i < m_nLineCount; i++ ) m_aLines.Add(line);
   if( IsWindow() ) Invalidate();
}

void CTelnetView::DoPaint(CDCHandle dc)
{
   CLockLineDataInit lock;

   CClientRect rcClient = m_hWnd;

   SIZE sizePage = { 0 };
   GetScrollSize(sizePage);     
   RECT rcPage = rcClient;
   rcPage.bottom = max(rcClient.bottom, sizePage.cy);

   dc.SetBkMode(OPAQUE);
   dc.SetBkColor(m_clrBack);

   HFONT hOldFont = dc.SelectFont(m_font);
   for( int i = 0; i < m_aLines.GetSize(); i++ ) {
      const LINE& line = m_aLines[i];
      // NOTE: We pick some psychedelic text colors based on the background-color of the
      //       window. We shouldn't really hardcode it that rudely.
      switch( line.nColor ) {
      case VT100_RED:   dc.SetTextColor(m_clrBack == m_clrDefBack ? RGB(240,220,200) : RGB(180,60,50)); break;
      case VT100_GREEN: dc.SetTextColor(m_clrBack == m_clrDefBack ? RGB(200,240,220) : RGB(30,90,30)); break;
      case VT100_BLUE:  dc.SetTextColor(m_clrBack == m_clrDefBack ? RGB(200,220,240) : RGB(30,30,90)); break;
      default:
         dc.SetTextColor(m_clrText);
         break;
      }
      dc.DrawText(line.szText, min(_tcslen(line.szText), MAX_CHARS), &rcPage, DT_SINGLELINE | DT_LEFT | DT_TOP | DT_NOPREFIX | DT_NOCLIP);
      rcPage.top += m_tm.tmHeight;
   }
   dc.SelectFont(hOldFont);
}

// Message handlers

LRESULT CTelnetView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ATLASSERT(m_pShell);
   
   m_font = AtlGetStockFont(SYSTEM_FIXED_FONT);

   _SetScrollSize();

   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( m_pShell != NULL ) {
      m_pShell->RemoveLineListener(this);
      if( (m_dwFlags & TELNETVIEW_TERMINATEONCLOSE) != 0 ) {
         m_pShell->Stop();
         m_pShell = NULL;
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == 'C' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0 ) m_pShell->WriteSignal(TERMINAL_BREAK);
   if( wParam == 'D' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0 ) m_pShell->WriteSignal(TERMINAL_QUIT);
   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnCompacting(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   // View is being closed. It's not being destroyed, since it's part of
   // the docking framework. The view runs in the background (still gets input)
   // and is ready to display itself if the user activates it from the memu.
   if( (m_dwFlags & TELNETVIEW_SAVEPOS) != 0 ) {
      RECT rcWin = { 0 }; GetWindowRect(&rcWin);
      CString sPos; sPos.Format(_T("%ld,%ld"), rcWin.left, rcWin.top);
      _pDevEnv->SetProperty(_T("window.telnetview.xy"), sPos);
   }
   if( m_pShell != NULL ) {
      if( (m_dwFlags & TELNETVIEW_TERMINATEONCLOSE) != 0 ) {
         _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, _T(""), FALSE);
         m_pShell->Stop();
         m_pShell = NULL;
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CDCHandle dc = (HDC) wParam;
   RECT rcClip = { 0 };
   dc.GetClipBox(&rcClip);
   dc.FillSolidRect(&rcClip, m_clrBack);
   return TRUE; // We've done the painting
}

LRESULT CTelnetView::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   MINMAXINFO* pMMI = (MINMAXINFO*) lParam;
   int cxMax = m_tm.tmAveCharWidth * MAX_CHARS;
   int cyMax = m_tm.tmHeight * MAX_DISPLAY_LINES;
   pMMI->ptMaxTrackSize.x = cxMax + 
      (2 * ::GetSystemMetrics(SM_CXSIZEFRAME)) + 
      (2 * ::GetSystemMetrics(SM_CXEDGE)) +
      ::GetSystemMetrics(SM_CXVSCROLL);
   pMMI->ptMaxTrackSize.y = cyMax + 
      ::GetSystemMetrics(SM_CYCAPTION) + 
      (2 * ::GetSystemMetrics(SM_CYSIZEFRAME)) + 
      (2 * ::GetSystemMetrics(SM_CYEDGE)) + 
      ::GetSystemMetrics(SM_CYHSCROLL);
   return 0;
}

LRESULT CTelnetView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SetFocus();
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   CMenu menu;
   menu.LoadMenu(IDR_TELNET);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE, this);
   if( nCmd != 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
   return 0;
}

LRESULT CTelnetView::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   SetFocus();
   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnViewClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SendMessage(WM_COMPACTING);
   _pDevEnv->AddDockView(m_hWnd, IDE_DOCK_HIDE, CWindow::rcDefault);
   return 0;
}

LRESULT CTelnetView::OnEditClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CLockLineDataInit lock;
   LINE newline = { VT100_BLACK, 0 };
   for( int i = 0; i < m_aLines.GetSize(); i++ ) m_aLines.SetAtIndex(i, newline);
   Invalidate();
   return 0;
}

LRESULT CTelnetView::OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CLockLineDataInit lock;
   CString sText;
   for( int i = 0; i < m_aLines.GetSize(); i++ ) {
      const LINE& line = m_aLines[i];
      sText += line.szText;
      sText += _T("\r\n");
   }
   sText.Replace(_T("\r\n\r\n"), _T("\r\n"));
   AtlSetClipboardText(m_hWnd, sText);
   return 0;
}

LRESULT CTelnetView::OnEditWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_dwFlags ^= TELNETVIEW_WORDWRAP;
   return 0;
}

// Implementation

void CTelnetView::_SetScrollSize()
{
   CClientDC dc(m_hWnd);
   HFONT hOldFont = dc.SelectFont(m_font);
   dc.GetTextMetrics(&m_tm);
   dc.SelectFont(hOldFont);

   SetScrollSize(m_tm.tmAveCharWidth * MAX_CHARS, m_tm.tmHeight * m_nLineCount, FALSE);
   SetScrollOffset(0, m_tm.tmHeight * m_nLineCount);
}

// IOutputLineListener 

void CTelnetView::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   CString sText = pstrText;

   // Filtering out debugger messages?
   // BUG: We're filtering lines indiscriminately. We should really try
   //      to figure out if this is GDB prompt/output or not. 
   //      But this is a little tricky!
   if( (m_dwFlags & TELNETVIEW_FILTERDEBUG) != 0 ) 
   {
      static LPCTSTR aGdbPrompts[] = { _T("&\""), _T("~\""), _T("(gdb"), _T("(dbx") };
      static LPCTSTR aGdbTokens[] = { _T("232^"), _T("232*"), _T("232-"), _T("*running"), _T("=running"), _T("*stopped"), _T("=stopped"), _T("=thread"), _T("=library") };
      int i;
      for( i = 0; i < sizeof(aGdbTokens) / sizeof(aGdbTokens[0]); i++ ) if( sText.Find(aGdbTokens[i]) >= 0 ) return;
      for( i = 0; i < sizeof(aGdbPrompts) / sizeof(aGdbPrompts[0]); i++ ) if( _tcsncmp(sText, aGdbPrompts[i], _tcslen(aGdbPrompts[i])) == 0 ) return;
      // Transform target output...
      // At lease we will try to convert to GDB/MI target-stream-output text to
      // standard console output.
      // TODO: Figure out if we really need this here?
      if( _tcsncmp(sText, _T("@\""), 2) == 0 ) {
         sText = sText.Mid(2, sText.GetLength() - 3);
         sText.Replace(_T("\\\""), _T("\""));
      }
   }

   // Color debug prompts differently
   if( _tcsncmp(sText, _T("(gdb"), 4) == 0 ) nColor = VT100_BLUE;
   if( _tcsncmp(sText, _T("(dbx"), 4) == 0 ) nColor = VT100_BLUE;

   // NOTE: Since lines may very well arrive from another thread we'll
   //       have to protect the data members.
   CLockLineDataInit lock;

   int nCount = m_aLines.GetSize();
   do 
   {
      int nSize = sText.GetLength();
      if( (m_dwFlags & TELNETVIEW_WORDWRAP) != 0 ) nSize = min(nSize, MAX_CHARS);

      LINE newline = { nColor, 0 };
      _tcscpy(newline.szText, sText.Left(MAX_CHARS));

      for( int i = 0; i < nCount - 1; i++ ) {
         LINE& line = m_aLines[i + 1];
         m_aLines.SetAtIndex(i, line);
      }
      m_aLines.SetAtIndex(nCount - 1, newline);

      sText = sText.Mid(nSize);
   } while( !sText.IsEmpty() );

   if( IsWindow() ) Invalidate();
}

// IIdleListener

void CTelnetView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_EDIT_COPY, TRUE);
   pUIBase->UIEnable(ID_EDIT_CLEAR, TRUE);
   pUIBase->UIEnable(ID_EDIT_WORDWRAP, TRUE);
   pUIBase->UIEnable(ID_VIEW_CLOSE, TRUE);
   pUIBase->UISetCheck(ID_EDIT_WORDWRAP, (m_dwFlags & TELNETVIEW_WORDWRAP) != 0);
}

void CTelnetView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

