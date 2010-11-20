
#include "StdAfx.h"
#include "resource.h"

#include "MemoryView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// CMemoryView

CMemoryView::CMemoryView() :
   m_pProject(NULL),
   m_iDisplaySize(8)
{
}

CMemoryView::~CMemoryView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

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
      m_bAllowUpdate = true;

      LONG lStartPos = 0, lEndPos =0;
      m_ctrlMemory.GetSel(lStartPos, lEndPos);

      CString sText = _T("{\\rtf1\\ansi\\deff0\\deftab720{\\colortbl\\red0\\green0\\blue0;\\red0\\green0\\blue180;\\red130\\green130\\blue130;}\\deflang1033\\pard\\plain\\f0\\fs18 ");
      CString sAddress;
      CString sMemory;
      CString sAscii;
      CString sTemp;
      int iStep = 0;
      m_sAddress = _T("");
      CString sValue = info.GetItem(_T("addr"), _T("memory"));
      while( !sValue.IsEmpty() ) {
         if( sAddress.IsEmpty() ) sAddress = sValue;
         if( m_sAddress.IsEmpty() ) m_sAddress = sAddress;
         sMemory += info.FindNext(_T("data")) + _T(" ");
         sAscii += info.FindNext(_T("ascii"));
         if( (++iStep % (MAX_NUM_BYTES_ON_LINE / m_iDisplaySize)) == 0 ) {
            sAscii.Replace(_T("\\"), _T("\\\\"));
            sAscii.Replace(_T("{"), _T("\\{"));
            sAscii.Replace(_T("}"), _T("\\}"));
            if( sMemory.GetLength() < 40 ) sMemory += CString(' ', 40 - sMemory.GetLength());
            sTemp.Format(_T("\\cf1 %s  \\cf0 %s   %s\\par"), sAddress, sMemory, sAscii);
            sText += sTemp;
            sAddress = sMemory = sAscii = _T("");
         }
         sValue = info.FindNext(_T("addr"), _T("memory"));
      }
      sText +=  _T("\\par}");
      EDITSTREAM stream = { 0 };
      STREAMCOOKIE cookie = { sText, 0 };
      stream.dwCookie = (DWORD_PTR) &cookie;
      stream.pfnCallback = _EditStreamCallback;
      m_ctrlMemory.StreamIn(SF_RTF, stream);

      CHARFORMAT cfDefault;
      cfDefault.cbSize = sizeof(cfDefault);
      cfDefault.dwEffects = CFE_PROTECTED; 
      cfDefault.dwMask = CFM_PROTECTED;
      m_ctrlMemory.SetDefaultCharFormat(cfDefault);

      m_ctrlMemory.SetSel(lStartPos, lStartPos);

      m_bAllowUpdate = false;
   }
}


/////////////////////////////////////////////////////////////////////////
// Implementation

DWORD CALLBACK CMemoryView::_EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   STREAMCOOKIE* cookie = (STREAMCOOKIE*) dwCookie;
   *pcb = min(cb, (LONG) _tcslen(cookie->pstr + cookie->pos) + 1);
   AtlW2AHelper( (LPSTR) pbBuff, cookie->pstr + cookie->pos, (int) *pcb );
   cookie->pos += *pcb;
   return *pcb >= 0 ? 0 : 1;
}

void CMemoryView::_UpdateDisplay()
{
   m_sExpression = CWindowText(m_ctrlAddress);
   // Default to displaying hexadecimal formatting
   m_sFormat = _T("x");
   int iPos = m_sExpression.Find(',');
   if( iPos > 0 ) {
      // Might be a different display format on command line
      // Format is:
      //   <expr/address>, <format>
      // where <format> is "x" (hex), "d" (decimal) or "o" (octal)
      m_sFormat = m_sExpression.Mid(iPos + 1);
      m_sExpression = m_sExpression.Left(iPos);
      m_sFormat.TrimLeft();
      m_sExpression.TrimRight();
   }
   if( m_sExpression.IsEmpty() ) return;
   // Estimate rows needed to fill display
   CClientDC dc = m_hWnd;
   TEXTMETRIC tm = { 0 };
   dc.GetTextMetrics(&tm);
   CClientRect rcClient = m_hWnd;
   long lRows = (rcClient.bottom - rcClient.top) / tm.tmHeight;     
   // Get data from debugger
   CString sCommand;
   sCommand.Format(_T("-data-read-memory \"%s\" %s %ld %ld 1 ."), 
      m_sExpression, 
      m_sFormat,
      m_iDisplaySize / 2,
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

   m_ctrlMemory.Create(this, 2, m_hWnd, &rcDefault, NULL, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | ES_MULTILINE | /*ES_READONLY |*/ ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
   m_ctrlMemory.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlMemory.SetBackgroundColor(clrBack);
   m_ctrlMemory.SetTargetDevice(NULL, 1);
   m_ctrlMemory.SetEventMask(ENM_PROTECTED);
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

LRESULT CMemoryView::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{   
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   POINT ptLocal = pt;
   m_ctrlMemory.ScreenToClient(&ptLocal);
   long lPos = m_ctrlMemory.CharFromPos(ptLocal);
   m_ctrlMemory.SetSel(lPos, lPos);
   CMenu menu;
   menu.LoadMenu(IDR_MEMORY);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE, this);
   if( nCmd != 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
   return 0;
}

LRESULT CMemoryView::OnProtected(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   // NOTE: The inital idea was to do memory manipulation from here, but as it turns
   //       out, the GDB debugger is extremely slow at these operations. Instead we'll
   //       force a more manual convoluted approach with an inline editor - which
   //       the user interface experience can justify.
   return m_bAllowUpdate ? 0 : 1;
}

LRESULT CMemoryView::OnMemoryEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   LONG lStartPos = 0, lEndPos = 0;
   m_ctrlMemory.GetSel(lStartPos, lEndPos);
   _DoInlineEditing(lStartPos, true);
   return 0;
}

LRESULT CMemoryView::OnMemoryRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateDisplay();
   return 0;
}

LRESULT CMemoryView::OnMemorySize(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( wID == ID_MEMORY_SIZE_DWORD ) m_iDisplaySize = 8;
   if( wID == ID_MEMORY_SIZE_WORD ) m_iDisplaySize = 4;
   if( wID == ID_MEMORY_SIZE_BYTE ) m_iDisplaySize = 2;
   _UpdateDisplay();
   return 0;
}

// Address control messages

LRESULT CMemoryView::OnEditKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) _UpdateDisplay();
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryView::OnEditChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == '\r' ) return 0;  // We hate the BEEP sound
   bHandled = FALSE;
   return 0;
}

// Memory control messages

LRESULT CMemoryView::OnEditDblClick(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   LONG lStartPos = 0, lEndPos = 0;
   m_ctrlMemory.GetSel(lStartPos, lEndPos);
   _DoInlineEditing(lStartPos, true);
   return 0;
}

// IIdleListener

void CMemoryView::OnIdle(IUpdateUI* pUIBase)
{
   LONG lStartPos = 0, lEndPos = 0;
   m_ctrlMemory.GetSel(lStartPos, lEndPos);
   pUIBase->UIEnable(ID_MEMORY_EDIT, _DoInlineEditing(lStartPos, false));
   pUIBase->UIEnable(ID_MEMORY_REFRESH, TRUE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_DWORD, TRUE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_WORD, TRUE);
   pUIBase->UIEnable(ID_MEMORY_SIZE_BYTE, TRUE);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_DWORD, m_iDisplaySize == 8);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_WORD, m_iDisplaySize == 4);
   pUIBase->UISetCheck(ID_MEMORY_SIZE_BYTE, m_iDisplaySize == 2);
}

void CMemoryView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

// Implementation

BOOL CMemoryView::_DoInlineEditing(LONG lPos, bool bCreateEdit)
{
   ATLASSERT(!m_ctrlInlineEdit.IsWindow());
   // First figure out where the line is; grab its contents
   int iLineNum = m_ctrlMemory.LineFromChar(lPos);
   long lLinePos = m_ctrlMemory.LineIndex(iLineNum);
   TCHAR szLine[400] = { 0 };
   TEXTRANGE tr;
   tr.chrg.cpMin = lLinePos;
   tr.chrg.cpMax = lLinePos + m_ctrlMemory.LineLength(iLineNum);
   tr.lpstrText = szLine;
   m_ctrlMemory.GetTextRange(&tr);
   CString sLine = szLine;
   if( sLine.Left(2) != _T("0x") ) return FALSE;
   // Selected position must be on memory values; lets determine which
   // entry on the line it is. We're doing parsing on space-characters
   // on the line. Not our best choice, but needed to support data-values
   // displayed in hex, decimal and octal.
   bool bFound = false;
   long lColumn = lPos - lLinePos;
   long lValueIndex = 0;
   long lValueStartPos = m_sAddress.GetLength() + 2;  // +2 spaces after address
   long lValueEndPos = 0;
   for( ; !bFound && lValueIndex < 32 / m_iDisplaySize; lValueIndex++ ) {
      lValueEndPos = sLine.Find(' ', lValueStartPos + 1);
      if( lValueStartPos <= lColumn && lValueEndPos >= lColumn) {
         bFound = true;
         break;
      }
      lValueStartPos = lValueEndPos + 1;
   }
   if( !bFound ) return FALSE;
   lValueStartPos += lLinePos;
   lValueEndPos += lLinePos;
   // Figure out where to place the inline edit control
   POINT ptStart = m_ctrlMemory.PosFromChar(lValueStartPos);
   POINT ptEnd = m_ctrlMemory.PosFromChar(lValueEndPos);
   CRect rcPos(ptStart, ptEnd);
   CClientDC dc = m_hWnd;
   TEXTMETRIC tm = { 0 };
   dc.GetTextMetrics(&tm);
   rcPos.bottom += tm.tmHeight;
   rcPos.InflateRect(3, 2);
   if( m_sFormat != _T("x") && rcPos.Width() < 40 ) rcPos.left -= 40;  // Expand window for Octal and decimal display
   // Get the current value
   TCHAR szValue[20] = { 0 };
   tr.chrg.cpMin = lValueStartPos;
   tr.chrg.cpMax = lValueEndPos;
   tr.lpstrText = szValue;
   m_ctrlMemory.GetTextRange(&tr);
   // Create and display the control if requested...
   if( m_ctrlInlineEdit.IsWindow() ) return FALSE;
   if( !bCreateEdit ) return TRUE;
   m_ctrlInlineEdit.Init(m_pProject, this, sLine.Left(12), lValueIndex * (m_iDisplaySize / 2));
   m_ctrlInlineEdit.Create(m_ctrlMemory, rcPos, szValue, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_WANTRETURN | ES_RIGHT, 0);
   m_ctrlInlineEdit.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlInlineEdit.SetFocus();
   m_ctrlInlineEdit.SetSel(0, -1);
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// CMemoryInlineEdit

void CMemoryInlineEdit::Init(CRemoteProject* pProject, CMemoryView* pMemoryView, LPCTSTR pstrAddress, LONG lByteOffset)
{
   m_pProject = pProject;
   m_pMemoryView = pMemoryView;
   m_sAddress = pstrAddress;
   m_lByteOffset = lByteOffset;
   m_bCommit = false;
}

LRESULT CMemoryInlineEdit::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   // If user pressed RETURN key we'll commit the changes using the
   // debugger interface...
   if( m_bCommit ) {
      CString sValue = CWindowText(m_hWnd);
      CString sType = _T("char");
      if( m_pMemoryView->m_iDisplaySize == 4 ) sType = _T("short");
      if( m_pMemoryView->m_iDisplaySize == 8 ) sType = _T("long");
      m_pProject->m_DebugManager.DoDebugCommandV(_T("-var-create memvarzzz * \"*(%s *)(%s + %ld)\""), sType, m_sAddress, m_lByteOffset);
      m_pProject->m_DebugManager.DoDebugCommandV(_T("-var-assign memvarzzz %s"), sValue);
      m_pProject->m_DebugManager.DoDebugCommand(_T("-var-delete memvarzzz"));
      m_pMemoryView->_UpdateDisplay();
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryInlineEdit::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{   
   if( wParam == VK_ESCAPE ) {
      PostMessage(WM_CLOSE);
      return 0;
   }
   if( wParam == VK_RETURN ) {
      m_bCommit = true;
      PostMessage(WM_CLOSE);
      return 0;
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryInlineEdit::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   PostMessage(WM_CLOSE);
   bHandled = FALSE;
   return 0;
}

