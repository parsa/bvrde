
#include "StdAfx.h"
#include "resource.h"

#include "GenEdit.h"
#include "ScintillaView.h"
#include "SciLexer.h"

#pragma code_seg( "MISC" )


/**
 *
 * NOTE:
 *  Parts of this Scintilla code was taken
 *  from the original Scintilla samples developed
 *  by Neil Hodgson <neilh@scintilla.org>.
 *
 */


CScintillaView::CScintillaView(IDevEnv* pDevEnv) :
   m_wndParent(this, 1),   
   m_pDevEnv(pDevEnv),
   m_bAutoCompleteNext(false),
   m_bAutoTextDisplayed(false)
{
   if( s_frFind.lStructSize == 0 ) {
      static CHAR s_szFindText[128] = { 0 };
      static CHAR s_szReplaceText[128] = { 0 };
      s_frFind.lStructSize = sizeof(s_frFind);
      s_frFind.hwndOwner = m_hWnd;
      s_frFind.hInstance = _Module.GetResourceInstance();
      s_frFind.Flags = FR_DOWN | FR_WRAP;
      s_frFind.lpstrFindWhat = s_szFindText;
      s_frFind.wFindWhatLen = (sizeof(s_szFindText) / sizeof(TCHAR)) - 1;
      s_frFind.lpstrReplaceWith = s_szReplaceText;
      s_frFind.wReplaceWithLen = (sizeof(s_szReplaceText) / sizeof(TCHAR)) - 1;
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

   pUIBase->UIEnable(ID_EDIT_VIEWWS, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWWS, GetViewWS() != SCWS_INVISIBLE);
   pUIBase->UIEnable(ID_EDIT_VIEWEOL, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWEOL, GetViewEOL());
   pUIBase->UIEnable(ID_EDIT_VIEWWORDWRAP, TRUE);
   pUIBase->UISetCheck(ID_EDIT_VIEWWORDWRAP, GetWrapMode() == SC_WRAP_WORD);

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

// Message map and handlers

LRESULT CScintillaView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_wndParent.SubclassWindow(GetParent());
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
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
   if( wParam == VK_ESCAPE && AutoCActive() ) AutoCCancel();
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{   
   ::SetFocus(m_hWnd);

   // Get the cursor position
   long lPos = GetCurrentPos();
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   POINT ptLocal = pt;
   ::MapWindowPoints(NULL, m_hWnd, &ptLocal, 1);
   if( lParam == (LPARAM) -1 ) {
      pt.x = PointXFromPosition(lPos);
      pt.y = PointYFromPosition(lPos);
   }
   // Place cursor at mouse if not clicked inside a selection
   lPos = PositionFromPoint(ptLocal.x, ptLocal.y);
   CharacterRange cr = GetSelection();
   if( lPos < cr.cpMin || lPos > cr.cpMax ) SetSel(lPos, lPos);

   // Grab EDIT submenu from main window's menu
   CMenuHandle menu = m_pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(1);
   m_pDevEnv->ShowPopupMenu(NULL, submenu, pt);
   return 0;
}

LRESULT CScintillaView::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   ATLASSERT(IsWindow());

   CString sFilename = m_sFilename;
   sFilename.MakeLower();

   CString sKey;
   SYNTAXCOLOR syntax[20] = { 0 };
   int* ppStyles = NULL;

   // Get syntax coloring
   for( long i = 1; i <= sizeof(syntax)/sizeof(SYNTAXCOLOR); i++ ) {
      sKey.Format(_T("editors.%s.style%ld."), m_sLanguage, i);
      _GetSyntaxStyle(sKey, syntax[i - 1]);
   }

   // Load keywords
   CString sKeywords1;
   sKey.Format(_T("editors.%s.keywords"), m_sLanguage);
   m_pDevEnv->GetProperty(sKey, sKeywords1.GetBuffer(2048), 2048);
   sKeywords1.ReleaseBuffer();

   CString sKeywords2;
   sKey.Format(_T("editors.%s.customwords"), m_sLanguage);
   m_pDevEnv->GetProperty(sKey, sKeywords2.GetBuffer(2048), 2048);
   sKeywords2.ReleaseBuffer();

   if( m_sLanguage == _T("cpp") ) 
   {
      // Make really sure it's the CPP language
      SetLexer(SCLEX_CPP);

      static int aCppStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_C_IDENTIFIER,     0,
         SCE_C_WORD,           1,
         SCE_C_DEFAULT,        1,
         SCE_C_COMMENT,        2,
         SCE_C_COMMENTLINE,    2,
         SCE_C_COMMENTDOC,     2,
         SCE_C_COMMENTLINEDOC, 2,
         SCE_C_NUMBER,         3,
         SCE_C_OPERATOR,       3,
         SCE_C_STRING,         4,
         SCE_C_STRINGEOL,      4,
         SCE_C_CHARACTER,      4,
         SCE_C_PREPROCESSOR,   5,
         SCE_C_WORD2,          6,
         STYLE_BRACELIGHT,     7,
         STYLE_BRACEBAD,       8,
         SCE_C_COMMENTDOCKEYWORD, 9,
         SCE_C_COMMENTDOCKEYWORDERROR, 9,
         -1, -1,
      };
      ppStyles = aCppStyles;

      // Create custom word- & brace highlight-styles from default
      syntax[6] = syntax[0];
      syntax[6].bBold = true;
      syntax[7] = syntax[0];
      syntax[7].bBold = true;
      syntax[8] = syntax[0];
      syntax[8].clrText = RGB(200,0,0);
      syntax[8].bBold = true;
      syntax[9] = syntax[2];
      syntax[9].bBold = true;
   }
   else if( m_sLanguage == _T("makefile") ) 
   {
      // Make really sure it's the MAKEFILE lexer
      SetLexer(SCLEX_MAKEFILE);

      static int aMakefileStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_MAKE_DEFAULT,       0,
         SCE_MAKE_COMMENT,       1,
         SCE_MAKE_TARGET,        2,
         SCE_MAKE_PREPROCESSOR,  3,
         -1, -1,
      };
      ppStyles = aMakefileStyles;
   }
   else if( m_sLanguage == _T("java") ) 
   {
      // Make really sure it's the CPP (includes JAVA syntax) lexer
      SetLexer(SCLEX_CPP);

      static int aJavaStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_C_IDENTIFIER,     0,
         SCE_C_WORD,           1,
         SCE_C_DEFAULT,        1,
         SCE_C_COMMENT,        2,
         SCE_C_COMMENTLINE,    2,
         SCE_C_COMMENTDOC,     2,
         SCE_C_COMMENTLINEDOC, 2,
         SCE_C_NUMBER,         3,
         SCE_C_OPERATOR,       3,
         SCE_C_STRING,         4,
         SCE_C_STRINGEOL,      4,
         SCE_C_CHARACTER,      4,
         SCE_C_PREPROCESSOR,   5,
         -1, -1,
      };
      ppStyles = aJavaStyles;
   }
   else if( m_sLanguage == _T("pascal") ) 
   {
      // Make really sure it's Pascal
      SetLexer(SCLEX_PASCAL);

      static int aPascalStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_C_IDENTIFIER,     0,
         SCE_C_WORD,           1,
         SCE_C_DEFAULT,        1,
         SCE_C_COMMENT,        2,
         SCE_C_COMMENTLINE,    2,
         SCE_C_COMMENTDOC,     2,
         SCE_C_COMMENTLINEDOC, 2,
         SCE_C_NUMBER,         3,
         SCE_C_OPERATOR,       3,
         SCE_C_STRING,         4,
         SCE_C_STRINGEOL,      4,
         SCE_C_CHARACTER,      4,
         SCE_C_PREPROCESSOR,   5,
         -1, -1,
      };
      ppStyles = aPascalStyles;
   }
   else if( m_sLanguage == _T("perl") ) 
   {
      // Make really sure it's the Perl language
      SetLexer(SCLEX_PERL);

      static int aPerlStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_PL_WORD,          1,
         SCE_PL_DEFAULT,       1,
         SCE_PL_COMMENTLINE,   2,
         SCE_PL_NUMBER,        3,
         SCE_PL_OPERATOR,      3,
         SCE_PL_STRING,        4,
         SCE_PL_STRING_Q,      4,
         SCE_PL_STRING_QQ,     4,
         SCE_PL_STRING_QX,     4,
         SCE_PL_STRING_QR,     4,
         SCE_PL_STRING_QW,     4,
         SCE_PL_CHARACTER,     4,
         SCE_PL_IDENTIFIER,    5,
         SCE_PL_PREPROCESSOR,  6,
         -1, -1,
      };
      ppStyles = aPerlStyles;
   }
   else if( m_sLanguage == _T("python") ) 
   {
      // Make really sure it's Python
      SetLexer(SCLEX_PYTHON);

      static int aPythonStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_P_IDENTIFIER,     0,
         SCE_P_WORD,           1,
         SCE_C_DEFAULT,        1,
         SCE_P_COMMENTLINE,    2,
         SCE_P_COMMENTBLOCK,   2,
         SCE_P_NUMBER,         3,
         SCE_P_OPERATOR,       3,
         SCE_P_STRING,         4,
         SCE_P_STRINGEOL,      4,
         SCE_P_CHARACTER,      4,
         SCE_P_TRIPLEDOUBLE,   5,
         -1, -1,
      };
      ppStyles = aPythonStyles;
   }
   else if( m_sLanguage == _T("basic") ) 
   {
      // Make really sure it's the MAKEFILE lexer
      SetLexer(SCLEX_VB);

      static int aBasicStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_B_DEFAULT,          0,
         SCE_B_DATE,             0,
         SCE_B_OPERATOR,         0,
         SCE_B_COMMENT,          1,
         SCE_B_KEYWORD,          2,
         SCE_B_IDENTIFIER,       3,
         SCE_B_NUMBER,           4,
         SCE_B_STRING,           5,
         SCE_B_PREPROCESSOR,     6,
         -1, -1,
      };
      ppStyles = aBasicStyles;
   }
   else if( m_sLanguage == _T("sql") ) 
   {
      // Make really sure it's the SQL lexer
      SetLexer(SCLEX_SQL);

      static int aSqlStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_C_WORD,           0,
         SCE_C_DEFAULT,        0,
         SCE_C_WORD,           1,
         SCE_C_IDENTIFIER,     2,
         SCE_C_COMMENT,        3,
         SCE_C_COMMENTLINE,    3,
         SCE_C_NUMBER,         4,
         SCE_C_OPERATOR,       4,
         SCE_C_STRING,         5,
         SCE_C_STRINGEOL,      5,
         SCE_C_CHARACTER,      5,
         -1, -1,
      };
      ppStyles = aSqlStyles;
   }
   else if( m_sLanguage == _T("xml") ) 
   {
      // Make really sure it's the XML lexer
      SetLexer(SCLEX_XML);

      static int aXmlStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_H_DEFAULT,          0,
         SCE_H_TAG,              1,
         SCE_H_TAGUNKNOWN,       1,
         SCE_H_ATTRIBUTE,        2,
         SCE_H_ATTRIBUTEUNKNOWN, 2,
         SCE_H_NUMBER,           3,
         SCE_H_DOUBLESTRING,     4,
         SCE_H_SINGLESTRING,     4,
         SCE_H_COMMENT,          5,
         SCE_H_XMLSTART,         6,
         SCE_H_XMLEND,           6,
         -1, -1,
      };
      ppStyles = aXmlStyles;
   }
   else if( m_sLanguage == _T("html") ) 
   {
      // Make really sure it's the HTML lexer or a matching lexer
      // for HTML with embedded scripts.
      if( sFilename.Find(_T(".php")) >= 0 ) SetLexer(SCLEX_PHP);
      else if( sFilename.Find(_T(".asp")) >= 0 ) SetLexer(SCLEX_ASP);
      else if( sFilename.Find(_T(".aspx")) >= 0 ) SetLexer(SCLEX_ASP);
      else SetLexer(SCLEX_HTML);

      static int aHtmlStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_H_DEFAULT,          0,
         SCE_H_TAG,              1,
         SCE_H_TAGUNKNOWN,       1,
         SCE_H_ATTRIBUTE,        2,
         SCE_H_ATTRIBUTEUNKNOWN, 2,
         SCE_H_NUMBER,           3,
         SCE_H_DOUBLESTRING,     4,
         SCE_H_SINGLESTRING,     4,
         SCE_H_COMMENT,          5,
         SCE_H_XCCOMMENT,        5,
         SCE_H_ASP,              6,
         //
         SCE_HB_DEFAULT,         6,
         SCE_HB_IDENTIFIER,      6,
         SCE_HB_COMMENTLINE,     5,
         //
         SCE_HJ_DEFAULT,         6,
         SCE_HJ_KEYWORD,         6,
         SCE_HJ_COMMENT,         5,
         SCE_HJ_COMMENTLINE,     5,
         //
         SCE_HP_DEFAULT,         6,
         SCE_HP_IDENTIFIER,      6,
         SCE_HP_COMMENTLINE,     5,
         //
         SCE_HBA_DEFAULT,        6,
         SCE_HBA_IDENTIFIER,     6,
         SCE_HBA_COMMENTLINE,    5,
         //
         SCE_HP_DEFAULT,         6,
         SCE_HP_IDENTIFIER,      6,
         SCE_HP_COMMENTLINE,     5,
         //
         SCE_HJA_DEFAULT,        6,
         SCE_HJA_KEYWORD,        6,
         SCE_HJA_COMMENT,        5,
         //
         SCE_HPHP_DEFAULT,       6,
         SCE_HPHP_WORD,          6,
         SCE_HPHP_COMMENT,       5,
         SCE_HPHP_COMMENTLINE,   5,
         //
         -1, -1,
      };
      ppStyles = aHtmlStyles;
   }

   // Clear all styles
   ClearDocumentStyle();
   SetStyleBits(7);

   // Define bookmark markers
   _DefineMarker(MARKER_BOOKMARK, SC_MARK_SMALLRECT, RGB(0, 0, 64), RGB(128, 128, 128));

   // If this Editor is part of a C++ project we
   // know how to link it up with the debugger
   if( m_sLanguage == _T("cpp") ) {
      // Define markers for current-line and breakpoint
      _DefineMarker(MARKER_BREAKPOINT, SC_MARK_CIRCLE, RGB(0,0,0), RGB(200,32,32));
      _DefineMarker(MARKER_CURLINE, SC_MARK_SHORTARROW, RGB(0,0,0), RGB(0,200,0));
      _DefineMarker(MARKER_RUNNING, SC_MARK_SHORTARROW, RGB(0,0,0), RGB(240,240,0));
      // Set other debugging options
      SetMouseDwellTime(800);
   }

   // Apply keywords
   USES_CONVERSION;
   SetKeyWords(0, T2A(sKeywords1));
   if( !sKeywords2.IsEmpty() ) SetKeyWords(1, T2A(sKeywords2));

   IndicSetStyle(0, 5); // INDIC_HIDDEN
   IndicSetStyle(1, 5);
   IndicSetStyle(2, 5);

   // Apply our color-styles according to the
   // style-definition lists declared above
   while( ppStyles && *ppStyles != -1 ) 
   {
      int iStyle = *ppStyles;
      const SYNTAXCOLOR& c = syntax[*(ppStyles + 1)];
      StyleSetFore(iStyle, c.clrText);
      StyleSetBack(iStyle, c.clrBack);
      StyleSetBold(iStyle, c.bBold);
      StyleSetItalic(iStyle, c.bItalic);
      if( _tcslen(c.szFont) > 0 ) StyleSetFont(iStyle, T2A(c.szFont));
      if( c.iHeight > 0 ) StyleSetSize(iStyle, c.iHeight);
      ppStyles += 2;
   }

   TCHAR szBuffer[64] = { 0 };
   sKey.Format(_T("editors.%s."), m_sLanguage);

   bool bValue;
   int iValue;

   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showLines"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(0, bValue ? TextWidth(STYLE_LINENUMBER, "_9999") : 0);
   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("showMargin"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(1, bValue ? 16 : 0);
   SetMarginMaskN(1, bValue ? ~SC_MASK_FOLDERS : 0);
   SetMarginSensitiveN(1, TRUE);
   m_bFolding = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showFolding"), szBuffer, 63) ) m_bFolding = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(2, m_bFolding ? 16 : 0);
   SetMarginMaskN(2, m_bFolding ? SC_MASK_FOLDERS : 0);
   SetMarginTypeN(2, SC_MARGIN_SYMBOL);
   SetMarginSensitiveN(2, TRUE);
   SetProperty("fold", m_bFolding ? "1" : "0");
   SetProperty("fold.html", m_bFolding ? "1" : "0");
   COLORREF clrFore = RGB(255,255,255);
   COLORREF clrBack = RGB(128,128,128);
   _DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED, clrFore, clrBack);
   _DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, clrFore, clrBack);
   SetFoldFlags(4);
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("wordWrap"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetWrapMode(bValue ? SC_WRAP_WORD : SC_WRAP_NONE);
   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("useTabs"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetUseTabs(bValue == true);
   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("tabIndent"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetTabIndents(bValue == true);
   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("backUnindent"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetBackSpaceUnIndents(bValue == true);
   iValue = 8;
   if( m_pDevEnv->GetProperty(sKey + _T("tabWidth"), szBuffer, 63) ) iValue = _ttol(szBuffer);
   SetTabWidth(max(iValue, 1));
   iValue = 0;
   if( m_pDevEnv->GetProperty(sKey + _T("indentWidth"), szBuffer, 63) ) iValue = _ttol(szBuffer);
   SetIndent(iValue);
   m_bAutoIndent = false;
   m_bSmartIndent = false;
   if( m_pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bAutoIndent = _tcscmp(szBuffer, _T("auto")) == 0;
   if( m_pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, 63) ) m_bSmartIndent = _tcscmp(szBuffer, _T("smart")) == 0;
   m_bMatchBraces = false;
   if( m_pDevEnv->GetProperty(sKey + _T("matchBraces"), szBuffer, 63) ) m_bMatchBraces = _tcscmp(szBuffer, _T("true")) == 0;
   m_bProtectDebugged = false;
   if( m_pDevEnv->GetProperty(sKey + _T("protectDebugged"), szBuffer, 63) ) m_bProtectDebugged = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoComplete = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoComplete"), szBuffer, 63) ) m_bAutoComplete = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoClose = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoClose"), szBuffer, 63) ) m_bAutoClose = _tcscmp(szBuffer, _T("true")) == 0;

   sKey = _T("editors.general.");

   iValue = 1;
   if( m_pDevEnv->GetProperty(sKey + _T("caretWidth"), szBuffer, 63) ) iValue = _ttol(szBuffer);
   SetCaretWidth(max(iValue, 1));
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showEdge"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetEdgeMode(bValue ? EDGE_LINE : EDGE_NONE);
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("bottomless"), szBuffer, 63) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetEndAtLastLine(!bValue);

   SetEdgeColumn(80);         // Place the right edge (if visible)
   UsePopUp(FALSE);           // We'll do our own context menu

   Colourise(0, -1);
   Invalidate();

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnUpdateUI(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   // Update statusbar cursor text
   long lPos = GetCurrentPos();
   long lCol = (long) GetColumn(lPos) + 1;
   long lRow = (long) LineFromPosition(lPos) + 1;
   CString sMsg;
   sMsg.Format(IDS_SB_POSITION, lRow, lCol);
   m_pDevEnv->ShowStatusText(ID_SB_PANE2, sMsg, TRUE);
   // Find matching brace
   _MatchBraces(lPos);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   if( m_bAutoCompleteNext 
       && (isalpha(pSCN->ch) || pSCN->ch == '_') ) 
   {
      _AutoComplete(m_cPrevChar);
   }
   m_bAutoCompleteNext = false;

   switch( pSCN->ch ) {
   case '\n':
   case '}':
      _MaintainIndent((CHAR)pSCN->ch);
      break;
   }

   switch( pSCN->ch ) {
   case '>':
   case '{':
   case '}':
      _MaintainTags((CHAR) pSCN->ch);
      break;
   }

   _AutoText((CHAR) pSCN->ch);
   
   m_cPrevChar = (CHAR) pSCN->ch;
   if( m_cPrevChar == '$' && GetLexer() == SCLEX_PHP ) m_bAutoCompleteNext = true;
   if( m_cPrevChar == '<' && m_sLanguage == _T("html") ) m_bAutoCompleteNext = true;
   if( m_cPrevChar == '<' && m_sLanguage == _T("xml") ) m_bAutoCompleteNext = true;
   
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   if( pSCN->margin == 2 ) {
      int iLine = LineFromPosition(pSCN->position);
      int iCount = GetLineCount();
      if( pSCN->modifiers & SCMOD_CTRL ) {
         for( int i = 1; i <= iCount; i++ ) if( GetFoldExpanded(i) ) ToggleFold(i);
      }
      else if( pSCN->modifiers & SCMOD_SHIFT ) {
         for( int i = 1; i <= iCount; i++ ) if( !GetFoldExpanded(i) ) ToggleFold(i);
      }
      else {
         ToggleFold(iLine);
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
   sMacro.Format(_T("Call ActiveWindow.SendRawMessage(%ld,%ld,\"%ls\")"), 
      (long) pSCN->message,
      (long) pSCN->wParam,
      pSCN->lParam == 0 ? "" : (LPCSTR) pSCN->lParam);
   m_pDevEnv->RecordMacro(sMacro);
   return 0;
}

// Implementation

void CScintillaView::_MatchBraces(long lPos)
{
   if( !m_bMatchBraces ) return;
   CHAR ch = (CHAR) GetCharAt(lPos);
   if( (ch == '{' && m_sLanguage == _T("cpp"))
       || (ch == '(' && m_sLanguage == _T("sql")) ) 
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

void CScintillaView::_AutoText(CHAR ch)
{
   static int s_iSuggestWord = 0;
   static long s_lSuggestPos = 0;
   static CString s_sAutoText;
   // Cancel auto-text tip if displayed already.
   // Insert replacement-text if needed.
   if( m_bAutoTextDisplayed ) {
      if( ch == '\t' || ch == '\0xFF' ) {
         USES_CONVERSION;
         long lPos = GetCurrentPos();
         int iLine = LineFromPosition(lPos);
         int iIndent = GetLineIndentation(iLine);
         // Extract text-replacement
         CString sText = s_sAutoText;
         int iCaretPos = sText.Find('$');
         int nLines = sText.Replace(_T("\\n"), _T("\n"));
         sText.Replace(_T("\\t"), _T("   "));
         sText.Replace(_T("\\$"), _T("\01"));
         sText.Replace(_T("$"), _T(""));
         sText.Replace(_T("\01"), _T("$"));
         // Replace text
         BeginUndoAction();
         SetSel(s_lSuggestPos, lPos);
         ReplaceSel(T2CA(sText));
         if( iCaretPos >= 0 ) SetSel(s_lSuggestPos + iCaretPos, s_lSuggestPos + iCaretPos);
         while( nLines > 0 ) {
            iLine++;
            nLines--;
            _SetLineIndentation(iLine, iIndent);
         }
         EndUndoAction();
      }
      CallTipCancel();
      m_bAutoTextDisplayed = false;
   }
   
   // Display new tip at all?
   if( AutoCActive() ) return;
   if( CallTipActive() ) return;

   // Display tip only after 3 characters has been entered
   // and avoid displaying unnessecarily...
   if( isalpha(ch) ) s_iSuggestWord++; 
   else if( isspace(ch) ) s_iSuggestWord = 0;
   else s_iSuggestWord = -999;
   if( s_iSuggestWord < 3 ) return;

   // Get the typed identifier
   long lPos = GetCurrentPos();
   CString sName = _GetNearText(lPos - 1);
   if( sName.IsEmpty() ) return;

   long i = 1;
   CString sKey;
   const int cchMax = 400;
   TCHAR szBuffer[cchMax] = { 0 };
   while( true ) {
      sKey.Format(_T("autotext.entry%ld.name"), i);
      szBuffer[0] = '\0';
      m_pDevEnv->GetProperty(sKey, szBuffer, cchMax - 1);
      if( szBuffer[0] == '\0' ) return;
      if( _tcsncmp(sName, szBuffer, sName.GetLength() ) == 0 ) break;
      i++;
   }

   szBuffer[0] = '\0';
   sKey.Format(_T("autotext.entry%ld.text"), i);
   m_pDevEnv->GetProperty(sKey, szBuffer, cchMax - 1);

   // Display suggestion-tip
   USES_CONVERSION;
   CallTipSetFore(::GetSysColor(COLOR_INFOTEXT));
   CallTipSetBack(::GetSysColor(COLOR_INFOBK));
   CString sTipText = szBuffer;
   sTipText.Replace(_T("\r"), _T(""));
   sTipText.Replace(_T("\\n"), _T("\n"));
   sTipText.Replace(_T("\\t"), _T("   "));
   CallTipShow(lPos, T2CA(sTipText));
   // Remember what triggered suggestion
   s_lSuggestPos = lPos - sName.GetLength();
   s_sAutoText = szBuffer;
   m_bAutoTextDisplayed = true;
}

void CScintillaView::_AutoComplete(CHAR ch)
{
   if( !m_bAutoComplete ) return;
   if( AutoCActive() ) return;

   USES_CONVERSION;
   CString sList;
   long lPos = GetCurrentPos();
   if( ch == '<' && m_sLanguage == _T("html") )
   {
      CString sProperty;
      sProperty.Format(_T("editors.%s.keywords"), m_sLanguage);
      CString sKeywords;
      m_pDevEnv->GetProperty(sProperty, sKeywords.GetBuffer(2048), 2048);
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
   }
   if( (ch == '$' && GetLexer() == SCLEX_PHP) ||
       (ch == '<' && m_sLanguage == _T("xml")) )
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
      while( pstr ) {
         pstr++;
         CString sWord;
         while( *pstr ) {
            sWord += (TCHAR) *pstr;
            pstr++;
            if( !isalnum(*pstr) && *pstr != '_' ) break;
         }
         if( sWord.GetLength() > 2 ) {
            // Only add uniques...
            bool bFound = false;
            for( int i = 0; !bFound && i < aList.GetSize(); i++ ) bFound = aList[i] == sWord;
            if( !bFound ) aList.Add(sWord);
         }
         pstr = strstr(pstr, szFind);
      }
      // Sort them
      for( int a = 0; a < aList.GetSize(); a++ ) {
         for( int b = a + 1; b < aList.GetSize(); b++ ) {
            if( _FunkyStrCmp(aList[a], aList[b]) > 0 ) {
               CString sTemp = aList[a];
               aList[a] = aList[b];
               aList[b] = sTemp;
            }
         }
      }
      for( int i = 0; i < aList.GetSize(); i++ ) sList += aList[i] + ' ';
   }
   // Display popup list
   sList.TrimRight();
   if( sList.IsEmpty() ) return;
   ClearRegisteredImages();
   AutoCSetIgnoreCase(TRUE);
   AutoCShow(1, T2CA(sList));
}

void CScintillaView::_MaintainIndent(CHAR ch)
{
   if( !m_bAutoIndent && !m_bSmartIndent ) return;
   int iCurLine = GetCurrentLine();
   int iLastLine = iCurLine - 1;
   int iIndentAmount = 0;
   while( iLastLine >= 0 && GetLineLength(iLastLine) == 0) iLastLine--;
   if( iLastLine >= 0 ) {
      iIndentAmount = GetLineIndentation(iLastLine);
      if( m_bSmartIndent ) {
         int iIndentWidth = GetIndent();
         if( iIndentWidth == 0 ) iIndentWidth = GetTabWidth();
         if( m_sLanguage == _T("cpp") 
             || m_sLanguage == _T("java") 
             || m_sLanguage == _T("perl") 
             || GetLexer() == SCLEX_PHP )             
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
         else if( m_sLanguage == _T("xml") ) 
         {
            if( ch == '\n' ) {
               CHAR szText[256] = { 0 };
               if( GetLineLength(iLastLine) >= sizeof(szText) ) return;
               GetLine(iLastLine, szText);
               if( strchr(szText, '>') != NULL && strchr(szText, '/') == NULL ) {
                  iIndentAmount += iIndentWidth;
               }
            }
         }
         else if( m_sLanguage == _T("html") )
         {
            if( ch == '\n' ) {
               CHAR szText[256] = { 0 };
               if( GetLineLength(iLastLine) >= sizeof(szText) ) return;
               GetLine(iLastLine, szText);
               if( strchr(szText, '>') != NULL && strchr(szText, '/') == NULL ) {
                  CString sLine = szText;
                  sLine.MakeUpper();
                  static LPCTSTR ppstrBlocks[] = 
                  {
                     _T("<BODY"),
                     _T("<FORM"),
                     _T("<FRAMESET"),
                     _T("<HEAD"),
                     _T("<HTML"),
                     _T("<OBJECT")
                     _T("<SCRIPT"),
                     _T("<TABLE"),
                     _T("<TD"),
                     _T("<THEAD"),
                     _T("<TR"),
                     _T("<UL"),
                     NULL,
                  };
                  for( LPCTSTR* ppstr = ppstrBlocks; *ppstr; ppstr++ ) {
                     if( sLine.Find(*ppstr) >= 0 ) {
                        iIndentAmount += iIndentWidth;
                        break;
                     }
                  }
               }
            }
         }
      }
   }
   if( iIndentAmount >= 0 ) _SetLineIndentation(iCurLine, iIndentAmount);
}

void CScintillaView::_MaintainTags(CHAR ch)
{
   if( !m_bAutoClose ) return;

   USES_CONVERSION;
   if( ch == '>'
       && (m_sLanguage == _T("html") || m_sLanguage == _T("xml")) )
   {
      // Grab the last 512 characters or so
      int nCaret = GetCurrentPos();
      CHAR szText[512] = { 0 };
      int nMin = nCaret - (sizeof(szText) - 1);
      if( nMin < 0 ) nMin = 0;
      if( nCaret - nMin < 3 ) return; // Smallest tag is 3 characters ex. <p>
      GetTextRange(nMin, nCaret, szText);

      if( szText[nCaret - nMin - 2] == '/' ) return;

      CString sFound = _FindOpenXmlTag(szText, nCaret - nMin);
      if( sFound.IsEmpty() ) return;
      // Ignore some of the non-closed HTML tags
      if( sFound.CompareNoCase(_T("BR")) == 0 ) return;
      if( sFound.CompareNoCase(_T("P")) == 0 ) return;
      // Insert end-tag into text
      CString sInsert;
      sInsert.Format(_T("</%s>"), sFound);
      BeginUndoAction();
      ReplaceSel(T2CA(sInsert));
      SetSel(nCaret, nCaret);
      EndUndoAction();
   }
   else if( (ch == '{' || ch == '}') 
            && m_sLanguage == _T("cpp") )
   {
      // Grab current line for inspection
      int nCaret = GetCurrentPos();
      int iLine = LineFromPosition(nCaret);
      CHAR szText[256] = { 0 };
      if( GetLineLength(iLine) >= sizeof(szText) ) return;
      GetLine(iLine, szText);
      CString sText = szText;
      sText.TrimLeft();
      sText.TrimRight();
      // Add insertion text
      CString sInsert;
      bool bIndentLine = false;
      if( sText == _T("{") ) {
         sInsert = _T("\r\n}");
         bIndentLine = true;
      }
      else if( sText.Right(1) == _T("{") ) {
         sInsert = _T("}");
      }
      BeginUndoAction();
      ReplaceSel(T2CA(sInsert));
      SetSel(nCaret, nCaret);
      if( bIndentLine ) _SetLineIndentation(iLine + 1, GetLineIndentation(iLine));
      EndUndoAction();
   }
}

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

void CScintillaView::_SetLineIndentation(int iLine, int iIndent)
{
   if( iIndent < 0 ) return;
   CharacterRange cr = GetSelection();
   int posBefore = GetLineIndentPosition(iLine);
   SetLineIndentation(iLine, iIndent);
   int posAfter = GetLineIndentPosition(iLine);
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
   else if( posAfter == posBefore ) {
      // Move to indent position
      cr.cpMin = cr.cpMax = posAfter;
   }
   SetSel(cr.cpMin, cr.cpMax);
}

CString CScintillaView::_GetProperty(CString sKey) const
{
   TCHAR szBuffer[64] = { 0 };
   m_pDevEnv->GetProperty(sKey, szBuffer, 63);
   return szBuffer;
}

int CScintillaView::_FindNext(int iFlags, LPCSTR pstrText, bool bWarnings)
{
   if( s_frFind.lStructSize == 0 ) return SendMessage(WM_COMMAND, MAKEWPARAM(ID_EDIT_FIND, 0));

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
      ATLASSERT(cr.cpMin!=cr.cpMax);
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
         HWND hWndMain = m_pDevEnv->GetHwnd(IDE_HWND_MAIN);
         ::PostMessage(hWndMain, WM_COMMAND, MAKEWPARAM(ID_EDIT_FIND, 0), 0L);
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

bool CScintillaView::_ReplaceOnce()
{
   CharacterRange cr = GetSelection();
   SetTargetStart(cr.cpMin);
   SetTargetEnd(cr.cpMax);
   int nReplaced = strlen(s_frFind.lpstrReplaceWith);
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
   int iLine = LineFromPosition(lPosition);
   CHAR szText[256] = { 0 };
   if( GetLineLength(iLine) >= sizeof(szText) ) return _T("");
   GetLine(iLine, szText);
   int iStart = lPosition - PositionFromLine(iLine);
   while( iStart > 0 && isalpha(szText[iStart - 1]) ) iStart--;
   if( !isalpha(szText[iStart]) ) return _T("");
   int iEnd = iStart;
   while( szText[iEnd] && isalpha(szText[iEnd + 1]) ) iEnd++;
   szText[iEnd + 1] = '\0';
   return szText + iStart;
}

void CScintillaView::_DefineMarker(int nMarker, int nType, COLORREF clrFore, COLORREF clrBack)
{
   MarkerDefine(nMarker, nType);
   MarkerSetFore(nMarker, clrFore);
   MarkerSetBack(nMarker, clrBack);
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

#define RGR2RGB(x) RGB((x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF)

   CString sKey = pstrName;
   LPTSTR p = NULL;
   TCHAR szBuffer[64] = { 0 };

   _tcscpy(syntax.szFont, _GetProperty(sKey + _T("font")));
   syntax.iHeight = _ttol(_GetProperty(sKey + _T("height")));
   if( m_pDevEnv->GetProperty(sKey + _T("color"), szBuffer, 63) ) {
      syntax.clrText = _tcstol(szBuffer + 1, &p, 16);
      syntax.clrText = RGR2RGB(syntax.clrText);
   }
   if( m_pDevEnv->GetProperty(sKey + _T("back"), szBuffer, 63) ) {
      syntax.clrBack = _tcstol(szBuffer + 1, &p, 16);
      syntax.clrBack = RGR2RGB(syntax.clrBack);
   }
   if( _GetProperty(sKey + _T("bold")) == _T("true") ) syntax.bBold = true;
   if( _GetProperty(sKey + _T("italic")) == _T("true") ) syntax.bItalic = true;
}

int CScintillaView::_FunkyStrCmp(LPCTSTR src, LPCTSTR dst)
{
   // Scintilla control has an obscure sorting of items!
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

FINDREPLACEA CScintillaView::s_frFind = { 0 };
