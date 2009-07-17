
#include "StdAfx.h"
#include "resource.h"

#include "ScintillaView.h"
#include "SciLexer.h"

#include "Project.h"
#include "Files.h"


#pragma code_seg( "EDITOR" )


/**
 * Extract information about a C++ member at an editor position.
 */
bool CScintillaView::_GetMemberInfo(long lPos, CTagDetails& Info, DWORD dwTimeout, MEMBERMATCHMODE Mode)
{
   // There are some limitations to our capabilities...
   if( lPos < 10 ) return false;
   if( ::GetTickCount() > dwTimeout ) return false;

   // Find the starting position of the name below the editor position
   long lStartPos = lPos;
   CString sBlockScope = _FindBlockType(lPos);

   int ch = m_ctrlEdit.GetCharAt(lPos);
   if( _iswhitechar(ch) ) ch = m_ctrlEdit.GetCharAt(--lPos);
   while( _iscppchar(ch) ) ch = m_ctrlEdit.GetCharAt(--lPos);
   int chDelim = ch;
   long lPosDelim = lPos;

   // Try to detect if the is an immediate right-hand-side expression
   if( m_ctrlEdit.GetCharAt(lPos) == '=' ) chDelim = '=';
   if( m_ctrlEdit.GetCharAt(lPos - 1) == '=' ) chDelim = '=';

   CString sName = _GetNearText(lPos + 1);

   CString sParentName;        // Name of parent that contains the member
   CString sParentType;        // Typename of parent
   CString sParentScope;       // Type (scope) of parent
   CString sNamespaceType;     // Typename of namespace

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
   if( chDelim == '=' && dwTimeout > ::GetTickCount() ) 
   {
      // Recurse to get types of members before equal-sign
      CTagDetails BeforeEqual;
      if( _GetMemberInfo(lPosDelim - 3, BeforeEqual, dwTimeout, MATCH_LHS) ) {
         CString sBeforeType = _UndecorateType(BeforeEqual.sDeclaration);
         CSimpleValArray<TAGINFO*> aTypeTag;
         if( m_pCppProject->m_TagManager.FindItem(sBeforeType, NULL, 0, dwTimeout, aTypeTag) ) {
            CTagDetails LeftType;
            m_pCppProject->m_TagManager.GetItemInfo(aTypeTag[0], LeftType);
            if( LeftType.TagType == TAGTYPE_TYPEDEF && LeftType.sDeclaration.Find(_T(" enum ")) > 0 ) {
               sParentType = sBeforeType;
            }
         }
      }
   }

   // There's always a chance that this is a locally defined variable.
   // We can try to catch the definition in the source code block.
   if( sParentType.IsEmpty() && dwTimeout > ::GetTickCount() )
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
      if( aResult.GetSize() == 0 && !sBlockScope.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sParentName, sBlockScope, 99, dwTimeout, aResult);
      if( aResult.GetSize() == 0 ) m_pCppProject->m_TagManager.FindItem(sParentName, _T(""), 0, dwTimeout, aResult);
      bool bFound = false;
      for( int i1 = 0; !bFound && i1 < aResult.GetSize(); i1++ ) {
         switch( aResult[i1]->Type ) {
         case TAGTYPE_NAMESPACE:
            sNamespaceType = sParentName;
            sParentType = sParentScope = sParentName = sBlockScope = _T("");
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
            CTagDetails Parent;
            m_pCppProject->m_TagManager.GetItemInfo(aResult[i2], Parent);
            sParentType = _UndecorateType(Parent.sDeclaration);
            sParentScope = _T("");
            bFound = true;
            break;
         }
      }
   }

   CSimpleValArray<TAGINFO*> aResult;
   if( aResult.GetSize() == 0 && !sParentType.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, sParentType, 99, dwTimeout, aResult);
   if( aResult.GetSize() == 0 && !sBlockScope.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, sBlockScope, 99, dwTimeout, aResult);
   if( sParentName.IsEmpty() ) m_pCppProject->m_TagManager.FindItem(sName, _T(""), 0, dwTimeout, aResult);
   for( int i1 = 0; i1 < aResult.GetSize(); i1++ ) {
      if( chEnd == '(' ) break;
      switch( aResult[i1]->Type ) {
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i1], Info);
         Info.sMemberOfScope = _T("");
         return true;
      }
   }
   // We'll allow a prioritized list.
   // This is the second round.
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
      case TAGTYPE_ENUM:
      case TAGTYPE_DEFINE:
      case TAGTYPE_IMPLEMENTATION:
         m_pCppProject->m_TagManager.GetItemInfo(aResult[i3], Info);
         Info.sMemberOfScope = sParentScope;
         return true;
      }
   }
   // If we found information about the parent but it was not one of the
   // types checked above we might as well give up.
   if( aResult.GetSize() != 0 ) return false;
   // Perhaps we just scraped the type without understanding it
   if( !sParentType.IsEmpty() ) {
      // We have very little to return; but it seems to be a valid
      // type. So let's format our own declaration. Filter out functions
      // and stuff that are complicated.
      if( ch == '(' || ch == ':' || ch == '<' ) return false;
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
   if( Mode == MATCH_ALLOW_PARTIAL && sParentName.IsEmpty() && !sNamespaceType.IsEmpty() ) {
      Info.sName = sName;
      Info.sBase = sBlockScope;
      Info.sMemberOfScope = _T("");
      Info.sNamespace = sNamespaceType;
      Info.TagType = TAGTYPE_MEMBER;
      Info.Protection = TAGPROTECTION_GLOBAL;
      Info.sFilename = _T("");
      Info.iLineNum = -1;
      Info.sDeclaration = _T("");
      Info.sComment = _T("");
      return true;
   }
   if( Mode == MATCH_ALLOW_PARTIAL && sParentName.IsEmpty() && !sBlockScope.IsEmpty() ) {
      Info.sName = sName;
      Info.sBase = sBlockScope;
      Info.sMemberOfScope = _T("");
      Info.sNamespace = _T("");
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

static int QCompareCONTEXTENTRY(const void* a, const void* b)
{
   DWORD t1 = ((CScintillaView::CONTEXTENTRY*)a)->dwTimestamp;
   DWORD t2 = ((CScintillaView::CONTEXTENTRY*)b)->dwTimestamp;
   if( t1 > t2 ) return -1; else if( t1 < t2 ) return 1; else return 0;
}

/**
 * Return a likely candidate from the context-list.
 * The context-list is a list of recently seen types around the editor position.
 */
bool CScintillaView::_GetContextList(long lPos, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aList)
{
   // Get text keyword under the editor position, scan the context-list for matches and
   // return those.
   CString sName = _GetNearText(lPos - 1);
   if( sName.IsEmpty() ) return false;
   // Before we begin, we'd like to know if there are any matches. Since the
   // verification process could take some time, it is wise to do this.
   int i, nMatches = 0, cchName = sName.GetLength();
   for( i = 0; i < MAX_CONTEXT_ENTRIES; i++ ) {
      if( _tcsncmp(sName, m_aContexts[i].sName, cchName) == 0 ) nMatches++;
   }
   if( nMatches == 0 ) return false;
   // Sort the list so we list the most recent matches first
   ::qsort(m_aContexts, MAX_CONTEXT_ENTRIES, sizeof(CONTEXTENTRY), QCompareCONTEXTENTRY);
   // Collect matches...
   for( i = 0; i < MAX_CONTEXT_ENTRIES; i++ ) {
      if( _tcsncmp(sName, m_aContexts[i].sName, cchName) != 0 ) continue;
      if( ::GetTickCount() > dwTimeout ) return false;
      // This is a potential match; look up the actual tag infomation
      CSimpleValArray<TAGINFO*> aResult;
      m_pCppProject->m_TagManager.FindItem(m_aContexts[i].sName, NULL, 0, dwTimeout, aResult);
      if( aResult.GetSize() == 0 ) continue;
      // Add to list
      aList.Add(aResult[0]);
   }
   return true;
}

/**
 * Add a candidate to the context-list.
 * Oldest entries are replaced first. If the entry already exists, we update
 * its timestamp so it becomes the newest entry.
 */
bool CScintillaView::_AddContextEntry(LPCTSTR pstrEntry)
{
   if( pstrEntry == NULL ) return false;
   if( pstrEntry[0] == '\0' ) return false;
   int iLastEntry = 0;
   DWORD dwMaxTimestamp = 0xFFFFFFFF;
   for( int i = 0; i < MAX_CONTEXT_ENTRIES; i++ ) {
      if( m_aContexts[i].sName == pstrEntry ) {
         iLastEntry = i;
         break;
      }
      if( m_aContexts[i].dwTimestamp < dwMaxTimestamp ) {
         dwMaxTimestamp = m_aContexts[i].dwTimestamp;
         iLastEntry = i;
      }
   }
   m_aContexts[iLastEntry].sName = pstrEntry;
   m_aContexts[iLastEntry].dwTimestamp = ::GetTickCount();
   return true;
}

/**
 * Determine scope name.
 * This function locates the class/struct-type relative to
 * the editor position given.
 */
CString CScintillaView::_FindBlockType(long lPos)
{
   // Locate the line where this block begins.
   // BUG: This somehow assumes that the source is formatted as:
   //   void Foo()
   //   {
   //      int test;
   // with all statements indented below the brace. This needs to
   // be done a little smarter.
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
   //    struct xxx
   //    void xxx::Foo()
   //    class xxx : public CFoo
   LPSTR p = NULL;
   long lOffset = 0;
   if( p == NULL ) {
      p = strchr(szBuffer, ':');
      lOffset = -2;
   }
   if( p == NULL ) {
      p = strstr(szBuffer, "class");
      lOffset = strlen("class") + 1;
   }
   if( p == NULL ) {
      p = strstr(szBuffer, "struct");
      lOffset = strlen("struct") + 1;
   }
   // Skip leading return type first:
   //    CFoo::TYPE Bar()
   //    CFoo::TYPE CFoo::Bar()
   if( p != NULL && strncmp(p, "::", 2) == 0 ) {
      LPSTR pstrParen = strchr(p, '(');
      LPSTR pstrSpace = strpbrk(p, " \t");
      if( pstrSpace != NULL && pstrParen != NULL && pstrSpace < pstrParen ) {
         p = pstrSpace;
         lOffset = 1;
      }
   }
   // None of the above? Assume we're inside a function
   // and its formatted like:
   //    TYPE CFoo::Bar()
   if( p == NULL ) {
      p = strpbrk(szBuffer, " \t");
      lOffset = 1;
   }
   if( p == NULL ) return _T("");

   long lLinePos = m_ctrlEdit.PositionFromLine(iStartLine);
   CString sType = _GetNearText(lLinePos + (p - szBuffer) + lOffset);
   if( sType.IsEmpty() ) return _T("");

   // Now, let's find the type in the TAG files
   CSimpleValArray<TAGINFO*> aTags;
   m_pCppProject->m_TagManager.FindItem(sType, NULL, 0, ::GetTickCount() + 100, aTags);
   for( int i = 0; i < aTags.GetSize(); i++ ) {
      switch( aTags[i]->Type ) {
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         return sType;
      case TAGTYPE_NAMESPACE:
         return _T("");
      }
   }

   return _T("");
}

/**
 * Attempt to get the base declaration of a member.
 */
CString CScintillaView::_UndecorateType(CString sType)
{
   // TODO: Less hard-coding; more logic
   // BUG:  Needs to test for word-start before erasing!
   static LPCTSTR ppstrKeywords[] = { 
      _T("const "), 
      _T("auto "), 
      _T("register "), 
      _T("inline "), 
      _T("extern "), 
      _T("static "), 
      _T("virtual "), 
      _T("volatile "), 
      _T("typedef "), 
      _T("struct "), 
      _T("enum "), 
      _T("constexpr "), 
      NULL 
   };
   for( LPCTSTR* pp = ppstrKeywords; *pp != NULL; pp++ ) sType.Replace(*pp, _T(""));
   // Strip trailing stuff
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

   int cchName = sName.GetLength();

   // See if we can find a matching type from the tag information.
   // NOTE: While it would be more correct to process the lines from the bottom-up (in case of variable 
   //       name re-use) , it has been shown to be faster to process them top-down. Also since our
   //       parsing is less than accurate, we have a better chance of determining the type on the
   //       first hit rather than needing to parse over lines where the type is just used but not
   //       declared.
   for( int iLineNum = iStartLine; iLineNum <= iEndLine; iLineNum++ ) {
      CHAR szBuffer[256] = { 0 };
      if( m_ctrlEdit.GetLineLength(iLineNum) >= sizeof(szBuffer) - 1 ) continue;
      m_ctrlEdit.GetLine(iLineNum, szBuffer);

      CString sLine = szBuffer;
      for( int iColPos = sLine.Find(sName); iColPos >= 0; iColPos = sLine.Find(sName, iColPos + 1) ) 
      {
         if( iColPos == 0 ) continue;
         if( iColPos >= sLine.GetLength() - cchName ) continue;

         if( _iscppcharw(sLine[iColPos - 1]) ) continue;
         if( _iscppcharw(sLine[iColPos + cchName]) ) continue;

         // Special case of:
         //    MYTYPE::name
         if( sLine[iColPos + cchName] == ':' && sLine[iColPos + cchName + 1] == ':' ) {
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
         //    foo(b
         TCHAR ch = sLine[iEndPos];
         if( _issepchar(ch) ) continue;

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
         long lLinePos = m_ctrlEdit.PositionFromLine(iLineNum);
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
            _T("auto"),
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
         m_pCppProject->m_TagManager.FindItem(sType, NULL, 0, ::GetTickCount() + 99, aTags);
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
 * Determine if there is an include file at the position.
 */
CString CScintillaView::_FindIncludeUnderCursor(long lPos)
{
   CHAR szBuffer[256] = { 0 };
   int iLineNum = m_ctrlEdit.LineFromPosition(lPos);
   if( m_ctrlEdit.GetLineLength(iLineNum) >= sizeof(szBuffer) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iLineNum, szBuffer);
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
   int iLineNum = m_ctrlEdit.LineFromPosition(lPos);
   long lLinePos = m_ctrlEdit.PositionFromLine(iLineNum);
   CHAR szText[256] = { 0 };
   if( m_ctrlEdit.GetLineLength(iLineNum) >= sizeof(szText) - 1 ) return _T("");
   m_ctrlEdit.GetLine(iLineNum, szText);
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
   case SCE_C_PREPROCESSOR:
   case SCE_C_COMMENTLINEDOC:
   case SCE_C_COMMENTDOCKEYWORD:
   case SCE_C_COMMENTDOCKEYWORDERROR:
      return false;
   case SCE_C_UUID:
   case SCE_C_REGEX:
   case SCE_C_NUMBER:
      return bIncludeNonIdentifiers;
   default:
      return true;
   }
}

