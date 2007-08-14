
#include "StdAfx.h"
#include "resource.h"

#include "GenEdit.h"
#include "ScintillaView.h"
#include "SciLexer.h"

#pragma code_seg( "MISC" )


#ifndef RGR2RGB
   #define RGR2RGB(x) RGB((x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF)
#endif


CScintillaView::CScintillaView(IDevEnv* pDevEnv) :
   m_wndParent(this, 1),   
   m_pDevEnv(pDevEnv),
   m_bInitialized(false),
   m_bAutoCompleteNext(false),
   m_bAutoTextDisplayed(false),
   m_bSuggestionDisplayed(false)
{
   // Prepare the "Find/Replce" dialog with static variables...
   if( s_frFind.lStructSize == 0 ) {
      static CHAR s_szFindText[128] = { 0 };
      static CHAR s_szReplaceText[128] = { 0 };
      s_frFind.lStructSize = sizeof(s_frFind);
      s_frFind.hwndOwner = m_hWnd;
      s_frFind.hInstance = _Module.GetResourceInstance();
      s_frFind.Flags = FR_DOWN | FR_WRAP;
      s_frFind.lpstrFindWhat = s_szFindText;
      s_frFind.wFindWhatLen = (sizeof(s_szFindText) / sizeof(CHAR)) - 1;
      s_frFind.lpstrReplaceWith = s_szReplaceText;
      s_frFind.wReplaceWithLen = (sizeof(s_szReplaceText) / sizeof(CHAR)) - 1;
   }
}

// Operations

void CScintillaView::SetFilename(LPCTSTR pstrFilename)
{
   m_sFilename = pstrFilename;
}

void CScintillaView::SetLanguage(LPCTSTR pstrLanguage)
{
   m_sLanguage = pstrLanguage;
}

void CScintillaView::OnFinalMessage(HWND /*hWnd*/)
{
   // Commit suicide
   delete this;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   BOOL bHasText = GetTextLength() > 0;
   long nMin = GetSelectionStart();
   long nMax = GetSelectionEnd();
   BOOL bHasSelection = nMin != nMax;
   BOOL bModified = GetModify();

   pUIBase->UIEnable(ID_FILE_SAVE, bModified);
   pUIBase->UIEnable(ID_FILE_PRINT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNDO, CanUndo());
   pUIBase->UIEnable(ID_EDIT_REDO, CanRedo());
   pUIBase->UIEnable(ID_EDIT_LOWERCASE, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_UPPERCASE, bHasSelection); 
   pUIBase->UIEnable(ID_EDIT_CUT, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_COPY, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_PASTE, CanPaste());
   pUIBase->UIEnable(ID_EDIT_DELETE, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_GOTO, bHasText);
   pUIBase->UIEnable(ID_EDIT_FIND, bHasText);
   pUIBase->UIEnable(ID_EDIT_REPLACE, bHasText);
   pUIBase->UIEnable(ID_EDIT_REPEAT, bHasText);
   pUIBase->UIEnable(ID_EDIT_DELLINE, bHasText);  
   pUIBase->UIEnable(ID_EDIT_CLEAR, CanClear());
   pUIBase->UIEnable(ID_EDIT_CLEAR_ALL, CanClearAll());
   pUIBase->UIEnable(ID_EDIT_SELECT_ALL, CanSelectAll());
   pUIBase->UIEnable(ID_EDIT_INDENT, GetIndent() > 0);
   pUIBase->UIEnable(ID_EDIT_UNINDENT, GetIndent() > 0);
   pUIBase->UIEnable(ID_EDIT_TABIFY, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_UNTABIFY, bHasSelection);
   pUIBase->UIEnable(ID_EDIT_ZOOM_IN, TRUE);
   pUIBase->UIEnable(ID_EDIT_ZOOM_OUT, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENINCLUDE, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENDECLARATION, TRUE);
   pUIBase->UIEnable(ID_EDIT_OPENIMPLEMENTATION, TRUE);

   pUIBase->UIEnable(ID_EDIT_VIEWWS, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWWS, GetViewWS() != SCWS_INVISIBLE);
   pUIBase->UIEnable(ID_EDIT_VIEWEOL, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWEOL, GetViewEOL());
   pUIBase->UIEnable(ID_EDIT_VIEWWORDWRAP, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWWORDWRAP, GetWrapMode() == SC_WRAP_WORD);
   pUIBase->UIEnable(ID_EDIT_VIEWTABS, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWTABS, GetIndentationGuides());
   pUIBase->UIEnable(ID_EDIT_RECTSELECTION, TRUE);
   pUIBase->UISetCheck(ID_EDIT_RECTSELECTION, GetSelectionMode() == SC_SEL_RECTANGLE);

   pUIBase->UIEnable(ID_SEARCH_TEXT, TRUE);  
   pUIBase->UIEnable(ID_SEARCH_GO, TRUE);  
   
   pUIBase->UIEnable(ID_BOOKMARKS_TOGGLE, TRUE);
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO, TRUE);  

   pUIBase->UIEnable(ID_BOOKMARKS_GOTO1, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO2, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO3, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO4, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO5, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO6, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO7, TRUE);  
   pUIBase->UIEnable(ID_BOOKMARKS_GOTO8, TRUE);  

   pUIBase->UIEnable(ID_BOOKMARK_NEXT, TRUE);
   pUIBase->UIEnable(ID_BOOKMARK_PREVIOUS, TRUE);
   pUIBase->UIEnable(ID_BOOKMARK_NEXT, TRUE);
   pUIBase->UIEnable(ID_BOOKMARK_TOGGLE, TRUE);
   pUIBase->UIEnable(ID_BOOKMARK_CLEAR, TRUE);

   if( bModified ) pUIBase->UIEnable(ID_FILE_SAVE_ALL, TRUE);
}

void CScintillaView::OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
{
   AtlLoadString(wID, pstrText, cchMax);
}

// Message map and handlers

LRESULT CScintillaView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_wndParent.SubclassWindow(GetParent());
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   _SaveBookmarks();
   if( m_wndParent.IsWindow() ) m_wndParent.UnsubclassWindow();
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   ATLASSERT(m_pDevEnv);
   m_pDevEnv->AddIdleListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ATLASSERT(m_pDevEnv);
   m_pDevEnv->RemoveIdleListener(this);
   m_pDevEnv->ShowStatusText(ID_SB_PANE2, _T(""), TRUE);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_TAB && m_bAutoTextDisplayed ) return 0;
   if( wParam == VK_TAB && m_bSuggestionDisplayed ) return 0;
   if( wParam == VK_BACK ) m_bSuggestionDisplayed = false;
   if( wParam == VK_ESCAPE && AutoCActive() ) AutoCCancel();
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{   
   ::SetFocus(m_hWnd);

   // Get the cursor position
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   POINT ptLocal = pt;
   ::MapWindowPoints(NULL, m_hWnd, &ptLocal, 1);
   if( lParam == (LPARAM) -1 ) {
      long lPos = GetCurrentPos();
      pt.x = PointXFromPosition(lPos);
      pt.y = PointYFromPosition(lPos);
   }
   // Place cursor at mouse if not clicked inside a selection
   long lPos = PositionFromPoint(ptLocal.x, ptLocal.y);
   CharacterRange cr = GetSelection();
   if( lPos < cr.cpMin || lPos > cr.cpMax ) SetSel(lPos, lPos);

   // Grab EDIT submenu from main window's menu
   CMenuHandle menu = m_pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(1);

   // Show popup menu now
   m_pDevEnv->ShowPopupMenu(NULL, submenu, pt);

   // Clean up in EDIT menu after C++ OpenInclude command
   submenu.RemoveMenu(ID_EDIT_OPENINCLUDE, MF_BYCOMMAND);
   submenu.RemoveMenu(ID_EDIT_OPENDECLARATION, MF_BYCOMMAND);
   submenu.RemoveMenu(ID_EDIT_OPENIMPLEMENTATION, MF_BYCOMMAND);

   return 0;
}

LRESULT CScintillaView::OnUpdateUI(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   // Update statusbar with cursor text
   long lPos = GetCurrentPos();
   long lCol = (long) GetColumn(lPos) + 1;
   long lRow = (long) LineFromPosition(lPos) + 1;
   CString sMsg;
   sMsg.Format(IDS_STATUS_POSITION, lRow, lCol);
   m_pDevEnv->ShowStatusText(ID_SB_PANE2, sMsg, TRUE);
   // Find matching brace
   _MatchBraces(lPos);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDwellEnd(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   m_bSuggestionDisplayed = false;
   m_bAutoTextDisplayed = false;
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;

   if( _iseditchar(pSCN->ch) ) _AutoComplete(m_cPrevChar);
   m_bAutoCompleteNext = false;

   switch( pSCN->ch ) {
   case '\n':
   case '}':
      _MaintainIndent(pSCN->ch);
      break;
   }

   switch( pSCN->ch ) {
   case '*':
   case '>':
   case '{':
   case '[':
   case '?':
   case '%':
   case '\n':
      _MaintainTags(pSCN->ch);
      break;
   }

   _AutoText(pSCN->ch);
   _AutoSuggest(pSCN->ch);

   m_cPrevChar = (CHAR) pSCN->ch;
   if( m_cPrevChar == '$' && m_sLanguage == _T("php") ) m_bAutoCompleteNext = true;
   if( m_cPrevChar == '<' && m_sLanguage == _T("xml") ) m_bAutoCompleteNext = true;
   if( m_cPrevChar == '<' && m_sLanguage == _T("html") ) m_bAutoCompleteNext = true;

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   if( pSCN->margin == 2 ) {
      int iLineNum = LineFromPosition(pSCN->position);
      int iCount = GetLineCount();
      if( (pSCN->modifiers & SCMOD_CTRL) != 0 ) {
         for( int i = 1; i <= iCount; i++ ) if( GetFoldExpanded(i) ) ToggleFold(i);
      }
      else if( (pSCN->modifiers & SCMOD_SHIFT) != 0 ) {
         for( int i = 1; i <= iCount; i++ ) if( !GetFoldExpanded(i) ) ToggleFold(i);
      }
      else {
         ToggleFold(iLineNum);
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnMacroRecord(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   // BUG: Not all SCI_xxx commands take a string argument, so we'll do
   //      an optimistic checking on the LPARAM parameter.
   if( pSCN->lParam != 0 && ::IsBadStringPtrA((LPCSTR) pSCN->lParam, -1) ) return 0;
   CString sMacro;
   sMacro.Format(_T("Call ActiveWindow.SendRawMessage(%ld,%ld,\"%hs\")"), 
      (long) pSCN->message,
      (long) pSCN->wParam,
      pSCN->lParam == 0 ? "" : (LPCSTR) pSCN->lParam);
   sMacro.Replace(_T("\r"), _T(""));
   sMacro.Replace(_T("\t"), _T("\" + Chr(9) + \""));
   sMacro.Replace(_T("\n"), _T("\" + vbCrLf + \""));
   m_pDevEnv->RecordMacro(sMacro);
   return 0;
}

LRESULT CScintillaView::OnNeedShown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   int iStartLine = LineFromPosition(pSCN->position);
   int iEndLine = LineFromPosition(pSCN->position + pSCN->length - 1);
   for( int iLineNum = iStartLine; iLineNum <= iEndLine; iLineNum++ ) EnsureVisible(iLineNum);
   return 0;
}

// Implementation

/**
 * Match brace.
 * We match brace chars, block-scope char and parens for several
 * languages.
 */
void CScintillaView::_MatchBraces(long lPos)
{
   if( !m_bMatchBraces ) return;
   CHAR ch = (CHAR) GetCharAt(lPos);
   int iStyle = GetStyleAt(lPos);
   if( (ch == '{' && m_sLanguage == _T("cpp") && iStyle == SCE_C_OPERATOR )
       || (ch == '(' && m_sLanguage == _T("cpp") && iStyle == SCE_C_OPERATOR )
       || (ch == '[' && m_sLanguage == _T("cpp") && iStyle == SCE_C_OPERATOR )
       || (ch == '<' && m_sLanguage == _T("html"))
       || (ch == '<' && m_sLanguage == _T("xml"))
       || (ch == '{' && m_sLanguage == _T("java"))
       || (ch == '(' && m_sLanguage == _T("java"))
       || (ch == '{' && m_sLanguage == _T("php") && iStyle == SCE_HPHP_OPERATOR )
       || (ch == '(' && m_sLanguage == _T("php") && iStyle == SCE_HPHP_OPERATOR )
       || (ch == '{' && m_sLanguage == _T("asp")) ) 
   {
      long lMatch = BraceMatch(lPos);
      if( lMatch == -1 ) BraceBadLight(lPos);
      else BraceHighlight(lPos, lMatch);
   }
   else {
      BraceBadLight(-1);
      BraceHighlight(-1, -1);
   }
}

/**
 * Show auto-text.
 * Auto-text is a user-configurable list of shortcuts that can be typed and
 * typically inserts a whole section of text (e.g. user types 'while' and presses
 * TAB key to insert a complete 'while(true) { ...'-block).
 */
void CScintillaView::_AutoText(CHAR ch)
{
   const WORD AUTOTEXT_AFTER_TYPED_CHARS = 3;

   static int s_iSuggestWord = 0;
   static long s_lSuggestPos = 0;
   static CString s_sAutoText;

   // Cancel auto-text tip if displayed already or insert replacement-text 
   // if needed.
   if( m_bAutoTextDisplayed ) 
   {
      if( ch == '\t' || ch == '\0xFF' ) {
         USES_CONVERSION;
         long lPos = GetCurrentPos();
         int iLineNum = LineFromPosition(lPos);
         int iIndent = GetLineIndentation(iLineNum);
         // Extract text-replacement
         CString sText = s_sAutoText;
         int iCaretPos = sText.Find('$');
         int nLines = sText.Replace(_T("\\n"), _T("\n"));
         sText.Remove('\r');
         sText.Replace(_T("\\t"), CString(' ', GetIndent()));
         sText.Replace(_T("\\$"), _T("\01"));
         sText.Remove('$');
         sText.Replace('\01', '$');
         // Replace text
         BeginUndoAction();
         SetSel(s_lSuggestPos, lPos);
         ReplaceSel(T2CA(sText));
         while( nLines > 0 ) {
            iLineNum++; nLines--;
            _SetLineIndentation(iLineNum, iIndent);
         }
         // Place the caret
         // BUG: Because of the indent reposition above we have some difficulty
         //      in finding the correct position. It will work if the caret is set
         //      in the first line, but probably not on other lines
         if( iCaretPos >= 0 ) SetSel(s_lSuggestPos + iCaretPos, s_lSuggestPos + iCaretPos);
         EndUndoAction();
      }
      CallTipCancel();
      m_bAutoTextDisplayed = false;
      return;
   }

   // Display tip only after 3 characters has been entered
   // and avoid displaying unnessecarily...
   if( _iseditchar(ch) || ch == '(' ) s_iSuggestWord = max(1, s_iSuggestWord + 1); 
   else if( isspace(ch) ) s_iSuggestWord = 0;
   else s_iSuggestWord = -999;
   if( s_iSuggestWord < AUTOTEXT_AFTER_TYPED_CHARS ) return;

   // Can we display new tooltip at all?
   if( AutoCActive() ) return;

   // Get the typed identifier
   long lPos = GetCurrentPos();
   CString sName = _GetNearText(lPos - 1);
   if( sName.IsEmpty() ) return;

   // Look for a match in auto-text list
   long i = 1;
   CString sKey;
   TCHAR szBuffer[400] = { 0 };
   while( true ) {
      sKey.Format(_T("autotext.entry%ld.name"), i);
      szBuffer[0] = '\0';
      m_pDevEnv->GetProperty(sKey, szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1);
      if( szBuffer[0] == '\0' ) return;
      if( _tcsncmp(sName, szBuffer, _tcslen(szBuffer) ) == 0 ) break;
      i++;
   }

   // Get replacement text
   szBuffer[0] = '\0';
   sKey.Format(_T("autotext.entry%ld.text"), i);
   m_pDevEnv->GetProperty(sKey, szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1);

   // Display suggestion-tip
   CallTipCancel();
   CallTipSetFore(::GetSysColor(COLOR_INFOTEXT));
   CallTipSetBack(::GetSysColor(COLOR_INFOBK));
   CString sTipText = szBuffer;
   sTipText.Remove('\r');
   sTipText.Replace(_T("\\n"), _T("\n"));
   sTipText.Replace(_T("\\t"), CString(' ', GetIndent()));
   USES_CONVERSION;
   CallTipShow(lPos, T2CA(sTipText));
   // Remember what triggered suggestion
   s_lSuggestPos = lPos - sName.GetLength();
   s_sAutoText = szBuffer;
   m_bAutoTextDisplayed = true;
}

/**
 * Display auto-suggestion.
 * Auto-suggest pops up a simple auto-completion tip based
 * on similar words located in the surrounding text.
 */
void CScintillaView::_AutoSuggest(CHAR ch)
{
   if( !m_bAutoSuggest ) return;

   static int s_iSuggestWord = 0;
   static long s_lSuggestPos = 0;
   static CString s_sSuggestWord;
   // Cancel suggestion tip if displayed already.
   // Do actual text-replacement if needed.
   if( m_bSuggestionDisplayed ) 
   {
      if( ch == '\t' ) {
         USES_CONVERSION;
         long lPos = GetCurrentPos();
         SetSel(s_lSuggestPos, lPos);
         ReplaceSel(T2CA(s_sSuggestWord));
      }
      CallTipCancel();
      m_bSuggestionDisplayed = false;
   }

   // Display tip at all?
   if( !m_bAutoSuggest && ch != '\b' ) return;
   if( CallTipActive() ) return;
   if( AutoCActive() ) return;

   // Display tip only after 3 valid characters has been entered.
   if( _iseditchar(ch) ) s_iSuggestWord++; else s_iSuggestWord = 0;
   if( s_iSuggestWord <= 3 && ch != '\b' ) return;

   // Get the typed identifier
   long lPos = GetCurrentPos();
   CString sName = _GetNearText(lPos - 1);
   if( sName.IsEmpty() ) return;

   // Don't popup within comments and stuff
   if( !_IsValidInsertPos(lPos) ) return;

   // Grab the last 256 characters or so
   CHAR szText[256] = { 0 };
   int nMin = lPos - (sizeof(szText) - 1);
   if( nMin < 0 ) nMin = 0;
   if( lPos - nMin < 3 ) return; // Smallest tag is 3 characters ex. <p>
   int nMax = lPos - sName.GetLength();
   if( nMax < 0 ) return;
   GetTextRange(nMin, nMax, szText);

   int i = 0;
   // Skip first word (may be incomplete)
   while( szText[i] != '\0' && !isspace(szText[i]) ) i++;
   // Find best keyword (assumed to be the latest match)
   CString sKeyword;
   CString sSuggest;
   while( szText[i] != '\0' ) {
      // Gather next word
      sSuggest = _T("");
      while( _iseditchar(szText[i]) ) sSuggest += szText[i++];
      // This could be a match
      if( _tcsncmp(sName, sSuggest, sName.GetLength()) == 0 ) sKeyword = sSuggest;
      // Find start of next word
      while( szText[i] != '\0' && !_iseditchar(szText[i]) ) i++;
   }
   if( sKeyword.IsEmpty() ) return;

   // Display suggestion tip
   USES_CONVERSION;
   CallTipSetFore(RGB(255,255,255));
   CallTipSetBack(RGB(0,0,0));
   CallTipShow(lPos, T2CA(sKeyword));
   // Remember what triggered suggestion
   s_sSuggestWord = sKeyword;
   m_bSuggestionDisplayed = true;
   s_lSuggestPos = lPos - sName.GetLength();
}

/**
 * Handle auto-completion.
 * Do auto-completion for general languages. We can handle auto-completion
 * for XML, HTML, PERL and PHP languages out-of-the-box.
 */
void CScintillaView::_AutoComplete(CHAR ch)
{
   if( !m_bAutoComplete ) return;
   if( AutoCActive() ) return;

   USES_CONVERSION;
   CString sList;
   long lPos = GetCurrentPos();
   // Get auto-completion words from HTML text
   if( ch == '<' && m_sLanguage == _T("html") )
   {
      // Get the list of known keywords from the general editor configuration
      CString sProperty;
      sProperty.Format(_T("editors.%s.keywords"), m_sLanguage);
      CString sKeywords;
      m_pDevEnv->GetProperty(sProperty, sKeywords.GetBufferSetLength(2048), 2048);
      sKeywords.ReleaseBuffer();
      //
      TCHAR cLetter = (TCHAR) GetCharAt(lPos - 1);
      TCHAR szFind[] = { ' ', (TCHAR) tolower(cLetter), '\0' };
      int iPos = sKeywords.Find(szFind, 0);
      while( iPos >= 0 ) {
         iPos++;             // Past space
         sList += cLetter;   // Copy first letter to maintain capitalization
         iPos++;             // Past first letter
         while( _istalnum(sKeywords[iPos]) ) {
            sList += sKeywords[iPos];
            iPos++;
         }
         sList += ' ';
         iPos = sKeywords.Find(szFind, iPos - 1);
      }
      if( m_bAutoCase ) sList.MakeLower();
      AutoCSetFillUps(">");
   }
   // Get auto-completion words from PHP or XML text
   if( (ch == '$' && m_sLanguage == _T("php")) 
       || (ch == '$' && m_sLanguage == _T("perl")) 
       || (ch == '<' && m_sLanguage == _T("xml")) )
   {
      // Grab the last 512 characters or so
      CHAR szText[512] = { 0 };
      int iMin = lPos - (sizeof(szText) - 1);
      if( iMin < 0 ) iMin = 0;
      if( lPos - iMin < 3 ) return; // Smallest tag is 3 characters ex. <p>
      GetTextRange(iMin, lPos, szText);

      // Construct the first letter of the search pattern
      CHAR cLetter = szText[lPos - iMin - 1];
      if( cLetter == '/' ) return;      
      CHAR szFind[] = { ch, cLetter, '\0' };
      // Create array of items found from text
      CSimpleArray<CString> aList;
      LPSTR pstr = strstr(szText, szFind);
      while( pstr != NULL ) {
         pstr++;
         CString sWord;
         while( *pstr != '\0' ) {
            if( !_iseditchar(*pstr) ) break;
            sWord += (TCHAR) *pstr++;
         }
         if( sWord.GetLength() > 2 ) _AddUnqiue(aList, sWord);
         pstr = strstr(pstr, szFind);
      }
      // Add globals for PHP...
      if( m_sLanguage == _T("php") ) 
      {
         _AddUnqiue(aList, _T("_GET"));
         _AddUnqiue(aList, _T("_FORM"));
         _AddUnqiue(aList, _T("_POST"));
         _AddUnqiue(aList, _T("_REQUEST"));
         _AddUnqiue(aList, _T("_SESSION"));
         _AddUnqiue(aList, _T("_SERVER"));
         _AddUnqiue(aList, _T("_ENV"));
         _AddUnqiue(aList, _T("_COOKIE"));
         _AddUnqiue(aList, _T("this"));
         AutoCSetFillUps("[");
      }
      // Add XML specific
      if( m_sLanguage == _T("xml") )
      {
         AutoCSetFillUps(">");
      }
      // Sort the list
      for( int a = 0; a < aList.GetSize(); a++ ) {
         for( int b = a + 1; b < aList.GetSize(); b++ ) {
            // Right; Scintilla uses strcmp() to compile its items
            // for non case-sensitive compares
            if( stricmp(T2CA(aList[a]), T2CA(aList[b])) > 0 ) {
               CString sTemp = aList[a];
               aList[a] = aList[b];
               aList[b] = sTemp;
            }
         }
      }
      // Build Scrintilla string
      for( int i = 0; i < aList.GetSize(); i++ ) sList += aList[i] + ' ';
   }
   // Display popup list
   sList.TrimRight();
   if( sList.IsEmpty() ) return;
   ClearRegisteredImages();
   AutoCSetMaxHeight(6);
   AutoCSetIgnoreCase(FALSE);
   AutoCShow(1, T2CA(sList));
}

/**
 * Maintains indentation.
 * The function maintains indentation for the general programming languages
 * and for HTML and XML as well.
 */
void CScintillaView::_MaintainIndent(CHAR ch)
{
   if( !m_bAutoIndent && !m_bSmartIndent ) return;
   int iCurPos = GetCurrentPos();
   if( ch != '\n' && !_IsValidInsertPos(iCurPos) ) return;
   int iCurLine = GetCurrentLine();
   int iLastLine = iCurLine - 1;
   // Find the most recent non-empty list.
   // We'll assume it contains the correct current indent.
   int iIndentAmount = 0;
   while( iLastLine >= 0 && GetLineLength(iLastLine) == 0 ) iLastLine--;
   if( iLastLine < 0 ) return;
   iIndentAmount = GetLineIndentation(iLastLine);
   if( m_bSmartIndent ) {
      int iIndentWidth = GetIndent();
      if( iIndentWidth == 0 ) iIndentWidth = GetTabWidth();
      if( m_sLanguage == _T("cpp") 
          || m_sLanguage == _T("java") 
          || m_sLanguage == _T("perl") 
          || m_sLanguage == _T("php") )             
      {
         if( ch == '}' ) {
            iLastLine = iCurLine;
            iIndentAmount = GetLineIndentation(iLastLine);
         }
         int iLen = GetLineLength(iLastLine);
         // Extract line (scintilla doesn't sz-terminate)
         LPSTR pstr = (LPSTR) _alloca(iLen + 1);
         GetLine(iLastLine, pstr);
         pstr[iLen] = '\0';
         // Strip line and check if smart indent is needed
         CString sLine = pstr;
         int iPos = sLine.Find(_T("//")); if( iPos >= 0 ) sLine = sLine.Left(iPos);
         sLine.TrimRight();
         if( ch == '\n' && sLine.Right(1) == _T("{") ) iIndentAmount += iIndentWidth;
         if( ch == '\n' && sLine.Right(1) == _T(",") ) iIndentAmount = max(iIndentAmount, sLine.Find('(') + 1);
         sLine.TrimLeft();
         if( ch == '}' && sLine == _T("}") ) iIndentAmount -= iIndentWidth;
      }
      if( m_sLanguage == _T("xml") ) 
      {
         if( ch == '\n' ) {
            CHAR szText[256] = { 0 };
            if( GetLineLength(iLastLine) >= sizeof(szText) - 1 ) return;
            GetLine(iLastLine, szText);
            if( strchr(szText, '>') != NULL && strchr(szText, '/') == NULL ) {
               if( m_bAutoClose ) {
                  ReplaceSel(GetEOLMode() == SC_EOL_CRLF ? "\r\n" : "\n");
                  _SetLineIndentation(iCurLine + 1, iIndentAmount);
                  SetSel(iCurPos, iCurPos);
               }
               iIndentAmount += iIndentWidth;
            }
         }
      }
      if( m_sLanguage == _T("bash") ) 
      {
         if( ch == '\n' ) {
            CHAR szText[256] = { 0 };
            if( GetLineLength(iLastLine) >= sizeof(szText) - 1 ) return;
            GetLine(iLastLine, szText);
            CString sLine = szText;
            int iPos = sLine.Find('#'); if( iPos >= 0 ) sLine = sLine.Left(iPos);
            sLine.TrimLeft();
            sLine.TrimRight();
            // Indent is controlled by certain keywords on a separate line
            if( sLine == _T("do") || sLine == _T("then") || sLine == _T("else") || sLine == _T("elif") || sLine == _T("{") ) {
               iIndentAmount += iIndentWidth;
            }
         }
      }
      if( m_sLanguage == _T("html") || m_sLanguage == _T("asp") || m_sLanguage == _T("php") )
      {
         if( ch == '\n' 
             && GetCharAt(iCurPos - (GetEOLMode() == SC_EOL_CRLF ? 3 : 2)) == '>' ) 
         {
            CHAR szLine1[256] = { 0 };
            CHAR szLine2[256] = { 0 };
            if( GetLineLength(iCurLine) >= sizeof(szLine1) - 1 ) return;
            if( GetLineLength(iCurLine - 1) >= sizeof(szLine2) - 1 ) return;
            GetLine(iCurLine, szLine1);
            GetLine(iCurLine - 1, szLine2);
            CString sLine1 = szLine1;
            CString sLine2 = szLine2;
            sLine1.TrimLeft();
            sLine1.TrimRight();
            sLine1.MakeUpper();
            sLine2.MakeUpper();
            // These are the HTML tags that we'll indent content
            // for on a new line.
            static LPCTSTR ppstrBlocks[] = 
            {
               _T("<body"),
               _T("<form"),
               _T("<frameset"),
               _T("<head"),
               _T("<html"),
               _T("<object")
               _T("<script"),
               _T("<table"),
               _T("<td"),
               _T("<thead"),
               _T("<tr"),
               _T("<ul"),
               NULL,
            };
            for( LPCTSTR* ppstr = ppstrBlocks; *ppstr; ppstr++ ) {
               if( sLine2.Find(*ppstr) >= 0 ) {
                  if( m_bAutoClose ) {
                     CString sCloseCommand = *ppstr;
                     sCloseCommand.Replace(_T("<"), _T("</"));
                     if( sLine1.Find(sCloseCommand) == 0 ) {
                        ReplaceSel(GetEOLMode() == SC_EOL_CRLF ? "\r\n" : "\n");
                        _SetLineIndentation(iCurLine + 1, iIndentAmount);
                        SetSel(iCurPos, iCurPos);
                     }
                  }
                  iIndentAmount += iIndentWidth;
                  break;
               }
            }
         }
      }
   }
   // So do we need to indent?
   if( iIndentAmount >= 0 ) _SetLineIndentation(iCurLine, iIndentAmount);
}

/**
 * Auto-close tags.
 * This function automatically closes tags for the HTML and XML languages.
 */
void CScintillaView::_MaintainTags(CHAR ch)
{
   USES_CONVERSION;

   if( !m_bAutoClose ) return;

   int lPosition = GetCurrentPos();

   // Special rules before comments

   if( ch == '\n' ) 
   {
      int iLineNum = LineFromPosition(lPosition);
      if( iLineNum == 0 ) return;
      CHAR szText[30] = { 0 };
      if( GetLineLength(iLineNum - 1) >= sizeof(szText) - 1 ) return;
      GetLine(iLineNum - 1, szText);
      //
      // Automatically close JavaDoc-style comments
      //     
      CString sText = szText;
      sText.TrimLeft();
      sText.TrimRight(_T("\r\n"));
      if( sText == _T("/**") ) 
      {
         BeginUndoAction();
         ReplaceSel("* \r\n*/");
         _SetLineIndentation(iLineNum, GetLineIndentation(iLineNum - 1) + 1);
         _SetLineIndentation(iLineNum + 1, GetLineIndentation(iLineNum - 1) + 1);
         LineUp();
         LineEnd();
         EndUndoAction();
         return;
      }
      //
      // Trim empty line
      //
      sText = szText;
      sText.TrimRight(_T("\r\n"));
      if( !sText.IsEmpty() ) 
      {
         // Line was not empty; let's see when we trim the string...
         sText.TrimLeft();
         if( sText.IsEmpty() ) {
            int iStart = PositionFromLine(iLineNum - 1);
            int iLength = GetLineLength(iLineNum - 1);
            if( iLength > 0 ) {
               BeginUndoAction();
               SetSel(iStart, iStart + iLength);
               ReplaceSel("");
               SetSel(lPosition - iLength, lPosition - iLength);
               EndUndoAction();
            }
         }
      }
   }

   // Rest of the rules do not apply within comments and stuff
   if( !_IsValidInsertPos(lPosition) ) return;

   // Standard auto-complete rules

   CHAR chPrev = GetCharAt(lPosition - 2);
   int iStylePrev = GetStyleAt(lPosition - 2);

   if( ch == '>' 
       && (m_sLanguage == _T("html") || m_sLanguage == _T("xml") || m_sLanguage == _T("php") || m_sLanguage == _T("asp")) )
   {
      //
      // Close HTML and XML tags
      //      
      // Grab the last 256 characters or so
      CHAR szText[256] = { 0 };
      int nMin = lPosition - (sizeof(szText) - 1);
      if( nMin < 0 ) nMin = 0;
      if( lPosition - nMin < 3 ) return; // Smallest tag is 3 characters ex. <p>
      GetTextRange(nMin, lPosition, szText);

      if( szText[lPosition - nMin - 2] == '/' ) return;
      if( iStylePrev != SCE_H_TAGUNKNOWN && iStylePrev != SCE_H_TAG ) return;

      CString sFound = _FindOpenXmlTag(szText, lPosition - nMin);
      if( sFound.IsEmpty() ) return;
      // Ignore some of the typical non-closed HTML tags
      static LPCTSTR ppstrNonClosed[] = { _T("meta"), _T("img"), _T("br"), _T("hr"), _T("p"), NULL };
      for( LPCTSTR* ppstr = ppstrNonClosed; *ppstr; ppstr++ ) {
         if( sFound.CollateNoCase(*ppstr) == 0 ) return;
      }
      // Insert end-tag into text
      CString sInsert;
      sInsert.Format(_T("</%s>"), sFound);
      BeginUndoAction();
      ReplaceSel(T2CA(sInsert));
      SetSel(lPosition, lPosition);
      EndUndoAction();
   }
   if( ch == '{' 
       && (m_sLanguage == _T("cpp") || m_sLanguage == _T("java") || m_sLanguage == _T("php")) )
   {
      //
      // Auto-close braces in C-like languages
      //
      // Grab current line for inspection
      int iLineNum = LineFromPosition(lPosition);
      CHAR szText[256] = { 0 };
      if( GetLineLength(iLineNum) >= sizeof(szText) - 1 ) return;
      GetLine(iLineNum, szText);
      CString sText = szText;
      sText.TrimLeft();
      sText.TrimRight();
      // Add insertion text
      if( sText == _T("{") || sText.Right(1) == _T("{") ) {
         BeginUndoAction();
         ReplaceSel(GetEOLMode() == SC_EOL_CRLF ? "\r\n\r\n}" : "\n\n}");
         SetSel(lPosition, lPosition);
         _SetLineIndentation(iLineNum + 1, GetLineIndentation(iLineNum) + GetIndent());
         _SetLineIndentation(iLineNum + 2, GetLineIndentation(iLineNum));
         LineUp();
         LineEnd();
         EndUndoAction();
      }
   }
   if( ch == '[' 
       && (m_sLanguage == _T("cpp") || m_sLanguage == _T("php") || m_sLanguage == _T("java")) ) 
   {
      //
      // Insert hard-braces for C-type arrays
      //
      ReplaceSel("]");
      SetSel(lPosition, lPosition);
   }
   if( ch == '?' && chPrev == '<' && m_sLanguage == _T("php") )
   {
      //
      // Autoclose PHP preprocessing tag
      //
      ReplaceSel("?>");
      SetSel(lPosition, lPosition);
   }
   if( ch == '%' && chPrev == '<' && m_sLanguage == _T("asp") )
   {
      //
      // Autoclose ASP preprocessing tag
      //
      ReplaceSel("%>");
      SetSel(lPosition, lPosition);
   }
}

/**
 *	Find the last open XML/HTML tag.
 */
CString CScintillaView::_FindOpenXmlTag(LPCSTR pstrText, int nSize) const
{
   CString sRet;
   if( nSize < 3 ) return sRet;
   LPCSTR pBegin = pstrText;
   LPCSTR pCur = pstrText + nSize - 1;
   pCur--; // Skip past the >
   while( pCur > pBegin ) {
      if( *pCur == '<' ) break;
      if( *pCur == '>' ) break;
      --pCur;
   }
   if( *pCur == '<' ) {
      pCur++;
      while( strchr(":_-.", *pCur) != NULL || isalnum(*pCur) ) {
         sRet += *pCur;
         pCur++;
      }
   }
   return sRet;
}

/**
 * Sets the indentation.
 * The indentation can be a little tricky in Scintilla because if there is a
 * text selection we should try to preserve it after the indent.
 */
void CScintillaView::_SetLineIndentation(int iLineNum, int iIndent)
{
   if( iIndent < 0 ) return;
   CharacterRange cr = GetSelection();
   int posBefore = GetLineIndentPosition(iLineNum);
   SetLineIndentation(iLineNum, iIndent);
   int posAfter = GetLineIndentPosition(iLineNum);
   int posDifference = posAfter - posBefore;
   if( posAfter >= posBefore ) {
      // Move selection forward
      if( cr.cpMin >= posBefore ) cr.cpMin += posDifference;
      if( cr.cpMax >= posBefore ) cr.cpMax += posDifference;
      if( cr.cpMin < posBefore && cr.cpMax < posBefore && cr.cpMin == cr.cpMax ) cr.cpMin = cr.cpMax = posAfter;
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
   SetSel(cr.cpMin, cr.cpMax);
}

CString CScintillaView::_GetProperty(CString sKey) const
{
   TCHAR szBuffer[64] = { 0 };
   m_pDevEnv->GetProperty(sKey, szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1);
   return szBuffer;
}

/**
/* Find substring next occourance in text.
/*/
int CScintillaView::_FindNext(int iFlags, LPCSTR pstrText, bool bWarnings, bool bShowFindDlg)
{
   int iLength = strlen(pstrText);
   if( iLength == 0 ) return -1;

   bool bDirectionDown = (iFlags & FR_DOWN) != 0;
   bool bInSelection = (iFlags & FR_INSEL) != 0;

   CharacterRange cr = GetSelection();
   int startPosition = cr.cpMax;
   int endPosition = GetLength();
   if( !bDirectionDown ) {
      startPosition = cr.cpMin - 1;
      endPosition = 0;
   }
   if( bInSelection ) {
      if( cr.cpMin == cr.cpMax ) return -1;
      startPosition = cr.cpMin;
      endPosition = cr.cpMax;
      iFlags &= ~FR_WRAP;
   }

   SetTargetStart(startPosition);
   SetTargetEnd(endPosition);
   SetSearchFlags(iFlags);
   int iFindPos = SearchInTarget(iLength, pstrText);
   if( iFindPos == -1 && (iFlags & FR_WRAP) != 0 ) {
      // Failed to find in indicated direction,
      // so search from the beginning (forward) or from the end (reverse) unless wrap is false
      if( !bDirectionDown ) {
         startPosition = GetLength();
         endPosition = 0;
      } 
      else {
         startPosition = 0;
         endPosition = GetLength();
      }
      SetTargetStart(startPosition);
      SetTargetEnd(endPosition);
      iFindPos = SearchInTarget(iLength, pstrText);
   }
   if( iFindPos == -1 ) {
      // Bring up another Find dialog
      bool bDocWarnings = _GetProperty(_T("gui.document.findMessages")) == _T("true");
      if( bDocWarnings && bWarnings ) {
         CString sMsg;
         sMsg.Format(IDS_FINDFAILED, CString(pstrText));
         m_pDevEnv->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONINFORMATION);
         if( bShowFindDlg ) {
            HWND hWndMain = m_pDevEnv->GetHwnd(IDE_HWND_MAIN);
            ::PostMessage(hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_FIND, 0), 0L);
         }
      }
   } 
   else {
      int start = GetTargetStart();
      int end = GetTargetEnd();
      // EnsureRangeVisible
      int lineStart = LineFromPosition(min(start, end));
      int lineEnd = LineFromPosition(max(start, end));
      for( int line = lineStart; line <= lineEnd; line++ ) EnsureVisible(line);
      SetSel(start, end);
   }
   return iFindPos;
}

/**
 * Replace a string with a substring.
 * The structure s_frFind contains the search/replace data.
 */
bool CScintillaView::_ReplaceOnce()
{
   CharacterRange cr = GetSelection();
   SetTargetStart(cr.cpMin);
   SetTargetEnd(cr.cpMax);
   int nReplaced = (int) strlen(s_frFind.lpstrReplaceWith);
   if( (s_frFind.Flags & SCFIND_REGEXP) != 0 ) {
      nReplaced = ReplaceTargetRE(nReplaced, s_frFind.lpstrReplaceWith);
   }
   else { 
      // Allow \0 in replacement
      ReplaceTarget(nReplaced, s_frFind.lpstrReplaceWith);
   }
   SetSel(cr.cpMin + nReplaced, cr.cpMin);
   return true;
}

CString CScintillaView::_GetNearText(long lPosition)
{
   // Get the "word" text under the caret
   if( lPosition < 0 ) return _T("");
   int iLineNum = LineFromPosition(lPosition);
   CHAR szText[256] = { 0 };
   if( GetLineLength(iLineNum) >= sizeof(szText) ) return _T("");
   GetLine(iLineNum, szText);
   int iStart = lPosition - PositionFromLine(iLineNum);
   while( iStart > 0 && _iseditchar(szText[iStart - 1]) ) iStart--;
   if( !_iseditchar(szText[iStart]) ) return _T("");
   if( isdigit(szText[iStart]) ) return _T("");
   int iEnd = iStart;
   while( szText[iEnd] && _iseditchar(szText[iEnd + 1]) ) iEnd++;
   szText[iEnd + 1] = '\0';
   return szText + iStart;
}

void CScintillaView::_DefineMarker(int nMarker, int nType, COLORREF clrFore, COLORREF clrBack)
{
   MarkerDefine(nMarker, nType);
   MarkerSetFore(nMarker, clrFore);
   MarkerSetBack(nMarker, clrBack);
}

void CScintillaView::_SaveBookmarks()
{
   LPCTSTR pstrName = ::PathFindFileName(m_sFilename);
   TCHAR szKey[200] = { 0 };
   TCHAR szValue[64] = { 0 };
   int iIndex = 1;
   int iLineNum = MarkerNext(0, 1 << MARKER_BOOKMARK);
   while( iLineNum > 0 ) {
      ::wsprintf(szKey, _T("bookmarks.%s.%d"), pstrName, iIndex++);
      ::wsprintf(szValue, _T("%d"), iLineNum);
      m_pDevEnv->SetProperty(szKey, szValue);
      iLineNum = MarkerNext(iLineNum + 1, 1 << MARKER_BOOKMARK);
   }
   for( ; iIndex < 20; iIndex++  ) {
      ::wsprintf(szKey, _T("bookmarks.%s.%d"), pstrName, iIndex);
      m_pDevEnv->SetProperty(szKey, _T(""));
   }
}

void CScintillaView::_RestoreBookmarks()
{
   LPCTSTR pstrName = ::PathFindFileName(m_sFilename);
   TCHAR szKey[200] = { 0 };
   for( int iIndex = 1; iIndex < 20; iIndex++  ) {
      ::wsprintf(szKey, _T("bookmarks.%s.%d"), pstrName, iIndex);
      TCHAR szValue[64] = { 0 };
      m_pDevEnv->GetProperty(szKey, szValue, (sizeof(szValue) / sizeof(TCHAR)) - 1);
      if( _tcslen(szValue) == 0 ) break;
      MarkerAdd(_ttoi(szValue), MARKER_BOOKMARK);
   }
}

void CScintillaView::_GetSyntaxStyle(LPCTSTR pstrName, SYNTAXCOLOR& syntax)
{
   // Reset syntax settings
   _tcscpy(syntax.szFont, _T(""));
   syntax.iHeight = 0;
   syntax.clrText = ::GetSysColor(COLOR_WINDOWTEXT);
   syntax.clrBack = ::GetSysColor(COLOR_WINDOW);
   syntax.bBold = FALSE;
   syntax.bItalic = FALSE;

   CString sKey = pstrName;
   LPTSTR p = NULL;
   TCHAR szBuffer[64] = { 0 };

   _tcscpy(syntax.szFont, _GetProperty(sKey + _T("font")));
   syntax.iHeight = _ttol(_GetProperty(sKey + _T("height")));
   if( m_pDevEnv->GetProperty(sKey + _T("color"), szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1) ) {
      syntax.clrText = _tcstol(szBuffer + 1, &p, 16);
      syntax.clrText = RGR2RGB(syntax.clrText);
   }
   if( m_pDevEnv->GetProperty(sKey + _T("back"), szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1) ) {
      syntax.clrBack = _tcstol(szBuffer + 1, &p, 16);
      syntax.clrBack = RGR2RGB(syntax.clrBack);
   }
   if( _GetProperty(sKey + _T("bold")) == _T("true") ) syntax.bBold = true;
   if( _GetProperty(sKey + _T("italic")) == _T("true") ) syntax.bItalic = true;
}

bool CScintillaView::_AddUnqiue(CSimpleArray<CString>& aList, LPCTSTR pstrText) const
{
   bool bFound = false;
   for( int i = 0; !bFound && i < aList.GetSize(); i++ ) bFound = aList[i] == pstrText;
   if( bFound ) return false;
   CString sText = pstrText;
   aList.Add(sText);
   return true;
}

/**
 * Is a valid language text spot?
 * We consider comments illegal insertion-points for many of our
 * text services.
 */
bool CScintillaView::_IsValidInsertPos(long lPos) const
{
   struct 
   {
      int iStyle; 
      LPCTSTR pstrLanguage;
   } Types[] =
   {
      { SCE_C_STRING,          _T("cpp") },
      { SCE_C_STRINGEOL,       _T("cpp") },
      { SCE_C_COMMENT,         _T("cpp") },
      { SCE_C_COMMENTDOC,      _T("cpp") },
      { SCE_C_COMMENTLINE,     _T("cpp") },
      { SCE_HJ_COMMENT,        _T("java") },
      { SCE_HJ_STRINGEOL,      _T("java") },
      { SCE_HJ_COMMENT,        _T("java") },
      { SCE_HJ_STRINGEOL,      _T("java") },
      { SCE_SH_STRING,         _T("bash") },
      { SCE_SH_COMMENTLINE,    _T("bash") },
      { SCE_HBA_STRING,        _T("basic") },
      { SCE_HBA_COMMENTLINE,   _T("basic") },
      { SCE_HPA_STRING,        _T("pascal") },
      { SCE_HPA_COMMENTLINE,   _T("pascal") },
      { SCE_HPHP_COMMENT,      _T("php") },
      { SCE_HPHP_COMMENTLINE,  _T("php") },
      { SCE_HPHP_HSTRING,      _T("php") },
      { SCE_HPHP_SIMPLESTRING, _T("php") },
   };
   int iStyle = GetStyleAt(lPos);
   for( int i = 0; i < sizeof(Types)/sizeof(Types[0]); i++ ) {
      if( iStyle == Types[i].iStyle && m_sLanguage == Types[i].pstrLanguage ) return false;
   }
   return true;
}

bool CScintillaView::_iseditchar(char ch) const
{
   return isalnum(ch) || ch == '_' || ch == '$' || ch == '~';
}


FINDREPLACEA CScintillaView::s_frFind = { 0 };
