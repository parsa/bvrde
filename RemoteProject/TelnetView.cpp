
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
// Constructor/destructor

CTelnetView::CTelnetView() :
  m_pShell(NULL),
  m_dwFlags(0UL),
  m_iStart(0),
  m_clrBack(RGB(120,120,200)),
  m_clrText(RGB(255,255,255))
{
}

CTelnetView::~CTelnetView()
{
   ATLASSERT(m_pShell==NULL);
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CTelnetView::Init(CShellManager* pShell, DWORD dwFlags /*= 0*/)
{
   ATLASSERT(pShell);
   Close();
   Clear();
   m_pShell = pShell;
   m_dwFlags = dwFlags;
   m_pShell->AddLineListener(this);
}

void CTelnetView::Close()
{
   if( m_pShell ) m_pShell->RemoveLineListener(this);
   m_pShell = NULL;
}

void CTelnetView::SetFlags(DWORD dwFlags)
{
   m_dwFlags = dwFlags;
}

void CTelnetView::SetColors(COLORREF clrText, COLORREF clrBack)
{
   m_clrText = clrText;
   m_clrBack = clrBack;
}

void CTelnetView::Clear()
{
   CLockLineDataInit lock;
   m_aLines.RemoveAll();
   LINE line = { VT100_DEFAULT, 0 };
   for( int i = 0; i < MAX_LINES; i++ ) m_aLines.Add(line);
   if( IsWindow() ) Invalidate();
}

void CTelnetView::DoPaint(CDCHandle dc)
{
   CLockLineDataInit lock;

   CClientRect rcClient = m_hWnd;

   SIZE sizePage;
   GetScrollSize(sizePage);     
   RECT rcPage = rcClient;
   rcPage.bottom = max(rcClient.bottom, sizePage.cy);

   dc.SetBkMode(OPAQUE);
   dc.SetBkColor(m_clrBack);

   HFONT hOldFont = dc.SelectFont(m_font);
   for( int i = 0; i < m_aLines.GetSize(); i++ ) {
      const LINE& line = m_aLines[i];
      dc.SetTextColor(m_clrText);
      if( line.nColor == VT100_RED ) dc.SetTextColor(RGB(180,60,50));
      if( line.nColor == VT100_GREEN ) dc.SetTextColor(RGB(50,120,50));
      dc.DrawText(line.szText, min(_tcslen(line.szText), MAX_CHARS), &rcPage, DT_SINGLELINE | DT_LEFT | DT_TOP | DT_NOPREFIX | DT_NOCLIP);
      rcPage.top += m_tm.tmHeight;
   }
   dc.SelectFont(hOldFont);
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CTelnetView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ATLASSERT(m_pShell);
   
   m_font = AtlGetStockFont(SYSTEM_FIXED_FONT);

   CClientDC dc(m_hWnd);
   HFONT hOldFont = dc.SelectFont(m_font);
   dc.GetTextMetrics(&m_tm);
   dc.SelectFont(hOldFont);

   SetScrollSize(m_tm.tmAveCharWidth * MAX_CHARS, m_tm.tmHeight * m_aLines.GetSize(), FALSE);
   SetScrollOffset(0, m_tm.tmHeight * MAX_LINES);

   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( m_pShell ) {
      m_pShell->RemoveLineListener(this);
      if( (m_dwFlags & TNV_TERMINATEONCLOSE) != 0 ) {
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
   if( m_pShell ) {
      if( (m_dwFlags & TNV_TERMINATEONCLOSE) != 0 ) {
         _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, _T(""), FALSE);
         m_pShell->Stop();
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CTelnetView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CDCHandle dc = (HDC) wParam;
   RECT rcClip;
   dc.GetClipBox(&rcClip);
   dc.FillSolidRect(&rcClip, m_clrBack);
   return TRUE; // We're done the painting
}

LRESULT CTelnetView::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   MINMAXINFO* pMMI = (MINMAXINFO*) lParam;
   SIZE size;
   GetScrollSize(size);
   pMMI->ptMaxTrackSize.x = size.cx + 
      (2 * ::GetSystemMetrics(SM_CXSIZEFRAME)) + 
      (2 * ::GetSystemMetrics(SM_CXEDGE)) +
      ::GetSystemMetrics(SM_CXVSCROLL);
   pMMI->ptMaxTrackSize.y = size.cy + 
      ::GetSystemMetrics(SM_CYCAPTION) + 
      (2 * ::GetSystemMetrics(SM_CYSIZEFRAME)) + 
      (2 * ::GetSystemMetrics(SM_CYEDGE)) + 
      ::GetSystemMetrics(SM_CYHSCROLL);
   return 0;
}

LRESULT CTelnetView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SetFocus();
   POINT pt;
   ::GetCursorPos(&pt);
   CMenu menu;
   menu.LoadMenu(IDR_TELNET);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE, this);
   if( nCmd != 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
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
   AtlSetClipboardText(m_hWnd, sText);
   return 0;
}


/////////////////////////////////////////////////////////////////////////
// IOutputLineListener 

void CTelnetView::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   CLockLineDataInit lock;

   LINE newline = { nColor, 0 };
   _tcsncpy(newline.szText, pstrText, MAX_CHARS);

   int nCount = m_aLines.GetSize();
   for( int i = 0; i < nCount - 1; i++ ) {
      LINE& line = m_aLines[i + 1];
      m_aLines.SetAtIndex(i, line);
   }
   m_aLines.SetAtIndex(nCount - 1, newline);

   Invalidate();
}

// IIdleListener

void CTelnetView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_EDIT_COPY, TRUE);
   pUIBase->UIEnable(ID_EDIT_CLEAR, TRUE);
   pUIBase->UIEnable(ID_VIEW_CLOSE, TRUE);
}

void CTelnetView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}
