
#include "StdAfx.h"
#include "resource.h"

#include "DisasmView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CDisasmView::CDisasmView() :
   m_pProject(NULL),
   m_bIntelStyle(true),
   m_bShowSource(false)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CDisasmView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CDisasmView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CDisasmView::PopulateView(CSimpleArray<CString>& aDbgCmd)
{
   // Estimate instruction count needed to fill screen
   const long AVG_INST_SIZE = 5;
   CClientRect rcClient = m_hWnd;
   long lRows = (rcClient.bottom - rcClient.top) / m_tm.tmHeight * AVG_INST_SIZE;
   // Execute command
   CString sCommand;
   sCommand.Format(_T("-gdb-set disassembly-flavor %s"), m_bIntelStyle ? _T("intel") : _T("att"));
   aDbgCmd.Add(sCommand);
   sCommand.Format(_T("-data-disassemble -s $pc -e \"$pc + %ld\" -- %ld"), 
      lRows,
      m_bShowSource ? 1 : 0);
   aDbgCmd.Add(sCommand);
}

void CDisasmView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("asm_insns")) == 0 ) 
   {
      CString sTemp;
      CString sTitle;
      CString sDisasm;
      CString sFunction;
      CString sLocation;
      CString sText = _T("{\\rtf1\\ansi\\deff0\\deftab720{\\colortbl\\red0\\green0\\blue0;\\red0\\green0\\blue180;\\red130\\green130\\blue130;\\red180\\green0\\blue0;}\\deflang1033\\pard\\plain\\f0\\fs18 ");
      CString sValue = info.GetItem(_T("address"));
      while( !sValue.IsEmpty() ) {
         sLocation = info.GetSubItem(_T("func-name"));
         // If the function-name changes, we'll print it...
         if( sLocation != sFunction ) {
            CString sOffset = info.GetSubItem(_T("offset"));      
            sTemp.Format(_T("\\cf1\\b1 %s + %s\\b0\\cf0\\par "), 
               sLocation, 
               sOffset);
            sText += sTemp;
            if( sTitle.IsEmpty() ) {
               sTitle.Format(_T("%s%s%s    (%s)"),
                  sLocation,
                  sOffset.IsEmpty() ? _T("") : _T(" + "),
                  sOffset,
                  sValue);
               sTitle.TrimLeft();
               m_ctrlAddress.SetWindowText(sTitle);
            }
            sFunction = sLocation;
         }
         // Build up disassembly line in RTF text.
         // We mark resolved address names with grey text.
         // Assembly lined marked as bad are coloured red.
         sDisasm = info.GetSubItem(_T("inst"));
         sDisasm.Replace(_T("\\"), _T("\\\\ "));
         sDisasm.Replace(_T("{"), _T("\\{ "));
         sDisasm.Replace(_T("}"), _T("\\} "));
         sDisasm.Replace(_T("(bad)"), _T("\\cf3 (bad)\\cf0 "));
         if( sDisasm.Find('<') > 0 ) {
            sDisasm.Replace(_T("<"), _T("\\cf2  <"));
            sDisasm += _T("\\cf0 ");
         }
         else if( sDisasm.Find('\t') > 0 ) {
            sDisasm.Replace(_T("\t"), _T("\\cf2  \t"));
            sDisasm += _T("\\cf0 ");
         }
         sTemp.Format(_T("\t %s\\par "), sDisasm);
         sText += sTemp;
         // Next instruction please...
         sValue = info.FindNext(_T("address"));         
      }
      sText +=  _T("\\par}");
      EDITSTREAM stream = { 0 };
      STREAMCOOKIE cookie = { sText, 0 };
      stream.dwCookie = (DWORD) &cookie;
      stream.pfnCallback = _EditStreamCallback;
      m_ctrlView.StreamIn(SF_RTF, stream);
   }
}

// Implementation

DWORD CALLBACK CDisasmView::_EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   STREAMCOOKIE* cookie = (STREAMCOOKIE*) dwCookie;
   *pcb = min(cb, (LONG) _tcslen(cookie->pstr + cookie->pos));
   AtlW2AHelper( (LPSTR) pbBuff, cookie->pstr + cookie->pos, (int) *pcb );
   cookie->pos += *pcb;
   return *pcb >= 0 ? 0 : 1;
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CDisasmView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 10);
   ModifyStyleEx(WS_EX_CLIENTEDGE, 0);

   m_ctrlAddress.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, WS_EX_CLIENTEDGE);
   m_ctrlAddress.SetFont(AtlGetDefaultGuiFont());
   m_ctrlView.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY, WS_EX_CLIENTEDGE);
   m_ctrlView.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlView.SetBackgroundColor(clrBack);
   m_ctrlView.SetTargetDevice(NULL, 1);
   m_ctrlView.SetUndoLimit(0);

   CClientDC dc = m_ctrlView;
   dc.GetTextMetrics(&m_tm);

   m_ctrlAddress.SetFocus();
   return 0;
}

LRESULT CDisasmView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rcClient = m_hWnd;
   RECT rcAddress = { 0, 0, rcClient.right - rcClient.left, ::GetSystemMetrics(SM_CYCAPTION) };
   RECT rcMemory = { 0, rcAddress.bottom, rcAddress.right, rcClient.bottom - rcClient.top };
   m_ctrlAddress.MoveWindow(&rcAddress);
   m_ctrlView.MoveWindow(&rcMemory);
   return 0;
}

LRESULT CDisasmView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   POINT pt;
   ::GetCursorPos(&pt);
   CMenu menu;
   menu.LoadMenu(IDR_DISASSEMBLY);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, TRUE, this);
   PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
   return 0;
}

LRESULT CDisasmView::OnShowSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_bShowSource = !m_bShowSource;
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   return 0;
}

LRESULT CDisasmView::OnIntelStyle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_bIntelStyle = !m_bIntelStyle;
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   return 0;
}

// IIdleListener

void CDisasmView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_DISASM_INTELSTYLE, TRUE);
   pUIBase->UIEnable(ID_DISASM_SHOWSOURCE, FALSE);  // Not supported yet
   pUIBase->UISetCheck(ID_DISASM_INTELSTYLE, m_bIntelStyle);
   pUIBase->UISetCheck(ID_DISASM_SHOWSOURCE, m_bShowSource);
}

void CDisasmView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

