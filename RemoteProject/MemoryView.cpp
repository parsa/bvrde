
#include "StdAfx.h"
#include "resource.h"

#include "MemoryView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CMemoryView::CMemoryView() :
   m_pProject(NULL),
   m_iDisplaySize(8)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CMemoryView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CMemoryView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CMemoryView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("addr")) == 0 ) 
   {
      CString sText = _T("{\\rtf1\\ansi\\deff0\\deftab720{\\colortbl\\red0\\green0\\blue0;\\red0\\green0\\blue180;\\red130\\green130\\blue130;}\\deflang1033\\pard\\plain\\f0\\fs18 ");
      CString sAddress;
      CString sMemory;
      CString sAscii;
      CString sTemp;
      int iStep = 0;
      CString sValue = info.GetItem(_T("addr"), _T("memory"));
      while( !sValue.IsEmpty() ) {
         if( sAddress.IsEmpty() ) sAddress = sValue;
         sMemory += info.FindNext(_T("data")) + _T(" ");
         sAscii += info.FindNext(_T("ascii"));
         if( (++iStep % (32 / m_iDisplaySize)) == 0 ) {
            sAscii.Replace(_T("\\"), _T("\\\\"));
            sAscii.Replace(_T("{"), _T("\\{"));
            sAscii.Replace(_T("}"), _T("\\}"));
            sTemp.Format(_T("\\cf1 %s  \\cf0 %s   %s\\par"), sAddress, sMemory, sAscii);
            sText += sTemp;
            sAddress = sMemory = sAscii = _T("");
         }
         sValue = info.FindNext(_T("addr"), _T("memory"));
      }
      sText +=  _T("\\par}");
      EDITSTREAM stream = { 0 };
      STREAMCOOKIE cookie = { sText, 0 };
      stream.dwCookie = (DWORD) &cookie;
      stream.pfnCallback = _EditStreamCallback;
      m_ctrlMemory.StreamIn(SF_RTF, stream);
   }
}

// Implementation

DWORD CALLBACK CMemoryView::_EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   STREAMCOOKIE* cookie = (STREAMCOOKIE*) dwCookie;
   *pcb = min(cb, (LONG) _tcslen(cookie->pstr + cookie->pos));
   AtlW2AHelper( (LPSTR) pbBuff, cookie->pstr + cookie->pos, (int) *pcb );
   cookie->pos += *pcb;
   return *pcb >= 0 ? 0 : 1;
}

void CMemoryView::_UpdateDisplay()
{
   CString sAddress = CWindowText(m_ctrlAddress);
   // Default to displaying hexadecimal formatting
   CString sFormat = _T("x");
   int iPos = sAddress.Find(',');
   if( iPos > 0 ) {
      // Might be a different display format on command line
      // Format is:
      //   <expr/address>, <format>
      // where <format> is "x" (hex), "d" (decimal) or "o" (octal)
      sFormat = sAddress.Mid(iPos + 1);
      sAddress = sAddress.Left(iPos);
      sFormat.TrimLeft();
      sAddress.TrimRight();
   }
   if( sAddress.IsEmpty() ) return;
   // Estimate rows needed to fill display
   CClientDC dc = m_hWnd;
   TEXTMETRIC tm = { 0 };
   dc.GetTextMetrics(&tm);
   CClientRect rcClient = m_hWnd;
   long lRows = (rcClient.bottom - rcClient.top) / tm.tmHeight;     
   // Get data from debugger
   CString sCommand;
   sCommand.Format(_T("-data-read-memory \"%s\" %s %ld %ld 1 ."), 
      sAddress, 
      sFormat,
      max(1, m_iDisplaySize / 2),
      (lRows + 3) * (32 / m_iDisplaySize));
   m_pProject->DelayedDebugCommand(sCommand);
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CMemoryView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ModifyStyleEx(WS_EX_CLIENTEDGE, 0);

   COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 10);
   
   m_ctrlAddress.Create(this, 1, m_hWnd, &rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
   m_ctrlAddress.SetFont(AtlGetDefaultGuiFont());
   m_ctrlMemory.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
   m_ctrlMemory.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlMemory.SetBackgroundColor(clrBack);
   m_ctrlMemory.SetTargetDevice(NULL, 1);
   m_ctrlMemory.SetUndoLimit(0);
   
   m_ctrlAddress.SetFocus();
   
   return 0;
}

LRESULT CMemoryView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rcClient = m_hWnd;
   RECT rcAddress = { 0, 0, rcClient.right - rcClient.left, ::GetSystemMetrics(SM_CYCAPTION) };
   RECT rcMemory = { 0, rcAddress.bottom, rcAddress.right, rcClient.bottom - rcClient.top };
   m_ctrlAddress.MoveWindow(&rcAddress);
   m_ctrlMemory.MoveWindow(&rcMemory);
   return 0;
}

LRESULT CMemoryView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   POINT pt;
   ::GetCursorPos(&pt);
   CMenu menu;
   menu.LoadMenu(IDR_MEMORY);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, TRUE, this);
   PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
   return 0;
}

LRESULT CMemoryView::OnEditChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == '\r' ) return 0;  // We hate the BEEP sound
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryView::OnEditKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) _UpdateDisplay();
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryView::OnMemoryEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateDisplay();
   return 0;
}

LRESULT CMemoryView::OnMemorySize(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( wID == ID_MEMORY_SIZE_DWORD ) m_iDisplaySize = 8;
   if( wID == ID_MEMORY_SIZE_WORD ) m_iDisplaySize = 4;
   if( wID == ID_MEMORY_SIZE_BYTE ) m_iDisplaySize = 1;
   _UpdateDisplay();
   return 0;
}

// IIdleListener

void CMemoryView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_MEMORY_EDIT, FALSE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_DWORD, TRUE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_WORD, TRUE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_BYTE, TRUE);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_DWORD, m_iDisplaySize == 8);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_WORD, m_iDisplaySize == 4);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_BYTE, m_iDisplaySize == 1);
}

