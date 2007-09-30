
#include "StdAfx.h"
#include "resource.h"

#include "ScintillaView.h"
#include "SciLexer.h"

#include "Project.h"
#include "Files.h"

#include "AcListBox.h"


#pragma code_seg( "EDITOR" )

#pragma comment(lib, "../GenEdit/Lib/GenEdit.lib")

enum { DEF_INDIC_ERRORS = 8 };

// Constructor

CScintillaView::CScintillaView() :
   m_ctrlEdit(this, 1),
   m_pCppProject(NULL),
   m_pView(NULL),
   m_bDwellEnds(true),
   m_bDelayedHoverData(false)
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

   // Let's obey the LineEndConversion rules
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.general.eolMode"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("cr")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CR);
   else if( _tcscmp(szBuffer, _T("lf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_LF);
   else if( _tcscmp(szBuffer, _T("crlf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CRLF);

   // Return the text from the editor...
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

   // Adjust the line-number margin width depending
   // on how many lines there is in the document!
   int iWidth = m_ctrlEdit.GetMarginWidthN(0);
   if( iWidth > 0 && m_ctrlEdit.GetLineCount() > 9999 ) m_ctrlEdit.SetMarginWidthN(0, m_ctrlEdit.TextWidth(STYLE_LINENUMBER, "_99999"));  

   // Let Scintilla repaint the view
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
   // Windows or application is shutting down.
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

   // Get active breakpoints for this file.
   // We call this here because WM_SETTINGCHANGE is one of the messages
   // sent at startup, and a new view should display its breakpoints.
   if( m_pCppProject != NULL ) m_pCppProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_BREAKPOINTS);

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
   typedef BOOL (APIENTRY* PFNSHOWPAGE)(IDevEnv*, LPCWSTR, LPCWSTR);
   PFNSHOWPAGE fnShowHelp = (PFNSHOWPAGE) ::GetProcAddress(s_hInst, "ManPages_ShowHelp");
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

   SetFocus();

   // Get the cursor position
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   POINT ptLocal = pt;
   ScreenToClient(&ptLocal);
   if( lParam == (LPARAM) -1 ) {
      long lPos = m_ctrlEdit.GetCurrentPos();
      pt.x = m_ctrlEdit.PointXFromPosition(lPos);
      pt.y = m_ctrlEdit.PointYFromPosition(lPos);
   }

   // Place cursor at mouse if not clicked inside a selection
   long lPos = m_ctrlEdit.PositionFromPoint(ptLocal.x, ptLocal.y);
   CharacterRange cr = m_ctrlEdit.GetSelection();
   if( lPos < cr.cpMin || lPos > cr.cpMax ) m_ctrlEdit.GotoPos(lPos);

   if( !m_pCppProject->m_DebugManager.IsDebugging() ) {
      // Is there an include directive under the cursor or a member that we can look
      // up? Add additional menu-items to edit menu.
      CString sMenuText;
      CMenuHandle menu = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
      CMenuHandle submenu = menu.GetSubMenu(1);
      CString sIncludeFile = _FindIncludeUnderCursor(lPos);
      if( !sIncludeFile.IsEmpty() ) 
      {
         m_PopupInfo.sIncludeFile = sIncludeFile;
         sMenuText.Format(IDS_MENU_OPENINCLUDE, sIncludeFile);
         submenu.InsertMenu(0, MF_BYPOSITION | MF_ENABLED, ID_EDIT_OPENINCLUDE, sMenuText);
      }
      else 
      {
         CTagDetails Info;
         if( _GetMemberInfo(lPos, Info) && !Info.sFilename.IsEmpty() ) {
            CString sImplLookup;
            sImplLookup.Format(_T("%s%s%s"), Info.sBase, Info.sBase.IsEmpty() ? _T("") : _T("::"), Info.sName);
            // Insert "Open Declaration" menu item
            m_PopupInfo.DeclTag = Info;
            int iMenuIdx = 0;
            sMenuText.Format(IDS_MENU_OPENDECLARATION, sImplLookup);
            submenu.InsertMenu(iMenuIdx++, MF_BYPOSITION | MF_ENABLED, ID_EDIT_OPENDECLARATION, sMenuText);
            // Insert "Open Implementation" menu item
            CSimpleValArray<TAGINFO*> aImplResult;
            if( m_pCppProject->m_TagManager.FindItem(sImplLookup, NULL, false, aImplResult) ) {
               if( aImplResult[0]->Type == TAGTYPE_IMPLEMENTATION ) {
                  CTagDetails Info;
                  m_pCppProject->m_TagManager.GetItemInfo(aImplResult[0], m_PopupInfo.ImplTag);
                  sMenuText.Format(IDS_MENU_OPENIMPLEMENTATION, sImplLookup);
                  submenu.InsertMenu(iMenuIdx++, MF_BYPOSITION | MF_ENABLED, ID_EDIT_OPENIMPLEMENTATION, sMenuText);
               }
            }
         }
      }
      // Just continue to display the standard menu from the GenEdit component.
      // We'll ignore all our previous work to position the menu...
      return 0;
   }

   // We're in debug mode and should display the debug menu.
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
   m_bDelayedHoverData = false;
   m_bDwellEnds = true;
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
      // Show nasty error dialog; allow user to CANCEL to stay alive!
      TCHAR szName[128] = { 0 };
      m_pView->GetName(szName, 127);
      CString sMsg;
      sMsg.Format(IDS_ERR_FILESAVE, szName, GetSystemErrorText(dwErr));
      if( IDOK == _pDevEnv->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONEXCLAMATION | MB_OKCANCEL) ) return 0;
      return 1; // Return ERROR indication
   }

   // Using the C++ realtime scanner? We should parse the new file,
   // merge the tags if possible and rebuild the ClassView.
   if( m_sLanguage == _T("cpp") && m_pCppProject != NULL ) {
      if( m_pCppProject->m_TagManager.m_LexInfo.IsAvailable() ) {
         LPSTR pstrText = NULL;
         if( !GetText(pstrText) ) return 0;
         if( !m_pCppProject->m_TagManager.m_LexInfo.MergeFile(m_sFilename, pstrText, 1) ) free(pstrText);
      }
   }

   _pDevEnv->PlayAnimation(FALSE, 0);

   _ClearAllSquigglyLines();

   return 0;
}

LRESULT CScintillaView::OnEditOpenInclude(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
   CWaitCursor cursor;
   if( !m_pCppProject->OpenView(m_PopupInfo.sIncludeFile, 0) ) ::MessageBeep((UINT)-1);
   return 0;
}

LRESULT CScintillaView::OnEditOpenDeclaration(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
   CWaitCursor cursor;
   if( !m_pCppProject->m_TagManager.OpenTagInView(m_PopupInfo.DeclTag) ) ::MessageBeep((UINT)-1);
   return 0;
}

LRESULT CScintillaView::OnEditOpenImplementation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
   CWaitCursor cursor;
   if( !m_pCppProject->m_TagManager.OpenTagInView(m_PopupInfo.ImplTag) ) ::MessageBeep((UINT)-1);
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
   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return 0;

   int iLine = m_ctrlEdit.GetCurrentLine();
   long lValue = m_ctrlEdit.MarkerGet(iLine);
   long lMarkerMask = (1 << MARKER_BREAKPOINT);

   CString sName;
   m_pView->GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   if( (lValue & lMarkerMask) != 0 ) {
      bool bRes = m_pCppProject->m_DebugManager.RemoveBreakpoint(sName, iLine);
      // HACK: Because the breakpoint may have been moved due to editing
      //       we'll try to delete any breakpoint in the neighborhood.
      const int DEBUG_LINE_FUDGE = 4;
      for( int iOffset = 1; !bRes && iOffset < DEBUG_LINE_FUDGE; iOffset++ ) {
         bRes = m_pCppProject->m_DebugManager.RemoveBreakpoint(sName, iLine + iOffset);
         if( !bRes ) bRes = m_pCppProject->m_DebugManager.RemoveBreakpoint(sName, iLine - iOffset);
      }
      if( bRes ) m_ctrlEdit.MarkerDelete(iLine, MARKER_BREAKPOINT);
      else ::MessageBeep((UINT)-1);
   }
   else {
      if( m_pCppProject->m_DebugManager.AddBreakpoint(sName, iLine) ) {
         m_ctrlEdit.MarkerAdd(iLine, MARKER_BREAKPOINT);
      }
   }
   return 0;
}

LRESULT CScintillaView::OnDebugRunTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return 0;

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
   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return 0;

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
            int iLineNum = pData->iLineNum;
            if( iLineNum >= 0 ) {
               m_ctrlEdit.MarkerAdd(iLineNum - 1, MARKER_CURLINE);
               m_ctrlEdit.EnsureVisible(iLineNum - 1);
            }
         }
         if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
      }
      break;
   case DEBUG_CMD_SET_RUNNING:
      {
         // Replace the current-line marker with a running-state marker
         int iLineNum = m_ctrlEdit.MarkerNext(0, 1 << MARKER_CURLINE);
         if( iLineNum >= 0 ) {
            m_ctrlEdit.MarkerDelete(iLineNum, MARKER_CURLINE);
            m_ctrlEdit.MarkerAdd(iLineNum, MARKER_RUNNING);
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
         int iLineNum = 0;
         CSimpleArray<int> aLines;
         while( (iLineNum = m_ctrlEdit.MarkerNext(iLineNum, 1 << MARKER_BREAKPOINT)) >= 0 ) {
            aLines.Add(iLineNum);
            iLineNum++;
         }
         m_pCppProject->m_DebugManager.SetBreakpoints(sName, aLines);
      }
      break;
   case DEBUG_CMD_SET_BREAKPOINTS:
      {
         if( m_pCppProject == NULL ) return 0;
         CString sName;
         m_pView->GetName(sName.GetBufferSetLength(128), 128);
         sName.ReleaseBuffer();
         CSimpleArray<int> aLines;
         m_pCppProject->m_DebugManager.GetBreakpoints(sName, aLines);
         m_ctrlEdit.MarkerDeleteAll(MARKER_BREAKPOINT);
         for( int i = 0; i < aLines.GetSize(); i++ ) m_ctrlEdit.MarkerAdd(aLines[i], MARKER_BREAKPOINT);
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
         if( !m_bDelayedHoverData ) return 0;
         m_bDelayedHoverData = false;
         CString sValue = m_TipInfo.sText;
         if( !sValue.IsEmpty() ) sValue += _T(" = ");
         sValue += pData->MiInfo.GetItem(_T("value"));
         _ShowToolTip(m_TipInfo.lPos, sValue, true, true, ::GetSysColor(COLOR_INFOTEXT), ::GetSysColor(COLOR_INFOBK));
      }
      break;
   case DEBUG_CMD_DEBUG_START:
      {
         _ClearAllSquigglyLines();
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
         _ClearAllSquigglyLines();
         if( m_pCppProject != NULL && m_bMarkErrors ) {
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
         FindText(FR_DOWN | FR_WRAP | (UINT) pData->iFlags, pData->szFilename, false);
      }
      break;
   case DEBUG_CMD_FOLDCURSOR:
      {
         // Place cursor at line beginning
         int iLineNum = pData->iLineNum;
         if( iLineNum == -1 ) iLineNum = m_ctrlEdit.GetCurrentLine();
         long lPos = m_ctrlEdit.PositionFromLine(iLineNum);
         m_ctrlEdit.GotoPos(lPos);
         m_ctrlEdit.EnsureVisible(iLineNum);
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

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnModified(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;

   if( (pSCN->modificationType & SC_MOD_INSERTTEXT) != 0 ) {
      _ClearSquigglyLine(pSCN->position);
   }
   if( (pSCN->modificationType & SC_MOD_DELETETEXT) != 0 ) {
      _ClearSquigglyLine(pSCN->position);
   }

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
   // Clicked on the tooltip while we're displaying function prototypes.
   // Let's display the next/previous in the list.
   long lCurTip = m_TipInfo.lCurTip;
   if( pSCN->position == 1 ) lCurTip--;
   if( pSCN->position == 2 ) lCurTip++;
   _ShowMemberToolTip(0, NULL, lCurTip, true, false, false, false, RGB(0,0,0), RGB(0,0,0));
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDwellStart(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   SCNotification* pSCN = (SCNotification*) pnmh;

   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return 0;

   if( m_bDelayedHoverData ) return 0;
   if( m_ctrlEdit.AutoCActive() ) return 0;

   // FIX: Bug in Scintilla which tries to start a hover tip
   //      while the window has no focus.
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   if( ::GetFocus() != m_ctrlEdit ) return 0;
   if( ::WindowFromPoint(pt) != m_ctrlEdit ) return 0;

   // Display tooltips at all?
   TCHAR szBuffer[16] = { 0 };
   _pDevEnv->GetProperty(_T("editors.cpp.noTips"), szBuffer, 15);
   if( _tcscmp(szBuffer, _T("true") ) == 0 ) return 0;

   long lPos = pSCN->position;

   // See if there is an error indicator below cursor
   int iIndicValue = m_ctrlEdit.IndicatorValueAt(DEF_INDIC_ERRORS, lPos);
   if( iIndicValue != 0 ) {
      for( int i = 0; i < m_aErrorInfo.GetSize(); i++ ) {
         const ERRORMARKINFO& Info = m_aErrorInfo[i];
         if( lPos >= Info.lStartPos && lPos <= Info.lEndPos ) {
            _ShowToolTip(lPos, Info.sText, true, true, RGB(0,0,0), RGB(240,200,200));
            return 0;
         }
      }
   }

   // Assume we can show tooltip for text below cursor.
   // Get text around cursor
   if( !_IsRealCppEditPos(lPos, true) ) return 0;   
   CString sText;
   CharacterRange cr = m_ctrlEdit.GetSelection();
   if( lPos >= cr.cpMin && lPos <= cr.cpMax ) sText = _GetSelectedText();
   else sText = _GetNearText(lPos);
   if( sText.IsEmpty() ) return 0;

   // Allow the debugger to speak up; debugger may return at a later point
   // with the debug-information (hover tip) so we should record as much
   // info as we know right now.
   if( m_pCppProject->m_DebugManager.IsDebugging() ) {
      // Evaluate expression (delayed)
      m_pCppProject->m_DebugManager.EvaluateExpression(sText);
      // At this point, we expect more data from the debugger; we may not even
      // be able to resolve the member info locally, but we still have to
      // display as much data as we can.
      m_TipInfo.lPos = lPos;
      m_TipInfo.sText = sText;
      m_TipInfo.aDecl.RemoveAll();
      m_bDelayedHoverData = true;
   }

   // Ask lexer to deliver info also; the lexer can
   // give us information about type and decoration.
   CTagDetails Info;
   if( !_GetMemberInfo(lPos, Info) ) return 0;

   // Show tooltip. Even if the debugger returns delayed information we
   // still show the name immediately. If the debugger finally arrives
   // with more information, we just append that information as well.
   _ShowMemberToolTip(lPos, &Info, 0, false, true, true, true, ::GetSysColor(COLOR_INFOTEXT), ::GetSysColor(COLOR_INFOBK));
   return 0;
}

LRESULT CScintillaView::OnDwellEnd(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   if( m_bDwellEnds ) m_ctrlEdit.CallTipCancel();
   m_bDelayedHoverData = false;  // We no longer expect more data
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
   CTagDetails ParentInfo;
   if( !_GetMemberInfo(pSCN->lParam, ParentInfo, MATCH_ALLOW_PARTIAL) ) return 0;
   CSimpleValArray<TAGINFO*> aList;
   m_pCppProject->m_TagManager.FindItem(CString(pSCN->text), ParentInfo.sBase, true, aList);
   if( aList.GetSize() == 0 ) return 0;
   CTagDetails MemberInfo;
   m_pCppProject->m_TagManager.GetItemInfo(aList[0], MemberInfo);
   CString sText;
   if( MemberInfo.sDeclaration.Find('(') >= 0 ) sText = _T("(");
   if( MemberInfo.sDeclaration.Find(_T("()")) >= 0 ) sText = _T("()");
   if( MemberInfo.sDeclaration.Find(_T("( )")) >= 0 ) sText = _T("()");
   // HACK: Insert text as delayed. The notification SCN_AUTOCSELECTION is fored 
   // before the insertion of the actual character.
   for( int i = 0; i < sText.GetLength(); i++ ) {
      m_ctrlEdit.PostMessage(WM_CHAR, sText.GetAt(i), 0);
   }
   bHandled = TRUE;
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   if( m_sLanguage != _T("cpp") ) return;

   BOOL bDebugging = m_pCppProject != NULL && m_pCppProject->m_DebugManager.IsDebugging();

   pUIBase->UIEnable(ID_EDIT_COMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNCOMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENINCLUDE, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENDECLARATION, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENIMPLEMENTATION, TRUE);
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
   LPCTSTR pstrToken = _tcsstr(pstrText, m_sOutputToken);
   if( pstrToken == NULL ) return;
   if( pstrToken != pstrText && _iscppcharw(*(pstrToken - 1)) ) return;
   int iLineNum = _ttoi(pstrToken + m_sOutputToken.GetLength());
   if( iLineNum == 0 ) return;
   --iLineNum;

   // If several errors are reported on the same line we assume
   // that the first entry contained the most useful description. Otherwise
   // we easily end up highlighting the entire line always. We should
   // still collect the text though.
   if( m_aErrorInfo.GetSize() > 0 ) {
      ERRORMARKINFO& Info = m_aErrorInfo[ m_aErrorInfo.GetSize() - 1 ];
      if( Info.iLineNum == iLineNum ) {
         Info.sText += _T("\n");
         Info.sText += pstrText;
      }
      if( iLineNum <= Info.iLineNum ) return;
   }

   // Get the line so we can analyze where the error occoured.
   // The GNU compilers rarely output at which column (position) the error
   // occured so we'll have to try to match a substring or message
   // with the content of the reported line.
   CHAR szLine[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLineNum) >= sizeof(szLine) - 1 ) return;
   m_ctrlEdit.GetLine(iLineNum, szLine);

   CString sLine = szLine;
   long lLinePos = m_ctrlEdit.PositionFromLine(iLineNum);

   // Determine the position on the line the error was reported
   long lMatchPos = 0;
   int iMatchLen = 0;

   // Find text embraced in quotes.
   // GCC/G++ output has strange quote characters, so we'll match any
   // combination of quotes.
   static LPCTSTR pstrQuotes = _T("\'\"\x60");
   LPCTSTR pstrStart = _tcspbrk(pstrText, pstrQuotes);
   while( pstrStart != NULL ) {
      CString sKeyword;
      LPCTSTR p = pstrStart + 1;
      while( _iscppcharw(*p) ) sKeyword += *p++;
      while( *p && _tcschr(pstrQuotes, *p) == NULL ) p++;
      if( !sKeyword.IsEmpty() && sLine.Find(sKeyword) >= 0 ) {
         lMatchPos = lLinePos + sLine.Find(sKeyword);
         iMatchLen = p - pstrStart;
         break;
      }
      pstrStart = _tcspbrk(p, pstrQuotes);
   }
   // If no match was found, highlight the entire line
   if( lMatchPos == 0 ) {
      lMatchPos = lLinePos;
      iMatchLen = m_ctrlEdit.GetLineLength(iLineNum);
      // Let's trim the string if it contains leading spaces (looks stupid)
      LPCTSTR p = sLine;
      while( *p && _istspace(*p++) && iMatchLen > 0 ) {
         lMatchPos++;
         iMatchLen--;
      }
   }

   // Invalid selection?
   if( iMatchLen <= 0 ) return;

   // Apply the squiggly lines
   m_ctrlEdit.IndicSetStyle(DEF_INDIC_ERRORS, INDIC_SQUIGGLE);
   m_ctrlEdit.IndicSetFore(DEF_INDIC_ERRORS, RGB(200,0,0));
   m_ctrlEdit.SetIndicatorCurrent(DEF_INDIC_ERRORS);
   m_ctrlEdit.SetIndicatorValue(0);
   m_ctrlEdit.IndicatorFillRange(lMatchPos, iMatchLen);
  
   ERRORMARKINFO Info;
   Info.iLineNum = iLineNum;
   Info.lStartPos = lMatchPos;
   Info.lEndPos = lMatchPos + iMatchLen;
   Info.sText = pstrText;
   m_aErrorInfo.Add(Info);
}

// Implementation

/**
 * Find the next text snippet.
 * This method is usually called during a Find or Find/Replace operation,
 * and locates a substring in the text.
 */
int CScintillaView::FindText(UINT uFlags, LPCTSTR pstrPattern, bool bShowWarnings)
{
   USES_CONVERSION;
   LPCSTR pstrText = T2CA(pstrPattern);
   int cchText = strlen(pstrText);
   if( cchText == 0 ) return -1;

   bool bDirectionDown = (uFlags & FR_DOWN) != 0;
   bool bInSelection = (uFlags & FR_INSEL) != 0;

   TCHAR szBuffer[16] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.findMessages"), szBuffer, 15);
   if( _tcscmp(szBuffer, _T("true")) != 0 ) bShowWarnings = false;

   CharacterRange cr = m_ctrlEdit.GetSelection();
   int startPosition = cr.cpMax;
   int endPosition = m_ctrlEdit.GetLength();
   if( !bDirectionDown ) {
      startPosition = cr.cpMin - 1;
      endPosition = 0;
   }
   if( bInSelection ) {
      ATLASSERT(cr.cpMin!=cr.cpMax);
      if( cr.cpMin >= cr.cpMax ) return -1;
      startPosition = cr.cpMin;
      endPosition = cr.cpMax;
      uFlags &= ~FR_WRAP;
   }

   m_ctrlEdit.SetTargetStart(startPosition);
   m_ctrlEdit.SetTargetEnd(endPosition);
   m_ctrlEdit.SetSearchFlags((int)uFlags);
   int iFindPos = m_ctrlEdit.SearchInTarget(cchText, pstrText);
   if( iFindPos == -1 && (uFlags & FR_WRAP) != 0 ) {
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
      iFindPos = m_ctrlEdit.SearchInTarget(cchText, pstrText);
   }
   if( iFindPos == -1 ) {
      // Bring up another Find dialog
      if( bShowWarnings ) {
         CString sMsg;
         sMsg.Format(IDS_FINDFAILED, CString(pstrText));
         _pDevEnv->ShowMessageBox(m_hWnd, (LPCTSTR) sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONINFORMATION);
         // Bring up the Find Text window again...
         CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_EDIT_FIND, 0), 0L);
      }
   } 
   else {
      int lStartPos = m_ctrlEdit.GetTargetStart();
      int lEndPos = m_ctrlEdit.GetTargetEnd();
      // EnsureRangeVisible
      int iStartLine = m_ctrlEdit.LineFromPosition(min(lStartPos, lEndPos));
      int iEndLine = m_ctrlEdit.LineFromPosition(max(lStartPos, lEndPos));
      for( int iLineNum = iStartLine; iLineNum <= iEndLine; iLineNum++ ) m_ctrlEdit.EnsureVisible(iLineNum);
      m_ctrlEdit.SetSel(lStartPos, lEndPos);
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
void CScintillaView::_AutoComplete(int ch)
{
   USES_CONVERSION;

   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return;

   // Auto-Completion turned off?
   if( !m_bAutoComplete && ch != '\b' ) return;

   long lPos = m_ctrlEdit.GetCurrentPos();

   // Show auto-completion or not? The \b char forces to show!
   bool bShow = (ch == '\b');
   // Attempt auto-completion in the following cases:
   //    foo.
   //    foo->
   //    CFoo::
   if( ch == '.' || ch == '>' || ch == ':' ) bShow = true;
   // Attempt auto-completion after 3 characters have been typed on a new line.
   // We'll also check if this really could be time to auto-complete!
   static long s_lCharsTyped = 0;
   if( ch == '\n' || ch == ';' || ch == ' ' || ch == '\t' || ch == '=' ) s_lCharsTyped = 0;
   const WORD AUTOCOMPLETE_AFTER_TYPED_CHARS = 3;
   if( _iscppchar(ch) && ++s_lCharsTyped == AUTOCOMPLETE_AFTER_TYPED_CHARS ) bShow = true;
   // Don't popup too close to start
   if( lPos < 10 ) bShow = false;
   // Don't auto-complete inside comments & strings
   if( !_IsRealCppEditPos(lPos - 1, false) ) bShow = false;
   if( !_IsRealCppEditPos(lPos - 2, false) ) bShow = false;
   // So, does it show?
   if( !bShow ) return;

   CTagDetails Info;
   if( !_GetMemberInfo(lPos, Info, MATCH_ALLOW_PARTIAL) ) return;

   // Yippie, we found one!!!
   CSimpleValArray<TAGINFO*> aList;
   m_pCppProject->m_TagManager.GetMemberList(Info.sBase, true, aList);

   // We'll not allow 0 nor more than 300 items in the list.
   // This prevents a global-scope dropdown which would be horrible slow!
   const int MAX_LIST_COUNT = 300;
   int nCount = aList.GetSize();
   if( nCount == 0 || nCount > MAX_LIST_COUNT ) return;

   // Need to sort the items Scintilla-style [bubble-sort]
   for( int a = 0; a < nCount; a++ ) {
      for( int b = a + 1; b < nCount; b++ ) {
         // So Scintilla uses strcmp() to compare its items.
         // BUG: Obviously there's a difference between the mapping of UNICODE and MBCS
         //      but since C++ identifiers are mainly ASCII we'll cross our fingers.
         if( _tcscmp(aList[a]->pstrName, aList[b]->pstrName) > 0 ) {
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
   sList.GetBuffer(nCount * 40);
   sList.ReleaseBuffer(0);
   TCHAR szText[200] = { 0 };
   for( int i = 0; i < nCount; i++ ) {
      // Avoid duplicated names in list
      if( i > 0 && _tcscmp(aList[i]->pstrName, aList[i - 1]->pstrName) == 0 ) continue;
      // Don't include operator overloads
      if( !_iscppcharw(aList[i]->pstrName[0]) ) continue;
      // Add this baby...
      _tcsncpy(szText, aList[i]->pstrName, (sizeof(szText) / sizeof(TCHAR)) - 4);
      _tcscat(szText, aList[i]->Type == TAGTYPE_MEMBER ? _T("?0 ") : _T("?1 "));
      sList += szText;
   }
   sList.TrimRight();

   // Display auto-completion popup
   _RegisterListImages();
   m_ctrlEdit.AutoCSetMaxHeight(8);
   m_ctrlEdit.AutoCSetFillUps("([");
   m_ctrlEdit.AutoCSetIgnoreCase(FALSE);
   m_ctrlEdit.AutoCShow(Info.sName.GetLength(), T2CA(sList));

   // Subclass the listbox so we can display info window as well
   HWND hWndList = ::FindWindow(_T("ListBoxX"), NULL);
   if( hWndList != NULL ) {
      CAcListBoxCtrl* pList = new CAcListBoxCtrl();
      pList->SubclassWindow(hWndList);
      pList->Init(m_pCppProject, Info.sBase, sList);
   }
}

/**
 * Show function tip.
 * Attempts to determine the function syntax of the currently entered
 * function call (if any).
 */
void CScintillaView::_FunctionTip(int ch)
{
   if( m_sLanguage != _T("cpp") || m_pCppProject == NULL ) return;
   if( !m_bAutoComplete ) return;  
   // Closing the function? Remove the tip.
   if( ch == ')' ) {
      if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
      return;
   }
   ATLASSERT(ch=='(');
   // Get information about the function below
   CTagDetails Info;
   long lPos = m_ctrlEdit.GetCurrentPos() - 2;
   if( !_GetMemberInfo(lPos, Info) ) return;
   // Show tooltip
   _ShowMemberToolTip(lPos, &Info, 0, true, false, false, false, RGB(0,0,0), RGB(255,255,255));
}

/**
 * Remove squiggly lines.
 * Removes all of the squiggly markers from the text editor.
 */
void CScintillaView::_ClearAllSquigglyLines()
{
   if( m_aErrorInfo.GetSize() == 0 ) return;  
   m_aErrorInfo.RemoveAll();
   // Clear the visual lines
   m_ctrlEdit.SetIndicatorCurrent(DEF_INDIC_ERRORS);
   m_ctrlEdit.IndicatorClearRange(0, m_ctrlEdit.GetLength());
   // FIX: Need this because next char will not get
   //      coloured properly in Scintilla.
   m_ctrlEdit.Colourise(0, -1);
}

/**
 * Remove squiggly markers around line.
 * Removes the squiggly markers around an area defined by a line/position.
 */
void CScintillaView::_ClearSquigglyLine(long lPos)
{
   if( m_aErrorInfo.GetSize() == 0 ) return;
   // Actually we don't just clear the line when editing where a squiggly marker
   // is located, we clear the entire line and then some.
   int iLineNum = m_ctrlEdit.LineFromPosition(lPos);
   long lLineStartPos = m_ctrlEdit.PositionFromLine(iLineNum == 0 ? 0 : iLineNum - 1);
   long lLineEndPos = m_ctrlEdit.PositionFromLine(iLineNum + 2);
   m_ctrlEdit.SetIndicatorCurrent(DEF_INDIC_ERRORS);
   m_ctrlEdit.IndicatorClearRange(lLineStartPos, lLineEndPos - lLineStartPos);
}

/**
 * Extract information about a C++ member at an editor position.
 */
bool CScintillaView::_GetMemberInfo(long lPos, CTagDetails& Info, MEMBERMATCHMODE Mode)
{
   // There are some limitations to our capabilities...
   if( lPos < 10 ) return false;

   // Find the starting position of the name below the cursor
   long lStartPos = lPos;
   CString sBlockScope = _FindBlockType(lPos);

   int ch = m_ctrlEdit.GetCharAt(lPos);
   if( _iswhitechar(ch) ) ch = m_ctrlEdit.GetCharAt(--lPos);
   while( _iscppchar(ch) ) ch = m_ctrlEdit.GetCharAt(--lPos);
   int chDelim = ch;
   long lPosDelim = lPos;
   if( m_ctrlEdit.GetCharAt(lPos) == '=' ) chDelim = '=';
   if( m_ctrlEdit.GetCharAt(lPos - 1) == '=' ) chDelim = '=';

   CString sName = _GetNearText(lPos + 1);

   CString sParentName;        // Name of parent that contains the member
   CString sParentType;        // Type of parent
   CString sParentScope;       // Type (scope) of parent

   switch( chDelim ) {
   case '>':
   case ':':
   case '.':
      if( chDelim == '>' ) if( m_ctrlEdit.GetCharAt(lPos - 1) == '-' ) lPos -= 1; else return false;
      if( chDelim == ':' ) if( m_ctrlEdit.GetCharAt(lPos - 1) == ':' ) lPos -= 1; else return false;
      sParentName = _GetNearText(lPos);
      if( sParentName.IsEmpty() && chDelim == ':' ) sBlockScope = _T("");
   }

   // Find character after focused identifier
   long lEndPos = lStartPos;
   ch = m_ctrlEdit.GetCharAt(lEndPos);
   while( _iscppchar(ch) ) ch = m_ctrlEdit.GetCharAt(++lEndPos);
   while( _iswhitechar(ch) ) ch = m_ctrlEdit.GetCharAt(++lEndPos);
   int chEnd = ch;

   // When we're testing a right-hand-side expression we can deduce
   // the type if the left-hand-side is an enumeration.
   if( chDelim == '=' ) 
   {
      // Recurse to get types of members before equal-sign
      CTagDetails BeforeEqual;
      if( _GetMemberInfo(lPosDelim - 3, BeforeEqual, MATCH_LHS) ) {
         CString sLType = _UndecorateType(BeforeEqual.sDeclaration);
         CSimpleValArray<TAGINFO*> aTypeTag;
         if( m_pCppProject->m_TagManager.FindItem(sLType, NULL, false, aTypeTag) ) {
            CTagDetails LeftType;
            m_pCppProject->m_TagManager.GetItemInfo(aTypeTag[0], LeftType);
            if( LeftType.TagType == TAGTYPE_ENUM || LeftType.sDeclaration.Find(_T(" enum ")) > 0 ) {
               sParentType = sLType;
            }
         }
      }
   }

   // There's always a chance that this is a locally defined variable.
   // We can try to catch the definition in the source code block.
   if( sParentType.IsEmpty() )
   {
      // BUG: The _FindLocalVariableType() method has the unfortunate property that it
      //      also returns the return-type of class member declarations. We'll make
      //      sure to search for more types even if it returns success.
      CTagDetails LocalType;
      if( _FindLocalVariableType(sParentName.IsEmpty() ? sName : sParentName, lStartPos, LocalType) ) {
         switch( LocalType.TagType ) {
         case TAGTYPE_UNKNOWN:
         case TAGTYPE_INTRINSIC:
            sParentType = LocalType.sName;
            break;
         case TAGTYPE_CLASS:
         case TAGTYPE_STRUCT:
         case TAGTYPE_TYPEDEF:
            sParentType = sParentScope = LocalType.sName;
            break;
         case TAGTYPE_ENUM:
            if( Mode == MATCH_LHS || chDelim == ' ' ) sParentType = sParentScope = LocalType.sName;
            break;
         }
      }
   }

   // If we have seen the identifier is a member of a container, then check that.
   // We need information about the container.
   if( !sParentName.IsEmpty() ) 
   {
      CSimpleValArray<TAGINFO*> aResult;
      if( aResult.GetSize() == 0 && !sBlockScope.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sParentName, sBlockScope, true, aResult);
      if( aResult.GetSize() == 0 ) m_pCppProject->m_TagManager.FindItem(sParentName, _T(""), false, aResult);
      bool bFound = false;
      for( int i1 = 0; !bFound && i1 < aResult.GetSize(); i1++ ) {
         switch( aResult[i1]->Type ) {
         case TAGTYPE_NAMESPACE:
            sParentName = sParentType = sParentScope = _T("");
            bFound = true;
            break;
         case TAGTYPE_CLASS:
         case TAGTYPE_STRUCT:
         case TAGTYPE_TYPEDEF:
            sParentType = sParentScope = sParentName;
            bFound = true;
            break;
         }
      }
      for( int i2 = 0; !bFound && i2 < aResult.GetSize(); i2++ ) {
         switch( aResult[i2]->Type ) {
         case TAGTYPE_MEMBER:
         case TAGTYPE_FUNCTION:
            CTagDetails ParentTag;
            m_pCppProject->m_TagManager.GetItemInfo(aResult[i2], ParentTag);
            sParentType = _UndecorateType(ParentTag.sDeclaration);
            sParentScope.Empty();
            bFound = true;
            break;
         }
      }
   }

   CSimpleValArray<TAGINFO*> aResult;
   if( aResult.GetSize() == 0 && !sParentType.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, sParentType, true, aResult);
   if( aResult.GetSize() == 0 && !sBlockScope.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, sBlockScope, true, aResult);
   if( sParentName.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, _T(""), false, aResult);
   for( int i1 = 0; i1 < aResult.GetSize(); i1++ ) {
      if( chEnd == '(' ) break;
      switch( aResult[i1]->Type ) {
      case TAGTYPE_ENUM:
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i1], Info);
         Info.sMemberOfScope = _T("");
         return true;
      }
   }
   // We'll allow a prioritized list.This is the second round.
   for( int i2 = 0; i2 < aResult.GetSize(); i2++ ) {
      switch( aResult[i2]->Type ) {
      case TAGTYPE_MEMBER:
      case TAGTYPE_FUNCTION:
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i2], Info);
         Info.sMemberOfScope = Info.sBase;
         return true;
      }
   }
   // ...and the 3rd priority.
   for( int i3 = 0; i3 < aResult.GetSize(); i3++ ) {
      switch( aResult[i3]->Type ) {
      case TAGTYPE_DEFINE:
      case TAGTYPE_IMPLEMENTATION:
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i3], Info);
         Info.sMemberOfScope = sParentScope;
         return true;
      }
   }
   if( aResult.GetSize() != 0 ) return false;
   // Perhaps we just scraped the type without understanding it
   if( !sParentType.IsEmpty() ) {
      // We have very little to return; but it seems to be a valid
      // type. So let's format our own declaration. Filter out functions
      // and stuff that is complicated.
      if( ch == '(' || ch == ':' ) return false;
      Info.sName = sName;
      Info.sBase = sParentType;
      Info.sMemberOfScope = _T("");
      Info.TagType = TAGTYPE_MEMBER;
      Info.Protection = TAGPROTECTION_GLOBAL;
      Info.sFilename = _T("");
      Info.iLineNum = -1;
      Info.sDeclaration.Format(_T("%s %s"), sParentType, sName);
      Info.sComment = _T("");
      return true;
   }
   // Auto expansion mode will fall back to block scope
   if( Mode == MATCH_ALLOW_PARTIAL && sParentName.IsEmpty() && !sBlockScope.IsEmpty() ) {
      Info.sName = sName;
      Info.sBase = sBlockScope;
      Info.sMemberOfScope = _T("");
      Info.TagType = TAGTYPE_MEMBER;
      Info.Protection = TAGPROTECTION_GLOBAL;
      Info.sFilename = _T("");
      Info.iLineNum = -1;
      Info.sDeclaration = _T("");
      Info.sComment = _T("");
      return true;
   }
   return false;
}

/**
 * Determine scope name.
 * This function locates the class/struct-type relative to
 * the poisition given.
 */
CString CScintillaView::_FindBlockType(long lPos)
{
   // Locate the line where this block begins.
   int iStartLine = m_ctrlEdit.LineFromPosition(lPos);
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

   long lLinePos = m_ctrlEdit.PositionFromLine(iStartLine);
   CString sType = _GetNearText(lLinePos + (p - szBuffer) + lOffset);
   if( sType.IsEmpty() ) return _T("");

   // Now, let's find the type in the TAG file
   CSimpleValArray<TAGINFO*> aTags;
   m_pCppProject->m_TagManager.FindItem(sType, NULL, false, aTags);
   for( int i = 0; i < aTags.GetSize(); i++ ) {
      switch( aTags[i]->Type ) {
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         return sType;
      }
   }

   return _T("");
}

CString CScintillaView::_UndecorateType(CString sType)
{
   // TODO: Less hard-coding; more logic
   static LPCTSTR ppstrKeywords[] = { 
      _T("const "), 
      _T("inline "), 
      _T("extern "), 
      _T("static "), 
      _T("virtual "), 
      _T("volatile "), 
      _T("typedef "), 
      _T("struct "), 
      _T("enum "), 
      NULL 
   };
   for( LPCTSTR* pp = ppstrKeywords; *pp != NULL; pp++ ) sType.Replace(*pp, _T(""));
   // Stript trailing stuff
   int iPos = sType.FindOneOf(_T(" \t(*&"));
   if( iPos > 0 ) sType = sType.Left(iPos);
   iPos = sType.Find(_T("::"));
   if( iPos > 0 ) sType = sType.Mid(iPos + 2);
   return sType;
}

/**
 * Determine type from name and text-position.
 * Looks up the member type. To do so, this function must determine
 * in which scope the text is located and try to deduce what members/function
 * the text is placed in.
 */
bool CScintillaView::_FindLocalVariableType(const CString& sName, long lPos, CTagDetails& Info)
{
   // Don't waste time of silly strings
   if( sName.IsEmpty() ) return false;

   // Locate the line where this function begins.
   // The function signature is important to get parsed, because
   // it will contain definitions of local members as well.
   // HACK: We look for the line where the text starts at column 1.
   int iStartLine = m_ctrlEdit.LineFromPosition(lPos);
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

         if( _iscppcharw(sLine[iColPos - 1]) ) continue;
         if( _iscppcharw(sLine[iColPos + sName.GetLength()]) ) continue;
         // Special case of:
         //    MYTYPE::name
         if( sLine[iColPos + sName.GetLength()] == ':' && sLine[iColPos + sName.GetLength() + 1] == ':' ) {
            Info.sName = sName;
            Info.TagType = TAGTYPE_UNKNOWN;
            return true;
         }

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
            int iTestPos = iEndPos;
            while( _iscppcharw(sLine[iTestPos]) ) iTestPos++;
            while( sLine[iTestPos] == ':' ) iEndPos = ++iTestPos;
            for( ; iTestPos < iEndPos; iTestPos++ ) if( sLine[iTestPos] == ';' ) return false;
         }

         // Extract the type string
         // We'll return the text directly from the editor as the matched type.
         long lLinePos = m_ctrlEdit.PositionFromLine(iStartLine);
         CString sType = _GetNearText(lLinePos + iEndPos, false);
         if( sType.IsEmpty() ) continue;

         // First look up among ordinary C++ types.
         // They are likely not to be defined in any header file.
         static LPCTSTR ppstrKnownTypes[] =
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
            _T("signed"),
            _T("unsigned"),
            NULL
         };
         for( LPCTSTR* ppTypes = ppstrKnownTypes; *ppTypes != NULL; ppTypes++ ) {
            if( sType.CompareNoCase(*ppTypes) == 0 ) {
               Info.sName = sType;
               Info.TagType = TAGTYPE_INTRINSIC;
               return true;
            }
         }

         // Now, let's find the type in the lex data
         CSimpleValArray<TAGINFO*> aTags;
         m_pCppProject->m_TagManager.FindItem(sType, NULL, false, aTags);
         for( int i = 0; i < aTags.GetSize(); i++ ) {
            switch( aTags[i]->Type ) {
            case TAGTYPE_ENUM:
            case TAGTYPE_CLASS:
            case TAGTYPE_STRUCT:
            case TAGTYPE_TYPEDEF:
               m_pCppProject->m_TagManager.GetItemInfo(aTags[i], Info);
               return true;
            }
         }
      }
   }

   return false;
}

/**
/* Determine if there is an include file at the position.
/*/
CString CScintillaView::_FindIncludeUnderCursor(long lPos)
{
   CHAR szBuffer[256] = { 0 };
   int iLine = m_ctrlEdit.LineFromPosition(lPos);
   if( m_ctrlEdit.GetLineLength(iLine) >= sizeof(szBuffer) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iLine, szBuffer);
   CString sLine = szBuffer;
   sLine.TrimLeft();
   if( sLine.Left(1) != _T("#") ) return _T("");
   LPCSTR pstr = strchr(szBuffer, '<');
   if( pstr == NULL ) pstr = strchr(szBuffer, '\"');
   if( pstr == NULL ) return _T("");
   pstr++;
   CHAR szFile[MAX_PATH] = { 0 };
   LPSTR pDest = szFile;
   for( ; ; ) {
      switch( *pstr ) {
      case '>':
      case '\"':
         return szFile;
      case '\r':
      case '\n':
      case '\0':
      case ';':
         return _T("");
      default:
         if( strlen(szFile) >= MAX_PATH ) break;
         *pDest++ = *pstr++;
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
void CScintillaView::_ShowToolTip(long lPos, CString sText, bool bAdjustPos, bool bAcceptTimeout, COLORREF clrText, COLORREF clrBack)
{
   const int MAX_LINE_WIDTH = 90;         // in chars
   const int SUGGESTED_LINE_WIDTH = 60;

   if( sText.IsEmpty() ) return;

   // CTAGS declarations may have excessive spaces in the text
   sText.Replace(_T("    "), _T(" "));
   sText.Replace(_T("   "), _T(" "));
   sText.Replace(_T("  "), _T(" "));

   m_ctrlEdit.CallTipCancel();
   m_ctrlEdit.CallTipSetFore(clrText);
   m_ctrlEdit.CallTipSetBack(clrBack);

   // Wordwrap tooltip line if it exceeds 60 chars and force
   // a new-line when it exceeds 90 chars.
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
         while( i + 1 < nLength && isspace(sText[i + 1]) ) i++;
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

   m_bDwellEnds = bAcceptTimeout;

   // Locate the new tip window and move it around.
   // Scintilla places the tooltip too close to the cursor (especially
   // when it's hovering over a selection), so we'll move it ourselves.
   if( !bAdjustPos ) return;
   CToolTipCtrl ctrlTip = ::FindWindow(NULL, _T("ACallTip"));
   if( !ctrlTip.IsWindow() ) return;
   if( !ctrlTip.IsWindowVisible() ) return;
   // Move tip window a bit down so the cursor doesn't block its view.
   // Only move if the tooltip is located below the cursor, and has a valid position.
   POINT ptCursor = { 0 };
   ::GetCursorPos(&ptCursor);
   RECT rcWindow = { 0 };
   GetWindowRect(&rcWindow);
   RECT rcTip = { 0 };
   ctrlTip.GetWindowRect(&rcTip);
   if( rcTip.top <= rcWindow.top ) return;
   if( ptCursor.y >= rcTip.bottom ) return;
   static HCURSOR s_hArrowCursor = ::LoadCursor(NULL, IDC_ARROW);
   ::OffsetRect(&rcTip, 3, 5);
   if( ::GetCursor() == s_hArrowCursor ) ::OffsetRect(&rcTip, 0, 15);
   ::InflateRect(&rcTip, 2, 0);
   ctrlTip.SetWindowPos(HWND_TOPMOST, &rcTip, SWP_NOACTIVATE);
}

/**
 * Show and adjust the Scintilla tooltip control.
 * This helper function splits a long tooltip text in muliple lines and 
 * adjusts the position of the Scintilla tool, most notably because it tends 
 * to be placed too close to the cursor.
 */
void CScintillaView::_ShowMemberToolTip(long lPos, CTagDetails* pInfo, long lCurTip, bool bFilterMembers, bool bExpand, bool bAdjustPos, bool bAcceptTimeout, COLORREF clrText, COLORREF clrBack)
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
      m_TipInfo.sMemberType = pInfo->sBase;
      m_TipInfo.sMemberScope = pInfo->sMemberOfScope;
      m_TipInfo.bExpand = bExpand;
      // Collect declarations...
      m_TipInfo.aDecl.RemoveAll();
      CSimpleValArray<TAGINFO*> aResult;
      m_pCppProject->m_TagManager.FindItem(m_TipInfo.sMemberName, m_TipInfo.sMemberType, true, aResult);
      for( int i = 0; i < aResult.GetSize(); i++ ) {
         CTagDetails TagDecl;
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i], TagDecl);
         if( TagDecl.sDeclaration.Find('(') < 0 ) continue;
         m_TipInfo.aDecl.Add(TagDecl.sDeclaration);
      }
      if( !bFilterMembers && m_TipInfo.aDecl.GetSize() == 0 )m_TipInfo.aDecl.Add(pInfo->sDeclaration);
   }

   if( m_TipInfo.aDecl.GetSize() == 0 ) return;

   // Find best signature match 
   if( pInfo != NULL && m_TipInfo.aDecl.GetSize() > 1 ) {
      CHAR szLine[256] = { 0 };
      int iLineNum = m_ctrlEdit.LineFromPosition(lPos);
      if( m_ctrlEdit.GetLineLength(iLineNum) < sizeof(szLine) - 1 ) {
         m_ctrlEdit.GetLine(iLineNum, szLine);
         int nCommas = _CountCommas(A2CT(szLine));
         for( int i = 0; i < m_TipInfo.aDecl.GetSize(); i++ ) {
            if( _CountCommas(m_TipInfo.aDecl[i]) == nCommas ) lCurTip = i;
         }
      }
   }

   // Which tip to display?
   m_TipInfo.lCurTip = lCurTip % m_TipInfo.aDecl.GetSize();

   // Multiple entries? Let's allow browsing the tip texts
   CString sText = m_TipInfo.aDecl[m_TipInfo.lCurTip];
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
   // This is nice for the debug watch, since we append more information
   // to the tooltip text.
   if( m_TipInfo.bExpand 
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

   _ShowToolTip(m_TipInfo.lPos, sText, bAdjustPos, bAcceptTimeout, m_TipInfo.clrText, m_TipInfo.clrBack);
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
CString CScintillaView::_GetNearText(long lPos, bool bExcludeKeywords /*= true*/)
{
   // Get the "word" text under the caret
   if( lPos < 0 ) return _T("");
   // First get the line of text
   int iLine = m_ctrlEdit.LineFromPosition(lPos);
   long lLinePos = m_ctrlEdit.PositionFromLine(iLine);
   CHAR szText[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLine) >= sizeof(szText) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iLine, szText);
   // We need to get a C++ identifier only, so let's find the
   // end position of the string
   int iStart = lPos - lLinePos;
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
   // Let's find the end then...
   int iEnd = iStart;
   while( szText[iEnd] != '\0' && _iscppchar(szText[iEnd + 1]) ) iEnd++;
   szText[iEnd + 1] = '\0';
   // Let's exclude stuff that we know really want to see...
   if( strcmp(szText + iStart, "this") == 0 ) return _T("");
   if( bExcludeKeywords && m_ctrlEdit.GetStyleAt(lLinePos + iStart) == SCE_C_WORD ) return _T("");
   // Cool, got it...
   return szText + iStart;
}

/**
 * Determines if the current position allows auto-completion.
 * The editor does not allow auto-completion inside comments
 * or strings (literals).
 */
bool CScintillaView::_IsRealCppEditPos(long lPos, bool bIncludeNonIdentifiers) const
{
   if( lPos <= 0 ) return false;
   switch( m_ctrlEdit.GetStyleAt(lPos) ) {
   case SCE_C_STRING:
   case SCE_C_STRINGEOL:
   case SCE_C_COMMENT:
   case SCE_C_COMMENTDOC:
   case SCE_C_COMMENTLINE:
   case SCE_C_COMMENTLINEDOC:
   case SCE_C_COMMENTDOCKEYWORD:
   case SCE_C_COMMENTDOCKEYWORDERROR:
      return false;
   case SCE_C_UUID:
   case SCE_C_NUMBER:
      return bIncludeNonIdentifiers;
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

bool CScintillaView::_iscppchar(int ch) const
{
   return isalnum(ch) || ch == '_' || ch == '~';
}

bool CScintillaView::_iscppcharw(WCHAR ch) const
{
   return iswalnum(ch) || ch == '_' || ch == '~';
}

bool CScintillaView::_iswhitechar(int ch) const
{
   return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
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

