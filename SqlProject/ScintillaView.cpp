
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
   m_bAutoCompleteNext = true;
   m_pProject = pProject;
   m_pView = pView;
   // Kick in timer...
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
         if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_SAVEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_ICONQUESTION | MB_YESNO) ) return TRUE;
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
   // Request most recently touched table to update column-information now
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

LRESULT CScintillaView::OnEditAutoComplete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iLenEntered = 0;
   long lPos = GetCurrentPos();
   while( lPos > 1 && _issqlchar( (CHAR) GetCharAt(--lPos) ) ) iLenEntered++;
   _AutoComplete(GetCharAt(lPos), iLenEntered);
   return 0;
}

LRESULT CScintillaView::OnCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   SCNotification* pSCN = (SCNotification*) pnmh;
   
   // Auto-complete now?
   switch( pSCN->ch ) {
   case ' ':
   case '.':
      _AutoComplete(pSCN->ch, 0);
      break;
   case '\n':
      m_bAutoCompleteNext = true;
      break;
   default:
      if( m_bAutoCompleteNext ) _AutoComplete(pSCN->ch, 1);
      m_bAutoCompleteNext = false;
   }
   
   // Postpone background processing until we are idle?
   if( m_bBackgroundLoad ) SetTimer(TIMER_ID, 2000L); 

   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnHistoryNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   SetText("");
   SetSavePoint();
   return 0;
}

LRESULT CScintillaView::OnHistoryDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   ATLASSERT(m_pView);
   int iPos = m_pView->GetHistoryPos();
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_HISTORY_LEFT, 0));
   m_pView->DeleteHistory(iPos);
   return 0;
}

LRESULT CScintillaView::OnHistoryLeft(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   ATLASSERT(m_pView);
   int iPos = m_pView->GetHistoryPos();
   if( iPos == 0 ) return 0;
   CString sText = m_pView->SetHistoryPos(iPos - 1);
   LPSTR pstr = (LPSTR) malloc(sText.GetLength() * 2);
   if( pstr == NULL ) return 0;
   AtlW2AHelper(pstr, sText, sText.GetLength() + 1);
   SetText(pstr);
   free(pstr);
   SetSavePoint();
   return 0;
}

LRESULT CScintillaView::OnHistoryRight(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   ATLASSERT(m_pView);
   int iPos = m_pView->GetHistoryPos();
   if( iPos >= m_pView->GetHistoryCount() ) return 0;
   CString sText = m_pView->SetHistoryPos(iPos + 1);
   LPSTR pstr = (LPSTR) malloc(sText.GetLength() * 2);
   if( pstr == NULL ) return 0;
   AtlW2AHelper(pstr, sText, sText.GetLength() + 1);
   SetText(pstr);
   free(pstr);
   SetSavePoint();
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_FILE_SAVE, GetModify());
   pUIBase->UIEnable(ID_EDIT_COMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNCOMMENT, TRUE);
   pUIBase->UIEnable(ID_EDIT_AUTOCOMPLETE, TRUE);   
}

void CScintillaView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

// Implementation

int CScintillaView::_FindItem(CSimpleArray<CString>& aList, LPCTSTR pstrName) const
{
   for( int i = 0; i < aList.GetSize(); i++ ) if( aList[i].CompareNoCase(pstrName) == 0 ) return i;
   return -1;
}

static int QCompareCString(const void* a, const void* b)
{
   return ((CString*)a)->Compare(*(CString*)b);
}

void CScintillaView::_AutoComplete(int ch, int iLenEntered)
{
   USES_CONVERSION;

   // Allow popups?
   if( !m_bAutoComplete ) return;

   int i = 0;

   // Analyze the SQL
   SQLANALYZE Info;
   _AnalyseText(Info, iLenEntered);

   // Mark all discovered tables so background thread can update
   // with column information.
   for( i = 0; i < Info.aTables.GetSize(); i++ ) m_pView->MarkTable(Info.aTables[i]);

   // Did scanner determine that next item is a database object?
   if( !Info.bIsObjectNext ) {
      // No, but let us load some more table information.
      // This is how we control the loading of table information when the "background
      // processing" option is not enabled. If it is, we will further delay the
      // timer as the user types stuff.
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
      LIST_NONE      = 0x00,
      LIST_TABLES    = 0x01,
      LIST_ALIAS     = 0x02,
      LIST_OWNERS    = 0x04,
      LIST_FIELDS    = 0x08,
      LIST_KEYWORDS  = 0x10,
   };

   CString sTable;
   int iDisplay = LIST_NONE;

   switch( Info.PrevKeyword ) {
   case SQL_CONTEXT_START:
      iDisplay = LIST_KEYWORDS;
      break;
   case SQL_CONTEXT_FIELD:
   case SQL_CONTEXT_OPERATOR:
   case SQL_CONTEXT_EXPRESSION:
      {
         if( Info.aTables.GetSize() == 0 ) return;

         iPos = _FindItem(Info.aAlias, Info.sPrevKeyword);
         if( iPos >= 0 ) {
            iDisplay = LIST_FIELDS;
            sTable = (iPos & 0x01) != 0 ? Info.aAlias[iPos] : Info.aAlias[iPos + 1];
            break;
         }
         iPos = _FindItem(aAllTables, Info.sPrevKeyword);
         if( iPos >= 0 ) {
            iDisplay = LIST_FIELDS;
            sTable = aAllTables[iPos];
            break;
         }
         if( Info.aTables.GetSize() == 1 ) {
            iDisplay = LIST_FIELDS;
            sTable = Info.aTables[0];
            break;
         }

         if( ch != '.' )
         {
            iDisplay = LIST_TABLES | LIST_ALIAS;
         }
      }
      break;
   case SQL_CONTEXT_TABLE:
      {
         iDisplay = LIST_TABLES | LIST_OWNERS;
         if( _FindItem(aAllOwners, Info.sPrevKeyword) >= 0 ) iDisplay = LIST_TABLES;
      }
      break;
   }

   if( iDisplay == LIST_NONE ) return;

   // Collect popup list items
   CSimpleArray<CString> aList;
   if( (iDisplay & LIST_OWNERS) != 0 ) {
      for( i = 0; i < aAllOwners.GetSize(); i++ ) aList.Add(aAllOwners[i] + _T("?0"));
   }
   if( (iDisplay & LIST_TABLES) != 0 ) {
      for( i = 0; i < aAllTables.GetSize(); i++ ) aList.Add(aAllTables[i] + _T("?1"));
   }
   if( (iDisplay & LIST_ALIAS) != 0 ) {
      for( i = 0; i < Info.aAlias.GetSize(); i++ ) aList.Add(Info.aAlias[i] + ((i & 0x01) != 0 ? _T("?1") : _T("?2")));
      if( aList.GetSize() == 0 ) for( i = 0; i < Info.aTables.GetSize(); i++ ) aList.Add(Info.aTables[i] + _T("?1"));
   }
   if( (iDisplay & LIST_FIELDS) != 0 ) {
      CSimpleArray<CString> aMembers;
      m_pView->GetColumnList(sTable, aMembers);
      for( i = 0; i < aMembers.GetSize(); i++ ) aList.Add(aMembers[i] + _T("?3"));
   }
   if( (iDisplay & LIST_KEYWORDS) != 0 ) {
      aList.Add(CString(_T("SELECT?4")));
      aList.Add(CString(_T("INSERT?4")));
      aList.Add(CString(_T("DELETE?4")));
      aList.Add(CString(_T("UPDATE?4")));
   }

   // Sort list items
   ::qsort(aList.GetData(), aList.GetSize(), sizeof(CString), QCompareCString);

   // Create popup contents
   CString sList;
   sList.GetBuffer(aList.GetSize() * 40);
   sList.ReleaseBuffer();
   for( i = 0; i < aList.GetSize(); i++ ) {
      sList += aList[i];
      sList += _T("|");
   }
   sList.TrimRight(_T("|"));
   if( sList.IsEmpty() ) return;

   // Display popup
   _RegisterListImages();
   AutoCSetSeparator('|');
   AutoCSetIgnoreCase(TRUE);
   AutoCShow(iLenEntered, T2CA(sList));
}

void CScintillaView::_AnalyseText(SQLANALYZE& Info, int iLenEntered)
{
   USES_CONVERSION;

   Info.bIsObjectNext = false;
   Info.PrevKeyword = SQL_CONTEXT_UNKNOWN;

   TCHAR szTerminator[8] = { 0 };
   _pDevEnv->GetProperty(_T("editors.sql.terminator"), szTerminator, (sizeof(szTerminator) / sizeof(TCHAR)) - 1);
   LPCSTR pstrTerminator = T2CA(szTerminator);

   // Grab the last 256 characters or so
   long lPos = GetCurrentPos();
   CHAR szText[256] = { 0 };
   int nMin = lPos - iLenEntered - (sizeof(szText) - 1);
   int nMax = lPos - iLenEntered;
   if( nMin < 0 ) nMin = 0;
   if( nMax < 0 ) nMax = 0;
   GetTextRange(nMin, nMax, szText);

   // Enable keyword popup if at the beginning of the text
   if( nMin == 0 ) {
      Info.bIsObjectNext = true;
      Info.PrevKeyword = SQL_CONTEXT_START;
   }

   LPSTR p;
   bool bQuote;
   CString sTable;
   CString sKeyword;

   // Do an optimistic scan of the SQL text to determine the
   // tables and alias definitions. Not a very exhaustive search.
   // BUG: Does not handle "<alias>" or [<name>] at all!
   p = szText;
   lPos = nMin;
   bQuote = false;
   while( *p != '\0' ) 
   {
      // Skip comments and stuff
      while( *p != '\0' && !_IsRealSqlEditPos(lPos, true) ) {
         p++; lPos++;
         Info.bIsObjectNext = false;
      }

      // Collect data for keyword
      if( _issqlchar(*p) ) {
         sKeyword += *p; 
      }
      else if( *p == '\"' ) {
         bQuote = !bQuote;
      }
      else if( bQuote ) {
         sKeyword += *p;
      }
      else {
         if( !sKeyword.IsEmpty() ) Info.sPrevKeyword = sKeyword;
         sKeyword = _T("");
      }

      if( *p == '.' || *p == ',' ) Info.bIsObjectNext = true;

      p++; lPos++;

      if( !_issqlchar(*p) && sKeyword.GetLength() > 0 ) 
      {
         bool bWasTerminated = sKeyword.CompareNoCase(szTerminator) == 0
            || ( *p == szTerminator[0] && szTerminator[1] == '\0' );
         if( bWasTerminated ) {
            Info.aTables.RemoveAll();
            Info.aAlias.RemoveAll();
            Info.sPrevTable = _T("");
            Info.bIsObjectNext = true;
            Info.PrevKeyword = SQL_CONTEXT_START;
         }
         else {
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
   }

   // Parse the next few characters of text as well to
   // see if we can recognize more table names.
   lPos = GetCurrentPos();
   nMin = lPos + iLenEntered;
   nMax = lPos + iLenEntered + 100;
   if( nMin > GetTextLength() ) nMin = GetTextLength();
   if( nMax > GetTextLength() ) nMax = GetTextLength();
   
   ::ZeroMemory(szText, sizeof(szText));
   GetTextRange(nMin, nMax, szText);

   lPos = nMin;
   bQuote = false;
   sKeyword = _T("");

   SQLKEYWORD CurKeyword = SQL_CONTEXT_UNKNOWN;
   bool bCurIsObjectNext = false;
   p = szText;
   while( *p != '\0' ) 
   {
      // Skip comments and stuff
      while( *p != '\0' && !_IsRealSqlEditPos(lPos, true) ) {
         p++; lPos++;
      }

      // Collect data for keyword
      if( _issqlchar(*p)  ) {
         sKeyword += *p; 
      }
      else if( *p == '\"' ) {
         bQuote = !bQuote;
      }
      else if( bQuote ) {
         sKeyword += *p;
      }
      else {
         // Need to exit as early as possible so we don't parse the SQL statement that follows.
         if( strncmp(p, pstrTerminator, strlen(pstrTerminator)) == 0 ) break;
         if( sKeyword == szTerminator ) break;
         sKeyword = _T("");
      }

      if( *p == ',' || *p == '.' ) bCurIsObjectNext = true;

      p++; lPos++;

      if( !_issqlchar(*p) && sKeyword.GetLength() > 0 )
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
   static struct 
   {
      LPCTSTR pstrName; SQLKEYWORD kw;
   } aList[] = {
      { _T("SELECT"),   SQL_CONTEXT_FIELD },
      { _T("VALUES"),   SQL_CONTEXT_FIELD },
      { _T("BY"),       SQL_CONTEXT_FIELD },
      //
      { _T("HAVING"),   SQL_CONTEXT_EXPRESSION },
      { _T("WHERE"),    SQL_CONTEXT_EXPRESSION },
      { _T("JOIN"),     SQL_CONTEXT_EXPRESSION },
      { _T("SET"),      SQL_CONTEXT_EXPRESSION },
      //
      { _T("UPDATE"),   SQL_CONTEXT_TABLE },
      { _T("FROM"),     SQL_CONTEXT_TABLE },
      { _T("INTO"),     SQL_CONTEXT_TABLE },
      //
      { _T("BETWEEN"),  SQL_CONTEXT_IGNORE },
      { _T("INSERT"),   SQL_CONTEXT_IGNORE },
      { _T("DELETE"),   SQL_CONTEXT_IGNORE },
      { _T("CREATE"),   SQL_CONTEXT_IGNORE },
      { _T("GROUP"),    SQL_CONTEXT_IGNORE },
      { _T("LIKE"),     SQL_CONTEXT_IGNORE },
      { _T("IN"),       SQL_CONTEXT_IGNORE },
      { _T("AS"),       SQL_CONTEXT_IGNORE },
      //
      { _T("OR"),       SQL_CONTEXT_OPERATOR },
      { _T("AND"),      SQL_CONTEXT_OPERATOR },
      { _T("NOT"),      SQL_CONTEXT_OPERATOR },
   };
   for( int i = 0; i < sizeof(aList) / sizeof(aList[0]); i++ ) {
      if( sKeyword.CompareNoCase(aList[i].pstrName) == 0 ) return aList[i].kw;
   }
   return SQL_CONTEXT_UNKNOWN;
}

bool CScintillaView::_issqlchar(CHAR ch) const
{
   return isalpha(ch) || ch == '_';
}

/**
 * Determines if the current position allows auto-completion.
 * The editor does not allow auto-completion inside comments
 * or strings (literals).
 */
bool CScintillaView::_IsRealSqlEditPos(long lPos, bool bIncludeNonIdentifiers) const
{
   if( lPos < 0 ) return false;
   int x = GetStyleAt(lPos); x;
   switch( GetStyleAt(lPos) ) {
   case SCE_SQL_CHARACTER:
   case SCE_SQL_COMMENT:
   case SCE_SQL_COMMENTDOC:
   case SCE_SQL_COMMENTLINE:
   case SCE_SQL_COMMENTLINEDOC:
   case SCE_SQL_COMMENTDOCKEYWORD:
   case SCE_SQL_COMMENTDOCKEYWORDERROR:
   case SCE_SQL_SQLPLUS_COMMENT:
      return false;
   case SCE_SQL_NUMBER:
   case SCE_SQL_OPERATOR:
      return bIncludeNonIdentifiers;
   default:
      return true;
   }
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
static char* KeywordImage[] = {
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
   ClearRegisteredImages();
   RegisterImage(0, (LPBYTE) OwnerImage);
   RegisterImage(1, (LPBYTE) TableImage);
   RegisterImage(2, (LPBYTE) AliasImage);
   RegisterImage(3, (LPBYTE) FieldImage);
   RegisterImage(4, (LPBYTE) KeywordImage);
}

