
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
   
   USES_CONVERSION;

   // Load the CppLexer module
   CString sModule;
   sModule.Format(_T("%sCppLexer.dll"), CModulePath());
   static HINSTANCE s_hInst = NULL;
   if( s_hInst == NULL ) s_hInst = ::LoadLibrary(sModule);
   if( s_hInst == NULL ) return false;

   // FIX: Empty files do appear as NULL text.
   if( pstrText == NULL ) return false;

   CString sLexFilename = _GetLexFilename(pstrFilename, true);
   if( sLexFilename.IsEmpty() ) return false;

   // Lex the text and write to the file.
   // It's not unlikely that the parse fails since it could stop at
   // normal syntax errors and less obvious LALR failures! In
   // this case we'll just ignore the content...
   typedef BOOL (APIENTRY* LPFNPARSE)(LPCSTR,LPCSTR,LPCWSTR);
   LPFNPARSE fnParse = (LPFNPARSE) ::GetProcAddress(s_hInst, "CppLexer_Parse");
   ATLASSERT(fnParse);
   if( fnParse == NULL ) return false;
   BOOL bRes = FALSE;
   ATLTRY( bRes = fnParse(T2CA(pstrFilename), pstrText, sLexFilename) );
   if( !bRes ) return false;

   // Get the new file into structured form
   LEXFILE* pFile = new LEXFILE;
   if( !_ParseFile(pstrFilename, *pFile) ) {
      delete pFile;
      return false;
   }

   // Since we're changing the parse data, and the ClassView references
   // this data, we must *lock* it down so it doesn't use the old pointers...
   m_pProject->GetClassView()->Lock();

   // Replace the old entry in the file list
   bool bFound = false;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      if( m_aFiles[i]->sFilename == pstrFilename ) {
         free(m_aFiles[i]->pData);
         m_aFiles[i] = pFile;
         bFound = true;
         break;
      }
   }
   // ...but this is a new file
   if( !bFound ) m_aFiles.Add(pFile);

   // Signal the tree to rebuild itself!
   // This is important since we've removed some of the TAGINFO pointers
   // from the list and need to unlock the data.
   m_pProject->GetClassView()->Rebuild();
   m_pProject->GetClassView()->Unlock();

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
      // Scan rest of file sequentially
      const LEXFILE& file = *m_aFiles[m_iFile];
      int nCount = file.aTags.GetSize();
      for( int iIndex = iStart; iIndex < nCount; iIndex++ ) {
         int cmp = _tcscmp(file.aTags[iIndex].pstrName, pstrName);
         if( cmp == 0 ) return iIndex;
         if( cmp > 0 ) break;  // The file is sorted, so it's OK to give up
      }
      // Scan the other files too
      for( int i = m_iFile + 1; i < m_aFiles.GetSize(); i++ ) {
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
   return -1;
}

bool CLexInfo::GetItemInfo(LPCTSTR pstrName, LPCTSTR pstrOwner, DWORD dwInfoType, CSimpleArray<CString>& aResult)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   // Now, let's look up information about the member...
   CString sResult;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      // Binary search on sorted list
      int nCount = file.aTags.GetSize();
      int min = 0;
      int max = nCount;
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
      // Scan this file for occurances
      while( true ) {
         const TAGINFO& info = m_aFiles[i]->aTags[n];
         switch( info.Type ) {
         case TAGTYPE_ENUM:
         case TAGTYPE_CLASS:
         case TAGTYPE_STRUCT:
         case TAGTYPE_MEMBER:
         case TAGTYPE_TYPEDEF:
         case TAGTYPE_FUNCTION:
            // Owner has to match...
            if( pstrOwner != NULL && _tcscmp(pstrOwner, info.pstrFields[0]) != 0 ) break;
            // So this is our function signature.
            // Yearh I know... it's a bit crappy to return the result in a |-separated string,
            // but apparently there was some problems with wrapping a structure inside an ATL array.
            // TODO: We really should look at this once again...
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
               sResult += info.pstrToken;
            }
            if( (dwInfoType & TAGINFO_COMMENT) != 0 ) {
               if( !sResult.IsEmpty() ) sResult += '|';
               if( info.nFields > 1 ) sResult += info.pstrFields[1];
            }
            if( (dwInfoType & TAGINFO_FILENAME) != 0 ) {
               if( !sResult.IsEmpty() ) sResult += '|';
               if( info.nFields > 1 ) sResult += info.pstrFile;
            }
            if( (dwInfoType & TAGINFO_LINENO) != 0 ) {
               if( !sResult.IsEmpty() ) sResult += '|';
               sResult.Append((int)info.lLineNum);
            }
            aResult.Add(sResult);
         }
         for( n++; n < nCount; n++ ) {
            int cmp = _tcscmp(file.aTags[n].pstrName, pstrName);
            if( cmp == 0 ) break;
            if( cmp > 0 ) n = nCount;
         }
         if( n >= nCount ) break;
      }
   }

   return aResult.GetSize() > 0;
}

bool CLexInfo::GetOuterList(CSimpleValArray<TAGINFO*>& aList)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   // List all classes available in TAG file...
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      if( !file.bIncludeInBrowser ) continue;
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = file.aTags[iIndex];
         if( info.pstrFile == NULL ) continue;
         switch( info.Type ) {
         case TAGTYPE_CLASS:
         case TAGTYPE_STRUCT:
         case TAGTYPE_TYPEDEF:
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
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      if( !file.bIncludeInBrowser ) continue;
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = file.aTags[iIndex];
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
   // NOTE: This is doing a full filescan!
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      int nCount = file.aTags.GetSize();
      for( int iIndex = 0; iIndex < nCount; iIndex++ ) {
         const TAGINFO& info = file.aTags[iIndex];
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

bool CLexInfo::FindImplementation(LPCTSTR pstrName, LPCTSTR pstrOwner, CString& sFilename, long& lLineNum)
{
   if( !m_bLoaded ) _LoadTags();
   if( m_aFiles.GetSize() == 0 ) return false;

   CString sName;
   sName.Format(_T("%s::%s"), pstrOwner, pstrName);

   // Now, let's look up information about the member...
   CString sResult;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      const LEXFILE& file = *m_aFiles[i];
      // Binary search on sorted list
      int nCount = file.aTags.GetSize();
      int min = 0;
      int max = nCount;
      int n = max / 2; 
      while( min < max ) { 
         int cmp = _tcscmp(sName, file.aTags[n].pstrName); 
         if( cmp == 0 ) break;
         if( cmp < 0 ) max = n; else min = n + 1;
         n = (min + max) / 2;
      }
      // Find first instance of the particular string
      if( min >= max ) continue;
      if( _tcscmp(sName, file.aTags[n].pstrName) != 0 ) continue;
      while( n > 0 && _tcscmp(sName, file.aTags[n - 1].pstrName) == 0 ) n--;
      // Scan this file for occurances
      while( true ) {
         const TAGINFO& info = m_aFiles[i]->aTags[n];
         switch( info.Type ) {
         case TAGTYPE_IMPLEMENTATION:
            sFilename = info.pstrFile;
            lLineNum = info.lLineNum;
            return true;
         }
         for( n++; n < nCount; n++ ) {
            int cmp = _tcscmp(file.aTags[n].pstrName, sName);
            if( cmp == 0 ) break;
            if( cmp > 0 ) n = nCount;
         }
         if( n >= nCount ) break;
      }
   }

   return false;
}

// Implementation

CString CLexInfo::_GetTagParent(const TAGINFO& info) const
{
   // Extract inheritance type.
   // HACK: We simply scoop up the "class CFoo : public CBar" text from
   //       the decl. line. Unfortunately the lex file doesn't really carry that
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
   // NOTE: Now what is GUI stuff doing here? Well, we're delay-loading much of this
   //       stuff so we can't really preditct when the tag files will load.
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
      CString sFileName;
      pView->GetFileName(sFileName.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sFileName.ReleaseBuffer();
      LEXFILE* pFile = new LEXFILE;
      if( _ParseFile(sFileName, *pFile) ) m_aFiles.Add(pFile); else delete pFile;
   }

   // Look for the global lex file (contains standard/system functions).
   // We're dealing specially with the "CommonLex" filename as it will contain
   // a great deal of precompiler header information.
   if( m_aFiles.GetSize() > 0 ) {
      LEXFILE* pFile = new LEXFILE;
      CString sName = _T("CommonLex");
      if( _ParseFile(sName, *pFile) ) {
         pFile->bIncludeInBrowser = false;
         m_aFiles.Add(pFile); 
      }
      else {
         delete pFile;
      }
   }
}

CString CLexInfo::_GetLexFilename(LPCTSTR pstrFilename, bool bCreatePath) const
{
   // Convert project-name and filename to valid names (strip filename characteristics).
   CString sProjectName;
   m_pProject->GetName(sProjectName.GetBuffer(128), 127);
   sProjectName.ReleaseBuffer();
   CString sName = ::PathFindFileName(pstrFilename);
   if( sName.IsEmpty() ) return _T("");
   sProjectName.Replace('.', '_'); sName.Replace('.', '_');
   sProjectName.Replace('*', '_'); sName.Replace('*', '_');
   sProjectName.Replace('?', '_'); sName.Replace('?', '_');
   sProjectName.Replace(':', '_'); sName.Replace(':', '_');
   sProjectName.Replace('\\', '_'); sName.Replace('\\', '_');
   // As part of the Windows LUA compliance test we'll need to
   // write stuff in the user's private private AppData folder.
   OSVERSIONINFO ver = { sizeof(ver) };
   ::GetVersionEx(&ver);
   if( _tcscmp(pstrFilename, _T("CommonLex")) == 0 ) {
      sProjectName = _T(""); ver.dwMajorVersion = 0;
   }
   if( ver.dwMajorVersion < 6 ) 
   {
      CString sDocPath;
      sDocPath.Format(_T("%sLex\\%s"), CModulePath(), sProjectName);
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      CString sLexFile;
      sLexFile.Format(_T("%s\\%s.lex"), sDocPath, sName);
      return sLexFile;
   }
   else 
   {
      TCHAR szPath[MAX_PATH] = { 0 };
      ::SHGetSpecialFolderPath(NULL, szPath, CSIDL_LOCAL_APPDATA, TRUE);
      CString sDocPath;
      sDocPath.Format(_T("%s\\BVRDE"), szPath);
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      sDocPath += _T("\\Lex");
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      sDocPath += _T("\\");
      sDocPath += sProjectName;
      if( bCreatePath ) ::CreateDirectory(sDocPath, NULL);
      CString sLexFile;
      sLexFile.Format(_T("%s\\%s.lex"), sDocPath, sName);
      return sLexFile;
   }
}

bool CLexInfo::_ParseFile(LPCTSTR pstrFilename, LEXFILE& file) const
{
   USES_CONVERSION;

   CString sLexFilename = _GetLexFilename(pstrFilename, false);
   if( sLexFilename.IsEmpty() ) return false;

   CFile f;
   if( !f.Open(sLexFilename) ) return false;
   DWORD dwSize = f.GetSize();
   LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
   f.Read(pstrData, dwSize);
   f.Close();
   pstrData[dwSize] = '\0';

   LPTSTR p = (LPTSTR) malloc((dwSize + 1) * sizeof(TCHAR));
   AtlA2WHelper(p, pstrData, dwSize + 1);
   free(pstrData);

   file.pData = p;
   file.sFilename = pstrFilename;
   file.bIncludeInBrowser = true;

   LPCTSTR pstrFile = NULL;
   LPCTSTR pstrNamePart = NULL;

   while( *p ) {
      TAGINFO info = { TAGTYPE_UNKNOWN, 0 };
      if( *p == '#' ) {
         pstrFile = p + 1;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p = '\0';
         if( !(_tcsicmp(pstrFilename, pstrFile) == 0 || _tcscmp(pstrFilename, _T("CommonLex")) == 0) ) break;
         pstrNamePart = ::PathFindFileName(pstrFile);
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
         case 'i': info.Type = TAGTYPE_IMPLEMENTATION; break;
         default:  info.Type = TAGTYPE_UNKNOWN;
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
         LPTSTR pend = _tcschr(p, '|');
         if (pend) *pend = '\0';
         info.lLineNum = _ttol(p);
         if( pend == NULL ) break;
         p = pend+1;
         //
         info.pstrFields[1] = p;
         p = _tcschr(p, '\n');
         if( p == NULL ) break;
         *p++ = '\0';
         //
         info.pstrFile = pstrNamePart;
         info.nFields = 2;
         file.aTags.Add(info);
      }
   }

   if( pstrFile == NULL || file.aTags.GetSize() == 0 ) {
      free(file.pData);
      return false;
   }

   return true;
}

