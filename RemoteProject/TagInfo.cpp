
#include "StdAfx.h"
#include "resource.h"

#include "TagInfo.h"
#include "Project.h"

#pragma code_seg( "MISC" )


// Constructor / destructor

CTagInfo::CTagInfo() :
   m_bLoaded(false),
   m_bClassBrowserDone(false)
{
}

CTagInfo::~CTagInfo()
{
   Clear();
}

// Operations

void CTagInfo::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CTagInfo::MergeFile(LPCTSTR /*pstrFilename*/)
{
   // TODO: Currently we just reload all the tag files again upon the
   //       file merge event. I assume that it would be better to maintain
   //       optimal sorting when dealing with multiple ctag files, but this 
   //       was never implemented.
   return _LoadTags();
}

void CTagInfo::Clear()
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) free(m_aFiles[i]);
   m_aFiles.RemoveAll();
   m_bLoaded = false;
   m_bSorted = false;
}

bool CTagInfo::IsLoaded() const
{
   return m_bLoaded;
}

bool CTagInfo::IsAvailable() const
{
   ATLASSERT(m_pProject);
   for( int i = 0; i < m_pProject->GetItemCount(); i++ ) {
      IView* pView = m_pProject->GetItem(i);
      CString sName;
      pView->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      sName.MakeUpper();
      if( sName.Find(_T("TAGS")) >= 0 ) return true;
   }
   return false;
}

int CTagInfo::FindItem(int iStart, LPCTSTR pstrName)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));

   if( !m_bLoaded ) _LoadTags();
   if( m_aTags.GetSize() == 0 ) return -1;

   ATLASSERT(iStart>=0);
   ATLASSERT(iStart<m_aTags.GetSize());

   if( m_bSorted && iStart == 0 )
   {
       // Binary search on sorted list
       int min = 0;
       int max = m_aTags.GetSize();
       int n = max / 2; 
       while( min < max ) { 
           int cmp = _tcscmp(pstrName, m_aTags[n].pstrName); 
           if( cmp == 0 ) break;
           if( cmp < 0 ) max = n; else min = n + 1;
           n = (min + max) / 2;
       }
       // Find first instance of the particular string
       if( min >= max ) return -1;
       if( _tcscmp(pstrName, m_aTags[n].pstrName) != 0 ) return -1;
       while( n > 0 && _tcscmp(pstrName, m_aTags[n - 1].pstrName) == 0 ) n--;
       return n;
   }
   else
   {
      // Quick test when list is sorted.
      // This is an optimization that allows us to terminate the search
      // quickly if we know the file was sorted.
      if( m_bSorted && _tcscmp(m_aTags[iStart].pstrName, pstrName) > 0 ) return -1;
      // Scan list sequentially
      int nCount = m_aTags.GetSize();
      for( int iIndex = iStart; iIndex < nCount; iIndex++ ) {
         if( _tcscmp(m_aTags[iIndex].pstrName, pstrName) == 0 ) return iIndex;
      }
   }
   return -1;
}

bool CTagInfo::GetItemInfo(LPCTSTR pstrName, LPCTSTR pstrOwner, DWORD dwInfoType, CSimpleArray<CString>& aResult)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   
   if( !m_bLoaded ) _LoadTags();
   if( m_aTags.GetSize() == 0 ) return false;

   // If an 'owner' is supplied, we look for a member within a specific
   // class/struct. The 'owner' searchstring must then be found in the TAG extension
   // fields of the member.
   CString sTypeTag;
   CString sTypeParent;
   if( pstrOwner != NULL ) {
      bool bFound = false;
      int iIndex = FindItem(0, pstrOwner);
      while( !bFound ) {
         if( iIndex < 0 ) return false;
         const TAGINFO& info = m_aTags[iIndex];
         switch( info.Type ) {
         case TAGTYPE_CLASS:
            sTypeTag.Format(_T("class:%s"), pstrOwner);
            sTypeParent.Format(_T("class:%s"), _GetTagParent(info));
            bFound = true;
            break;
         case TAGTYPE_STRUCT:
         case TAGTYPE_TYPEDEF:
            sTypeTag.Format(_T("struct:%s"), pstrOwner);
            bFound = true;
            break;
         default:
            iIndex = FindItem(iIndex + 1, pstrOwner);
         }
      }
   }

   // Now, let's look up information about the member
   CString sResult;
   int iIndex = FindItem(0, pstrName);
   while( iIndex >= 0 ) {
      const TAGINFO& info = m_aTags[iIndex];
      bool bAccept = false;
      switch( info.Type ) {
      case TAGTYPE_ENUM:
      case TAGTYPE_CLASS:
      case TAGTYPE_MEMBER:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
      case TAGTYPE_FUNCTION:
         bAccept = true;
         // Could be a linenumber
         if( _ttoi(info.pstrToken) > 0 ) bAccept = false;
         // Don't want to see items with comments and stuff
         if( bAccept && _tcsstr(info.pstrToken, _T("/*")) != NULL ) bAccept = false;
         if( bAccept && _tcsstr(info.pstrToken, _T("//")) != NULL ) bAccept = false;
         // Look at the owner?
         if( bAccept && !sTypeTag.IsEmpty() ) {
            bool bFound = false;
            for( short i = 0; i < info.nFields; i++ ) {
               if( sTypeTag == info.pstrFields[i] ) {
                  bFound = true;
                  break;
               }
               if( sTypeParent == info.pstrFields[i] ) {
                  bFound = true;
                  break;
               }
            }
            if( !bFound ) bAccept = false;
         }
      }
      if( bAccept ) {
         // Create information.
         // Default is the stripped version of the search-expression
         sResult = _T("");
         if( (dwInfoType & TAGINFO_NAME) != 0 ) {
            if( !sResult.IsEmpty() ) sResult += '|';
            sResult += info.pstrName;
         }
         if( (dwInfoType & TAGINFO_TYPE) != 0 ) {
            if( !sResult.IsEmpty() ) sResult += '|';
            sResult.Append((int)info.Type);
         }
         if( (dwInfoType & TAGINFO_DECLARATION) != 0 ) {
            if( !sResult.IsEmpty() ) sResult += '|';
            CString sValue = info.pstrToken;
            sValue.TrimLeft(_T("/$~^* \t.,;"));
            sValue.TrimRight(_T("/$~^* \t.,;"));
            sValue.Replace(_T("\\/"), _T("/"));
            // We might also have a complete tag signature.
            // NOTE: Crappy CTAG doesn't include return value type!
            for( short i = 0; i < info.nFields; i++ ) {
               if( _tcsncmp(info.pstrFields[i], _T("signature:"), 10) == 0 ) {
                  sValue = CString(pstrName) + CString(info.pstrFields[i]).Mid(10);
                  break;
               }
            }
            sResult += sValue;
         }
         if( (dwInfoType & TAGINFO_COMMENT) != 0 ) {
            sResult += '|';
         }
         aResult.Add(sResult);
      }
      iIndex = FindItem(iIndex + 1, pstrName);
   }

   return aResult.GetSize() > 0;
}

TAGTYPE CTagInfo::GetItemType(int iIndex)
{
   ATLASSERT(m_aTags.GetSize()>0);
   return m_aTags[iIndex].Type;
}

bool CTagInfo::GetOuterList(CSimpleValArray<TAGINFO*>& aList)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aTags.GetSize() == 0 ) return false;

   // List all classes/structs available in TAG file...
   int nCount = m_aTags.GetSize();
   for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
      TAGINFO& info = m_aTags[iIndex];
      switch( info.Type ) {
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         TAGINFO* pTag = &m_aTags[iIndex];
         aList.Add(pTag);
         break;
      }
   }

   return aList.GetSize() > 0;
}

bool CTagInfo::GetGlobalList(CSimpleValArray<TAGINFO*>& aList)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aTags.GetSize() == 0 ) return false;

   // List all classes/structs available in TAG file...
   int nCount = m_aTags.GetSize();
   for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
      TAGINFO& info = m_aTags[iIndex];
      switch( info.Type ) {
      case TAGTYPE_DEFINE:
      case TAGTYPE_FUNCTION:
         TAGINFO* pTag = &m_aTags[iIndex];
         aList.Add(pTag);
      }
   }

   return aList.GetSize() > 0;
}

bool CTagInfo::GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aTags.GetSize() == 0 ) return false;

   // First make sure we can find the type (class or struct)
   CString sTypeTag;
   int iTypeIndex = FindItem(0, pstrType);

   bool bFound = false;
   while( !bFound ) {
      // Oops, didn't find the type! Bail out...
      if( iTypeIndex < 0 ) return false;  

      // Construct our search string for the extension fields
      const TAGINFO& info = m_aTags[iTypeIndex];
      switch( info.Type ) {
      case TAGTYPE_CLASS:
         sTypeTag.Format(_T("class:%s"), pstrType);
         bFound = true;
         break;
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
         sTypeTag.Format(_T("struct:%s"), pstrType);
         bFound = true;
         break;
      default:
         // Not a class/struct! Let's look at the next matching item
         iTypeIndex = FindItem(iTypeIndex + 1, pstrType);
         break;
      }
   }

   // Now look up the members
   // OPTI: We'll have to look at all entries in the CTAG file here.
   //       To otherwise optimize this we could 'link' the entries at load-time, 
   //       which would be a slow/nasty process...
   int nCount = m_aTags.GetSize();
   for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
      // Not a lot of information in the CTAG entries, but      
      // we might be able to extract something...
      const TAGINFO& info = m_aTags[iIndex];
      for( short i = 0; i < info.nFields; i++ ) {
         if( sTypeTag == info.pstrFields[i] ) {
            CString sName = info.pstrName;
            // Filter names (excludes "operator +=" etc etc)
            if( sName.IsEmpty() ) continue;
            if( !_istalpha(sName[0]) && sName[0] != '_' ) continue;
            if( sName.FindOneOf(_T(" <>+-")) > 0 ) continue;
            // Only add unique entries
            bool bDuplicate = false;
            for( int j = 0; !bDuplicate && j < aList.GetSize() && !bDuplicate; j++ ) bDuplicate = ( sName == aList[j]->pstrName );
            if( bDuplicate ) continue;
            // Add it
            TAGINFO* pTag = &m_aTags[iIndex];
            aList.Add(pTag);
         }
      }
   }

   // Scan parent class as well?
   if( bInheritance ) {
      const TAGINFO& info = m_aTags[iTypeIndex];      
      // Append structures from parent type
      // NOTE: We'll not risk an endless loop in CTAG scanning by allowing
      //       nesting into more than the parent type; thus the 'false' argument.
      CString sParent = _GetTagParent(info);
      if( !sParent.IsEmpty() ) GetMemberList(sParent, aList, false);
   }

   return aList.GetSize() > 0;
}

// Implementation

CString CTagInfo::_GetTagParent(const TAGINFO& info) const
{
   // Extract inheritance type.
   // HACK: We simply scoop up the " class CFoo : public CBar" text from
   //       the CTAG line. Unfortunately CTAG doesn't really carry that
   //       much information to safely determine the inheritance tree!
   static LPCTSTR pstrTokens[] = 
   {
      _T("public"),
      _T("typedef"),
      NULL
   };
   LPCTSTR* ppstrToken = pstrTokens;
   while( *ppstrToken ) {
      LPCTSTR p = _tcsstr(info.pstrToken, *ppstrToken);
      if( p ) {
         p += _tcslen(*ppstrToken);
         while( _istspace(*p) ) p++;
         CString sName;
         while( _istalnum(*p) || *p == '_' ) sName += *p++;
         return sName;      
      }
      ppstrToken++;
   }
   return _T("");
}

bool CTagInfo::_LoadTags()
{
   ATLASSERT(m_aFiles.GetSize()==0);

   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_LOADTAG)));

   Clear();

   // Mark as "loaded" even if we don't actually find any
   // tag files to load. This prevent repeated attempts to
   // load the files in the future.
   m_bLoaded = true;

   // Look for all files in the project that contains the name "TAGS"...
   int i;
   for( i = 0; i < m_pProject->GetItemCount(); i++ ) {
      IView* pView = m_pProject->GetItem(i);
      CString sName;
      pView->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      sName.MakeUpper();
      if( sName.Find(_T("TAGS")) >= 0 ) 
      {
         // Load the file content. This gets us an UNICODE version
         // which is just what we need...
         CComBSTR bstrText;
         if( !pView->GetText(&bstrText) ) return false;
         if( bstrText.Length() == 0 ) continue;
         // Done. Now parse it...
         LPTSTR pstrText = (LPTSTR) malloc( (bstrText.Length() + 1) * sizeof(TCHAR) );
         if( pstrText == NULL ) return false;
         _tcscpy(pstrText, bstrText);
         if( _ParseTagFile(pstrText) ) {
            // File parsed successfully! Notice that we
            // own the memory because we only store pointers
            // into the file memory.
            m_aFiles.Add(pstrText);
         }
         else {
            free(pstrText);
         }
      }
   }

   // Check if list is sorted.
   // NOTE: When multiple files are encountered we cannot rely
   //       on the list being sorted because we don't merge the files.
   //       This will significantly slow down the processing!!
   // BUG: This is a lame attempt to detect if the list is sorted.
   // TODO: Base this on CTAG meta/header-information instead.
   m_bSorted = m_aFiles.GetSize() == 1;
   for( i = 0; m_bSorted && i < m_aTags.GetSize() - 51; i += 50 ) {
      if( _tcscmp(m_aTags[i].pstrName, m_aTags[i + 50].pstrName) > 0 ) m_bSorted = false;
   }

   return true;
}

bool CTagInfo::_ParseTagFile(LPTSTR pstrText)
{
   // Validate that this really is a tag file
   if( _tcsncmp(pstrText, _T("!_TAG"), 5) != 0 ) return false;

   // Parse all lines...
   LPTSTR pstrNext = NULL;
   for( ; *pstrText; pstrText = pstrNext ) {
      // First find the EOL marker
      pstrNext = _tcschr(pstrText, '\n');
      if( pstrNext == NULL ) pstrNext = pstrText + _tcslen(pstrText) - 1;
      *pstrNext = '\0';
      if( *(pstrNext - 1) == '\r' ) *(pstrNext - 1) = '\0';
      pstrNext++;

      // Ignore comments
      if( *pstrText == '!' ) continue;

      // Parse tag structure
      TAGINFO info = { TAGTYPE_UNKNOWN, 0 };

      LPTSTR p = _tcschr(pstrText, '\t');
      ATLASSERT(p);
      if( p == NULL ) continue;
      *p = '\0';
      info.pstrName = pstrText;

      pstrText = p + 1;
      p = _tcschr(pstrText, '\t');
      ATLASSERT(p);
      if( p == NULL ) continue;
      *p = '\0';
      info.pstrFile = pstrText;

      pstrText = p + 1;
      p = _tcschr(pstrText, '\t');
      ATLASSERT(p);
      if( p == NULL ) continue;
      *p = '\0';
      if( *(p - 2) == ';' ) *(p - 2) = '\0';
      info.pstrToken = pstrText;

      // Parse the optional properties
      pstrText = p + 1;
      while( *pstrText ) {
         p = _tcschr(pstrText, '\t');
         if( p ) *p = '\0';

         info.pstrFields[info.nFields] = pstrText;
         info.nFields++;

         if( p == NULL ) break;
         if( info.nFields >= sizeof(info.pstrFields) / sizeof(LPCTSTR) ) break;
         pstrText = p + 1;
      }

      // Look up type information at once
      info.Type = _GetTagType(info);
      info.lLineNum = -1;

      m_aTags.Add(info);
   }
   return true;
}

TAGTYPE CTagInfo::_GetTagType(const TAGINFO& info) const
{
   for( short i = 0; i < info.nFields; i++ ) {
      const LPCTSTR& pstrField = info.pstrFields[i];
      if( pstrField[0] != '\0' &&  pstrField[1] == '\0' ) {
         if( _tcscmp(pstrField, _T("e")) == 0 ) return TAGTYPE_ENUM;
         if( _tcscmp(pstrField, _T("c")) == 0 ) return TAGTYPE_CLASS;
         if( _tcscmp(pstrField, _T("m")) == 0 ) return TAGTYPE_MEMBER;
         if( _tcscmp(pstrField, _T("d")) == 0 ) return TAGTYPE_DEFINE;
         if( _tcscmp(pstrField, _T("s")) == 0 ) return TAGTYPE_STRUCT;
         if( _tcscmp(pstrField, _T("t")) == 0 ) return TAGTYPE_TYPEDEF;
         if( _tcscmp(pstrField, _T("f")) == 0 ) return TAGTYPE_FUNCTION;
      }
      //
      if( _tcscmp(pstrField, _T("class")) == 0 ) return TAGTYPE_CLASS;
      if( _tcscmp(pstrField, _T("macro")) == 0 ) return TAGTYPE_DEFINE;
      if( _tcscmp(pstrField, _T("member")) == 0 ) return TAGTYPE_MEMBER;
      if( _tcscmp(pstrField, _T("struct")) == 0 ) return TAGTYPE_STRUCT;
      if( _tcscmp(pstrField, _T("typedef")) == 0 ) return TAGTYPE_TYPEDEF;
      if( _tcscmp(pstrField, _T("enumerator")) == 0 ) return TAGTYPE_ENUM;
      if( _tcscmp(pstrField, _T("function")) == 0 ) return TAGTYPE_FUNCTION;
      //
      if( _tcsncmp(pstrField, _T("kind:"), 5) == 0 ) {
         if( _tcscmp(pstrField, _T("kind:class")) == 0 ) return TAGTYPE_CLASS;
         if( _tcscmp(pstrField, _T("kind:macro")) == 0 ) return TAGTYPE_DEFINE;
         if( _tcscmp(pstrField, _T("kind:member")) == 0 ) return TAGTYPE_MEMBER;
         if( _tcscmp(pstrField, _T("kind:struct")) == 0 ) return TAGTYPE_STRUCT;
         if( _tcscmp(pstrField, _T("kind:typedef")) == 0 ) return TAGTYPE_TYPEDEF;
         if( _tcscmp(pstrField, _T("kind:enumerator")) == 0 ) return TAGTYPE_ENUM;
         if( _tcscmp(pstrField, _T("kind:function")) == 0 ) return TAGTYPE_FUNCTION;
      }
   }
   return TAGTYPE_UNKNOWN;
}

