
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
   m_bAutoComplete(false),
   m_bIgnoreViews(false),
   m_pProject(NULL),
   m_pView(NULL)
{
}

// Operations

void CScintillaView::Init(CSqlProject* pProject, CView* pView)
{
   ATLASSERT(::IsWindow(m_hWnd));
   m_pProject = pProject;
   m_pView = pView;
   // Kick in timer
   SetTimer(TIMER_ID, 2000L);
}

// Message handlers

LRESULT CScintillaView::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   // If the original text came from a file and has changed, we'll
   // ask the user if he wants to save changes?
   if( !m_sFilename.IsEmpty() && GetModify() ) {
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("editors.general.savePrompt"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) {
         if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_SAVEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_ICONINFORMATION | MB_YESNO) ) return TRUE;
      }
      if( SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0)) != 0 ) return FALSE;
   }
   return TRUE;
}

LRESULT CScintillaView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   KillTimer(TIMER_ID);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{   
   bHandled = FALSE;
   if( wParam != TIMER_ID ) return 0;
   if( m_pView->m_thread.IsBusy() ) return 0;
   // Request most recent table to update column-information now
   m_pView->m_thread.InfoRequest();
   if( m_bBackgroundLoad ) SetTimer(TIMER_ID, 4000L); else KillTimer(TIMER_ID);
   return 0;
}

LRESULT CScintillaView::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CString sKey = _T("editors.sql.");

   TCHAR szBuffer[64] = { 0 };
   m_bIgnoreViews = false;
   if( _pDevEnv->GetProperty(sKey + _T("ignoreViews"), szBuffer, 63) ) m_bIgnoreViews = _tcscmp(szBuffer, _T("true")) == 0;
   m_bAutoComplete = false;
   if( _pDevEnv->GetProperty(sKey + _T("autoComplete"), szBuffer, 63) ) m_bAutoComplete = _tcscmp(szBuffer, _T("true")) == 0;
   m_bBackgroundLoad = false;
   if( _pDevEnv->GetProperty(sKey + _T("backgroundLoad"), szBuffer, 63) ) m_bBackgroundLoad = _tcscmp(szBuffer, _T("true")) == 0;

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_SQL));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
   DWORD dwStyle = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
   CFileDialog dlg(TRUE, NULL, NULL, dwStyle, sFilter, m_hWnd);
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   if( dlg.DoModal() != IDOK ) return 0;
   CWaitCursor cursor;
   m_sFilename = dlg.m_ofn.lpstrFile;
   CFile f;
   if( !f.Open(m_sFilename) ) return 0;
   DWORD dwSize = f.GetSize();
   LPSTR pstr = (LPSTR) malloc(dwSize + 1);
   if( pstr == NULL ) return 0;
   f.Read(pstr, dwSize);
   pstr[dwSize] = '\0';
   SetText(pstr);
   free(pstr);
   SetSavePoint();
   EmptyUndoBuffer();
   SendMessage(WM_SETTINGCHANGE);
   return 0;
}

LRESULT CScintillaView::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);

   if( !GetModify() ) return 0;
   if( m_pView == NULL ) return 0;

   if( m_sFilename.IsEmpty() ) {
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_SQL));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
      DWORD dwStyle = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
      CFileDialog dlg(FALSE, _T("sql"), NULL, dwStyle, sFilter, m_hWnd);
      dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
      if( dlg.DoModal() != IDOK ) return 0;
      m_sFilename = dlg.m_ofn.lpstrFile;
   }
   if( m_sFilename.IsEmpty() ) return 0;

   CWaitCursor cursor;

   m_pView->Save();

   int iSize = GetWindowTextLength();
   LPSTR pstr = (LPSTR) malloc(iSize + 1);
   GetText(iSize + 1, pstr);
   pstr[iSize] = '\0';
   CFile f;
   if( f.Create(m_sFilename) ) {
      f.Write(pstr, iSize);
      f.Close();
   }  
   free(pstr);

   SetSavePoint();

   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.clearUndo"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) EmptyUndoBuffer();

   return 0;
}

LRESULT CScintillaView::OnCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   switch( pSCN->ch ) {
   case ' ':
   case '.':
      _AutoComplete(pSCN->ch);
      break;
   }
   bHandled = FALSE;
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_FILE_SAVE, GetModify());
   pUIBase->UIEnable(ID_EDIT_COMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNCOMMENT, TRUE);
}

int CScintillaView::_FindItem(CSimpleArray<CString>& aList, LPCTSTR pstrName) const
{
   for( int i = 0; i < aList.GetSize(); i++ ) if( aList[i].CompareNoCase(pstrName) == 0 ) return i;
   return -1;
}

void CScintillaView::_AutoComplete(CHAR ch)
{
   // Allow popups?
   if( !m_bAutoComplete ) return;

   // Analyze the SQL
   SQLANALYZE Info;
   _AnalyseText(Info);

   // Mark all known tables so background thread can update
   // with column information
   int i = 0;
   for( i = 0; i < Info.aTables.GetSize(); i++ ) m_pView->MarkTable(Info.aTables[i]);

   // Did scanner determine that next item is a database object?
   if( !Info.bIsObjectNext ) {
      // No, but let's background load some more table information
      if( Info.aTables.GetSize() > 0 ) SetTimer(TIMER_ID, 2000L);
      return;
   }

   // Collect known database objects
   CSimpleArray<CString> aAllOwners;
   CSimpleArray<CString> aAllTables;
   m_pView->GetOwnerList(aAllOwners);
   m_pView->GetTableList(aAllTables, !m_bIgnoreViews);

   int iPos = 0;
   iPos =  _FindItem(aAllTables, Info.sPrevTable);
   if( iPos > 0 && _FindItem(Info.aTables, Info.sPrevTable) == -1 ) Info.aTables.Add(Info.sPrevTable);

   enum
   {
      LIST_NONE    = 0x00,
      LIST_TABLES  = 0x01,
      LIST_ALIAS   = 0x02,
      LIST_OWNERS  = 0x04,
      LIST_FIELDS  = 0x08,
   };

   CString sTable;
   int Display = LIST_NONE;

   switch( Info.PrevKeyword ) {
   case SQL_CONTEXT_FIELD:
   case SQL_CONTEXT_OPERATOR:
   case SQL_CONTEXT_EXPRESSION:
      {
         if( Info.aTables.GetSize() == 0 ) return;

         iPos = _FindItem(Info.aAlias, Info.sPrevKeyword);
         if( iPos >= 0 ) {
            Display = LIST_FIELDS;
            sTable = (iPos & 0x01) != 0 ? Info.aAlias[iPos] : Info.aAlias[iPos + 1];
            break;
         }
         iPos = _FindItem(aAllTables, Info.sPrevKeyword);
         if( iPos >= 0 ) {
            Display = LIST_FIELDS;
            sTable = aAllTables[iPos];
            break;
         }
         if( Info.aTables.GetSize() == 1 ) {
            Display = LIST_FIELDS;
            sTable = Info.aTables[0];
            break;
         }

         if( ch != '.' )
         {
            Display = LIST_TABLES | LIST_ALIAS;
         }
      }
      break;
   case SQL_CONTEXT_TABLE:
      {
         Display = LIST_TABLES | LIST_OWNERS;
         if( _FindItem(aAllOwners, Info.sPrevKeyword) >= 0 ) Display = LIST_TABLES;
      }
      break;
   }

   if( Display == LIST_NONE ) return;

   // Collect popup list items
   CSimpleArray<CString> aList;
   if( Display & LIST_OWNERS ) {
      for( i = 0; i < aAllOwners.GetSize(); i++ ) aList.Add(aAllOwners[i] + _T("?0"));
   }
   if( Display & LIST_TABLES ) {
      for( i = 0; i < aAllTables.GetSize(); i++ ) aList.Add(aAllTables[i] + _T("?1"));
   }
   if( Display & LIST_ALIAS ) {
      for( i = 0; i < Info.aAlias.GetSize(); i++ ) aList.Add(Info.aAlias[i] + ((i & 0x01) != 0 ? _T("?1") : _T("?2")));
      if( aList.GetSize() == 0 ) for( i = 0; i < Info.aTables.GetSize(); i++ ) aList.Add(Info.aTables[i] + _T("?1"));
   }
   if( Display & LIST_FIELDS ) {
      int iStartIndex = aList.GetSize();
      m_pView->GetColumnList(sTable, aList);
      for( i = iStartIndex; i < aList.GetSize(); i++ ) aList[i] = aList[i] + _T("?3");
   }

   // Sort list items
   for( int a = 0; a < aList.GetSize(); a++ ) {
      for( int b = a + 1; b < aList.GetSize(); b++ ) {
         if( _FunkyStrCmp(aList[a], aList[b]) > 0 ) {
            CString sTemp = aList[a];
            aList[a] = aList[b];
            aList[b] = sTemp;
         }
      }
   }

   // Create popup contents
   CString sList;
   for( i = 0; i < aList.GetSize(); i++ ) {
      sList += aList[i];
      sList += _T(" ");
   }
   sList.TrimRight();
   if( sList.IsEmpty() ) return;

   // Display popup
   USES_CONVERSION;
   ClearRegisteredImages();
   _RegisterListImages();
   AutoCSetIgnoreCase(TRUE);
   AutoCShow(0, T2CA(sList));
}

void CScintillaView::_AnalyseText(SQLANALYZE& Info)
{
   Info.bIsObjectNext = false;
   Info.PrevKeyword = SQL_CONTEXT_UNKNOWN;

   TCHAR szTerminator[8] = { 0 };
   _pDevEnv->GetProperty(_T("editors.sql.terminator"), szTerminator, 7);

   // Grab the last 300 characters or so
   long lPos = GetCurrentPos();
   CHAR szText[300] = { 0 };
   int nMin = lPos - (sizeof(szText) - 1);
   if( nMin < 0 ) nMin = 0;
   int nMax = lPos;
   GetTextRange(nMin, nMax, szText);

   // Do an optimistic scan of the SQL text to determine the
   // tables and alias definitions. Not a very exhaustive search.
   // BUG: Does not handle "<alias>" or [<name>] at all!
   CString sTable;
   CString sKeyword;
   LPSTR p = szText;
   while( *p ) {
      if( isalpha(*p) || *p == '_' ) {
         sKeyword += *p; 
      }
      else {
         if( !sKeyword.IsEmpty() ) Info.sPrevKeyword = sKeyword;
         sKeyword = _T("");
      }

      if( *p == '.' || *p == ',' ) Info.bIsObjectNext = true;

      p++;

      if( !isalpha(*p) && *p != '_' && sKeyword.GetLength() > 0 ) 
      {
         if( sKeyword.CompareNoCase(szTerminator) == 0 ) {
            Info.aTables.RemoveAll();
            Info.aAlias.RemoveAll();
            Info.sPrevTable = "";
         }
         SQLKEYWORD kw = _GetKeyword(sKeyword);
         if( Info.PrevKeyword == SQL_CONTEXT_TABLE 
             && kw == SQL_CONTEXT_UNKNOWN 
             && *p != '.' ) 
         {
            if( Info.bIsObjectNext ) {
               Info.aTables.Add(sKeyword); 
               Info.sPrevTable = sKeyword;
               Info.bIsObjectNext = false;
            }
            else {
               if( Info.aTables.GetSize() > 0 ) {
                  Info.aAlias.Add(sKeyword);
                  Info.aAlias.Add(Info.aTables[ Info.aTables.GetSize() - 1 ]);
               }
            }
         }
         else {
            if( *p == '.' ) Info.sPrevTable = sKeyword;
            Info.bIsObjectNext = false;
         }
         if( kw != SQL_CONTEXT_UNKNOWN ) {
            Info.PrevKeyword = kw;
            Info.bIsObjectNext = true;
         }
      }
   }

   // Parse the next few bytes of text as well to
   // see if we can recognize more table names.
   nMin = lPos;
   nMax = lPos + 100;
   if( nMax > GetTextLength() ) nMax = GetTextLength();
   
   ::ZeroMemory(szText, sizeof(szText));
   GetTextRange(nMin, nMax, szText);

   sKeyword = _T("");

   SQLKEYWORD CurKeyword = SQL_CONTEXT_UNKNOWN;
   bool bCurIsObjectNext = false;
   p = szText;
   while( *p ) {
      if( isalpha(*p) || *p == '_' ) {
         sKeyword += *p; 
      }
      else {
         if( *p == szTerminator[0] ) break;
         sKeyword = _T("");
      }

      if( *p == ',' || *p == '.' ) bCurIsObjectNext = true;

      p++;

      if( !isalpha(*p) && *p != '_' && sKeyword.GetLength() > 0 )
      {
         if( sKeyword.CompareNoCase(szTerminator) == 0 ) break;

         SQLKEYWORD kw = _GetKeyword(sKeyword);
         if( CurKeyword == SQL_CONTEXT_TABLE 
             && kw == SQL_CONTEXT_UNKNOWN 
             && *p != '.' ) 
         {
            if( bCurIsObjectNext ) {
               Info.aTables.Add(sKeyword); 
               bCurIsObjectNext = false;
            }
            else {
               if( Info.aTables.GetSize() > 0 ) {
                  Info.aAlias.Add(sKeyword);
                  Info.aAlias.Add(Info.aTables[ Info.aTables.GetSize() - 1 ]);
               }
            }
         }
         if( kw != SQL_CONTEXT_UNKNOWN ) {
            CurKeyword = kw;
            bCurIsObjectNext = true;
         }
      }
   }
}

CScintillaView::SQLKEYWORD CScintillaView::_GetKeyword(CString& sKeyword) const
{
   struct 
   {
      LPCTSTR pstrName;
      SQLKEYWORD kw;
   } aList[] = {
      { _T("SELECT"), SQL_CONTEXT_FIELD },
      { _T("VALUES"), SQL_CONTEXT_FIELD },
      { _T("BY"), SQL_CONTEXT_FIELD },
      //
      { _T("HAVING"), SQL_CONTEXT_EXPRESSION },
      { _T("WHERE"), SQL_CONTEXT_EXPRESSION },
      { _T("JOIN"), SQL_CONTEXT_EXPRESSION },
      { _T("SET"), SQL_CONTEXT_EXPRESSION },
      //
      { _T("UPDATE"), SQL_CONTEXT_TABLE },
      { _T("FROM"), SQL_CONTEXT_TABLE },
      { _T("INTO"), SQL_CONTEXT_TABLE },
      //
      { _T("BETWEEN"), SQL_CONTEXT_IGNORE },
      { _T("INSERT"), SQL_CONTEXT_IGNORE },
      { _T("DELETE"), SQL_CONTEXT_IGNORE },
      { _T("CREATE"), SQL_CONTEXT_IGNORE },
      { _T("GROUP"), SQL_CONTEXT_IGNORE },
      { _T("LIKE"), SQL_CONTEXT_IGNORE },
      { _T("IN"), SQL_CONTEXT_IGNORE },
      { _T("AS"), SQL_CONTEXT_IGNORE },
      //
      { _T("OR"), SQL_CONTEXT_OPERATOR },
      { _T("AND"), SQL_CONTEXT_OPERATOR },
      { _T("NOT"), SQL_CONTEXT_OPERATOR },
   };
   for( int i = 0; i < sizeof(aList)/sizeof(aList[0]); i++ ) {
      if( sKeyword.CompareNoCase(aList[i].pstrName) == 0 ) return aList[i].kw;
   }
   return SQL_CONTEXT_UNKNOWN;
}

int CScintillaView::_FunkyStrCmp(LPCTSTR src, LPCTSTR dst) const
{
   // Scintilla control has an obscure sorting of items!
   while( *dst ) {
      TCHAR c1 = *src < 'a' || *src > 'z' ? *src : *src - 'a' + 'A';
      TCHAR c2 = *dst < 'a' || *dst > 'z' ? *dst : *dst - 'a' + 'A';
      if( c1 - c2 != 0 ) return c1 - c2;
      src++;
      dst++;
   }
   return 1;
}

/* XPM */
static char *OwnerImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 9 1",
/* colors */
"a c None",
"b c #808000",
"c c #F0A050",
"d c #808080",
"e c #008000",
"f c #C0C0C0",
"g c #FFFFFF",
"h c #008080",
"i c #0008a0",
/* pixels */
"aaaaaabcccbaaaaa",
"aaaabbcccccaaaaa",
"aaaabbbbcccbdaaa",
"aaaaccccccccbaaa",
"aaaaccccccccbaaa",
"aaaaccccccccbaaa",
"aaaaccccccccbaaa",
"aaaabbcccccbdaaa",
"aaaabbcccccbaaaa",
"aaaaaadbbbbbbaaa",
"aaaaaaebbbddeaaa",
"aaaaaaebbbddeaaa",
"aaaadddfggfddeaa",
"aaaadddeggfdddaa",
"aaaehhddeeeddeei",
"aaaeeehdeeedeeda"
};
static char *TableImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 4 1",
/* colors */
"a c None",
"b c #000a80",
"c c #000a00",
"d c #C0C0C0",
/* pixels */
"aaaaaaaaaaaaaaaa"
"aaaabbbbbbbbbbba",
"aaaabbbbbbbbbbba",
"aaaacdddddddddca",
"aaaacdcdcdcdcdca",
"bbbbcbbbbbbdddca",
"bbbbcbbbbbbdcdca",
"cdddcdddddcdddca",
"cdcdcdcdcdcdcdca",
"cdddcdddddcdddca",
"cdcdccccccccccca",
"cdddddddddcaaaaa",
"cdcdcdcdcdcaaaaa",
"cdddddddddcaaaaa",
"cccccccccccaaaaa",
"aaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaa",
};
static char *AliasImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 6 1",
/* colors */
"a c None",
"b c #808080",
"c c #000a00",
"d c #FFFFFF",
"e c #000a80",
"f c #C0C0C0",
/* pixels */
"aaaaaaaaaaaaaaaa",
"aaaabbbbbbbcaaaa",
"aaaabddddddccaaa",
"aaeeeeeeeedcacaa",
"aaeeeeeeeefcccca",
"aacaaaaaacfdddca",
"eecfffffacbffdca",
"eecffffcacfdddca",
"cfcffffcacbffdca",
"cfccccccccfdddca",
"cffffffcbbbffdca",
"cffffffcfdddddca",
"ccccccccbffffdca",
"aaaacffffdddddca",
"aaaaccccccccccca",
"aaaaaaaaaaaaaaaa"
};
static char *FieldImage[] = {
/* width height num_colors chars_per_pixel */
"16 16 6 1",
/* colors */
"a c None",
"b c #808080",
"c c #000000",
"d c #FFFFFF",
"e c #00FFFF",
"f c #C0C0C0",
/* pixels */
"aaaaaaaaaaaaaaaa",
"aaaabbbbbbbcaaaa",
"aaaabddddddbcaaa",
"ccccccccccdbacaa",
"ceeeeeeaecfbccca",
"ccccccccccfdddca",
"cddddddddcbffdca",
"cddddddddcfdddca",
"cdcccddddcbffdca",
"cddddddddcfdddca",
"cdcccdccdcbffdca",
"cddddddddcfdddca",
"ccccccccccbffdca",
"aaaacffffffdddca",
"aaaaccccccccccca",
"aaaaaaaaaaaaaaaa"
};

void CScintillaView::_RegisterListImages()
{
   RegisterImage(0, (LPBYTE) OwnerImage);
   RegisterImage(1, (LPBYTE) TableImage);
   RegisterImage(2, (LPBYTE) AliasImage);
   RegisterImage(3, (LPBYTE) FieldImage);
}
