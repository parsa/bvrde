
#include "StdAfx.h"
#include "resource.h"

#include "ScintillaView.h"
#include "SciLexer.h"

#include "Project.h"
#include "Files.h"

#pragma code_seg( "EDITOR" )

#pragma comment(lib, "../GenEdit/Lib/GenEdit.lib")


// Constructor

CScintillaView::CScintillaView() :
   m_ctrlEdit(this, 1),
   m_pCppProject(NULL),
   m_pView(NULL),
   m_bMouseDwell(false)
{
}

// Operations

void CScintillaView::OnFinalMessage(HWND /*hWnd*/)
{
   // Don't want compile output any longer
   if( m_pCppProject != NULL ) m_pCppProject->m_CompileManager.m_ShellManager.RemoveLineListener(this);
   // If it's a file that was dragged into the app, then
   // the view was probably created by CreateViewFromFilename()
   // and thus we own the memory.
   if( m_pProject == NULL ) delete m_pView;
}

BOOL CScintillaView::Init(CRemoteProject* pCppProject, IProject* pProject, IView* pView, LPCTSTR pstrFilename, LPCTSTR pstrLanguage)
{
   m_pCppProject = pCppProject;
   m_pProject = pProject;
   m_pView = pView;
   m_sLanguage = pstrLanguage;
   m_sFilename = pstrFilename;
   return TRUE;
}

BOOL CScintillaView::GetText(LPSTR& pstrText)
{
   ATLASSERT(pstrText==NULL);

   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.general.eolMode"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("cr")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CR);
   else if( _tcscmp(szBuffer, _T("lf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_LF);
   else if( _tcscmp(szBuffer, _T("crlf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CRLF);

   int nLength = m_ctrlEdit.GetTextLength() + 1;
   pstrText = (LPSTR) malloc(nLength);
   if( pstrText == NULL ) {
      ::SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
   }
   m_ctrlEdit.GetText(nLength, pstrText);

   return TRUE;
}

BOOL CScintillaView::SetText(LPCSTR pstrText)
{
   m_ctrlEdit.SetText(pstrText);
   m_ctrlEdit.EmptyUndoBuffer();
   m_ctrlEdit.SetSavePoint();

   // Adjust the line-number margin width.
   int iWidth = m_ctrlEdit.GetMarginWidthN(0);
   if( iWidth > 0 && m_ctrlEdit.GetLineCount() > 9999 ) m_ctrlEdit.SetMarginWidthN(0, m_ctrlEdit.TextWidth(STYLE_LINENUMBER, "_99999"));  

   SendMessage(WM_SETTINGCHANGE);
   return TRUE;
}

// Message handlers

LRESULT CScintillaView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   // Create Scintilla editor. The editor is created as a child
   // window of the view window.
   m_ctrlEdit.SubclassWindow( Bvrde_CreateScintillaView(m_hWnd, _pDevEnv, m_sFilename, m_sLanguage) );
   ATLASSERT(m_ctrlEdit.IsWindow());
   m_ctrlEdit.ShowWindow(SW_SHOW);
   return 0;
}

LRESULT CScintillaView::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{  
   // Check if we need to prompt for save...
   if( m_ctrlEdit.IsWindow() && m_ctrlEdit.GetModify() ) 
   {
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("editors.general.savePrompt"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) {
         if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_SAVEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_ICONINFORMATION | MB_YESNO) ) return TRUE;
      }
      if( SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0)) != 0 ) return FALSE;
   }
   return TRUE;
}

LRESULT CScintillaView::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);

   TCHAR szBuffer[64] = { 0 };

   // Get settings for local view
   m_bAutoIndent = false;
   m_bSmartIndent = false;
   if( _pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bAutoIndent = _tcscmp(szBuffer, _T("auto")) == 0;
   if( _pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bSmartIndent = _tcscmp(szBuffer, _T("smart")) == 0;
   m_bProtectDebugged = false;
   if( _pDevEnv->GetProperty(sKey + _T("protectDebugged"), szBuffer, 63) ) m_bProtectDebugged = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoComplete = false;
   if( _pDevEnv->GetProperty(sKey + _T("autoComplete"), szBuffer, 63) ) m_bAutoComplete = _tcscmp(szBuffer, _T("true")) == 0;
   m_bMarkErrors = false;
   if( _pDevEnv->GetProperty(sKey + _T("markErrors"), szBuffer, 63) ) m_bMarkErrors = _tcscmp(szBuffer, _T("true")) == 0;

   // Get active breakpoints for this file
   // We call this here because WM_SETTINGCHANGE is one of the messages
   // sent at startup...
   if( m_pCppProject ) {
      CString sName;
      m_pView->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      CSimpleArray<long> aLines;
      m_pCppProject->m_DebugManager.GetBreakpoints(sName, aLines);
      m_ctrlEdit.MarkerDeleteAll(MARKER_BREAKPOINT);
      for( int i = 0; i < aLines.GetSize(); i++ ) {
         m_ctrlEdit.MarkerAdd(aLines[i] - 1, MARKER_BREAKPOINT);
      }
   }

   SendMessageToDescendants(WM_SETTINGCHANGE);

   return 0;
}

LRESULT CScintillaView::OnHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   // If we don't succeed, we'll just pass the message on to the parent...
   bHandled = FALSE;
   // Load the ManPage module
   CString sModule;
   sModule.Format(_T("%sManPages.dll"), CModulePath());
   static HINSTANCE s_hInst = NULL;
   if( s_hInst == NULL ) s_hInst = ::LoadLibrary(sModule);
   if( s_hInst == NULL ) return false;
   // Display the page if possible
   typedef BOOL (APIENTRY* LPFNSHOWPAGE)(IDevEnv*, LPCWSTR, LPCWSTR);
   LPFNSHOWPAGE fnShowHelp = (LPFNSHOWPAGE) ::GetProcAddress(s_hInst, "ManPages_ShowHelp");
   ATLASSERT(fnShowHelp);
   if( fnShowHelp == NULL ) return 0;
   // Locate the text near the cursor
   bHandled = TRUE;
   long lPos = m_ctrlEdit.GetCurrentPos();
   CString sText = _GetNearText(lPos, false);
   if( sText.IsEmpty() ) return 0;
   // Open man-page
   BOOL bRes = FALSE;
   ATLTRY( bRes = fnShowHelp(_pDevEnv, sText, m_sLanguage) );
   if( !bRes ) return 0;
   return 0;
}

LRESULT CScintillaView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   CClientRect rcClient = m_hWnd;
   m_ctrlEdit.MoveWindow(&rcClient);
   return 0;
}

LRESULT CScintillaView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   ::SetFocus(m_ctrlEdit);
   return 0;
}

LRESULT CScintillaView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{   
   // Debugger has its own context menu
   bHandled = FALSE;
   if( m_pCppProject == NULL ) return 0;
   if( m_sLanguage != _T("cpp") ) return 0;
   if( !m_pCppProject->m_DebugManager.IsDebugging() ) return 0;

   SetFocus();

   long lPos = m_ctrlEdit.GetCurrentPos();
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

   // Get the cursor position
   POINT ptLocal = pt;
   ScreenToClient(&ptLocal);
   if( lParam == (LPARAM) -1 ) {
      pt.x = m_ctrlEdit.PointXFromPosition(lPos);
      pt.y = m_ctrlEdit.PointYFromPosition(lPos);
   }

   // Place cursor at mouse if not clicked inside a selection
   lPos = m_ctrlEdit.PositionFromPoint(ptLocal.x, ptLocal.y);
   CharacterRange cr = m_ctrlEdit.GetSelection();
   if( lPos < cr.cpMin || lPos > cr.cpMax ) m_ctrlEdit.GotoPos(lPos);

   CMenu menu;
   menu.LoadMenu(IDR_EDIT_DEBUG);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   _pDevEnv->ShowPopupMenu(NULL, submenu, pt);

   bHandled = TRUE;
   return 0;
}

LRESULT CScintillaView::OnSetEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   _pDevEnv->AddIdleListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnKillEditFocus(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   _pDevEnv->RemoveIdleListener(this);      
   m_ctrlEdit.CallTipCancel();   // FIX: Scintilla doesn't always remove the tooltip
   m_bMouseDwell = false;
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   if( m_pView == NULL ) return 0;
   if( !m_ctrlEdit.GetModify() ) return 0;

   CWaitCursor cursor;
   _pDevEnv->PlayAnimation(TRUE, ANIM_SAVE);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_SAVEFILE)));

   // Just save the file...
   if( !m_pView->Save() ) {
      DWORD dwErr = ::GetLastError();
      _pDevEnv->PlayAnimation(FALSE, 0);
      // Show nasy error dialog; allow user to CANCEL to stay alive!
      TCHAR szName[128] = { 0 };
      m_pView->GetName(szName, 127);
      CString sMsg;
      sMsg.Format(IDS_ERR_FILESAVE, szName, GetSystemErrorText(dwErr));
      if( IDOK == _pDevEnv->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONEXCLAMATION | MB_OKCANCEL) ) return 0;
      return 1; // Return ERROR indication
   }

   if( m_sLanguage == _T("cpp") && m_pCppProject != NULL ) {
      // Using the C++ realtime scanner? We should parse the new file,
      // merge the tags if possible and rebuild the ClassView.
      if( m_pCppProject->m_TagManager.m_LexInfo.IsAvailable() ) {
         LPSTR pstrText = NULL;
         if( !GetText(pstrText) ) return 0;
         m_pCppProject->m_TagManager.m_LexInfo.MergeFile(m_sFilename, pstrText);
         free(pstrText);
      }
   }

   _pDevEnv->PlayAnimation(FALSE, 0);

   _ClearSquigglyLines();

   return 0;
}

LRESULT CScintillaView::OnEditAutoComplete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // The use of \b (BELL) character here is a hack, and is used to
   // force the auto-complete/suggest dropdown to open...
   _AutoComplete('\b');
   return 0;
}

LRESULT CScintillaView::OnDebugBreakpoint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_pCppProject == NULL ) return 0;
   if( m_sLanguage != _T("cpp") ) return 0;

   int iLine = m_ctrlEdit.GetCurrentLine();
   long lValue = m_ctrlEdit.MarkerGet(iLine);
   long lMask = (1 << MARKER_BREAKPOINT);

   CString sName;
   m_pView->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   CString sBreakpoint;
   sBreakpoint.Format(_T("%s:%ld"), sName, (long) iLine + 1);

   if( (lValue & lMask) != 0 ) {
      if( m_pCppProject->m_DebugManager.RemoveBreakpoint(sBreakpoint) ) {
         m_ctrlEdit.MarkerDelete(iLine, MARKER_BREAKPOINT);
      }
   }
   else {
      if( m_pCppProject->m_DebugManager.AddBreakpoint(sBreakpoint) ) {
         m_ctrlEdit.MarkerAdd(iLine, MARKER_BREAKPOINT);
      }
   }
   return 0;
}

LRESULT CScintillaView::OnDebugRunTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_pCppProject == NULL ) return 0;
   if( m_sLanguage != _T("cpp") ) return 0;
   if( !m_pCppProject->m_DebugManager.IsDebugging() ) return 0;

   CString sName;
   m_pView->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();
   long iLine = m_ctrlEdit.GetCurrentLine();
   m_pCppProject->m_DebugManager.RunTo(sName, iLine + 1);

   return 0;
}

LRESULT CScintillaView::OnDebugSetNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_pCppProject == NULL ) return 0;
   if( m_sLanguage != _T("cpp") ) return 0;
   if( !m_pCppProject->m_DebugManager.IsDebugging() ) return 0;

   CString sName;
   m_pView->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();
   long iLine = m_ctrlEdit.GetCurrentLine();

   if( m_pCppProject->m_DebugManager.SetNextStatement(sName, iLine + 1) ) {
      // HACK: GDB doesn't support a MI command like 'jump' and it
      //       doesn't react normal on the breakpoint either <sigh> so
      //       we simulate the action...
      m_ctrlEdit.MarkerDeleteAll(MARKER_CURLINE);
      m_ctrlEdit.MarkerAdd(iLine, MARKER_CURLINE);
   }
   return 0;
}

LRESULT CScintillaView::OnDebugLink(WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled)
{
   bHandled = FALSE; // Other editors must also be allowed to process this!!!

   if( m_sLanguage != _T("cpp") ) return 0;

   CString sName;
   m_pView->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   LAZYDATA* pData = (LAZYDATA*) hWndCtl;
   ATLASSERT(!::IsBadReadPtr(pData, sizeof(LAZYDATA)));

   switch( wNotifyCode ) {
   case DEBUG_CMD_SET_CURLINE:
      {
         // Debugger broadcasts the filename & line where it
         // sets its cursor. We'll follow its bidding.
         m_ctrlEdit.MarkerDeleteAll(MARKER_CURLINE);
         m_ctrlEdit.MarkerDeleteAll(MARKER_RUNNING);
         if( _tcscmp(::PathFindFileName(pData->szFilename), sName) == 0 ) {
            int iLine = pData->lLineNum;
            if( iLine >= 0 ) m_ctrlEdit.MarkerAdd(iLine - 1, MARKER_CURLINE);
         }
         if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
      }
      break;
   case DEBUG_CMD_SET_RUNNING:
      {
         // Replace the current-line marker with a running-state marker
         int iLine = m_ctrlEdit.MarkerNext(0, 1 << MARKER_CURLINE);
         if( iLine >= 0 ) {
            m_ctrlEdit.MarkerDelete(iLine, MARKER_CURLINE);
            m_ctrlEdit.MarkerAdd(iLine, MARKER_RUNNING);
         }
      }
      break;
   case DEBUG_CMD_CLEAR_BREAKPOINTS:
      {
         m_ctrlEdit.MarkerDeleteAll(MARKER_BREAKPOINT);
      }
      break;
   case DEBUG_CMD_REQUEST_BREAKPOINTS:
      {
         if( m_pCppProject == NULL ) return 0;
         // Before the debugger starts, it collects breakpoint information
         // directly from the views.
         int iLine = 0;
         CSimpleArray<CString> aBreakpoints;
         while( (iLine = m_ctrlEdit.MarkerNext(iLine, 1 << MARKER_BREAKPOINT)) >= 0 ) {
            CString sBreakpoint;
            sBreakpoint.Format(_T("%s:%ld"), sName, (long) iLine + 1);
            aBreakpoints.Add(sBreakpoint);
            iLine++;
         }
         m_pCppProject->m_DebugManager.SetBreakpoints(sName, aBreakpoints);
      }
      break;
   case DEBUG_CMD_GET_CARET_TEXT:
      {
         // What's under the cursor?
         // Primarily used by the "QuickWatch" functionality.
         CString sText = _GetSelectedText();
         if( sText.IsEmpty() ) sText = _GetNearText(m_ctrlEdit.GetCurrentPos());
         _tcsncpy(pData->szMessage, sText, (sizeof(pData->szMessage) / sizeof(TCHAR)) - 1);
      }
      break;
   case DEBUG_CMD_HOVERINFO:
      {
         // Debugger delivers delayed debug/tooltip information
         if( !m_bMouseDwell ) return 0;
         CString sValue = m_TipInfo.sText;
         if( !sValue.IsEmpty() ) sValue += _T(" = ");
         sValue += pData->MiInfo.GetItem(_T("value"));
         _ShowToolTip(m_TipInfo.lPos, sValue, true, ::GetSysColor(COLOR_INFOTEXT), ::GetSysColor(COLOR_INFOBK));
      }
      break;
   case DEBUG_CMD_DEBUG_START:
      {
         _ClearSquigglyLines();
         if( m_bProtectDebugged ) m_ctrlEdit.SetReadOnly(TRUE);
      }
      break;
   case DEBUG_CMD_DEBUG_STOP:
      {
         if( m_bProtectDebugged ) m_ctrlEdit.SetReadOnly(FALSE);
      }
      break;
   case DEBUG_CMD_COMPILE_START:
      {
         _ClearSquigglyLines();
         if( m_pCppProject != NULL && m_bMarkErrors ) {
            m_iOutputLine = 0;
            m_sOutputToken.Format(_T("%s:"), ::PathFindFileName(m_sFilename));
            m_pCppProject->m_CompileManager.m_ShellManager.AddLineListener(this);
         }
      }
      break;
   case DEBUG_CMD_COMPILE_STOP:
      {
         if( m_pCppProject != NULL ) {
            m_pCppProject->m_CompileManager.m_ShellManager.RemoveLineListener(this);
         }
      }
      break;
   case DEBUG_CMD_FINDTEXT:
      {
         // Locate text in source file.
         // Primarily used in "Go To Declaration" functionality.
         USES_CONVERSION;
         _FindNext(FR_DOWN | FR_WRAP | pData->iFlags, T2CA(pData->szFilename), false);
      }
      break;
   case DEBUG_CMD_FOLDCURSOR:
      {
         // Place cursor at line beginning
         int iLineNo = pData->lLineNum;
         if( iLineNo == -1 ) iLineNo = m_ctrlEdit.GetCurrentLine();
         long lPos = m_ctrlEdit.PositionFromLine(iLineNo);
         m_ctrlEdit.GotoPos(lPos);
         m_ctrlEdit.EnsureVisible(iLineNo);
      }
      break;
   }
   return 0;
}

LRESULT CScintillaView::OnCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;

   switch( pSCN->ch ) {
   case ')':
   case '(':
      _FunctionTip(pSCN->ch);
      break;
   }
   
   _AutoComplete(pSCN->ch);

   _ClearSquigglyLines();

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   if( pSCN->margin == 1 
       && m_pCppProject != NULL 
       && m_sLanguage == _T("cpp")) 
   {
      m_ctrlEdit.GotoPos(pSCN->position);
      PostMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_BREAKPOINT, 0));
      return 0;
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnCallTipClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   long lCurTip = m_TipInfo.lCurTip;
   if( pSCN->position == 1 ) lCurTip--;
   if( pSCN->position == 2 ) lCurTip++;
   _ShowMemberToolTip(0, NULL, lCurTip, false, RGB(0,0,0), RGB(0,0,0));
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDwellStart(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   SCNotification* pSCN = (SCNotification*) pnmh;

   if( m_bMouseDwell ) return 0;
   if( m_pCppProject == NULL ) return 0;
   if( m_sLanguage != _T("cpp") ) return 0;
   if( m_ctrlEdit.AutoCActive() ) return 0;

   // HACK: Bug in Scintilla which tries to start a hover tip
   //       while the window has no focus.
   if( ::GetFocus() != m_ctrlEdit ) return 0;

   // Display tooltip at all?
   TCHAR szBuffer[16] = { 0 };
   _pDevEnv->GetProperty(_T("editors.cpp.noTips"), szBuffer, 15);
   if( _tcscmp(szBuffer, _T("true") ) == 0 ) return 0;

   // Get text around cursor
   long lPos = pSCN->position;
   if( !_IsRealCppEditPos(lPos) ) return 0;
   CString sText;
   CharacterRange cr = m_ctrlEdit.GetSelection();
   if( lPos >= cr.cpMin && lPos <= cr.cpMax ) sText = _GetSelectedText();
   else sText = _GetNearText(lPos);
   if( sText.IsEmpty() ) return 0;        
   // Allow the debugger to speak up
   CSimpleArray<CString> aRes;
   if( m_pCppProject->GetTagInfo(sText, true, aRes, NULL) ) m_bMouseDwell = true;
   // Ask lexer to deliver info also
   MEMBERINFO info;
   if( !_GetMemberInfo(lPos, info) ) return 0;
   // Show tooltip
   _ShowMemberToolTip(lPos, &info, 0, true, ::GetSysColor(COLOR_INFOTEXT), ::GetSysColor(COLOR_INFOBK));
   m_bMouseDwell = true;
   return 0;
}

LRESULT CScintillaView::OnDwellEnd(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   if( m_bMouseDwell ) m_ctrlEdit.CallTipCancel();
   m_bMouseDwell = false;
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnAutoExpand(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   USES_CONVERSION;
   bHandled = FALSE;
   // We just expanded an item from the autocomplete list.
   // Do we need to add the starting function brace?
   SCNotification* pSCN = (SCNotification*) pnmh;
   if( m_pCppProject == NULL ) return 0;
   MEMBERINFO info;
   _GetMemberInfo(pSCN->lParam - 3, info);
   CSimpleArray<CString> aResult;
   m_pCppProject->m_TagManager.GetItemDeclaration(A2CT(pSCN->text), aResult, info.sType);
   if( aResult.GetSize() == 0 ) return 0;
   CString sText;
   if( aResult[0].Find('(') >= 0 ) sText = _T("(");
   if( aResult[0].Find(_T("()")) >= 0 ) sText = _T("()");
   // HACK: Insert text as delayed (SCN_AUTOCSELECTION is fored before the insertion)
   for( int i = 0; i < sText.GetLength(); i++ ) {
      m_ctrlEdit.PostMessage(WM_CHAR, sText.GetAt(i), 0);
   }
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   if( m_sLanguage != _T("cpp") ) return;

   BOOL bDebugging = m_pCppProject && m_pCppProject->m_DebugManager.IsDebugging();

   pUIBase->UIEnable(ID_EDIT_COMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNCOMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_AUTOCOMPLETE, TRUE);
   pUIBase->UIEnable(ID_DEBUG_BREAKPOINT, TRUE);
   pUIBase->UIEnable(ID_DEBUG_STEP_RUN, bDebugging); 
   pUIBase->UIEnable(ID_DEBUG_STEP_SET, bDebugging); 
   pUIBase->UIEnable(ID_DEBUG_QUICKWATCH, bDebugging); 
}

void CScintillaView::OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
{
   AtlLoadString(wID, pstrText, cchMax);
}


// IOutputLineListener

void CScintillaView::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   // Ignore some sterotypic messages
   if( _tcsstr(pstrText, _T("warning")) != NULL ) return;

   // This part assumes that the compiler output is formatted as
   //   <filename>:<line>
   // The variable 'm_sOutputToken' contains the editor's filename.
   if( _tcsncmp(pstrText, m_sOutputToken, m_sOutputToken.GetLength() ) != 0 ) return;
   int iLineNo = _ttol(pstrText + m_sOutputToken.GetLength());
   if( iLineNo == 0 ) return;
   --iLineNo;

   // If several errors are reported on the same line we assume
   // that the first entry contained the most useful error. Otherwise
   // we easily end up highlighting the entire line always.
   if( iLineNo <= m_iOutputLine ) return;

   // Get the line so we can analyze where the error occoured.
   // The GNU compilers rarely output at which column (position) the error
   // occured so we'll have to try to match a substring or message
   // with the content of the reported line.
   CHAR szLine[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLineNo) >= sizeof(szLine) - 1 ) return;
   m_ctrlEdit.GetLine(iLineNo, szLine);

   CString sLine = szLine;
   long lLinePos = m_ctrlEdit.PositionFromLine(iLineNo);

   // Determine the position on the line the error was reported
   int iMatchPos = 0;
   int iMatchLength = 0;

   // Find text embraced in quotes.
   // GCC/G++ output has strange quote characters, so we'll match any
   // combination of quotes.
   static LPCTSTR pstrQuotes = _T("\'\"\x60");
   LPCTSTR pstrStart = _tcspbrk(pstrText, pstrQuotes);
   if( pstrStart != NULL ) {
      CString sKeyword;
      LPCTSTR p = pstrStart + 1;
      while( _iscppchar(*p) ) sKeyword += *p++;
      while( *p && _tcschr(pstrQuotes, *p) == NULL ) p++;
      if( !sKeyword.IsEmpty() && sLine.Find(sKeyword) >= 0 ) {
         iMatchPos = lLinePos + sLine.Find(sKeyword);
         iMatchLength = p - pstrStart;
      }
   }

   // If no match was found, highlight the entire line
   if( iMatchPos == 0 ) {
      iMatchPos = lLinePos;
      iMatchLength = m_ctrlEdit.GetLineLength(iLineNo);
      // Let's trim the string if it contains leading spaces (looks stupid)
      LPCTSTR p = sLine;
      while( *p && _istspace(*p++) && iMatchLength > 0 ) {
         iMatchPos++;
         iMatchLength--;
      }
   }

   // Invalid selection?
   if( iMatchLength <= 0 ) return;

   // Apply the squiggly lines
   m_ctrlEdit.IndicSetStyle(0, INDIC_SQUIGGLE);
   m_ctrlEdit.IndicSetFore(0, RGB(200,0,0));
   m_ctrlEdit.StartStyling(iMatchPos, INDIC0_MASK);
   m_ctrlEdit.SetStyling(iMatchLength, INDIC0_MASK);

   m_iOutputLine = iLineNo;
   m_bClearSquigglyLines = true;
}

// Implementation

/**
 * Find the next text snippet.
 * This method is usually called during a Find or Find/Replace operation,
 * and locates a substring in the text.
 */
int CScintillaView::_FindNext(int iFlags, LPCSTR pstrText, bool bWarnings)
{
   int iLength = strlen(pstrText);
   if( iLength == 0 ) return -1;

   bool bDirectionDown = (iFlags & FR_DOWN) != 0;
   bool bInSelection = (iFlags & FR_INSEL) != 0;

   CharacterRange cr = m_ctrlEdit.GetSelection();
   int startPosition = cr.cpMax;
   int endPosition = m_ctrlEdit.GetLength();
   if( !bDirectionDown ) {
      startPosition = cr.cpMin - 1;
      endPosition = 0;
   }
   if( bInSelection ) {
      ATLASSERT(cr.cpMin!=cr.cpMax);
      if( cr.cpMin == cr.cpMax ) return -1;
      startPosition = cr.cpMin;
      endPosition = cr.cpMax;
      iFlags &= ~FR_WRAP;
   }

   m_ctrlEdit.SetTargetStart(startPosition);
   m_ctrlEdit.SetTargetEnd(endPosition);
   m_ctrlEdit.SetSearchFlags(iFlags);
   int iFindPos = m_ctrlEdit.SearchInTarget(iLength, pstrText);
   if( iFindPos == -1 && (iFlags & FR_WRAP) != 0 ) {
      // Failed to find in indicated direction,
      // so search from the beginning (forward) or from the end (reverse) unless wrap is false
      if( !bDirectionDown ) {
         startPosition = m_ctrlEdit.GetLength();
         endPosition = 0;
      } 
      else {
         startPosition = 0;
         endPosition = m_ctrlEdit.GetLength();
      }
      m_ctrlEdit.SetTargetStart(startPosition);
      m_ctrlEdit.SetTargetEnd(endPosition);
      iFindPos = m_ctrlEdit.SearchInTarget(iLength, pstrText);
   }
   if( iFindPos == -1 ) {
      // Bring up another Find dialog
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("gui.document.findMessages"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) != 0 ) bWarnings = false;
      if( bWarnings ) {
         CString sMsg;
         sMsg.Format(IDS_FINDFAILED, CString(pstrText));
         _pDevEnv->ShowMessageBox(m_hWnd, (LPCTSTR) sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONINFORMATION);
         HWND hWndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
         ::PostMessage(hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_FIND, 0), 0L);
      }
   } 
   else {
      int start = m_ctrlEdit.GetTargetStart();
      int end = m_ctrlEdit.GetTargetEnd();
      // EnsureRangeVisible
      int lineStart = m_ctrlEdit.LineFromPosition(min(start, end));
      int lineEnd = m_ctrlEdit.LineFromPosition(max(start, end));
      for( int line = lineStart; line <= lineEnd; line++ ) m_ctrlEdit.EnsureVisible(line);
      m_ctrlEdit.SetSel(start, end);
   }
   return iFindPos;
}

/**
 * Handle auto-completion.
 * Here we handle auto-completion of the C++ language. Support for other languages is
 * located in the default handler of the GenEdit component.
 * Getting auto-completion to work for C++ is a very labour intensive job, and we need
 * to manually scan and verify parts of the surrounding source-code.
 * The auto-completion here is very basic since we don't do an exchaustive parse of
 * the C++ file. We are in fact 'guessing' the scope, and not trying to determine the
 * exact parse-tree for the file.
 */
void CScintillaView::_AutoComplete(CHAR ch)
{
   USES_CONVERSION;
   if( m_pCppProject == NULL ) return;
   if( !m_bAutoComplete && ch != '\b' ) return;

   const WORD AUTOCOMPLETE_AFTER_TYPED_CHARS = 3;
   long lPos = m_ctrlEdit.GetCurrentPos();

   // Show auto-completion or not? The \b char forces to show!
   bool bShow = ch == '\b';
   // Attempt auto-completion in the following cases:
   //    foo.
   //    foo->
   //    CFoo::
   if( ch == '.' || ch == '>' || ch == ':' ) bShow = true;
   // Attempt auto-completion after 3 characters have been typed on a new line.
   // We'll later check if this really could be time to auto-complete!
   static WORD s_nChars = 0;
   if( ch == '\n' || ch == ';' || ch == ' ' || ch == '\t' ) s_nChars = 0;
   if( _iscppchar(ch) && ++s_nChars == AUTOCOMPLETE_AFTER_TYPED_CHARS ) bShow = true;
   // Don't popup too close to start
   if( lPos < 10 ) bShow = false;
   // Don't auto-complete comments & strings
   if( !_IsRealCppEditPos(lPos - 1) ) bShow = false;
   if( !_IsRealCppEditPos(lPos - 2) ) bShow = false;
   // So?
   if( !bShow ) return;

   MEMBERINFO info;
   long lStartPos = lPos;
   if( !_iscppchar(ch) ) lStartPos--;
   _GetMemberInfo(lStartPos, info);

   if( info.sType.IsEmpty() ) info.sType = info.sScope;
   if( info.sType.IsEmpty() ) return;

   // Yippie, we found one!!!
   CSimpleValArray<TAGINFO*> aList;
   m_pCppProject->m_TagManager.GetMemberList(info.sType, aList, true);
   int nCount = aList.GetSize();
   if( nCount == 0 || nCount > 300 ) return;

   // Need to sort the items Scintilla-style
   for( int a = 0; a < nCount; a++ ) {
      for( int b = a + 1; b < nCount; b++ ) {
         // Right; Scintilla uses strcmp() to compile its items.
         if( strcmp(T2CA(aList[a]->pstrName), T2CA(aList[b]->pstrName)) > 0 ) {
            TAGINFO* pTemp1 = aList[a];
            TAGINFO* pTemp2 = aList[b];
            aList.SetAtIndex(a, pTemp2);
            aList.SetAtIndex(b, pTemp1);
         }
      }
   }   

   // Right, Scintilla requires a list of space-separated tokens
   // so let's build one...
   CString sList;
   TCHAR szText[200] = { 0 };
   for( int i = 0; i < nCount; i++ ) {
      // Avoid duplciated names in list
      if( i > 0 && _tcscmp(aList[i]->pstrName, aList[i - 1]->pstrName) == 0 ) continue;
      // Don't include operator overloads
      if( !_iscppchar(aList[i]->pstrName[0]) ) continue;
      // Add this baby...
      _tcsncpy(szText, aList[i]->pstrName, (sizeof(szText) / sizeof(TCHAR)) - 3);
      _tcscat(szText, aList[i]->Type == TAGTYPE_MEMBER ? _T("?0 ") : _T("?1 "));
      sList += szText;
   }
   sList.TrimRight();

   // Display auto-completion popup
   _RegisterListImages();
   m_ctrlEdit.AutoCSetIgnoreCase(FALSE);
   m_ctrlEdit.AutoCShow(info.sName.GetLength(), T2CA(sList));
}

/**
 * Show function tip.
 * Attempts to determine the function syntax of the currently entered
 * function call (if any). The algorithm to resolve the types/signatures
 * is somewhat less complete than the auto-completion code.
 */
void CScintillaView::_FunctionTip(CHAR ch)
{
   if( m_pCppProject == NULL ) return;
   if( !m_bAutoComplete ) return;
   // Closing the function? Remove the tip.
   if( ch == ')' ) {
      if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
      return;
   }
   ATLASSERT(ch=='(');
   // Get information about the function below
   MEMBERINFO info;
   long lPos = m_ctrlEdit.GetCurrentPos() - 2;
   if( !_IsRealCppEditPos(lPos) ) return;
   if( !_GetMemberInfo(lPos, info) ) return;
   // Remove all non-functions since we're currently asking to see a function
   // signature only
   for( int i = info.aDecl.GetSize() - 1; i >= 0; i-- ) {
      if( info.aDecl[i].Find('(') < 0 ) info.aDecl.RemoveAt(i);
   }
   // Show tooltip
   _ShowMemberToolTip(lPos, &info, 0, false, RGB(0,0,0), RGB(255,255,255));
}

/**
 * Remove squiggly lines.
 * Removes all of the squiggly lines from the text editor.
 */
void CScintillaView::_ClearSquigglyLines()
{
   if( !m_bClearSquigglyLines ) return;  
   m_bClearSquigglyLines = false;
   m_ctrlEdit.StartStyling(0, INDIC0_MASK);
   m_ctrlEdit.SetStyling(m_ctrlEdit.GetLength(), 0);
   // FIX: Need this because next char will not get
   //      coloured properly in Scintilla.
   m_ctrlEdit.Colourise(0, -1);
}

/**
 * Extract information about a C++ member at a editor position.
 */
bool CScintillaView::_GetMemberInfo(long lPos, MEMBERINFO& info)
{
   // There are some limitations to our capabilities...
   if( lPos < 10 ) return false;
   // Find the starting position of the name below the cursor
   CSimpleArray<CString> aRes;
   long lStartPos = lPos;
   while( _iscppchar( (char) m_ctrlEdit.GetCharAt(lStartPos) ) ) lStartPos--;
   // Determine the scope of the function/member
   info.sScope = _FindBlockType(lPos);
   // Try to figure out if we're just starting to type a new member
   CHAR chDelim = m_ctrlEdit.GetCharAt(lStartPos);
   if( chDelim == '.' || chDelim == '>' || chDelim == ':' ) {
      // Oh, it's a member of another type
      info.sName = _GetNearText(lStartPos + 1);
      long lOffset = chDelim == '.' ? 0 : 1;
      CString sParent = _GetNearText(lStartPos - lOffset);
      if( !sParent.IsEmpty() ) {
         info.sType = _FindTagType(sParent, lStartPos);
         if( info.sType.IsEmpty() ) {
            CSimpleArray<CString> aLocalType;
            m_pCppProject->GetTagInfo(sParent, false, aLocalType, info.sScope);
            if( aLocalType.GetSize() == 1 ) {
               info.sType = aLocalType[0];
               info.sType.Replace(_T("const "), _T(""));
               info.sType.Replace(_T("inline "), _T(""));
               info.sType.Replace(_T("extern "), _T(""));
               info.sType.Replace(_T("virtual "), _T(""));
               int iPos = info.sType.FindOneOf(_T(" \t(*&"));
               if( iPos > 0 ) info.sType = info.sType.Left(iPos);
               // This is a complex type; let's set the new scope...
               info.sScope = info.sType;
            }
         }
         else {
            // Local type found; let's investigate if this is a complex type
            CSimpleArray<CString> aLocalType;
            if( m_pCppProject->GetTagInfo(info.sType, false, aLocalType, NULL) ) {
               info.sScope = info.sType;
            }
         }
      }
      if( chDelim != ':' && info.sType.IsEmpty() ) {
         info.sType = _FindTagType(info.sName, lStartPos);
         if( !info.sType.IsEmpty() ) info.sScope = info.sType;
      }
   }
   else {
      // It's just a regular function or variable
      info.sName = _GetNearText(lPos);
      if( m_ctrlEdit.GetCharAt(lStartPos + info.sName.GetLength() + 1) == ':' ) info.sScope.Empty();
      CSimpleArray<CString> aLocalType;
      m_pCppProject->GetTagInfo(info.sName, false, aLocalType, info.sScope);
      if( aLocalType.GetSize() != 1 ) {
         info.sType = _FindTagType(info.sName, lPos);
         CSimpleArray<CString> aLocalType;
         if( m_pCppProject->GetTagInfo(info.sType, false, aLocalType, NULL) ) {
            info.sScope = info.sType;
         }
      }
   }
   // Didn't catch the type yet? Let's examine for a local type
   if( !info.sName.IsEmpty() && info.sType.IsEmpty() ) {
      CSimpleArray<CString> aLocalType;
      m_pCppProject->GetTagInfo(info.sName, false, aLocalType, info.sScope);
      if( aLocalType.GetSize() == 1 ) {
         info.sType = aLocalType[0];
         info.sType.Replace(_T("const "), _T(""));
         info.sType.Replace(_T("inline "), _T(""));
         info.sType.Replace(_T("extern "), _T(""));
         info.sType.Replace(_T("virtual "), _T(""));
         int iPos = info.sType.FindOneOf(_T(" \t(*&"));
         if( iPos > 0 ) info.sType = info.sType.Left(iPos);
      }
   }
   // Is it a global function?
   if( info.sName.IsEmpty() ) return false;
   // Now it's time to test our theory...
   if( !m_pCppProject->GetTagInfo(info.sName, false, aRes, info.sScope) ) {
      if( !m_pCppProject->GetTagInfo(info.sName, false, aRes, NULL) ) {
         if( !m_pCppProject->GetTagInfo(info.sName, false, aRes, info.sType) ) {
            if( info.sName.IsEmpty() || info.sType.IsEmpty() ) return false;
            // We still have a valid name and type; let's just return that...
            CString sType;
            sType.Format(_T("%s %s"), info.sType, info.sName);
            aRes.Add(sType);
            info.sScope.Empty();
         }
      }
      else {
         info.sScope.Empty();
      }
   }
   if( aRes.GetSize() == 0 ) return false;
   for( int i = 0; i < aRes.GetSize(); i++ ) info.aDecl.Add(aRes[i]);
   return true;
}

/**
 * Determine scope name.
 * This function located the class/struct-type relative to
 * the 'lPosition' location.
 */
CString CScintillaView::_FindBlockType(long lPosition)
{
   // Locate the line where this block begins.
   int iStartLine = m_ctrlEdit.LineFromPosition(lPosition);
   int iEndLine = iStartLine;
   while( iStartLine >= 0 ) {
      long lPos = m_ctrlEdit.PositionFromLine(iStartLine);
      int ch = m_ctrlEdit.GetCharAt(lPos);
      if( ch == '}' ) return _T("");
      if( isalpha(ch) && ch != 'p' ) break;        // as in 'public', 'private', 'protected'...
      iStartLine--;
   }
   if( iStartLine < 0 ) iStartLine = 0;

   CHAR szBuffer[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iStartLine) >= sizeof(szBuffer) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iStartLine, szBuffer);
  
   // Now we need to determine the scope type. We'll look for name 'xxx' in the
   // following cases:
   //    class xxx
   //    class xxx : public CFoo
   //    void xxx::Foo()
   //    struct xxx
   LPSTR p = strchr(szBuffer, ':');
   long lOffset = -2;
   if( p == NULL ) {
      p = strstr(szBuffer, "class");
      lOffset = strlen("class") + 1;
   }
   if( p == NULL ) {
      p = strstr(szBuffer, "struct");
      lOffset = strlen("struct") + 1;
   }
   LPCSTR pStart = szBuffer;
   if( p == NULL || strncmp(p, "::", 2) == 0 ) {
      // None of the above? Assume we're inside a function
      // and its formatted like:
      //    TYPE Foo()
      // Skip leading return type first...
      if( p != NULL ) *p = '\0';
      if( p != NULL ) p = strrchr(szBuffer, ' ');
      if( p == NULL ) p = strpbrk(szBuffer, " \t");
      if( p == NULL ) p = szBuffer;
      lOffset = 1;
   }
   if( p == NULL ) return _T("");

   long lLineStart = m_ctrlEdit.PositionFromLine(iStartLine);
   CString sType = _GetNearText(lLineStart + (p - szBuffer) + lOffset);
   if( sType.IsEmpty() ) return _T("");

   // Now, let's find the type in the TAG file
   int iIndex = m_pCppProject->m_TagManager.FindItem(0, sType);
   while( iIndex >= 0 ) {
      switch( m_pCppProject->m_TagManager.GetItemType(iIndex) ) {
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
         return sType;
      }
      iIndex = m_pCppProject->m_TagManager.FindItem(iIndex + 1, sType);
   }

   return _T("");
}

/**
 * Determine type from name and text-position.
 * Looks up the member type. To do so, this function must determine
 * in which scope the text is located and try to deduce what members/function
 * the text is placed in.
 */
CString CScintillaView::_FindTagType(const CString& sName, long lPosition)
{
   // Don't waste time of silly strings
   if( sName.IsEmpty() ) return _T("");
   
   // Locate the line where this function begins.
   // The function signature is important to get parsed, because
   // it will contain definitions of local members as well.
   // HACK: We look for the line where the text starts at column 1.
   int iStartLine = m_ctrlEdit.LineFromPosition(lPosition);
   int iEndLine = iStartLine;
   bool bFoundScope = false;
   while( iStartLine > 0 ) {
      long lPos = m_ctrlEdit.PositionFromLine(iStartLine);
      iStartLine--;
      int ch = m_ctrlEdit.GetCharAt(lPos);
      if( !isspace(ch) && bFoundScope ) break;
      if( ch == '{' || ch == '}' ) bFoundScope = true;
   }

   // See if we can find a matching type from the tag information.
   for( ; iStartLine <= iEndLine; iStartLine++ ) {
      CHAR szBuffer[256] = { 0 };
      if( m_ctrlEdit.GetLineLength(iStartLine) >= sizeof(szBuffer) - 1 ) continue;
      m_ctrlEdit.GetLine(iStartLine, szBuffer);
      
      CString sLine = szBuffer;
      for( int iColPos = sLine.Find(sName); iColPos >= 0; iColPos = sLine.Find(sName, iColPos + 1) ) 
      {
         if( iColPos == 0 ) continue;
         if( iColPos >= sLine.GetLength() - sName.GetLength() ) continue;

         if( _iscppchar(sLine[iColPos - 1]) ) continue;
         if( _iscppchar(sLine[iColPos + sName.GetLength()]) ) continue;

         // Need to guess the ending position of the type keyword.
         // Special case of:
         //    MYTYPE* a
         //    MYTYPE& a
         int iEndPos = iColPos - 1;
         while( iEndPos > 0 && _tcschr(_T(" \t*&"), sLine[iEndPos - 1]) != NULL ) iEndPos--;
         // Special case of:
         //    MYTYPE a; b->
         //    a = b->
         //    a = (CASTTYPE) b->
         TCHAR ch = sLine[iEndPos];
         if( ch == ';' || ch == ')' || ch == '=' || ch == '<' ) continue;
         // Special case of:
         //    MYTYPE a, b;
         if( iEndPos > 0 && sLine[iEndPos - 1] == ',' ) {
            iEndPos = 0;
            while( isspace(sLine[iEndPos]) ) iEndPos++;
         }

         // Extract the type string
         // We'll return the text directly from the editor as the matched type.
         int iLinePos = m_ctrlEdit.PositionFromLine(iStartLine);
         CString sType = _GetNearText(iLinePos + iEndPos, false);
         if( sType.IsEmpty() ) continue;
         
         // First look up among ordinary C++ types.
         // They are likely not to be defined in any header file.
         static LPCTSTR pstrKnownTypes[] =
         {
            _T("int"),
            _T("char"),
            _T("wchar_t"),
            _T("short"),
            _T("bool"),
            _T("long"),
            _T("float"),
            _T("double"),
            _T("void"),
            NULL
         };
         LPCTSTR* ppTypes = pstrKnownTypes;
         while( *ppTypes ) {
            if( sType.CompareNoCase(*ppTypes) == 0 ) return sType;
            ppTypes++;
         }

         // Now, let's find the type in the lex files
         int iIndex = m_pCppProject->m_TagManager.FindItem(0, sType);
         while( iIndex >= 0 ) {
            switch( m_pCppProject->m_TagManager.GetItemType(iIndex) ) {
            case TAGTYPE_ENUM:
            case TAGTYPE_DEFINE:
            case TAGTYPE_MEMBER:
            case TAGTYPE_UNKNOWN:
            case TAGTYPE_FUNCTION:
               // These are not types; but rather members of one
               // so we ignore them...
               break;
            default:
               return sType;
            }
            iIndex = m_pCppProject->m_TagManager.FindItem(iIndex + 1, sType);
         }
      }
   }
   return _T("");
}

/**
 * Show and adjust the Scintilla tooltip control.
 * This helper function splits a long tooltip text in muliple lines and 
 * adjusts the position of the Scintilla tool, most notably because it tends 
 * to be placed too close to the cursor.
 */
void CScintillaView::_ShowToolTip(long lPos, CString& sText, bool bAdjustPos, COLORREF clrText, COLORREF clrBack)
{
   const int MAX_LINE_WIDTH = 90;         // in chars
   const int SUGGESTED_LINE_WIDTH = 60;

   m_ctrlEdit.CallTipCancel();
   m_ctrlEdit.CallTipSetFore(clrText);
   m_ctrlEdit.CallTipSetBack(clrBack);

   // Make tooltip multi-line if a line exceeds 60 chars
   int cxWidth = 0;
   int nLength = sText.GetLength();
   for( int i = 0; i < nLength; i++ ) {
      cxWidth++;
      TCHAR ch = sText[i];
      if( (cxWidth > SUGGESTED_LINE_WIDTH && (ch == '(' || ch == '{' || ch == ','))
          || cxWidth > MAX_LINE_WIDTH ) 
      {
         CString sTemp = sText.Left(i + 1);
         sTemp += _T("\n    ");
         while( isspace(sText[i + 1]) ) i++;
         sTemp += sText.Mid(i + 1);
         sText = sTemp;
         nLength = sText.GetLength();
         cxWidth = 0;
      }
      if( sText[i] == '\n' ) cxWidth = 0;
   }

   // Show the tip
   USES_CONVERSION;
   m_ctrlEdit.CallTipShow(lPos, T2CA(sText));

   // Locate the new tip window and move it around...
   if( !bAdjustPos ) return;
   CToolTipCtrl ctrlTip = ::FindWindow(NULL, _T("ACallTip"));
   if( !ctrlTip.IsWindow() ) return;
   if( !ctrlTip.IsWindowVisible() ) return;
   // Move tip window a bit down so the cursor doesn't block its view
   POINT ptCursor = { 0 };
   ::GetCursorPos(&ptCursor);
   RECT rcWindow;
   ctrlTip.GetWindowRect(&rcWindow);
   if( ptCursor.y > rcWindow.bottom ) return;
   ::OffsetRect(&rcWindow, 3, 5);
   static HCURSOR hArrow = ::LoadCursor(NULL, IDC_ARROW);
   if( ::GetCursor() == hArrow ) ::OffsetRect(&rcWindow, 0, 15);
   ::InflateRect(&rcWindow, 2, 0);
   ctrlTip.SetWindowPos(HWND_TOPMOST, &rcWindow, SWP_NOACTIVATE);
}

/**
 * Show and adjust the Scintilla tooltip control.
 * This helper function splits a long tooltip text in muliple lines and 
 * adjusts the position of the Scintilla tool, most notably because it tends 
 * to be placed too close to the cursor.
 */
void CScintillaView::_ShowMemberToolTip(long lPos, MEMBERINFO* pInfo, long lCurTip, bool bExpand, COLORREF clrText, COLORREF clrBack)
{
   USES_CONVERSION;

   // Providing new tip information?
   if( pInfo != NULL ) 
   {
      m_TipInfo.lPos = lPos;
      m_TipInfo.lCurTip = lCurTip;
      m_TipInfo.clrBack = clrBack;
      m_TipInfo.clrText = clrText;
      m_TipInfo.sMemberName = pInfo->sName;
      m_TipInfo.sMemberType = pInfo->sScope;
      m_TipInfo.sMemberScope = pInfo->sScope;
      m_TipInfo.bExpand = bExpand;
      m_TipInfo.aDecl.RemoveAll();
      for( int i = 0; i < pInfo->aDecl.GetSize(); i++ ) m_TipInfo.aDecl.Add(pInfo->aDecl[i]);
   }

   if( m_TipInfo.aDecl.GetSize() == 0 ) return;

   // Find best signature match 
   if( m_TipInfo.bExpand && pInfo != NULL && m_TipInfo.aDecl.GetSize() > 1 ) {
      CHAR szLine[256] = { 0 };
      int iLineNo = m_ctrlEdit.LineFromPosition(lPos);
      if( m_ctrlEdit.GetLineLength(iLineNo) < sizeof(szLine) - 1 ) {
         m_ctrlEdit.GetLine(iLineNo, szLine);
         int nCommas = _CountCommas(A2CT(szLine));
         for( int i = 0; i < m_TipInfo.aDecl.GetSize(); i++ ) {
            if( _CountCommas(m_TipInfo.aDecl[i]) == nCommas ) lCurTip = i;
         }
      }
   }

   // Which tip to display
   m_TipInfo.lCurTip = lCurTip % m_TipInfo.aDecl.GetSize();

   CString sText = m_TipInfo.aDecl[m_TipInfo.lCurTip];
   // Multiple entries? Let's allow browsing the tip texts
   if( !m_TipInfo.bExpand 
       && m_TipInfo.aDecl.GetSize() > 1 ) 
   {
      if( m_TipInfo.lCurTip > 0 ) {
         CString sTemp = sText;
         sText.Format(_T("\001 %s"), sTemp);
      }
      if( m_TipInfo.lCurTip < m_TipInfo.aDecl.GetSize() - 1 ) {
         CString sTemp = sText;
         sText.Format(_T("\002 %s"), sTemp);
      }
   }
   // Expand function names with scope-type?
   if( m_TipInfo.bExpand 
       && !m_TipInfo.sMemberType.IsEmpty() 
       && !m_TipInfo.sMemberScope.IsEmpty() 
       && sText.FindOneOf(_T("{<")) < 0
       && sText.Find(_T(" : ")) < 0 ) 
   {
      CString sFind;
      sFind.Format(_T(" %s"), m_TipInfo.sMemberName);
      CString sReplace;
      sReplace.Format(_T(" %s::%s"), m_TipInfo.sMemberScope, m_TipInfo.sMemberName);
      sText.Replace(sFind, sReplace);
   }
   m_TipInfo.sText = sText;

   _ShowToolTip(m_TipInfo.lPos, sText, true, m_TipInfo.clrText, m_TipInfo.clrBack);
}

void CScintillaView::_SetLineIndentation(int iLine, int iIndent) 
{
   if( iIndent < 0 ) return;
   CharacterRange cr = m_ctrlEdit.GetSelection();
   int posBefore = m_ctrlEdit.GetLineIndentPosition(iLine);
   m_ctrlEdit.SetLineIndentation(iLine, iIndent);
   int posAfter = m_ctrlEdit.GetLineIndentPosition(iLine);
   int posDifference = posAfter - posBefore;
   if( posAfter > posBefore ) {
      // Move selection on
      if( cr.cpMin >= posBefore ) cr.cpMin += posDifference;
      if( cr.cpMax >= posBefore ) cr.cpMax += posDifference;
   } 
   else if( posAfter < posBefore ) {
      // Move selection back
      if( cr.cpMin >= posAfter ) {
         if( cr.cpMin >= posBefore ) cr.cpMin += posDifference; else cr.cpMin = posAfter;
      }
      if( cr.cpMax >= posAfter ) {
         if( cr.cpMax >= posBefore ) cr.cpMax += posDifference; else cr.cpMax = posAfter;
      }
   }
   m_ctrlEdit.SetSel(cr.cpMin, cr.cpMax);
}

/**
 * Return the currently selected text.
 */
CString CScintillaView::_GetSelectedText()
{
   // Get the selected text if any
   TextRange tr = { 0 };
   tr.chrg = m_ctrlEdit.GetSelection();
   tr.chrg.cpMax = min(tr.chrg.cpMax, tr.chrg.cpMin + 127);
   CHAR szText[256] = { 0 };
   tr.lpstrText = szText;
   m_ctrlEdit.GetTextRange(&tr);
   // Now validate the selection
   if( strlen(szText) == 0 ) return _T("");
   if( !_iscppchar(szText[0]) ) return _T("");
   if( strpbrk(szText, "\r\n=") != NULL ) return _T("");
   return szText;
}

/**
 * Return the text near the position.
 * This function extracts the text that is currently located at the position requested.
 * Only text that resembles C++ identifiers is returned, and the function
 * will "search" in the near-by editor content for a valid text string.
 * If no "valid" text is found, an empty string is returned.
 */
CString CScintillaView::_GetNearText(long lPosition, bool bExcludeKeywords /*= true*/)
{
   // Get the "word" text under the caret
   if( lPosition < 0 ) return _T("");
   // First get the line of text
   int iLine = m_ctrlEdit.LineFromPosition(lPosition);
   long lLinePos = m_ctrlEdit.PositionFromLine(iLine);
   CHAR szText[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLine) >= sizeof(szText) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iLine, szText);
   // We need to get a C++ identifier only, so let's find the
   // end position of the string
   int iStart = lPosition - lLinePos;
   while( iStart > 0 && _iscppchar(szText[iStart - 1]) ) iStart--;
   // Might be a special case of:
   //     CFoo<CMyType> member
   //     foo[32].iMemberVar = 0
   //     foo().iMemberVar = 0
   if( iStart > 1 && szText[iStart - 1] == '>' && szText[iStart - 2] != '-' ) while( iStart > 0 && szText[--iStart] != '<' ) /* */;
   if( iStart > 0 && szText[iStart - 1] == ']' ) while( iStart > 0 && szText[--iStart] != '[' ) /* */;
   if( iStart > 0 && szText[iStart - 1] == ')' ) while( iStart > 0 && szText[--iStart] != '(' ) /* */;
   while( iStart > 0 && _iscppchar(szText[iStart - 1]) ) iStart--;
   // Is it really an identifier?
   if( !_iscppchar(szText[iStart]) ) return _T("");
   if( isdigit(szText[iStart]) ) return _T("");
   if( bExcludeKeywords && m_ctrlEdit.GetStyleAt(lLinePos + iStart) == SCE_C_WORD ) return _T("");
   // Let's find the end then
   int iEnd = iStart;
   while( szText[iEnd] != '\0' && _iscppchar(szText[iEnd + 1]) ) iEnd++;
   szText[iEnd + 1] = '\0';
   // Cool, got it...
   return szText + iStart;
}

/**
 * Determines if the current position allows auto-completion.
 * The editor does not allow auto-completion inside comments
 * or strings (literals).
 */
bool CScintillaView::_IsRealCppEditPos(long lPos) const
{
   if( lPos <= 0 ) return false;
   switch( m_ctrlEdit.GetStyleAt(lPos) ) {
   case SCE_C_STRING:
   case SCE_C_STRINGEOL:
   case SCE_C_COMMENT:
   case SCE_C_COMMENTDOC:
   case SCE_C_COMMENTLINE:
      return false;
   default:
      return true;
   }
}

bool CScintillaView::_HasSelection() const
{
   long nMin = m_ctrlEdit.GetSelectionStart();
   long nMax = m_ctrlEdit.GetSelectionEnd();
   return nMin != nMax;
}

int CScintillaView::_CountCommas(LPCTSTR pstrText) const
{
   int nCount = 0;
   while( *pstrText != '\0' ) {
      if( *pstrText == ',' ) nCount++;
      if( *pstrText == '(' ) nCount = 0;
      if( *pstrText == ')' ) break;
      pstrText = ::CharNext(pstrText);
   }
   return nCount;
}

bool CScintillaView::_iscppchar(CHAR ch) const
{
   return isalnum(ch) || ch == '_';
}

bool CScintillaView::_iscppchar(WCHAR ch) const
{
   return iswalnum(ch) || ch == '_';
}

/* XPM */
static char* MemberImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 4 1",
/* colors */
"a c None",
"b c #808080",
"c c #00FFFF",
"d c #000a00",
/* pixels */
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaabaaaaaaaa",
"aaaaaabcbaaaaaaa",
"aaaaabcccbaaaaaa",
"aaaadcccccbaaaaa",
"aaaaddcccccbaaaa",
"aaaadbdcccbbaaaa",
"aaaadbbdcbcbaaaa",
"aaaaadbbdccbaaaa",
"aaaaaadbdcbaaaaa",
"aaaaaaaddbaaaaaa",
"aaaaaaaadaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
};
static char* FunctionImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 4 1",
/* colors */
"a c None",
"b c #808080",
"c c #008080",
"d c #000a00",
/* pixels */
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaabaaaaaaaa",
"aaaaaabcbaaaaaaa",
"aaaaabcccbaaaaaa",
"aaaadcccccbaaaaa",
"aaaaddcccccbaaaa",
"aaaadbdcccbbaaaa",
"aaaadbbdcbcbaaaa",
"aaaaadbbdccbaaaa",
"aaaaaadbdcbaaaaa",
"aaaaaaaddbaaaaaa",
"aaaaaaaadaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
};

void CScintillaView::_RegisterListImages()
{
   m_ctrlEdit.ClearRegisteredImages();
   m_ctrlEdit.RegisterImage(0, (LPBYTE) MemberImage);
   m_ctrlEdit.RegisterImage(1, (LPBYTE) FunctionImage);
}

