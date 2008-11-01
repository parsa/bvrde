
#include "StdAfx.h"
#include "resource.h"

#include "GenEdit.h"
#include "ScintillaView.h"
#include "SciLexer.h"

#pragma code_seg( "EDITOR" )


#ifndef BlendRGB
   #define BlendRGB(c1, c2, factor) \
      RGB( GetRValue(c1) + ((GetRValue(c2) - GetRValue(c1)) * factor / 100L), \
           GetGValue(c1) + ((GetGValue(c2) - GetGValue(c1)) * factor / 100L), \
           GetBValue(c1) + ((GetBValue(c2) - GetBValue(c1)) * factor / 100L) )
#endif


LRESULT CScintillaView::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   ATLASSERT(IsWindow());

   CString sFilename = m_sFilename;
   sFilename.MakeLower();

   CString sKey;
   SYNTAXCOLOR syntax[20] = { 0 };
   int* ppStyles = NULL;

   // Get syntax coloring
   for( long i = 1; i <= sizeof(syntax) / sizeof(SYNTAXCOLOR); i++ ) {
      sKey.Format(_T("editors.%s.style%ld."), m_sLanguage, i);
      _GetSyntaxStyle(sKey, syntax[i - 1]);
   }

   // Load keywords
   CString sKeywords1;
   sKey.Format(_T("editors.%s.keywords"), m_sLanguage);
   m_pDevEnv->GetProperty(sKey, sKeywords1.GetBuffer(2048), 2048);
   sKeywords1.ReleaseBuffer();

   int iKeywordSet = 1;
   CString sKeywords2;
   sKey.Format(_T("editors.%s.customwords"), m_sLanguage);
   m_pDevEnv->GetProperty(sKey, sKeywords2.GetBuffer(2048), 2048);
   sKeywords2.ReleaseBuffer();

   SetStyleBits(5);

   int iWordStyle = STYLE_DEFAULT;

   if( m_sLanguage == _T("cpp") ) 
   {
      // Make really sure it's the CPP language. We'll use our
      // own custom lexer for Scintilla
      //SetLexer(SCLEX_CPP);
      CHAR szDllModule[MAX_PATH];
      ::GetModuleFileNameA(_Module.GetModuleInstance(), szDllModule, MAX_PATH);
      LoadLexerLibrary(szDllModule);
      SetLexerLanguage("BVRDE_cpp");

      static int aCppStyles[] = 
      {
         STYLE_DEFAULT,                0,
         SCE_C_DEFAULT,                0,
         SCE_C_WORD,                   1,
         SCE_C_IDENTIFIER,             2,
         SCE_C_GLOBALCLASS,            3,
         SCE_C_FUNCTIONCLASS,          4,
         SCE_C_TYPEDEFCLASS,           5,
         SCE_C_COMMENT,                6,
         SCE_C_COMMENTLINE,            6,
         SCE_C_NUMBER,                 7,
         SCE_C_OPERATOR,               8,
         SCE_C_STRING,                 9,
         SCE_C_STRINGEOL,              9,
         SCE_C_CHARACTER,              9,
         SCE_C_UUID,                   9,
         SCE_C_REGEX,                  9,
         SCE_C_PREPROCESSOR,           10,
         SCE_C_WORD2,                  11,
         STYLE_BRACELIGHT,             12,
         STYLE_BRACEBAD,               13,
         SCE_C_COMMENTDOC,             14,
         SCE_C_COMMENTLINEDOC,         14,
         SCE_C_COMMENTDOCKEYWORD,      14,
         SCE_C_COMMENTDOCKEYWORDERROR, 14,
         -1, -1,
      };
      ppStyles = aCppStyles;
      iWordStyle = SCE_C_WORD2;

      // Create custom word- & brace highlight-styles from default
      syntax[11] = syntax[1];
      syntax[11].bBold = true;
      syntax[12] = syntax[0];
      syntax[12].clrText = BlendRGB(syntax[0].clrText, RGB(0,0,0), 20);
      syntax[12].clrBack = BlendRGB(syntax[0].clrBack, RGB(0,0,0), 8);
      syntax[12].bBold = true;
      syntax[13] = syntax[0];
      syntax[13].clrText = RGB(200,0,0);
      syntax[13].bBold = true;
      syntax[14] = syntax[6];
      syntax[14].bBold = true;
      syntax[14].clrText= BlendRGB(syntax[6].clrText, syntax[6].clrBack, 30);
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
   else if( m_sLanguage == _T("bash") ) 
   {
      // Make really sure it's the BASH lexer
      SetLexer(SCLEX_BASH);

      static int aBashStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_SH_DEFAULT,         0,
         SCE_SH_WORD,            0,
         SCE_SH_OPERATOR,        1,
         SCE_SH_COMMENTLINE,     2,
         SCE_SH_STRING,          3,
         SCE_SH_CHARACTER,       3,
         SCE_SH_NUMBER,          4,
         SCE_SH_IDENTIFIER,      5,
         -1, -1,
      };
      ppStyles = aBashStyles;
   }
   else if( m_sLanguage == _T("java") ) 
   {
      // Make really sure it's the CPP (includes JAVA syntax) lexer
      SetLexer(SCLEX_CPP);

      static int aJavaStyles[] = 
      {
         STYLE_DEFAULT,        0,
         SCE_C_DEFAULT,        0,
         SCE_C_WORD,           0,
         SCE_C_IDENTIFIER,     1,
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
         SCE_C_DEFAULT,        0,
         SCE_C_WORD,           0,
         SCE_C_IDENTIFIER,     1,
         SCE_C_WORD2,          1,
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
         SCE_PL_DEFAULT,       0,
         SCE_PL_ERROR,         0,
         SCE_PL_WORD,          0,
         SCE_PL_IDENTIFIER,    1,
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
         SCE_PL_POD,           5,
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
         SCE_P_DEFAULT,        0,
         SCE_P_WORD,           0,
         SCE_P_IDENTIFIER,     1,
         SCE_P_CLASSNAME,      1,
         SCE_P_DEFNAME,        1,
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
      // Make really sure it's the BASIC lexer
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
         STYLE_DEFAULT,          0,
         SCE_SQL_DEFAULT,        0,
         SCE_SQL_WORD,           1,
         SCE_SQL_IDENTIFIER,     2,
         SCE_SQL_STRING,         2,
         SCE_SQL_COMMENT,        3,
         SCE_SQL_COMMENTLINE,    3,
         SCE_SQL_COMMENTDOC,     3,
         SCE_SQL_NUMBER,         4,
         SCE_SQL_OPERATOR,       4,
         SCE_SQL_CHARACTER,      5,
         SCE_SQL_WORD2,          5,
         -1, -1,
      };
      ppStyles = aSqlStyles;
      iWordStyle = SCE_SQL_WORD;
   }
   else if( m_sLanguage == _T("xml") ) 
   {
      // Make really sure it's the XML lexer
      SetLexer(SCLEX_XML);

      static int aXmlStyles[] = 
      {
         STYLE_DEFAULT,          0,
         SCE_H_DEFAULT,          0,
         SCE_H_OTHER,            0,
         SCE_H_TAG,              1,
         SCE_H_TAGUNKNOWN,       1,
         SCE_H_ATTRIBUTE,        2,
         SCE_H_ATTRIBUTEUNKNOWN, 2,
         SCE_H_NUMBER,           3,
         SCE_H_DOUBLESTRING,     4,
         SCE_H_SINGLESTRING,     4,
         SCE_H_CDATA,            4,
         SCE_H_VALUE,            4,
         SCE_H_COMMENT,          5,
         SCE_H_XCCOMMENT,        5,
         SCE_H_XMLSTART,         6,
         SCE_H_XMLEND,           6,
         SCE_H_TAGEND,           6,
         //
         STYLE_BRACELIGHT,       7,
         STYLE_BRACEBAD,         8,
         -1, -1,
      };
      ppStyles = aXmlStyles;

      syntax[7] = syntax[1];
      syntax[7].clrText = BlendRGB(syntax[1].clrText, RGB(0,0,0), 20);
      syntax[7].clrBack = BlendRGB(syntax[1].clrBack, RGB(0,0,0), 8);
      syntax[7].bBold = true;
      syntax[8] = syntax[1];
      syntax[8].clrText = RGB(200,0,0);
      syntax[8].bBold = true;
   }
   else if( m_sLanguage == _T("html") 
            || m_sLanguage == _T("php") 
            || m_sLanguage == _T("asp") ) 
   {
      // Make really sure it's the HTML lexer or a matching lexer
      // for HTML with embedded scripts.
      // FIX: Scintilla's stylers for embedded HTML scripts are pretty
      //      broken and the default HTML lexer actually works much better.
      /*
      if( sFilename.Find(_T(".php")) >= 0 ) SetLexer(SCLEX_PHP);
      else if( sFilename.Find(_T(".asp")) >= 0 ) SetLexer(SCLEX_ASP);
      else if( sFilename.Find(_T(".aspx")) >= 0 ) SetLexer(SCLEX_ASP);
      */
      SetLexer(SCLEX_HTML);

      if( m_sLanguage == _T("asp") ) iKeywordSet = 1;  // JavaScript keylist
      if( m_sLanguage == _T("php") ) iKeywordSet = 4;  // PHP keylist

      // Turn on indicator style bits for languages with embedded scripts
      // since the Scintilla lexer uses all possible styles
      SetStyleBits(7);

      static int aHtmlStyles[] = 
      {
         STYLE_DEFAULT,             0,
         SCE_H_DEFAULT,             0,
         SCE_H_OTHER,               0,
         SCE_H_TAG,                 1,
         SCE_H_TAGUNKNOWN,          1,
         SCE_H_ATTRIBUTE,           2,
         SCE_H_ATTRIBUTEUNKNOWN,    2,
         SCE_H_NUMBER,              3,
         SCE_H_DOUBLESTRING,        4,
         SCE_H_SINGLESTRING,        4,
         SCE_H_COMMENT,             5,
         SCE_H_XCCOMMENT,           5,
         SCE_H_ASP,                 6,
         SCE_H_ASPAT,               6,
         SCE_H_QUESTION,            8,
         //
         SCE_HB_DEFAULT,            6,
         SCE_HB_WORD,               7,
         SCE_HB_IDENTIFIER,         7,
         SCE_HB_COMMENTLINE,        5,
         SCE_HB_STRING,             4,
         SCE_HB_STRINGEOL,          4,
         //
         SCE_HJ_DEFAULT,            6,
         SCE_HJ_WORD,               7,
         SCE_HJ_KEYWORD,            7,
         SCE_HJ_COMMENT,            5,
         SCE_HJ_COMMENTLINE,        5,
         SCE_HJ_DOUBLESTRING,       4,
         SCE_HJ_SINGLESTRING,       4,
         //
         SCE_HP_DEFAULT,            6,
         SCE_HP_IDENTIFIER,         7,
         SCE_HP_COMMENTLINE,        5,
         SCE_HP_STRING,             4,
         //
         SCE_HBA_DEFAULT,           6,
         SCE_HBA_WORD,              7,
         SCE_HBA_IDENTIFIER,        7,
         SCE_HBA_COMMENTLINE,       5,
         SCE_HBA_STRING,            4,
         SCE_HBA_STRINGEOL,         4,
         //
         SCE_HP_DEFAULT,            6,
         SCE_HP_WORD,               7,
         SCE_HP_CLASSNAME,          7,
         SCE_HP_DEFNAME,            7,
         SCE_HP_IDENTIFIER,         6,
         SCE_HP_COMMENTLINE,        5,
         SCE_HP_STRING,             4,
         //
         SCE_HJA_DEFAULT,           6,
         SCE_HJA_SYMBOLS,           6,
         SCE_HJA_WORD,              7,
         SCE_HJA_KEYWORD,           7,
         SCE_HJA_DOUBLESTRING,      4,
         SCE_HJA_SINGLESTRING,      4,
         SCE_HJA_STRINGEOL,         4,
         SCE_HJA_COMMENT,           5,
         SCE_HJA_COMMENTLINE,       5,
         SCE_HJA_COMMENTDOC,        5,
         //
         SCE_HPHP_DEFAULT,          6,
         SCE_HPHP_OPERATOR,         6,
         SCE_HPHP_NUMBER,           6,
         SCE_HPHP_WORD,             7,
         SCE_HPHP_VARIABLE,         7,
         SCE_HPHP_HSTRING,          4,
         SCE_HPHP_SIMPLESTRING,     4,
         SCE_HPHP_HSTRING_VARIABLE, 4,
         SCE_HPHP_COMMENT,          5,
         SCE_HPHP_COMMENTLINE,      5,
         //
         STYLE_BRACELIGHT,          9,
         STYLE_BRACEBAD,            10,
         //
         -1, -1,
      };
      ppStyles = aHtmlStyles;

      // Some of the derived styles need some adjustment
      // so they conform with the color-scheme.
      // Derive Bracelight color from script when used with a
      // PHP or other scripting language.
      int iBraceIndex = m_sLanguage == _T("html") ? 1 : 6;
      syntax[7] = syntax[6];
      syntax[7].clrText = BlendRGB(syntax[6].clrText, RGB(0,0,0), 40);
      syntax[8] = syntax[0];
      syntax[8].clrText = RGB(100,0,0);
      syntax[8].bBold = true;
      syntax[9] = syntax[iBraceIndex];
      syntax[9].clrText = BlendRGB(syntax[iBraceIndex].clrText, RGB(0,0,0), 20);
      syntax[9].clrBack = BlendRGB(syntax[iBraceIndex].clrBack, RGB(0,0,0), 8);
      syntax[9].bBold = true;
      syntax[10] = syntax[iBraceIndex];
      syntax[10].clrText = RGB(200,0,0);
      syntax[10].bBold = true;
   }

   // Clear all styles
   ClearDocumentStyle();

   // Define bookmark markers
   _DefineMarker(MARKER_BOOKMARK, SC_MARK_SMALLRECT, RGB(0,0,64), RGB(128,128,128));

   // If this Editor is part of a C++ project we
   // know how to link it up with the debugger
   if( m_sLanguage == _T("cpp") ) 
   {
      // Breakpoint marked as background color
      sKey.Format(_T("editors.%s."), m_sLanguage);
      TCHAR szBuffer[64] = { 0 };
      bool bBreakpointAsLines = false;
      if( m_pDevEnv->GetProperty(sKey + _T("breakpointLines"), szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1) ) bBreakpointAsLines = _tcscmp(szBuffer, _T("true")) == 0;
      // Define markers for current-line and breakpoint
      if( bBreakpointAsLines ) {
         _DefineMarker(MARKER_BREAKPOINT, SC_MARK_BACKGROUND, RGB(250,62,62), RGB(250,62,62));
         _DefineMarker(MARKER_CURLINE, SC_MARK_BACKGROUND, RGB(0,245,0), RGB(0,245,0));
         _DefineMarker(MARKER_RUNNING, SC_MARK_BACKGROUND, RGB(245,245,0), RGB(245,245,0));
      }
      else {
         _DefineMarker(MARKER_BREAKPOINT, SC_MARK_CIRCLE, RGB(0,0,0), RGB(200,32,32));
         _DefineMarker(MARKER_CURLINE, SC_MARK_SHORTARROW, RGB(0,0,0), RGB(0,200,0));
         _DefineMarker(MARKER_RUNNING, SC_MARK_SHORTARROW, RGB(0,0,0), RGB(240,240,0));
      }
      // Set other debugging options
      SetMouseDwellTime(800);
   }

   // Apply keywords
   USES_CONVERSION;
   SetKeyWords(0, T2A(sKeywords1));
   if( !sKeywords2.IsEmpty() ) SetKeyWords(iKeywordSet, T2A(sKeywords2));

   // Apply our color-styles according to the
   // style-definition lists declared above
   COLORREF clrBack = RGB(255,255,255);
   while( ppStyles != NULL && *ppStyles != -1 ) 
   {
      int iStyle = *ppStyles;
      const SYNTAXCOLOR& c = syntax[*(ppStyles + 1)];
      StyleSetFore(iStyle, c.clrText);
      StyleSetBack(iStyle, c.clrBack);
      StyleSetBold(iStyle, c.bBold);
      StyleSetItalic(iStyle, c.bItalic);
      if( _tcslen(c.szFont) > 0 ) StyleSetFont(iStyle, T2A(c.szFont));
      if( c.iHeight > 0 ) StyleSetSize(iStyle, c.iHeight);
      if( iStyle == STYLE_DEFAULT ) clrBack = c.clrBack;
      ppStyles += 2;
   }

   COLORREF clrBackShade = BlendRGB(clrBack, RGB(0,0,0), 3);
   COLORREF clrBackDarkShade = BlendRGB(clrBack, RGB(0,0,0), 8);

   // Special contrast settings (BUG #1348377)
   if( clrBack == RGB(0,0,0) ) {
      COLORREF clrWhite = RGB(255,255,255);
      clrBackShade = BlendRGB(clrBack, clrWhite, 3);
      clrBackDarkShade = BlendRGB(clrBack, clrWhite, 8);
      SetCaretFore(RGB(255,255,255));
   }

   TCHAR szBuffer[64] = { 0 };
   int cchBuffer = 63;

   m_pDevEnv->GetProperty(_T("editors.general.eolMode"), szBuffer, cchBuffer);
   if( _tcscmp(szBuffer, _T("cr")) == 0 ) SetEOLMode(SC_EOL_CR);
   else if( _tcscmp(szBuffer, _T("lf")) == 0 ) SetEOLMode(SC_EOL_LF);
   else if( _tcscmp(szBuffer, _T("crlf")) == 0 ) SetEOLMode(SC_EOL_CRLF);

   sKey.Format(_T("editors.%s."), m_sLanguage);

   bool bValue;
   int iValue;
   char szValue[64] = { 0 };

   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showLines"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(0, bValue ? TextWidth(STYLE_LINENUMBER, "_9999") : 0);

   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("showMargin"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(1, bValue ? 16 : 0);
   SetMarginMaskN(1, bValue ? ~SC_MASK_FOLDERS : 0);
   SetMarginSensitiveN(1, TRUE);

   m_bFolding = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showFolding"), szBuffer, cchBuffer) ) m_bFolding = _tcscmp(szBuffer, _T("true")) == 0;
   SetMarginWidthN(2, m_bFolding ? 16 : 0);
   SetMarginMaskN(2, m_bFolding ? SC_MASK_FOLDERS : 0);
   SetMarginTypeN(2, SC_MARGIN_SYMBOL);
   SetMarginSensitiveN(2, TRUE);
   SetProperty("fold", m_bFolding ? "1" : "0");
   SetProperty("fold.html", m_bFolding ? "1" : "0");
   COLORREF clrMarkerFore = RGB(255,255,255);
   COLORREF clrMarkerBack = RGB(128,128,128);
   _DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED, clrMarkerFore, clrMarkerBack);
   _DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, clrMarkerFore, clrMarkerBack);
   SetFoldFlags(4);

   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showIndents"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetIndentationGuides(bValue == true);

   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("wordWrap"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetWrapMode(bValue ? SC_WRAP_WORD : SC_WRAP_NONE);

   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("useTabs"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetUseTabs(bValue == true);

   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("tabIndent"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetTabIndents(bValue == true);

   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("backUnindent"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetBackSpaceUnIndents(bValue == true);

   bValue = true;
   if( m_pDevEnv->GetProperty(sKey + _T("rectSelection"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetSelectionMode(bValue ? SC_SEL_RECTANGLE : SC_SEL_STREAM);

   iValue = 8;
   if( m_pDevEnv->GetProperty(sKey + _T("tabWidth"), szBuffer, cchBuffer) ) iValue = _ttol(szBuffer);
   SetTabWidth(max(iValue, 1));

   iValue = 0;
   if( m_pDevEnv->GetProperty(sKey + _T("indentWidth"), szBuffer, cchBuffer) ) iValue = _ttol(szBuffer);
   SetIndent(iValue);

   m_bAutoIndent = false;
   m_bSmartIndent = false;
   m_bAutoSuggest = false;
   if( m_pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, cchBuffer) ) m_bAutoIndent = _tcscmp(szBuffer, _T("auto")) == 0;
   if( m_pDevEnv->GetProperty(sKey + _T("indentMode"), szBuffer, cchBuffer) ) m_bSmartIndent = _tcscmp(szBuffer, _T("smart")) == 0;
   
   m_bMatchBraces = false;
   if( m_pDevEnv->GetProperty(sKey + _T("matchBraces"), szBuffer, cchBuffer) ) m_bMatchBraces = _tcscmp(szBuffer, _T("true")) == 0;

   m_bProtectDebugged = false;
   if( m_pDevEnv->GetProperty(sKey + _T("protectDebugged"), szBuffer, cchBuffer) ) m_bProtectDebugged = _tcscmp(szBuffer, _T("true")) == 0;

   m_bAutoComplete = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoComplete"), szBuffer, cchBuffer) ) m_bAutoComplete = _tcscmp(szBuffer, _T("true")) == 0;

   m_bAutoClose = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoClose"), szBuffer, cchBuffer) ) m_bAutoClose = _tcscmp(szBuffer, _T("true")) == 0;

   m_bAutoCase = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoCase"), szBuffer, cchBuffer) ) m_bAutoCase = _tcscmp(szBuffer, _T("true")) == 0;

   strcpy(szValue, "unchanged");
   if( m_pDevEnv->GetProperty(sKey + _T("caseMode"), szBuffer, cchBuffer) ) strcpy(szValue, T2CA(szBuffer));
   if( iWordStyle != STYLE_DEFAULT && strcmp(szValue, "upper") == 0 ) StyleSetCase(iWordStyle, SC_CASE_UPPER);
   if( iWordStyle != STYLE_DEFAULT && strcmp(szValue, "lower") == 0 ) StyleSetCase(iWordStyle, SC_CASE_LOWER);

   m_bAutoSuggest = false;
   if( m_pDevEnv->GetProperty(sKey + _T("autoSuggest"), szBuffer, cchBuffer) ) m_bAutoSuggest = _tcscmp(szBuffer, _T("true")) == 0;

   sKey = _T("editors.general.");

   iValue = 1;
   if( m_pDevEnv->GetProperty(sKey + _T("caretWidth"), szBuffer, cchBuffer) ) iValue = _ttol(szBuffer);
   SetCaretWidth(max(iValue, 1));
   
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("showEdge"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetEdgeMode(bValue ? EDGE_LINE : EDGE_NONE);
   
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("bottomless"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   
   bValue = false;
   if( m_pDevEnv->GetProperty(sKey + _T("markCaretLine"), szBuffer, cchBuffer) ) bValue = _tcscmp(szBuffer, _T("true")) == 0;
   SetCaretLineVisible(bValue);
   SetCaretLineBack(clrBackShade);

   sKey = _T("editors.folding.");

   strcpy(szValue, "0");
   if( m_pDevEnv->GetProperty(sKey + _T("compact"), szBuffer, cchBuffer) ) strcpy(szValue, _tccmp(szBuffer, _T("true")) == 0 ? "1" : "0");
   SetProperty("fold.compact", szValue);

   strcpy(szValue, "0");
   if( m_pDevEnv->GetProperty(sKey + _T("atPreprocessor"), szBuffer, cchBuffer) ) strcpy(szValue, _tccmp(szBuffer, _T("true")) == 0 ? "1" : "0");
   SetProperty("fold.preprocessor", szValue);

   strcpy(szValue, "0");
   if( m_pDevEnv->GetProperty(sKey + _T("atComment"), szBuffer, cchBuffer) ) strcpy(szValue, _tccmp(szBuffer, _T("true")) == 0 ? "1" : "0");
   SetProperty("fold.comment", szValue);

   strcpy(szValue, "0");
   if( m_pDevEnv->GetProperty(sKey + _T("atElse"), szBuffer, cchBuffer) ) strcpy(szValue, _tccmp(szBuffer, _T("true")) == 0 ? "1" : "0");
   SetProperty("fold.at.else", szValue);

   SetEdgeColumn(80);              // Place the right edge (if visible)
   UsePopUp(FALSE);                // We'll do our own context menu
   SetPasteConvertEndings(TRUE);   // Preserve EOL characters

   // Make sure we bump Scintilla into DBCS mode on supported asian
   // languages...
   static UINT aCP[] = { 932, 936, 949, 950, 1361, 0 };
   UINT acp = ::GetACP();
   for( UINT* pCP = aCP; *pCP != 0; pCP++ ) if( acp == *pCP ) SetCodePage((int)acp);

   // Never been initialized?
   if( !m_bInitialized ) {
      _RestoreBookmarks();
      m_bInitialized = true;
   }

   Colourise(0, -1);
   Invalidate();

   bHandled = FALSE;
   return 0;
}

