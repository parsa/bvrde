
#include "StdAfx.h"
#include "resource.h"

#include "LexInfo.h"

#include "Project.h"
#include "ClassView.h"

#pragma code_seg( "MISC" )


/////////////////////////////////////////////////////////////////////////////
// CLexInfo

CLexInfo::CLexInfo() : m_bLoaded(false), m_iFile(-1)
{
}

CLexInfo::~CLexInfo()
{
   Clear();
}

void CLexInfo::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CLexInfo::IsLoaded() const
{
   return m_aFiles.GetSize() > 0;
}

bool CLexInfo::IsAvailable() const
{
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.cpp.onlineScanner"), szBuffer, 31);
   return _tcscmp(szBuffer, _T("true")) == 0;
}

bool CLexInfo::MergeFile(LPCTSTR pstrFilename, LPCSTR pstrText)
{
   ATLASSERT(m_pProject);

   CString sName = ::PathFindFileName(pstrFilename);

   // Load the CppLexer module
   CString sModule;
   sModule.Format(_T("%sCppLexer.dll"), CModulePath());
   static HINSTANCE s_hInst = NULL;
   if( s_hInst == NULL ) s_hInst = ::LoadLibrary(sModule);
   if( s_hInst == NULL ) return false;

   // Lex the file.
   // It's not unlikely that the parse fails since it should stop at
   // normals syntax errors and less obvious LALR failures! In
   // this case we just ignore the contents...
   typedef BOOL (CALLBACK* LPFNPARSE)(LPCWSTR, LPCSTR);
   LPFNPARSE pParse = (LPFNPARSE) ::GetProcAddress(s_hInst, "CppLexer_Parse");
   ATLASSERT(pParse);
   BOOL bRes = FALSE;
   ATLTRY( bRes = pParse(pstrFilename, pstrText) );
   if( !bRes ) return false;

   // Get the new file into structured form
   LEXFILE* pFile = new LEXFILE;
   if( !_ParseFile(sName, *pFile) ) {
      delete pFile;
      return false;
   }

   m_pProject->GetClassView()->Clear();

   // Replace the old entry in the file list
   bool bFound = false;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      if( m_aFiles[i]->sFilename == sName ) {
         free(m_aFiles[i]->pData);
         m_aFiles[i] = pFile;
         bFound = true;
         break;
      }
   }
   // ...but this is a new file
   if( !bFound ) m_aFiles.Add(pFile);

   // Signal the tree to rebuild itself!
   // This important since we're removed some of the TAGINFO pointers
   // from the list and they're referenced from this control.
   m_pProject->GetClassView()->Rebuild();

   return true;
}

void CLexInfo::Clear()
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) free(m_aFiles[i]->pData);
   m_aFiles.RemoveAll();
}

TAGTYPE CLexInfo::GetItemType(int iIndex)
{
   ATLASSERT(m_iFile>=0);
   ATLASSERT(m_aFiles.GetSize()>0);
   return m_aFiles[m_iFile]->aTags[iIndex].Type;
}

int CLexInfo::FindItem(int iStart, LPCTSTR pstrName)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));

   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return -1;

   ATLASSERT(iStart>=0);

   if( iStart == 0 )
   {
      m_iFile = -1;
      for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
         const LEXFILE& file = *m_aFiles[i];
         // Binary search on sorted list
         int min = 0;
         int max = file.aTags.GetSize();
         int n = max / 2; 
         while( min < max ) { 
            int cmp = _tcscmp(pstrName, file.aTags[n].pstrName); 
            if( cmp == 0 ) break;
            if( cmp < 0 ) max = n; else min = n + 1;
            n = (min + max) / 2;
         }
         // Find first instance of the particular string
         if( min >= max ) continue;
         if( _tcscmp(pstrName, file.aTags[n].pstrName) != 0 ) continue;
         while( n > 0 && _tcscmp(pstrName, file.aTags[n - 1].pstrName) == 0 ) n--;
         m_iFile = i;
         return n;
      }
   }
   else
   {
      ATLASSERT(m_iFile>=0 && m_iFile<m_aFiles.GetSize());
      // Scan rest of list sequentially
      const LEXFILE& file = *m_aFiles[m_iFile];
      int nCount = file.aTags.GetSize();
      for( int iIndex = iStart; iIndex < nCount; iIndex++ ) {
         int cmp = _tcscmp(file.aTags[iIndex].pstrName, pstrName);
         if( cmp == 0 ) return iIndex;
         if( cmp > 0 ) return -1;
      }
   }
   return -1;
}

CString CLexInfo::GetItemDeclaration(LPCTSTR pstrName, LPCTSTR pstrOwner /*= NULL*/)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return _T("");

   // Now, let's look up information about the member
   CString sResult;
   int iIndex = FindItem(0, pstrName);
   while( iIndex >= 0 ) {
      const TAGINFO& info = m_aFiles[m_iFile]->aTags[iIndex];
      switch( info.Type ) {
      case TAGTYPE_ENUM:
      case TAGTYPE_CLASS:
      case TAGTYPE_STRUCT:
      case TAGTYPE_TYPEDEF:
      case TAGTYPE_FUNCTION:
         // Owner has to match...
         if( pstrOwner != NULL && _tcscmp(pstrOwner, info.pstrFields[0]) != 0 ) break;
         // Did we already find an entry? Cannot handle duplicates!
         if( !sResult.IsEmpty() ) return _T("");
         // So this is our function signature...
         sResult = info.pstrToken;
      }
      iIndex = FindItem(iIndex + 1, pstrName);
   }

   return sResult;
}

bool CLexInfo::GetOuterList(CSimpleValArray<TAGINFO*>& aList)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   // List all classes available in TAG file...
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         TAGINFO& info = file.aTags[iIndex];
         if( info.pstrFile == NULL ) continue;
         switch( info.Type ) {
         case TAGTYPE_CLASS:
         case TAGTYPE_TYPEDEF:
         case TAGTYPE_STRUCT:
            TAGINFO* pTag = &m_aFiles[i]->aTags.m_aT[iIndex];
            aList.Add(pTag);
            break;
         }
      }
   }

   return aList.GetSize() > 0;
}

bool CLexInfo::GetGlobalList(CSimpleValArray<TAGINFO*>& aList)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   // List all globals available in TAG file...
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         TAGINFO& info = file.aTags[iIndex];
         // Must not be parent of something
         if( info.pstrFields[0][0] != '\0' ) continue;
         if( info.pstrFile == NULL ) continue;
         // Functions and variables are accepted
         switch( info.Type ) {
         case TAGTYPE_MEMBER:
         case TAGTYPE_FUNCTION:
            TAGINFO* pTag = &m_aFiles[i]->aTags.m_aT[iIndex];
            aList.Add(pTag);
         }
      }
   }

   return aList.GetSize() > 0;
}

bool CLexInfo::GetMemberList(LPCTSTR pstrType, CSimpleValArray<TAGINFO*>& aList, bool bInheritance)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   // List all classes available in TAG file...
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         TAGINFO& info = file.aTags[iIndex];
         if( _tcscmp(info.pstrFields[0], pstrType) == 0 ) {
            TAGINFO* pTag = &m_aFiles[i]->aTags.m_aT[iIndex];
            aList.Add(pTag);
         }
      }
   }

   // Scan parent class as well?
   if( bInheritance ) {
      int iTypeIndex = FindItem(0, pstrType);
      if( iTypeIndex >= 0 ) {
         const TAGINFO& info = m_aFiles[m_iFile]->aTags[iTypeIndex];
         CString sParent = _GetTagParent(info);
         if( !sParent.IsEmpty() ) GetMemberList(sParent, aList, false);
      }
   }

   return aList.GetSize() > 0;
}

// Implementation

CString CLexInfo::_GetTagParent(const TAGINFO& info) const
{
   // Extract inheritance type.
   // HACK: We simply scoop up the "class CFoo : public CBar" text from
   //       the decl. line. Unfortunately CTAG doesn't really carry that
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

void CLexInfo::_LoadTags()
{
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_LOADTAG)));

   Clear();

   // Mark as "loaded" even if we don't actually find any
   // lex files to load. This prevent repeated attempts to
   // load the files in the future.
   m_bLoaded = true;

   // Don't load if configuration is turned off
   if( !IsAvailable() ) return;

   // Look for all project's lex files
   for( int i = 0; i < m_pProject->GetItemCount(); i++ ) {
      IView* pView = m_pProject->GetItem(i);
      CString sName;
      pView->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      LEXFILE* pFile = new LEXFILE;
      if( _ParseFile(sName, *pFile) ) m_aFiles.Add(pFile); else delete pFile;
   }

   // Look for the global lex file (contains standard/system functions)
   if( m_aFiles.GetSize() > 0 ) {
      LEXFILE* pFile = new LEXFILE;
      CString sName = _T("common.lex");
      if( _ParseFile(sName, *pFile) ) m_aFiles.Add(pFile); else delete pFile;
   }
}

bool CLexInfo::_ParseFile(CString& sName, LEXFILE& file) const
{
   CString sFilename = ::PathFindFileName(sName);
   CString sLexName = sFilename;
   sLexName.Replace('.', '_');
   CString sLexFile;
   sLexFile.Format(_T("%sLex\\%s.lex"), CModulePath(), sLexName);

   CFile f;
   if( !f.Open(sLexFile) ) return false;
   DWORD dwSize = f.GetSize();
   LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
   f.Read(pstrData, dwSize);
   f.Close();
   pstrData[dwSize] = '\0';

   LPTSTR p = (LPTSTR) malloc((dwSize + 1) * sizeof(TCHAR));
   AtlA2WHelper(p, pstrData, dwSize);
   free(pstrData);

   file.pData = p;
   file.sFilename = sName;

   LPCTSTR pstrFilename = NULL;

   while( *p ) {
      TAGINFO info = { TAGTYPE_UNKNOWN, 0 };
      if( *p == '#' ) {
         pstrFilename = p + 1;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p = '\0';
         p++;
      }
      else {
         info.pstrName = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         if( *p == '\0' ) break;
         switch( *p++ ) {
         case 'c': info.Type = TAGTYPE_CLASS; break;
         case 'm': info.Type = TAGTYPE_FUNCTION; break;
         case 'v': info.Type = TAGTYPE_MEMBER; break;
         case 'd': info.Type = TAGTYPE_DEFINE; break;
         case 't': info.Type = TAGTYPE_TYPEDEF; break;
         case 's': info.Type = TAGTYPE_STRUCT; break;
         case 'e': info.Type = TAGTYPE_ENUM; break;
         default: info.Type = TAGTYPE_UNKNOWN;
         }
         p++;  // Skip protection
         p++;
         //
         info.pstrToken = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrFields[0] = p;
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.iLineNo = _ttol(p);
         p = _tcschr(p, '|');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrFields[1] = p;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrFile = pstrFilename;
         info.nFields = 2;
         file.aTags.Add(info);
      }
   }

   if( file.aTags.GetSize() == 0 ) {
      free(file.pData);
      return false;
   }

   return true;
}

