
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
   m_lDwellPos(0),
   m_bMouseDwell(false),
   m_bSuggestionDisplayed(false)
{
}

// Operations

void CScintillaView::OnFinalMessage(HWND /*hWnd*/)
{
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
   // Create Scintilla editor
   m_ctrlEdit.SubclassWindow( Bvrde_CreateScintillaView(m_hWnd, _pDevEnv, m_sFilename, m_sLanguage) );
   ATLASSERT(m_ctrlEdit.IsWindow());
   m_ctrlEdit.ShowWindow(SW_SHOW);
   return 0;
}

LRESULT CScintillaView::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   if( m_ctrlEdit.GetModify() ) {
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
   TCHAR szBuffer[64] = { 0 };
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);

   m_bAutoIndent = false;
   if( _pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bAutoIndent = _tcscmp(szBuffer, _T("auto")) == 0;
   m_bSmartIndent = false;
   if( _pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bSmartIndent = _tcscmp(szBuffer, _T("smart")) == 0;
   m_bProtectDebugged = false;
   if( _pDevEnv->GetProperty(sKey + _T("protectDebugged"), szBuffer, 63) ) m_bProtectDebugged = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoComplete = false;
   if( _pDevEnv->GetProperty(sKey + _T("autoComplete"), szBuffer, 63) ) m_bAutoComplete = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoSuggest = false;
   if( _pDevEnv->GetProperty(sKey + _T("autoSuggest"), szBuffer, 63) ) m_bAutoSuggest = _tcscmp(szBuffer, _T("true")) == 0;

   SendMessageToDescendants(WM_SETTINGCHANGE);
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

LRESULT CScintillaView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   // Eat TAB key for auto-suggestion
   if( wParam == VK_TAB && m_bSuggestionDisplayed ) return 0;
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{   
   SetFocus();

   long lPos = m_ctrlEdit.GetCurrentPos();
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

   UINT nRes = IDR_EDIT_TEXT;
   if( m_pCppProject 
       && m_sLanguage == _T("cpp") ) 
   {
      nRes = IDR_EDIT_CPP;
      if( m_pCppProject->m_DebugManager.IsDebugging() ) nRes = IDR_EDIT_DEBUG;
   }

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
   if( lPos < cr.cpMin || lPos > cr.cpMax ) m_ctrlEdit.SetSel(lPos, lPos);

   CMenu menu;
   menu.LoadMenu(nRes);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   _pDevEnv->ShowPopupMenu(NULL, submenu, pt);
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
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   if( m_pView == NULL ) return 0;
   if( !m_ctrlEdit.GetModify() ) return 0;
   if( !m_pView->Save() ) {
      TCHAR szName[128] = { 0 };
      m_pView->GetName(szName, 127);
      CString sMsg;
      sMsg.Format(IDS_ERR_FILESAVE, szName);
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONEXCLAMATION);
      return 1; // Return ERROR indication
   }
   return 0;
}

LRESULT CScintillaView::OnEditAutoComplete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _AutoComplete('\b');
   if( !m_ctrlEdit.AutoCActive() ) _AutoSuggest('\b');
   return 0;
}

LRESULT CScintillaView::OnDebugBreakpoint(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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

LRESULT CScintillaView::OnDebugRunTo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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

LRESULT CScintillaView::OnDebugSetNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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
      //       doesn't react normal on the breakpoint either <sign> so
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
         USES_CONVERSION;
         CString sValue = pData->MiInfo.GetItem(_T("value"));
         m_ctrlEdit.CallTipSetFore(RGB(0,0,0));
         m_ctrlEdit.CallTipSetBack(RGB(255,255,255));
         m_ctrlEdit.CallTipShow(m_lDwellPos, T2CA(sValue));
         m_bMouseDwell = false;
         m_bSuggestionDisplayed = false;
      }
      break;
   case DEBUG_CMD_DEBUG_START:
      {
         if( m_bProtectDebugged ) m_ctrlEdit.SetReadOnly(TRUE);
      }
      break;
   case DEBUG_CMD_DEBUG_STOP:
      {
         if( m_bProtectDebugged ) m_ctrlEdit.SetReadOnly(FALSE);
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
         m_ctrlEdit.SetSel(lPos, lPos);
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
   case '.':
   case '>':
   case ':':
      _AutoComplete(pSCN->ch);
      break;
   case ')':
   case '(':
      _FunctionTip(pSCN->ch);
      break;
   }
   _AutoSuggest(pSCN->ch);

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
      m_ctrlEdit.SetSel(pSCN->position, pSCN->position);
      PostMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_BREAKPOINT, 0));
      return 0;
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDwellStart(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   // HACK: Bug in Scintilla which tries to start a hover tip
   //       while the window has no focus.
   if( ::GetFocus() != m_ctrlEdit ) return 0;

   if( m_pCppProject ) {
      TCHAR szBuffer[16] = { 0 };
      _pDevEnv->GetProperty(_T("editors.cpp.noTips"), szBuffer, 15);
      if( _tcscmp(szBuffer, _T("true") ) != 0 ) {
         long lPos = pSCN->position;
         CString sText = _GetNearText(lPos);
         CharacterRange cr = m_ctrlEdit.GetSelection();
         if( lPos >= cr.cpMin && lPos <= cr.cpMax ) sText = _GetSelectedText();
         if( sText.IsEmpty() ) return 0;        
         // Find raw declaration in TAG file or from debugger
         CString sValue = m_pCppProject->GetTagInfo(sText);
         if( sValue.IsEmpty() ) {
            // Try to locate the function and variable name
            // BUG: The following just picks a "random" position in the line
            //      It should calculate where the variable name is likely to start
            CString sMember = _GetNearText(lPos - sText.GetLength() - 2);
            CString sType = _FindTagType(sMember, lPos);
            if( !sType.IsEmpty() ) sValue = m_pCppProject->GetTagInfo(sText, sType);
         }
         if( sValue.IsEmpty() ) {
            // Just see if we can match up a type directly for the name
            CString sType = _FindTagType(sText, lPos);
            if( !sType.IsEmpty() ) sValue.Format(_T("%s %s"), sType, sText);
         }
         // We always flag the MouseDwell start because the debugger
         // might want to deliver delayed type information.
         // The SCN_DWELLEND should reset this state.
         m_lDwellPos = lPos;
         m_bMouseDwell = true;
         // The call may not return any tag information at all - or it
         // may return delayed information. In any case, we might not need
         // to show any information now!
         if( !sValue.IsEmpty() ) {
            USES_CONVERSION;
            m_ctrlEdit.CallTipSetFore(RGB(0,0,0));
            m_ctrlEdit.CallTipSetBack(RGB(255,255,255));
            m_ctrlEdit.CallTipShow(lPos, T2CA(sValue));
         }
      }
   }
   return 0;
}

LRESULT CScintillaView::OnDwellEnd(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
   m_bMouseDwell = false;
   m_bSuggestionDisplayed = false;
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   if( m_sLanguage == _T("cpp") ) {
      pUIBase->UIEnable(ID_EDIT_COMMENT, TRUE);
      pUIBase->UIEnable(ID_EDIT_UNCOMMENT, TRUE);
      pUIBase->UIEnable(ID_EDIT_AUTOCOMPLETE, TRUE);
      pUIBase->UIEnable(ID_DEBUG_BREAKPOINT, TRUE); 
      if( m_pCppProject && m_pCppProject->m_DebugManager.IsDebugging() ) {
         pUIBase->UIEnable(ID_DEBUG_STEP_RUN, TRUE); 
         pUIBase->UIEnable(ID_DEBUG_STEP_SET, TRUE); 
         pUIBase->UIEnable(ID_DEBUG_QUICKWATCH, TRUE); 
      }
   }
}

// Implementation

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
   if( iFindPos == -1 && (iFlags & FR_WRAP) ) {
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

void CScintillaView::_AutoComplete(CHAR ch)
{
   USES_CONVERSION;
   if( m_pCppProject == NULL ) return;
   if( !m_bAutoComplete && ch != '\b' ) return;

   // Find the name of the variable right before
   // the character was pressed...
   long lPos = m_ctrlEdit.GetCurrentPos();
   if( lPos < 10 ) return;
   long lBack = 1;
   if( ch == '>' || ch == ':' ) lBack = 2;
   CString sName = _GetNearText(lPos - lBack);
   if( sName.IsEmpty() ) return;
   CString sType = _FindTagType(sName, lPos);
   if( sType.IsEmpty() ) return;

   // Yippie, we found one!!!
   CSimpleValArray<CTagInfo::TAGINFO*> aList;
   m_pCppProject->m_TagInfo.GetMemberList(sType, aList, true);
   int nCount = aList.GetSize();
   if( nCount == 0 ) return;
   // Need to sort the items Scintilla-style
   for( int a = 0; a < nCount; a++ ) {
      for( int b = a + 1; b < nCount; b++ ) {
         if( _FunkyStrCmp(aList[a]->pstrName, aList[b]->pstrName) > 0 ) {
            CTagInfo::TAGINFO* pTemp1 = aList[a];
            CTagInfo::TAGINFO* pTemp2 = aList[b];
            aList.SetAtIndex(a, pTemp2);
            aList.SetAtIndex(b, pTemp1);
         }
      }
   }   
   // Right, Scintilla requires a list of space-separated tokens
   // so let's build one...
   CString sList;
   for( int i = 0; i < nCount; i++ ) {
      if( _iscppchar(aList[i]->pstrName[0]) ) {
         sList += aList[i]->pstrName;
         sList += aList[i]->Type == CTagInfo::TAGTYPE_MEMBER ? _T("?0") : _T("?1");
         sList += _T(" ");
      }
   }
   sList.TrimRight();
   // Display popup
   m_ctrlEdit.ClearRegisteredImages();
   _RegisterListImages();
   m_ctrlEdit.AutoCSetIgnoreCase(TRUE);
   m_ctrlEdit.AutoCShow(0, T2CA(sList));
}

void CScintillaView::_AutoSuggest(CHAR ch)
{
   static int s_iSuggestWord = 0;
   static long s_lSuggestPos = 0;
   static CString s_sSuggestWord;
   // Cancel suggestion tip if displayed already.
   // Do actual text-replacement if needed.
   if( m_bSuggestionDisplayed ) {
      if( ch == '\t' ) {
         USES_CONVERSION;
         long lPos = m_ctrlEdit.GetCurrentPos();
         m_ctrlEdit.SetSel(s_lSuggestPos, lPos);
         m_ctrlEdit.ReplaceSel(T2CA(s_sSuggestWord));
      }
      m_ctrlEdit.CallTipCancel();
      m_bSuggestionDisplayed = false;
   }

   // Display tip at all?
   if( !m_bAutoSuggest ) return;
   if( m_ctrlEdit.CallTipActive() ) return;
   if( m_ctrlEdit.AutoCActive() ) return;

   // Display tip only after 3 valid characters has been entered.
   if( _iscppchar(ch) ) s_iSuggestWord++; else s_iSuggestWord = 0;
   if( s_iSuggestWord <= 3 && ch != '\b' ) return;

   // Get the typed identifier
   long lPos = m_ctrlEdit.GetCurrentPos();
   CString sName = _GetNearText(lPos - 1);
   if( sName.IsEmpty() ) return;

   // Grab the last 300 characters or so
   CHAR szText[300] = { 0 };
   int nMin = lPos - (sizeof(szText) - 1);
   if( nMin < 0 ) nMin = 0;
   if( lPos - nMin < 3 ) return; // Smallest tag is 3 characters ex. <p>
   int nMax = lPos - sName.GetLength();
   if( nMax < 0 ) return;
   m_ctrlEdit.GetTextRange(nMin, nMax, szText);

   int i = 0;
   // Skip first word (may be incomplete)
   while( szText[i] != '\0' && !isspace(szText[i]) ) i++;
   // Find best keyword (assumed to be the latest match)
   CString sKeyword;
   CString sSuggest;
   while( szText[i] != '\0' ) {
      // Skip whites
      while( isspace(szText[i]) ) i++;
      // Gather next word
      sSuggest = _T("");
      while( _iscppchar(szText[i]) ) sSuggest += szText[i++];
      // This could be a match
      if( _tcsncmp(sName, sSuggest, sName.GetLength()) == 0 ) sKeyword = sSuggest;
      // Find start of next word
      while( szText[i] != '\0' && !isspace(szText[i]) ) i++;
   }
   if( sKeyword.IsEmpty() ) return;

   // Display suggestion tip
   USES_CONVERSION;
   m_ctrlEdit.CallTipSetFore(RGB(255,255,255));
   m_ctrlEdit.CallTipSetBack(RGB(0,0,0));
   m_ctrlEdit.CallTipShow(lPos, T2CA(sKeyword));
   // Remember what triggered suggestion
   s_lSuggestPos = lPos - sName.GetLength();
   s_sSuggestWord = sKeyword;
   m_bSuggestionDisplayed = true;
}

void CScintillaView::_FunctionTip(CHAR ch)
{
   if( m_pCppProject == NULL ) return;
   if( !m_bAutoComplete ) return;

   if( ch == ')' ) {
      if( m_ctrlEdit.CallTipActive() ) m_ctrlEdit.CallTipCancel();
      return;
   }

   // Find the name of the variable right before
   // the character was pressed...
   long lPos = m_ctrlEdit.GetCurrentPos();
   int iLine = m_ctrlEdit.LineFromPosition(lPos);
   if( lPos < 10 ) return;
   // Get the function name
   CString sName = _GetNearText(lPos - 2);
   if( sName.IsEmpty() ) return;
   // Find the member
   long lMemberPos = lPos - 2 - sName.GetLength();
   CString sMember = _GetNearText(lMemberPos);
   if( sMember.IsEmpty() ) return;
   // Make sure we can recognize the class type
   CString sType = _FindTagType(sMember, lPos);
   if( sType.IsEmpty() ) return;
   // Finally get the function text
   CString sValue = m_pCppProject->GetTagInfo(sName, sType);
   if( sValue.IsEmpty() ) return;
   sValue.Replace(_T("\r"), _T(""));
   // Display tip
   USES_CONVERSION;
   m_ctrlEdit.CallTipSetFore(RGB(0,0,0));
   m_ctrlEdit.CallTipSetBack(RGB(255,255,255));
   m_ctrlEdit.CallTipShow(lPos, T2CA(sValue));
}

CString CScintillaView::_FindTagType(CString& sName, long lPosition)
{
   // Locate the line where this function begins.
   // HACK: We look for the line where the text
   //       starts at column 1.
   int iStartLine = m_ctrlEdit.LineFromPosition(lPosition);
   int iEndLine = iStartLine;
   while( iStartLine >= 0 ) {
      long lPos = m_ctrlEdit.PositionFromLine(iStartLine);
      iStartLine--;
      int ch = m_ctrlEdit.GetCharAt(lPos);
      if( ch == '{' ) break;
      if( isalpha(ch) ) break;
   }
   if( iStartLine < 0 ) iStartLine = 0;

   // See if we can find a matching type from the tag information.
   for( ; iStartLine <= iEndLine; iStartLine++ ) {
      CHAR szBuffer[256] = { 0 };
      if( m_ctrlEdit.GetLineLength(iStartLine) >= sizeof(szBuffer) ) continue;
      m_ctrlEdit.GetLine(iStartLine, szBuffer);
      CString sLine = szBuffer;
      int iColPos = sLine.Find(sName);
      while( iColPos > 0 ) {
         if( _iscppchar(sLine[iColPos + sName.GetLength()]) ) break;
         if( _iscppchar(sLine[iColPos - 1]) ) break;

         int iLinePos = m_ctrlEdit.PositionFromLine(iStartLine);
         int iEndPos = iColPos - 1;
         while( iEndPos > 0 && _tcschr(_T(" \t-*&)"), sLine[iEndPos - 1]) != NULL ) iEndPos--;
         CString sType = _GetNearText(iLinePos + iEndPos);
         
         // First look up among ordinary C++ types.
         // They are likely not to be defined in any header file.
         static LPCTSTR pstrKnownTypes[] = 
         {
            _T("int"),
            _T("char"),
            _T("short"),
            _T("bool"),
            _T("long"),
            _T("float"),
            _T("double"),
            _T("void"),
            _T("string"),
            _T("vector"),
            _T("list"),
            _T("map"),
            NULL
         };
         LPCTSTR* ppTypes = pstrKnownTypes;
         while( *ppTypes ) {
            if( sType.CompareNoCase(*ppTypes) == 0 ) return sType;
            ppTypes++;
         }

         // Now, let's find the type in the TAG file
         int iIndex = m_pCppProject->m_TagInfo.FindItem(0, sType);
         while( iIndex >= 0 ) {
            switch( m_pCppProject->m_TagInfo.GetItemType(iIndex) ) {
            case CTagInfo::TAGTYPE_ENUM:
            case CTagInfo::TAGTYPE_DEFINE:
            case CTagInfo::TAGTYPE_MEMBER:
            case CTagInfo::TAGTYPE_UNKNOWN:
            case CTagInfo::TAGTYPE_FUNCTION:
               // These are not types; but rather members of one
               // so we ignore them...
               break;
            default:
               return sType;
            }
            iIndex = m_pCppProject->m_TagInfo.FindItem(iIndex + 1, sType);
         }

         iColPos = sLine.Find(sName, iColPos + 1);
      }
   }
   return _T("");
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

CString CScintillaView::_GetSelectedText()
{
   // Get the selected text if any
   TextRange tr;
   tr.chrg = m_ctrlEdit.GetSelection();
   tr.chrg.cpMax = min(tr.chrg.cpMax, tr.chrg.cpMin + 127);
   CHAR szText[256] = { 0 };
   tr.lpstrText = szText;
   m_ctrlEdit.GetTextRange(&tr);
   if( strlen(szText) == 0 ) return _T("");
   if( !_iscppchar(szText[0]) ) return _T("");
   if( strpbrk(szText, "\r\n=") != NULL ) return _T("");
   return szText;
}

CString CScintillaView::_GetNearText(long lPosition)
{
   // Get the "word" text under the caret
   if( lPosition < 0 ) return _T("");
   int iLine = m_ctrlEdit.LineFromPosition(lPosition);
   CHAR szText[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLine) >= sizeof(szText) ) return _T("");
   m_ctrlEdit.GetLine(iLine, szText);
   int iStart = lPosition - m_ctrlEdit.PositionFromLine(iLine);
   while( iStart > 0 && _iscppchar(szText[iStart - 1]) ) iStart--;
   if( !_iscppchar(szText[iStart]) ) return _T("");
   if( isdigit(szText[iStart]) ) return _T("");
   int iEnd = iStart;
   while( szText[iEnd] && _iscppchar(szText[iEnd + 1]) ) iEnd++;
   szText[iEnd + 1] = '\0';
   return szText + iStart;
}

bool CScintillaView::_HasSelection() const
{
   long nMin = m_ctrlEdit.GetSelectionStart();
   long nMax = m_ctrlEdit.GetSelectionEnd();
   return nMin != nMax;
}

bool CScintillaView::_iscppchar(CHAR ch) const
{
   return isalnum(ch) || ch == '_';
}

bool CScintillaView::_iscppchar(WCHAR ch) const
{
   return iswalnum(ch) || ch == '_';
}

int CScintillaView::_FunkyStrCmp(LPCTSTR src, LPCTSTR dst) const
{
   while( *dst ) 
   {
      TCHAR c1 = *src < 'a' || *src > 'z' ? *src : *src - 'a' + 'A';
      TCHAR c2 = *dst < 'a' || *dst > 'z' ? *dst : *dst - 'a' + 'A';
      if( c1 - c2 != 0 ) return c1 - c2;
      src++;
      dst++;
   }
   return 1;
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
   m_ctrlEdit.RegisterImage(0, (LPBYTE) MemberImage);
   m_ctrlEdit.RegisterImage(1, (LPBYTE) FunctionImage);
}
